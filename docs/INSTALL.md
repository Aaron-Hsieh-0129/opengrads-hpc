# Installing OpenGrADS with ADIOS2 BP5

This guide installs the BP5-enabled OpenGrADS fork from source on a new
64-bit Linux machine. The supported baseline is a serial CPU-only ADIOS2
2.11.0 build. MPI, CUDA, Kokkos, and GPU support are intentionally disabled.

There is currently no supported portable binary release of this fork. A clone
contains source code, not the compiled `grads` executable or ADIOS2. Build
once using the steps below; afterward, use `./opengrads` directly.

## 1. Find or load the bootstrap tools

After the graphics prerequisites are available, this workflow builds and
installs below `$HOME`. On a cluster, check available modules with
`module avail` and load a compiler or CMake module before continuing.

A C compiler, C++ compiler, `make`, `curl`, `tar`, and `sha256sum` must already
exist, along with the compiler's linker/binutils and a basic Unix userland
(`sh`, `awk`, `sed`, `grep`, and `install`). A compiler cannot be bootstrapped
from source without another working compiler. Check the main tools with:

```bash
export CC="${CC:-$(command -v gcc || command -v cc)}"
export CXX="${CXX:-$(command -v g++ || command -v c++)}"

"$CC" --version
"$CXX" --version
make --version
curl --version
tar --version
sha256sum --version
```

All six commands must succeed. If the compiler or `make` is missing, load a
site-provided toolchain module or ask the machine administrator for a basic
build toolchain. No root privileges are needed after that bootstrap.

For the recommended graphical build, Cairo, X11, and Xext development files
and `pkg-config` must also be installed. On Ubuntu or Debian, install them with:

```bash
sudo apt install pkg-config libcairo2-dev libx11-dev libxext-dev
pkg-config --modversion cairo x11 xext
```

On other distributions, install the corresponding development packages. If
they are unavailable, use the explicit headless build described in step 4.

## 2. Build local CMake, ncurses, and Readline

Use one private prefix for locally built tools and libraries:

```bash
export OPENGRADS_DEPS_ROOT="$HOME/.local/opengrads-deps"
deps_root="$OPENGRADS_DEPS_ROOT"
deps_source="$HOME/src/opengrads-deps"
deps_build="$HOME/build/opengrads-deps"
jobs="${OPENGRADS_BUILD_JOBS:-4}"

mkdir -p "$deps_root" "$deps_source" "$deps_build"
export PATH="$deps_root/bin:$PATH"
export CPPFLAGS="-I$deps_root/include"
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$deps_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

If `cmake --version` already works, keep it. Otherwise build CMake 3.31.8
locally:

```bash
if ! command -v cmake >/dev/null 2>&1; then
  cd "$deps_source"
  curl -fL --retry 3 \
    https://github.com/Kitware/CMake/releases/download/v3.31.8/cmake-3.31.8.tar.gz \
    -o cmake-3.31.8.tar.gz
  printf '%s  %s\n' \
    e3cde3ca83dc2d3212105326b8f1b565116be808394384007e7ef1c253af6caa \
    cmake-3.31.8.tar.gz | sha256sum --check -
  tar -xzf cmake-3.31.8.tar.gz
  cd cmake-3.31.8
  ./bootstrap --prefix="$deps_root" --parallel="$jobs"
  make --jobs "$jobs"
  make install
fi
cmake --version
```

Build ncurses 6.5 and GNU Readline 8.2 in the same prefix. The explicit
`SHLIB_LIBS=-lncurses` is important: it prevents the private Readline shared
library from exposing unresolved terminal symbols to unrelated programs.

```bash
cd "$deps_source"
curl -fL --retry 3 https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz \
  -o ncurses-6.5.tar.gz
curl -fL --retry 3 https://ftp.gnu.org/gnu/readline/readline-8.2.tar.gz \
  -o readline-8.2.tar.gz

printf '%s  %s\n' \
  136d91bc269a9a5785e5f9e980bc76ab57428f604ce3e5a5a90cebc767971cc6 \
  ncurses-6.5.tar.gz | sha256sum --check -
printf '%s  %s\n' \
  3feb7171f16a84ee82ca18a36d7b9be109a52c04f492a053331d7d1095007c35 \
  readline-8.2.tar.gz | sha256sum --check -

tar -xzf ncurses-6.5.tar.gz
tar -xzf readline-8.2.tar.gz

cd "$deps_source/ncurses-6.5"
CC="$CC" CXX="$CXX" ./configure \
  --prefix="$deps_root" \
  --with-shared \
  --without-debug \
  --without-ada \
  --without-tests \
  --disable-widec \
  --enable-overwrite
