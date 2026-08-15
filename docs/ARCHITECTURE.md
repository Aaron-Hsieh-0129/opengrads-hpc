# OpenGrADS Architecture and Contributor Guide

This document describes the repository structure, runtime architecture,
development constraints, extension points, and remaining engineering roadmap.
Build and end-user setup instructions are maintained separately in
[INSTALL.md](INSTALL.md).

## Project snapshot

OpenGrADS is a distribution around the GrADS (Grid Analysis and Display System) engine. It combines the upstream-style COLA core, dynamically loaded OpenGrADS commands/functions, scripts, documentation, sample data, tests, and platform bundle tooling.

- Version in this tree: `2.2.1.oga.1` (`cola/src/VERSION`).
- Release described by `NEWS`: 9 February 2019, based on COLA GrADS 2.2.1.
- License: GPL v2; see `COPYRIGHT` and `COPYING`.
- Main implementation: C, with some C++ configuration support, Fortran extensions, GrADS scripts, Perl bundle scripts, and a legacy Python 2 test/client layer.
- Main executable: `grads`.
- Core model: a five-dimensional X/Y/Z/T/E environment with at most two varying dimensions in an evaluated `gagrid`.
- Existing formats include raw binary, GRIB1/2, NetCDF, HDF4/HDF5, BUFR station data, OPeNDAP, and ADIOS2 BP5 when their optional libraries are available.

This is a mature, global-state-heavy C codebase. Prefer small, well-tested seams over broad rewrites.

## Contributor guidelines

1. Treat `cola/src` as the canonical core source. Do not edit generated `Makefile`, `Makefile.in`, `configure`, `config.h`, or `buildinfo.h` unless the task is explicitly about generated build artifacts. Make source-list and dependency changes in `cola/src/Makefile.am` and feature checks in `cola/configure.ac` or `cola/m4/`.
2. Preserve the data invariants:
   - `gagrid.grid` stores `gadouble` values.
   - `gagrid.umask` stores validity independently (`1` valid, `0` missing).
   - X is the fastest-varying on-disk dimension for the legacy row reader.
   - Dimension indexes are `0=X`, `1=Y`, `2=Z`, `3=T`, `4=E`; `-1` means non-varying.
3. Use the project allocation wrappers (`galloc`, `gree`) in core code unless an external API owns the memory. Every new external handle must have a close path for `close`, `reinit`, error unwinding, and normal process shutdown.
4. Keep optional features compile-time optional. A machine without ADIOS2, NetCDF, Cairo, or X11 must either build a documented reduced configuration or fail at configure time with a useful message.
5. Do not make a new format masquerade as an existing flag such as `ncflg`. Add a named backend/type flag or, preferably, a backend operations interface.
6. Add a small reproducible data fixture and an automated test for every new reader, grid topology, or accelerated kernel. Test missing values, edge dimensions, cleanup, and error paths, not only a successful plot.
7. Measure performance changes against a fixed dataset and command script. Record wall time, peak memory, build flags, hardware, and output-equivalence checks.
8. The core is not generally thread-safe. `gaio.c`, graphics scaling, and other modules use file-scope mutable state. Do not introduce parallel calls into them until the relevant state has been moved into an explicit request/context object.
9. Avoid unrelated formatting churn. Much of the code uses older C style, and large mechanical changes make scientific behavior difficult to review.
10. Before handing off, report the exact build/test command, enabled optional features, and any untested hardware/backend.


## Repository map

