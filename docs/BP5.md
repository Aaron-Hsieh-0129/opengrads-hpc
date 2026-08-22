# ADIOS2 BP5 support

This fork adds a serial ADIOS2 backend to OpenGrADS 2.2.1.oga.1. It can discover and open many BP5 datasets directly with `bpopen`, or use an explicit GrADS descriptor when the dataset needs unambiguous dimension metadata.

The backend is CPU-only. Self-contained release archives include its runtime;
the maintainer release builder downloads and builds the pinned source
automatically.

## What works

- Descriptor-free `bpopen /path/to/data.bp`.
- Passing a parent directory that contains exactly one `*.bp` dataset.
- Explicit descriptors with `dtype bp5` or `dtype adios2`.
- Serial `ReadRandomAccess` access to global numeric arrays.
- Float, double, long double, and signed/unsigned 8/16/32/64-bit integers, converted to GrADS `gadouble`.
- ADIOS2 steps mapped to GrADS T when the array has no explicit T dimension.
- One bulk ADIOS2 selection for in-bounds X/Y grids, with the row reader retained as a fallback.
- Ascending, descending, and irregular X/Y/Z coordinate arrays.
- `_FillValue` and `missing_value` masks for descriptor-free opens.
- `long_name`, `description`, `standard_name`, `units`, and dataset `title` metadata.
- Cleanup on failed open, `close`, and `reinit`.

An explicit descriptor takes precedence. Its `UNDEF` line controls masking unless attribute names are also supplied on that line.

## Prerequisites

A manual source build needs a C/C++ toolchain and an ADIOS2 installation containing the serial C API and `adios2-config`. The release builder supplies the pinned dependency automatically. ADIOS2 2.11.0 was used for the verified build.

A small CPU-only ADIOS2 configuration is sufficient:

```bash
cmake -S /path/to/ADIOS2-2.11.0 -B /tmp/adios2-cpu-build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/adios2-cpu \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTING=OFF \
  -DADIOS2_BUILD_EXAMPLES=OFF \
  -DADIOS2_USE_MPI=OFF \
  -DADIOS2_USE_Fortran=OFF \
  -DADIOS2_USE_Python=OFF \
  -DADIOS2_USE_CUDA=OFF \
  -DADIOS2_USE_Kokkos=OFF
cmake --build /tmp/adios2-cpu-build --parallel
cmake --install /tmp/adios2-cpu-build
```

ADIOS2 enables other optional packages when it finds them. Inspect its CMake summary and disable unwanted transports, compression packages, and language bindings for a minimal deployment. The exact dependency-minimized configuration used in this workspace is recorded in [INSTALL.md](INSTALL.md).

BSD libedit is optional. When available at configure time it provides command history, editing, and the Tab completion added by this fork. Color prompts are enabled at runtime by the development launcher.

## Build OpenGrADS

Build out of tree:

```bash
repo_root=/path/to/opengrads-Grads
build_root=/tmp/opengrads-build-cpu
adios2_root=/opt/adios2-cpu

mkdir -p "$build_root/src" "$build_root/lib"
cd "$build_root"
"$repo_root/cola/configure" \
  --disable-dyn-supplibs \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$adios2_root"
make -C src -j4 grads libgxdummy.la
```

The configuration summary must say `ADIOS2 BP5 enabled`. At startup, the configuration line includes `adios2-bp5`; `q config` prints the ADIOS2 version.

This reduced target deliberately builds only the executable and dummy graphics device. The historical full-bundle build and this workspace's libedit/Cairo launcher setup are documented in [INSTALL.md](INSTALL.md).

## Open a dataset without a CTL

```text
bpopen /path/to/simulation.bp
q file
q vars
q ctlinfo
set t 1
set z 1
set gxout shaded
display temperature
```

If `/path/to/run` contains exactly one BP5 child such as `output.bp`, `bpopen /path/to/run` resolves it. If it contains more than one BP5 child, specify the dataset explicitly.

Discovery selects the largest numeric rank-3 global array as the reference grid, or the largest rank-2 array when no rank-3 field exists. Matching fields are interpreted as `z,y,x` or `y,x`. Only compatible rank-2 and rank-3 global arrays are exposed.

Recognized coordinate names are:

- X: `coordinates/x`, `x`, `lon`, `longitude`
- Y: `coordinates/y`, `y`, `lat`, `latitude`
- Z: `coordinates/z_mid`, `coordinates/z`, `z`, `lev`, `level`, `height`

