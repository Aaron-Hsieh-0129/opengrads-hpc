# Installing OpenGrADS with ADIOS2 BP5

This guide installs the BP5-enabled OpenGrADS fork from source on a new
64-bit Linux machine. The supported baseline is a serial CPU-only ADIOS2
2.11.0 build. MPI, CUDA, Kokkos, and GPU support are intentionally disabled.

There is currently no supported portable binary release of this fork. A clone
contains source code, not the compiled `grads` executable or ADIOS2. Build
once using the steps below; afterward, use `./opengrads` directly.

## 1. Clone the repository

```bash
git clone https://github.com/Aaron-Hsieh-0129/opengrads-update.git
cd opengrads-update
repo_root="$PWD"
```

## 2. Install build prerequisites

Debian or Ubuntu headless build:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake git pkg-config \
  autoconf automake libtool \
  libreadline-dev libncurses-dev
```

For an X window and Cairo graphics, also install:

```bash
sudo apt-get install -y \
  libx11-dev libxext-dev libxrender-dev \
  libcairo2-dev libfontconfig1-dev libfreetype6-dev libpixman-1-dev \
  libpng-dev zlib1g-dev libxml2-dev
```

Equivalent compiler, CMake, Readline, X11, and Cairo development packages may
be used on another distribution.

## 3. Build serial CPU-only ADIOS2 2.11.0

Keep third-party source, build files, and installation outside the OpenGrADS
repository:

```bash
mkdir -p "$HOME/src" "$HOME/build" "$HOME/.local"
git clone --branch v2.11.0 --depth 1 \
  https://github.com/ornladios/ADIOS2.git "$HOME/src/ADIOS2-2.11.0"

adios2_source="$HOME/src/ADIOS2-2.11.0"
adios2_build="$HOME/build/adios2-2.11.0-cpu"
adios2_root="$HOME/.local/adios2-2.11.0-cpu"

cmake -S "$adios2_source" -B "$adios2_build" \
  -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/gcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
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

cmake --build "$adios2_build" --parallel
cmake --install "$adios2_build"
"$adios2_root/bin/adios2-config" --version
```

Inspect the CMake summary. It must report MPI, CUDA, and Kokkos as disabled.
Warnings about unused options are acceptable when an option is unavailable in
the pinned release.

## 4. Build OpenGrADS

The checked-in `configure` and generated Makefiles are sufficient for a
normal clone; regenerating Autotools files is not required.

```bash
cd "$repo_root"
build_root="$repo_root/build"
mkdir -p "$build_root/src" "$build_root/lib"

cd "$build_root"
CC=/usr/bin/gcc \
CXX=/usr/bin/g++ \
ac_cv_header_hdf5_h=no \
  "$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --with-opengrads \
  --without-gadap \
  --without-gui \
  --with-adios2="$adios2_root"

make -C src --jobs 4 grads libgxdummy.la
```

The configuration summary must include:

```text
+ ADIOS2 BP5 enabled
+ READLINE enabled
```

Readline is optional for data access, but it provides command history and Tab
completion.

### Optional Cairo/X11 graphical plug-ins

If the Cairo/X11 development packages were installed and configure reports
Cairo enabled, build:

```bash
make -C "$build_root/src" --jobs 4 libgxpCairo.la libgxdCairo.la libgxdX11.la
```

If this optional command fails, the headless BP5 executable and tests can
still be used.

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

## 6. Run OpenGrADS

From the repository:

```bash
cd "$repo_root"
./opengrads
```

The launcher searches for `repo/build/src/grads` first. When locally built
Cairo/X11 plug-ins and an X display are available, it opens a graphical
window. Otherwise it automatically starts in headless `gxdummy` mode.

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

Set the paths again in a new shell, then update and rebuild:

```bash
cd /path/to/opengrads-update
repo_root="$PWD"
build_root="$repo_root/build"
adios2_build="$HOME/build/adios2-2.11.0-cpu"
adios2_root="$HOME/.local/adios2-2.11.0-cpu"

git pull --ff-only origin main
cmake --build "$adios2_build" --parallel
cmake --install "$adios2_build"
make -C "$build_root/src" --jobs 4 grads libgxdummy.la

OPENGRADS_BUILD_ROOT="$build_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  ./pytests/TestBP5.sh
```

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

## Direct binary use

A binary copied from another Linux machine may fail because of glibc,
libstdc++, graphics, or ADIOS2 ABI differences. This repository therefore
does not claim a universal downloadable binary. A future binary release must
include a tested runtime layout, corresponding source, license notices, and
supported platform description. See [../THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)
before redistributing linked executables.

For BP5 commands, supported shapes, and descriptor syntax, continue with
[BP5.md](BP5.md).