| Path | Role | Notes |
|---|---|---|
| `GNUmakefile` | Top-level orchestration | Builds/installs the core, extensions, tests, docs, and relocatable bundle. It assumes supplemental libraries for the normal bundle workflow. |
| `BUILD`, `INSTALL`, `README`, `NEWS`, `ChangeLog` | Historical project documentation | `BUILD` describes the intended supplemental-library build; `INSTALL` mostly describes installing a completed bundle. Some URLs and platform assumptions are old. |
| `oga_configure` | OpenGrADS configure wrapper | C shell script. Requires an absolute `SUPPLIBS` path and configures `cola/` in-tree. The top Makefile invokes `oga_configure` without `./`, so calling `./oga_configure` explicitly is safer on shells whose `PATH` excludes the current directory. |
| `cola/` | Core GrADS source distribution | Autoconf/Automake project. `cola/src` is the runtime, I/O, expression, and graphics code; `cola/doc` is the detailed user manual; `cola/data` contains fonts, maps, BUFR tables, and support data. |
| `extensions/` | OpenGrADS UDC/UDF extensions | Dynamically loaded C/Fortran `.gex` modules plus `.udxt` registration tables. `extensions/hello` is the smallest example. The active aggregate list is in `extensions/GNUmakefile`; `gxyat` and `shape` are not in its default `SUBDIRS`. |
| `etc/udpt`, `etc/udpt-local` | Bundle and local-build graphics plug-in tables | Maps display/print backend names (`Cairo`, `X11`, `gxdummy`, `GD`) to shared libraries. Runtime lookup also honors `GAUDPT`. |
| `lib/scripts/` | GrADS scripts and GUIs | Plot helpers, color bars, interpolation, meteorological products, and example GUIs. These are runtime assets, not core C tests. |
| `data/` | Extension/sample support data | Shapefiles and satellite TLE data. Core fonts/maps are under `cola/data`. |
| `pytests/` | Integration tests and fixtures | Includes GRIB, GRIB2, NetCDF, HDF, PDEF, and raw test data. The test runner and bundled `pytests/lib/grads` client use Python 2 syntax and do not run under Python 3 unchanged. |
| `bundle/` | Relocatable bundle builder | Builds into `opengrads/Contents`, gathers libraries/assets, and generates Perl wrappers that set `GADDIR`, `GASCRP`, `GAUDPT`, and related variables. |
| `doc/` and `doc/opengrads/` | Bundled/OpenGrADS web documentation | Mostly generated or mirrored HTML/PHP assets. Core command/function documentation is easier to locate in `cola/doc`. |
| `win32/` | Historical Windows/Cygwin packaging | Contains installer definitions, Xming runtime assets, fonts, and binary payloads. It is large and usually unrelated to Linux core work. |
| `darwin/` | Historical macOS packaging | Package project, wrapper scripts, and installer artwork. |
| `Java/` | Legacy Java graphics/VM work | The top Makefile labels the Java distribution path “No Longer Maintained.” Do not use it as the basis for new architecture. |

Useful documentation entry points:

- Descriptor files: `cola/doc/descriptorfile.html`
- Self-describing files: `cola/doc/SDFdescriptorfile.html`
- Coordinates and dimensions: `cola/doc/coordinate.html`, `cola/doc/dimenv.html`
- Expressions/functions: `cola/doc/expressions.html`, `cola/doc/functions.html`
- Display and graphics: `cola/doc/display.html`, `cola/doc/graphics.html`
- Plug-ins/extensions: `NEWS`, `extensions/README`, `extensions/BUILD`, and `extensions/hello/`

## Runtime architecture

The normal grid request follows this path:

```text
grads main loop (grads.c)
  -> command parser (gauser.c:gacmd)
     -> open/sdfopen/xdfopen/bpopen
        -> descriptor or self-describing metadata
        -> gafile + gavar structures
     -> display expression
        -> expression parser/functions (gaexpr.c, gafunc.c)
        -> gagrid request/result
        -> gaggrd (gaio.c)
           -> whole-grid NetCDF fast path, or rows through gagrow/garrow
           -> format backend (binary, indexed GRIB, NetCDF, HDF4, HDF5, BP5, ...)
        -> gaplot (gagx.c)
           -> contour/shade/vector/grid routine
           -> gx coordinate/projection layer
           -> dynamically loaded display/print device
```

### Core state and data types