make --jobs "$jobs"
make install

cd "$deps_source/readline-8.2"
CC="$CC" CPPFLAGS="-I$deps_root/include" \
LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib" \
  ./configure --prefix="$deps_root" --enable-shared --disable-static
make --jobs "$jobs" SHLIB_LIBS=-lncurses
make install SHLIB_LIBS=-lncurses

ldd "$deps_root/lib/libreadline.so"
```

An ncurses message saying that `ldconfig` was not run can be ignored. The
build embeds a private-library run path, and the `opengrads` launcher also
loads `$OPENGRADS_DEPS_ROOT/lib`.

In every new shell, restore the local paths before rebuilding:

```bash
export OPENGRADS_DEPS_ROOT="$HOME/.local/opengrads-deps"
deps_root="$OPENGRADS_DEPS_ROOT"
deps_source="$HOME/src/opengrads-deps"
deps_build="$HOME/build/opengrads-deps"
jobs="${OPENGRADS_BUILD_JOBS:-4}"
export PATH="$deps_root/bin:$PATH"
export CPPFLAGS="-I$deps_root/include"
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$deps_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export CC="${CC:-$(command -v gcc || command -v cc)}"
export CXX="${CXX:-$(command -v g++ || command -v c++)}"
```

## 3. Obtain the source and build serial CPU-only ADIOS2 2.11.0

Clone this repository when `git` is available:

```bash
git clone https://github.com/Aaron-Hsieh-0129/opengrads-update.git
cd opengrads-update
repo_root="$PWD"
```

Without `git`, download the GitHub source archive instead:

```bash
mkdir -p "$HOME/src"
cd "$HOME/src"
curl -fL --retry 3 \
  https://github.com/Aaron-Hsieh-0129/opengrads-update/archive/refs/heads/main.tar.gz \
  -o opengrads-update-main.tar.gz
tar -xzf opengrads-update-main.tar.gz
cd opengrads-update-main
repo_root="$PWD"
```

Download the pinned ADIOS2 source release and verify it:

```bash
cd "$deps_source"
curl -fL --retry 3 \
  https://github.com/ornladios/ADIOS2/archive/refs/tags/v2.11.0.tar.gz \
  -o adios2-v2.11.0.tar.gz
printf '%s  %s\n' \
  0a2bd745e3f39745f07587e4a5f92d72f12fa0e2be305e7957bdceda03735dbf \
  adios2-v2.11.0.tar.gz | sha256sum --check -
tar -xzf adios2-v2.11.0.tar.gz

adios2_source="$deps_source/ADIOS2-2.11.0"
adios2_build="$deps_build/adios2-2.11.0-cpu"
adios2_root="$HOME/.local/adios2-2.11.0-cpu"
```

Build ADIOS2 with the existing or locally built CMake:

```bash
# Prevent site-wide compiler include variables from injecting an unrelated
# config.h into ADIOS2's bundled third-party projects.
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH

cmake -S "$adios2_source" -B "$adios2_build" \
  -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$CC" \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_INSTALL_PREFIX="$adios2_root" \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTING=OFF \
  -DADIOS2_BUILD_EXAMPLES=OFF \
  -DADIOS2_USE_MPI=OFF \
  -DADIOS2_USE_CUDA=OFF \
  -DADIOS2_USE_Kokkos=OFF \
  -DADIOS2_USE_Fortran=OFF \
  -DADIOS2_USE_Python=OFF \
  -DADIOS2_USE_HDF5=OFF \
  -DADIOS2_USE_HDF5_VOL=OFF \
  -DADIOS2_USE_SST=OFF \
  -DADIOS2_USE_DataMan=OFF \
  -DADIOS2_USE_DataSpaces=OFF \
  -DADIOS2_USE_MHS=OFF \
  -DADIOS2_USE_ZeroMQ=OFF \
  -DADIOS2_USE_UCX=OFF \
  -DADIOS2_USE_BigWhoop=OFF \
  -DADIOS2_USE_Blosc2=OFF \
  -DADIOS2_USE_BZip2=OFF \
  -DADIOS2_USE_Caliper=OFF \
  -DADIOS2_USE_ZFP=OFF \
  -DADIOS2_USE_SZ=OFF \
  -DADIOS2_USE_LIBPRESSIO=OFF \
  -DADIOS2_USE_MGARD=OFF \
  -DADIOS2_USE_PNG=OFF \
  -DADIOS2_USE_DAOS=OFF \
  -DADIOS2_USE_IME=OFF \
  -DADIOS2_USE_Sodium=OFF \
  -DADIOS2_USE_Catalyst=OFF \
  -DADIOS2_USE_Campaign=OFF \
  -DADIOS2_USE_OpenSSL=OFF \
  -DADIOS2_USE_AWSSDK=OFF \
  -DADIOS2_USE_XRootD=OFF \
  -DADIOS2_USE_Profiling=OFF