Cartesian coordinates with units `m`, `meter`, `meters`, `metre`, or `metres` are converted to kilometers. Missing coordinates become one-based index axes. Field aliases are lowercase sanitized basenames, limited to 15 characters, with suffixes for collisions.

## Use an explicit descriptor

Use this path when automatic shape inference is ambiguous, when selecting only some fields, or when aliases, axes, calendar time, or missing-value rules need exact control:

```text
dset ^simulation.bp
dtype bp5
title Example simulation
undef -9999
xdef 4 linear 0 1
ydef 3 linear -1 1
zdef 2 levels 1000 500
tdef 2 linear 00z01jan2000 1hr
vars 2
temperature=>temp 2 z,y,x Air temperature
surface_pressure=>ps 0 y,x Surface pressure
endvars
```

The name before `=>` is the exact, case-sensitive BP variable name. The name after it is the GrADS alias. The array-dimension list is in native ADIOS2 order. Letters map axes to GrADS X/Y/Z/T/E; a nonnegative number fixes that array axis at a zero-based index.

When the array omits T, ADIOS2 engine steps map to GrADS T. When it contains an explicit T array dimension, the reader selects ADIOS2 step zero and indexes that dimension.

`TDEF` may declare the planned length of a running simulation even when fewer
BP5 steps have been completed. For example, a descriptor with `tdef 144` can
open when only 100 steps currently exist. Times 1 through 100 are readable;
requests for 101 through 144 return undefined data instead of preventing the
dataset from opening. Close and reopen the dataset to refresh random-access
metadata after the writer adds more steps.

To request missing-value attributes explicitly while retaining descriptor control, use:

```text
undef -9.99e33 _FillValue missing_value
```

## Run the BP5 regression

```bash
cd /path/to/opengrads-Grads/pytests
OPENGRADS_BUILD_ROOT=/tmp/opengrads-build-cpu \
OPENGRADS_ADIOS2_ROOT=/opt/adios2-cpu \
  ./TestBP5.sh
```

The test creates a temporary two-step BP5 fixture. It covers a planned TDEF
that is longer than the currently available BP5 steps, attribute metadata, descriptor precedence, two time steps, float and double conversion, missing masks, full 2-D statistics, shaded contours, invalid paths and shapes, ambiguous parent directories, and repeated open/close/reinit cleanup.

For sanitizer testing, configure a separate build with:

```bash
repo_root=/path/to/opengrads-Grads
sanitize_root=/tmp/opengrads-build-bp5-sanitize
adios2_root=/opt/adios2-cpu

mkdir -p "$sanitize_root/src" "$sanitize_root/lib"
cd "$sanitize_root"
CFLAGS="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" \
  "$repo_root/cola/configure" \
  --disable-dyn-supplibs \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$adios2_root"
make -C src -j4 grads libgxdummy.la

ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
OPENGRADS_BUILD_ROOT="$sanitize_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  "$repo_root/pytests/TestBP5.sh"
```

## Verified real dataset

On 15 August 2026, `bpopen /raid/mog/rcemip_f_tc_bp5` resolved `vvm_output.bp`, discovered 35 fields on a 512 x 512 x 44 grid with 21 steps, and plotted `w` at T=21 and Z=7. The full plane contained 262,144 valid values with a range of approximately -0.0731651 to 0.102873. This is a functional observation on one host, not a portable performance claim.

## Launcher troubleshooting

The historical graphics bundle contains its own old `libstdc++.so.6`. ADIOS2
2.11 built with a newer compiler may then report missing `GLIBCXX_*` or
`CXXABI_*` versions. The `opengrads` launcher prevents this by preloading
the `libstdc++.so.6` next to the resolved ADIOS2 library before exposing the
graphics plug-in directory. Override the selected file only when necessary:

```bash
OPENGRADS_CXX_RUNTIME=/absolute/path/to/libstdc++.so.6 ./opengrads
```

A message such as `Unable to connect to X server` is a separate display
configuration issue. Use a desktop X session, SSH X forwarding, or run
headlessly with `./opengrads -bl -d gxdummy -h gxdummy`.

## Current limitations

- Serial random access only; no MPI collective reader or streaming engine.
- Global arrays only; no ADIOS2 local arrays or complex values.
- Descriptor-free inference is intentionally limited to matching rank-2/rank-3 fields.
- No templates or PDEF in the BP5 backend.
- Bulk reads currently cover in-bounds X/Y requests; other requests fall back to row reads.
- GrADS retains global request state and is not generally thread-safe.
- Native cubed-sphere/curvilinear topology is not implemented.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the architecture and remaining roadmap.