| Type | Defined in | Purpose and constraints |
|---|---|---|
| `struct gacmn` | `cola/src/grads.h` | Session-wide command, dimension, output, file-list, and graphics state. It is passed through much of the command/plot path. |
| `struct gastat` | `cola/src/grads.h` | Expression-evaluation state and result union. Holds the active dimension environment and default file. |
| `struct gafile` | `cola/src/grads.h` | Open dataset metadata and handles: sizes, transforms, variables, native file handles, indexes, buffers, packing, cache, templates, and format flags. New backend lifetime belongs here or behind a pointer owned here. |
| `struct gavar` | `cola/src/grads.h` | Per-variable name, levels, type/packing, offsets, format-specific IDs, and dimension mapping. |
| `struct gagrid` | `cola/src/grads.h` | Evaluated/requested grid: contiguous values, missing mask, two varying dimensions, dimension bounds, coordinate conversion callbacks, and expression metadata. It currently assumes separable one-dimensional coordinate transforms. |
| `struct gastn`/`garpt` | `cola/src/grads.h` | Station requests and linked station reports. Separate from the gridded-data path. |
| `struct gxpsubs`, `gxdsubs` | `cola/src/gx.h` | Function tables loaded from print/display shared libraries by `gxsubs.c`. |

### File and command responsibilities

| File | Main responsibility | Likely modification reason |
|---|---|---|
| `cola/src/grads.c` | Process startup, command-line options, plug-in table parsing, main loop | New top-level option or startup configuration only. |
| `cola/src/gauser.c` | User command parser, open/close/reinit, display evaluation, queries, output commands | Owns `bpopen` dispatch and shared backend cleanup hooks. Keep parsing thin. |
| `cola/src/gaddes.c` | `.ctl` descriptor parser and `gafile` population | Parses explicit `dtype bp5`/`dtype adios2` descriptors and native BP array-dimension mappings. |
| `cola/src/gaadios.c` | ADIOS2 BP5 metadata discovery, backend lifetime, and typed row and bulk X/Y reads | Extend BP5 attributes, selections, supported array kinds, or diagnostics. |
| `cola/src/gasdf.c` | NetCDF/HDF self-describing open and metadata inference | Reference for metadata conventions; BP5 self-discovery belongs in `gaadios.c`. |
| `cola/src/gaio.c` | Grid allocation, bounds handling, row dispatch, native reads, caches | Dispatches BP5 bulk X/Y reads and row fallbacks alongside legacy formats; first refactor file-scope request state if adding threads. |
| `cola/src/gaexpr.c` | Expression parsing and grid arithmetic | CPU/vector expression acceleration after profiling. |
| `cola/src/gafunc.c` | Built-in scientific functions | Add/accelerate a built-in function; preserve masks and dimension compatibility. |
| `cola/src/gagx.c` | Plot-type dispatch, scaling, contour/shade/vector preparation | Add topology-aware plot dispatch or a raster fast path. |
| `cola/src/gxcntr.c` | Contour construction | Contour performance or curvilinear-cell contour behavior. |
| `cola/src/gxshad.c`, `gxshad2.c` | Shaded polygon construction | Shading performance/topology changes. `gxshad2b` intentionally makes smaller polygons and is slower. |
| `cola/src/gxsubs.c` | Coordinate pipeline and dynamic graphics devices | Device loading or low-level coordinate transformation. |
| `cola/src/gxC.c`, `gxX.c`, `gxX11.c`, `gxprint.c`, `gxGD.c` | Cairo/X11/print/GD backends | Output-device changes, not data-format work. |
| `cola/src/galloc.c` | Allocation wrappers | Instrumentation for memory profiling; avoid bypassing without reason. |
| `cola/configure.ac`, `cola/m4/` | Optional dependency detection and feature macros | Contains optional serial ADIOS2 detection; use it for further dependency behavior changes. |
| `cola/src/Makefile.am` | Canonical core sources and link flags | Registers `gaadios.c` and ADIOS2 flags conditionally. Regenerate Autotools outputs deliberately. |

