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

GNU Readline is optional. When available at configure time it provides command history, editing, and the Tab completion added by this fork. Color prompts are enabled at runtime by the development launcher.

## Build opengrads-hpc

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

This reduced target deliberately builds only the executable and dummy graphics device. The historical full-bundle build and this workspace's Readline/Cairo launcher setup are documented in [INSTALL.md](INSTALL.md).

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

Discovery selects the largest numeric rank-3 global array as the reference grid, or the largest rank-2 array when no rank-3 field exists. Matching fields are interpreted as `z,y,x` or `y,x`.

1-D data is exposed too:

- **A rank-1 array** whose length matches exactly one axis is a profile along that axis, such as a reference state `thbar(z)`. It holds the same value everywhere along the axes it lacks, so it combines with full fields directly: `d th - thbar` works in an x-y map, an x-z section, or a profile. A length that matches two axes (say 96 levels on a 96 × 96 grid) is skipped with a warning, since the axis cannot be told from the shape; give such a field a descriptor line. Coordinate variables are never fields.
- **A global value** written at every step, one number per step, is a time series. A descriptor lists it with the dimension list `t`.
- A dataset with no 2-D or 3-D field at all, a single column, opens on its Z coordinate with X and Y as single points.

The open lists the 1-D fields it found:

    1-D fields, the same along the axes they lack: thbar(z), pibar(z), rhobar(z), rhobar_up(z)

Recognized coordinate names are:

- X: `coordinates/x`, `x`, `lon`, `longitude`
- Y: `coordinates/y`, `y`, `lat`, `latitude`
- Z: `coordinates/z_mid`, `coordinates/z`, `z`, `lev`, `level`, `height`

Missing coordinates become one-based index axes. Field aliases are lowercase sanitized basenames, limited to 15 characters, with suffixes for collisions.

### A descriptor-free open behaves like a descriptor

`bpopen` builds the descriptor a person would write for the dataset, so opening it either way gives the same results. On a VVM-shaped dataset (96 × 96 × 300, coordinates in metres, a CF time coordinate), every command checked, `aave` and level selection included, printed the same through `bpopen` as through a descriptor carrying the same axes, and the plots were byte-identical. `q ctlinfo` after `bpopen` shows that descriptor, ready to copy.

**Z** is used as the dataset stores it. A Z axis in metres stays in metres, so `set lev 700` picks the level nearest 700 m.

**X and Y in a length unit** (`m`, `meter`, `metre`, `km`, and their plurals) become degrees. GrADS has no Cartesian horizontal axes: X and Y are longitude and latitude, and area averages (`aave`), the spherical derivatives (`hdivg`, `hcurl`), and map drawing all assume it. Read as degrees, a domain 3325 m wide spans more than nine trips round the globe; the map labels wrap and `aave` is badly wrong (70 % high on one measured field). So, as a descriptor for a Cartesian model does, each axis is mapped onto GrADS's own 6370 km sphere and centred on 0:

    degrees = (value - midpoint of the axis) / (6.37e6 m × π/180)

Centring keeps cos(latitude) at 1 to within 4 × 10⁻⁸ across a 3.3 km domain, so `aave` matches the arithmetic mean, and using GrADS's own radius means `hdivg` and `hcurl` recover the original grid spacing. A 35 m grid becomes `xdef 96 linear -0.0149536 0.000314812`, the values VVM descriptors carry. X and Y in any other units, or without a units attribute, are used as stored; a Cartesian dataset without units needs an explicit descriptor.

**T** comes from a CF time coordinate: a variable named `time` or `coordinates/time` holding one value per step, or one 1-D array of all steps, with `units` of the form `seconds since 1998-01-01 00:00:00` (also `minutes`, `hours`, `days`; an ISO `T` separator is accepted). The calendar must be standard or Gregorian, and the step must be a whole number of minutes, since that is the finest a GrADS `TDEF` can express. Uneven steps are labelled with the first interval, with a warning. Without a usable time coordinate, T counts steps, labelled from 00Z01JAN2000 at one-minute intervals.

The open reports each of these decisions, for example:

    X and Y are Cartesian; mapped to degrees on GrADS's 6370 km sphere, centred on 0, ...
    T from 'time': 1441 steps from 00:00Z01JAN1998 every 1mn

A field is skipped, with a warning naming it, when its BP variable name cannot be written into a GrADS descriptor: a name holding whitespace, a `~`, an `=>`, a non-printable byte, more than 256 characters, or starting with anything other than a letter, a digit, or `/`. Such a dataset needs an explicit descriptor, or a writer that names its variables differently.

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

A dimension list may leave out X or Y: `thbar=>thbar 300 z Reference potential temperature` describes a profile, which is then the same at every X and Y. A global value, one number per step, is listed as `t`: `domain_mean=>dmean 0 t Domain mean`.

A variable written at one step only, in a dataset whose other variables have more, holds for every time: terrain, land use, and reference profiles are typically written once. The open names such variables:

    Written once, so the same at every time: topo, albedo, thbar

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
that is longer than the currently available BP5 steps, attribute metadata, descriptor precedence, two time steps, float and double conversion, missing masks, full 2-D statistics, shaded contours, invalid paths and shapes, ambiguous parent directories, and repeated open/close/reinit cleanup. It also checks that a descriptor-free open and the fixture's descriptor report the same X, Y, Z, and T axes, with metre X and Y mapped to degrees and T taken from the CF time coordinate, that a dataset without a time coordinate falls back to counted steps and says so, and that a Z profile, a field written at the first step only, and a per-step global value read the same through both opens, including a profile subtracted from a 3-D field in an x-z section.

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
- Global arrays, and global values read as time series; no ADIOS2 local arrays or complex values.
- Descriptor-free inference covers matching rank-2/rank-3 fields, rank-1 profiles whose length fits one axis, and per-step global values. Other shapes, such as a 2-D x-z section, need a descriptor.
- Descriptor-free time axes need a CF time coordinate with a standard calendar and whole-minute steps; anything else counts steps.
- No templates or PDEF in the BP5 backend.
- Bulk reads currently cover in-bounds X/Y requests; other requests fall back to row reads.
- GrADS retains global request state and is not generally thread-safe.
- Native cubed-sphere/curvilinear topology is not implemented.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the architecture and remaining roadmap.