cmake --build "$adios2_build" --parallel "$jobs"
cmake --install "$adios2_build"
"$adios2_root/bin/adios2-config" --version
"$adios2_root/bin/adios2-config" --serial --c-libs
ldd "$adios2_root/lib64/libadios2_core.so" | \
  grep -E 'libmpi|libfabric|not found' || true
```

Inspect the CMake summary. It must report MPI, CUDA, and Kokkos as disabled.
The final `ldd`/`grep` command must produce no output; a serial C library is
not sufficient if its ADIOS2 core was built with MPI or has unresolved runtime
dependencies. Warnings about unused options are acceptable when an option is
unavailable in the pinned release.

## 4. Build OpenGrADS

The checked-in `configure` and generated Makefiles are sufficient for a
normal clone; regenerating Autotools files is not required.

```bash
cd "$repo_root"
build_root="$repo_root/build"
mkdir -p "$build_root/src" "$build_root/lib"

cd "$build_root"
CC="$CC" \
CXX="$CXX" \
ac_cv_header_hdf5_h=no \
  "$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --with-opengrads \
  --without-gadap \
  --without-gui \
  --with-adios2="$adios2_root"

make -C src --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la
```

The configuration summary must include:

```text
+ ADIOS2 BP5 enabled
+ READLINE enabled
+ CAIRO enabled
```

Readline is optional for data access, but it provides command history and Tab
completion.

### Graphical and headless builds

The default command above builds the Cairo display and printing plug-ins, the
X11 display fallback, and the dummy headless device. `--without-gui` disables
the legacy Athena Widget interface; it does not disable graphical plots.

For a machine without Cairo/X11 development files, build only the supported
headless device:

```bash
make -C "$build_root/src" --jobs "$jobs" grads libgxdummy.la
```

If Cairo is unavailable but X11 development files exist, also build
`libgxdX11.la`. The launcher selects X11 automatically for an interactive
window and uses `gxdummy` for hardcopy output.

Another no-root option is to reuse the graphics plug-ins and resource files
from an existing original OpenGrADS bundle. Keep the BP5-enabled executable
from this repository and point the launcher at the bundle's `Contents`
directory:

```bash
OPENGRADS_BUNDLE_ROOT=/path/to/opengrads/Contents ./opengrads
```

The launcher also finds a compatible sibling directory named
`opengrads-2.2.1.oga.1/Contents` automatically. A bundle without plug-ins for
the current platform and architecture is ignored. If neither local graphics
nor a compatible bundle is available, use the supported headless mode.

## 5. Verify the installation

```bash
cd "$repo_root"
OPENGRADS_BUILD_ROOT="$build_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  ./pytests/TestBP5.sh
```

Expected final line:

```text
BP5 regression test passed: attributes, descriptor precedence, bulk 2-D/shaded reads, errors, and repeated lifecycle.
```

Check the compiled configuration:

```bash
./opengrads -bl -d gxdummy -h gxdummy
```

At the prompt:

```text
q config
quit
```

The startup configuration must contain `readline adios2-bp5`.

On a desktop with a working X display, start the graphical application:

```bash
./opengrads
```

At the prompt, verify the selected graphics devices:

```text
q gxconfig
quit
```

The recommended build reports `GX Display "Cairo"` and `GX Print "Cairo"`.
An X11-only build reports `GX Display "X11"`.

## 6. Run OpenGrADS

From the repository:

```bash
cd "$repo_root"
./opengrads
```

The launcher automatically adds `$OPENGRADS_DEPS_ROOT/lib` (defaulting to
`$HOME/.local/opengrads-deps/lib`) to its runtime library path.

The launcher searches for `repo/build/src/grads` first. With an X display, it
prefers the Cairo display/print plug-ins and falls back to the X11 display
plug-in when Cairo was not built. Without a display plug-in or `$DISPLAY`, it
automatically starts in headless `gxdummy` mode.

Open BP5 data without a CTL:

```text
bpopen /path/to/output.bp
q file
q vars
set t 1
set z 1
set gxout shaded
display variable_name
```

For an explicit headless session:

```bash
./opengrads -bl -d gxdummy -h gxdummy
```

For an existing build or ADIOS2 installation in another location:

```bash
OPENGRADS_BUILD_ROOT=/path/to/opengrads-build \
OPENGRADS_ADIOS2_ROOT=/path/to/adios2-install \
  ./opengrads