### Important architectural limitations

- `gagrid` exposes only two varying dimensions per result.
- Its I/J coordinates are separable scalar functions (`i -> x`, `j -> y`). A cubed-sphere or general curvilinear grid needs `lon(i,j,face)` and `lat(i,j,face)`, so it cannot be represented faithfully by only swapping the existing callbacks.
- `gaio.c` keeps active request pointers (`pfi`, `pgr`, `pvr`) and caches in file-scope globals. Parallel reads through the current entry points are unsafe.
- Graphics scaling also stores callbacks and transformation state globally in `gxsubs.c`/`gagx.c`.
- Existing PDEF support (`gaddes.c` and `gaio.c`) interpolates projected/native data to a GrADS grid. It is useful as an early compatibility path but is not native complex-grid topology.

## Extension development

Use `extensions/hello` for a new dynamically loaded command/function:

- `libhello.c` shows the API-0 command and expression-function signatures.
- `hello.udxt` maps public names to exported symbols and a `.gex` library.
- `hello/GNUmakefile` delegates common compilation to `extensions/gex.mk`.

OpenGrADS UDX is suitable for optional scientific functions and commands. It is not the best location for a first-class storage backend because the core owns open-file lifetime, expression requests, and row/grid reads.

Graphics devices are a separate COLA plug-in mechanism. `etc/udpt` registers shared libraries, and `gxsubs.c` fills the `gxdsubs`/`gxpsubs` function tables with `dlopen`/`dlsym`.

## Remaining roadmap

### Build baseline

Complete these before broader performance or topology work.

1. Add a CI/container recipe for a full supplemental-library build and a reduced headless build.
2. Conditionally omit `libgxdX11.la` when X11 headers are absent so the ordinary full target can also serve a headless build.
3. Fix the top-level call to use `./oga_configure`, or document/implement a normal top-level `configure` entry point.
4. Port the integration harness to Python 3 or replace the minimum core suite.
5. Add debug/ASan/UBSan build instructions and run the standard model smoke test.
6. Add benchmark scripts for open, read, expression evaluation, contour, shaded plot, and image output.

Acceptance: a clean checkout has one documented full-build job, one headless job, automated smoke tests, and repeatable baseline timings.

### Further BP5 work

Direct serial BP5 opening, attribute metadata, bulk X/Y reads, invalid-input
coverage, lifecycle tests, and sanitizer tests are implemented and documented
above. Remaining work, in order:

1. Add support for selected local-array layouts with explicit decomposition
   semantics and reproducible fixtures.
2. Add templates only after defining how GrADS T/E selection maps to BP files,
   ADIOS2 steps, and explicit array dimensions.
3. Evaluate streaming engines separately from random access; they require a
   different session and step-lifetime model.
4. Consider higher-rank descriptor-free inference only with an explicit,
   testable metadata convention.

The supported data path remains host-resident `gadouble`; keep that buffer
contract until profiling demonstrates that a different contract is necessary.

Current ADIOS2 references:

