# Building from source

This guide builds opengrads-hpc — GrADS with the ADIOS2/BP5 reader and
OpenMP-threaded calculations — from source on 64-bit Linux. The supported
baseline is a serial, CPU-only ADIOS2 build; MPI, CUDA, and Kokkos are
deliberately disabled.

**Most people do not need this page.** Unpack a release archive and run
`./opengrads` — no compiler and no ADIOS2 installation required. See
[RELEASES.md](RELEASES.md). Build from source if you want to modify the code,
target a platform we do not publish, or avoid the redistribution question
described in [LICENSING.md](LICENSING.md).

## Prerequisites

A C and C++ compiler, `make`, `cmake`, `curl`, `tar`, and `sha256sum`, plus the
graphics and data-format development headers. On Ubuntu or Debian:

```bash
sudo apt install autoconf automake build-essential cmake curl gawk libtool \
  pkg-config libcairo2-dev libx11-dev libxext-dev libxmu-dev libice-dev \
  libsm-dev libgeotiff-dev libtiff-dev libhdf5-dev libjpeg-dev \
  libnetcdf-dev libudunits2-dev zlib1g-dev
```

Use the equivalent packages on other distributions. On a cluster, `module
avail` may provide the compiler and CMake. No root access is needed beyond
these packages.

## One command

```bash
./release/build-release.sh
```

This downloads checksum-pinned dependencies into `.release-work`, builds
everything, runs the BP5, SDF, and OpenMP regression tests, and writes a
relocatable archive to `release-dist`. It is the same script CI runs, so it is
the best-tested path.

Use the steps below instead if you want a development build you can rebuild
incrementally.

## Step by step

### 1. Build the pinned dependencies

```bash
export OPENGRADS_WORK="$HOME/opengrads-deps"
./release/build-dependencies.sh "$OPENGRADS_WORK"

deps_root="$OPENGRADS_WORK/deps"        # ncurses, readline
adios2_root="$OPENGRADS_WORK/adios2"    # serial ADIOS2
```

This builds ncurses, GNU Readline, and ADIOS2 from checksum-pinned upstream
releases recorded in [`release/versions.env`](../release/versions.env). It is
incremental: components already installed are skipped.

To reuse dependencies you already have, skip this and point `deps_root` and
`adios2_root` at your own prefixes. `$adios2_root/bin/adios2-config` must
exist.

### 2. Configure and build

```bash
repo_root="$PWD"
build_root="$repo_root/build"
jobs="${OPENGRADS_BUILD_JOBS:-4}"
mkdir -p "$build_root/src" "$build_root/lib"

unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH SUPPLIBS
export PATH="$deps_root/bin:$PATH"
export CPPFLAGS="-I$deps_root/include"
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig"
export LD_LIBRARY_PATH="$adios2_root/lib:$adios2_root/lib64:$deps_root/lib"

cd "$build_root"
"$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --enable-openmp \
  --enable-sdfopen \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$adios2_root"

make -C src --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la
```

The `build_root/lib` directory must exist before configuring — libtool fails
without it. Clearing `SUPPLIBS` and the include/library environment variables
matters on systems with site-wide toolchains, which otherwise inject unrelated
headers.

The configuration summary should report:

```text
+ ADIOS2 BP5 enabled     + OPENMP enabled      + CAIRO enabled
+ NETCDF4 enabled        + READLINE enabled
```

`READLINE enabled` means line editing, history, and Tab completion are
available, provided by GNU Readline.

To build without BP5, pass `--without-adios2`. Note the flag defaults to
`auto`, so omitting it is not enough — a system ADIOS2 on `PATH` is still
detected.

### 3. Verify

```bash
cd "$repo_root"
export OPENGRADS_BUILD_ROOT="$build_root"
export OPENGRADS_ADIOS2_ROOT="$adios2_root"

./pytests/TestBP5.sh
./pytests/TestSDFOpen.sh
./pytests/TestOpenMP.sh
```

All three must pass. Then check the compiled configuration:

```bash
printf 'q config\nquit\n' | ./opengrads -bl -d gxdummy -h gxdummy
```

The startup line should contain `readline netcdf adios2-bp5 openmp`.

### 4. Run

```bash
./opengrads
```

With an X display the launcher selects the Cairo plug-ins, falling back to X11
and then to headless `gxdummy`. Opening BP5 data needs no descriptor file:

```text
bpopen /path/to/output.bp
q vars
set gxout shaded
display variable_name
```

Point the launcher at a build or ADIOS2 installation elsewhere with
`OPENGRADS_BUILD_ROOT` and `OPENGRADS_ADIOS2_ROOT`.

## Rebuilding after an update

Restore the environment from step 2, then:

```bash
git pull --ff-only origin main
make -C "$build_root/src" clean
make -C "$build_root/src" --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la
```

Re-run `configure` only if you changed dependency prefixes or configure flags.
Avoid `make -B`, which can trigger unnecessary Autotools regeneration.

## Troubleshooting

**Executable not found.** Check `ls -l "$build_root/src/grads"`. If it is
missing, repeat step 2; if it lives elsewhere, set `OPENGRADS_BUILD_ROOT`.

**No X window.** Check `echo "$DISPLAY"`. Remote sessions need `ssh -X` or
`-Y`. Headless mode needs no X.

**ADIOS2 not detected.** Confirm `"$adios2_root/bin/adios2-config" --serial
--c-flags` works, then reconfigure with that exact prefix.

**ADIOS2 pulls in MPI or libfabric.** An existing installation may advertise
serial flags while its core was built with MPI. Check the library itself:

```bash
ldd "$adios2_root"/lib*/libadios2_core.so | grep -E 'libmpi|libfabric|not found'
```

Any output means it is unsuitable; rebuild with `build-dependencies.sh`. After
changing `adios2_root`, run `make -C "$build_root/src" clean` so the executable
is relinked.

**ADIOS2 fails with a duplicate `writev`.** Site-wide include variables make
one bundled ADIOS2 component pick up another's `config.h`. Clear them and
resume:

```bash
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH
./release/build-dependencies.sh "$OPENGRADS_WORK"
```

**Missing `GLIBCXX` or `CXXABI` versions.** A legacy OpenGrADS graphics bundle
may carry an old `libstdc++.so.6`. Override the runtime only if needed:

```bash
OPENGRADS_CXX_RUNTIME=/absolute/path/to/libstdc++.so.6 ./opengrads
```

## Next

For BP5 commands, supported shapes, and descriptor syntax, continue with
[BP5.md](BP5.md). For threading controls, see
[PERFORMANCE.md](PERFORMANCE.md).