```

## Updating an existing installation

Restore the local-prefix variables from step 2, then update and rebuild:

```bash
export OPENGRADS_DEPS_ROOT="$HOME/.local/opengrads-deps"
deps_root="$OPENGRADS_DEPS_ROOT"
export PATH="$deps_root/bin:$PATH"
export CPPFLAGS="-I$deps_root/include"
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export CC="${CC:-$(command -v gcc || command -v cc)}"
export CXX="${CXX:-$(command -v g++ || command -v c++)}"
export LD_LIBRARY_PATH="$deps_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH

cd /path/to/opengrads-update
repo_root="$PWD"
build_root="$repo_root/build"
adios2_build="$HOME/build/opengrads-deps/adios2-2.11.0-cpu"
adios2_root="$HOME/.local/adios2-2.11.0-cpu"
jobs="${OPENGRADS_BUILD_JOBS:-4}"

git pull --ff-only origin main
cmake --build "$adios2_build" --parallel "$jobs"
cmake --install "$adios2_build"
cd "$build_root"
CC="$CC" \
CXX="$CXX" \
ac_cv_header_hdf5_h=no \
  "$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --with-opengrads \
  --without-gadap \
  --without-gui \
  --with-adios2="$adios2_root"
make -C src clean
make -C src --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la
cd "$repo_root"

OPENGRADS_BUILD_ROOT="$build_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  ./pytests/TestBP5.sh
```

If you installed from the GitHub archive instead of a Git clone, download a
new archive and rebuild rather than running `git pull`.

## Troubleshooting

### Development executable not found

Confirm that this file exists and is executable:

```bash
ls -l "$repo_root/build/src/grads"
```

Otherwise repeat step 4 or set `OPENGRADS_BUILD_ROOT`.

### No X window

```bash
echo "$DISPLAY"
```

A local desktop needs a working X display. Remote sessions normally need
`ssh -X` or `ssh -Y`. Headless mode does not require X.

### Missing GLIBCXX or CXXABI versions

A legacy OpenGrADS graphics bundle may contain an old `libstdc++.so.6`.
The launcher normally selects the runtime associated with ADIOS2. Override it
only when necessary:

```bash
OPENGRADS_CXX_RUNTIME=/absolute/path/to/libstdc++.so.6 ./opengrads
```

### ADIOS2 is not detected

```bash
"$adios2_root/bin/adios2-config" --serial --c-flags
"$adios2_root/bin/adios2-config" --serial --c-libs
```

Reconfigure OpenGrADS with the exact same `adios2_root`.

### ADIOS2 unexpectedly links MPI or libfabric

An existing ADIOS2 installation may provide `--serial` compiler flags while
its shared core library was still built with MPI. Check the core, not only the
output of `adios2-config`:

```bash
ldd "$adios2_root/lib64/libadios2_core.so" | \
  grep -E 'libmpi|libfabric|not found' || true
```

The command must produce no output for the supported build. If it lists MPI,
libfabric, or a missing library, rebuild ADIOS2 with the step 3 options and
reconfigure OpenGrADS with that new installation prefix.

When changing `adios2_root` in an existing OpenGrADS build, clean the compiled
targets after reconfiguring so that the executable is relinked. Do not use
`make -B`, because it can unnecessarily invoke Autotools regeneration.

```bash
make -C "$build_root/src" clean
make -C "$build_root/src" --jobs "$jobs" grads libgxdummy.la
```

### ADIOS2 EVPath fails with a duplicate `writev`

An error such as `static declaration of 'writev' follows non-static
declaration` can occur when site-wide compiler include variables make one
bundled ADIOS2 component include another component's generic `config.h`.
Clear those variables and resume the existing build:

```bash
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH
cmake --build "$adios2_build" --parallel "$jobs"
cmake --install "$adios2_build"
```

## Direct binary use

A binary copied from another Linux machine may fail because of glibc,
libstdc++, graphics, or ADIOS2 ABI differences. This repository therefore
does not claim a universal downloadable binary. A future binary release must
include a tested runtime layout, corresponding source, license notices, and
supported platform description. See [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)
before redistributing linked executables.

For BP5 commands, supported shapes, and descriptor syntax, continue with
[BP5.md](BP5.md).