- [ADIOS2 2.12 installation and C bindings](https://adios2.readthedocs.io/en/v2.12.0/setting_up/setting_up.html)
- [`adios2-config` flags for non-CMake builds](https://adios2.readthedocs.io/en/v2.12.0/ecosystem/utilities.html)
- [Read versus `ReadRandomAccess` and step selection](https://adios2.readthedocs.io/en/v2.11.0/components/components.html)
- [ADIOS2 full C API](https://adios2.readthedocs.io/en/v2.12.0/api_full/api_full.html)


### CPU I/O and plotting performance

Profile first. Likely hotspots are row dispatch/conversion in `gaio.c`, expression loops in `gaexpr.c`/`gafunc.c`, and polygon/topology work in `gxcntr.c`, `gxshad.c`, and `gxshad2.c`.

Priorities:

1. Use whole-slab reads for NetCDF/HDF5 when the requested I/J dimensions are contiguous.
2. Remove avoidable temporary allocations and repeated type conversion in hot loops.
3. Add request-local backend state and make pure numerical kernels take explicit buffers, shapes, and masks.
4. Vectorize or parallelize only pure kernels. Start with OpenMP/SIMD over independent expression cells or raster color classification, not global command/I/O entry points.
5. Preserve deterministic missing masks and compare output arrays/polygons, not just timings.
6. Separate data preparation from device drawing so CPU kernels remain independently testable and optimizable.

Useful benchmark groups:

- cold and warm dataset open;
- full field, subregion, one-row, and one-point read;
- simple arithmetic and representative built-in functions;
- contour, `shaded`, `shaded2`, and `shaded2b`;
- raster output at fixed pixel sizes;
- regular versus projected/PDEF data.

Acceptance: benchmark data and commands are checked in, output equivalence is automated, and each optimization reports speedup plus memory impact on at least one small and one large case.

### Cubed-sphere and complex grids

There are two useful levels of support.

#### Compatibility level

Read each face and regrid/interpolate to a regular lat/lon GrADS grid, using or extending PDEF. This enables existing expressions and plots quickly but loses native cells, may blur face seams, and costs interpolation time.

#### Native topology level

Extend the result model rather than pretending the grid is separable X/Y:

1. Add a grid-topology enum to `gagrid`/dataset metadata: rectilinear, curvilinear structured, cubed-sphere faces, and later unstructured if needed.
2. Store or reference 2-D center coordinates and preferably cell-corner coordinates. For cubed sphere, include face ID and edge-neighbor/orientation metadata.
3. Keep face topology separate from the ensemble dimension even if a producer stores `face` as an array dimension.
4. Generalize grid-to-world conversion from separate `i -> x` and `j -> y` callbacks to a request-aware `(i,j,face) -> (lon,lat)` operation.
5. Make plotting iterate cells/faces and split geometry at seams before map projection. Avoid connecting the last column of one face to the first column of an unrelated face.
6. Define vector rotation from face-local components to east/north before using the existing vector plot routines.
7. Decide expression semantics at face boundaries. Stencils, derivatives, smoothing, and contour following need neighbor topology, not six unrelated arrays.
8. Add analytic tests for face corners/edges, global coverage, seam continuity, vector orientation, and missing cells. Include at least one scalar and one vector field.

`gxgrid(gaconv)` already provides a grid-coordinate transformation hook, but `gaconv` currently applies independent I and J conversions stored in global variables. It can inspire the call site, not serve as the complete curvilinear implementation.

Acceptance: all six faces render without false seam connections or holes, scalar values agree at shared boundaries, vector direction is correct, and a reference regridded plot is available for comparison.

## Suggested feature branch sequence

The BP5 hardening work is complete in this revision. Keep future changes
reviewable in this order:

1. `build-baseline`: optional Cairo/X11 fixes, headless CI, and modern core
   smoke tests.
2. `backend-interface`: explicit backend lifecycle/read operations with
   existing formats unchanged.
3. `perf-baseline`: benchmark harness and profiles.
4. `bulk-cpu`: bulk reads for remaining formats and pure CPU kernel
   extraction/optimization.
5. `grid-topology`: curvilinear/cubed-sphere data model and scalar native
   rendering.
6. `grid-vectors`: face-neighbor stencils and vector rotation.

Keep topology work separate from storage-backend changes; they share buffers
and metadata but have different correctness criteria and failure modes.

## Definition of done

A core feature is complete when:

- configure detects or cleanly disables its dependencies;
- full and headless builds still work;
- open/read/close and failure cleanup are tested;
- masks, dimension order, coordinate transforms, and type conversion are tested;
- existing `model.ctl` display behavior is unchanged;
- new commands and metadata conventions are documented;
- performance claims include reproducible commands and output-equivalence checks;
- this file is updated if architectural assumptions changed.
