# BP5 development revision

Date: 15 August 2026

This document summarizes the changes made after OpenGrADS 2.2.1.oga.1. It is a change record, not a new license; the repository remains under the terms in `COPYING` and `COPYRIGHT`.

## User-visible additions

- `bpopen PATH` opens supported ADIOS2 BP5 data without a user-written CTL.
- A directory containing exactly one `*.bp` child can be passed to `bpopen`.
- `dtype bp5` and `dtype adios2` enable BP5 in explicit descriptors.
- `q config` reports whether ADIOS2 BP5 is enabled and prints its version.
- `q ctlinfo` reports `dtype bp5`.
- Direct discovery reads coordinate variables, dataset title, field descriptions, units, `_FillValue`, and `missing_value`.
- In-bounds X/Y plots use one bulk ADIOS2 selection instead of one request per row.
- GNU Readline builds complete common GrADS commands and common `set`, `query`, and `draw` words with Tab.
- The `opengrads` launcher uses a repository-local build without requiring the historical binary bundle, configures available graphics plug-ins, enables color by default, and supports explicit runtime path overrides.

## Internal changes

- `cola/configure.ac` adds `--with-adios2[=PREFIX]`, validates the serial C API with `adios2-config`, and defines `USEADIOS2`.
- `cola/src/gaadios.c` owns BP5 metadata discovery, handle lifetime, type conversion, masking, row reads, and bulk grid reads.
- `gafile` now carries an explicit BP5 flag and an opaque backend state pointer.
- `gaddes.c` recognizes BP5 descriptors and rejects currently unsupported templates, PDEF, and station data.
- `gauser.c` dispatches `bpopen`, opens descriptor-driven BP5 data, and reports it in queries.
- `gaio.c` dispatches the bulk grid path and retains the BP5 row fallback.
- Descriptor token scanning in `gaddes.c` and `gasdf.c` now stops at NUL as well as whitespace. This fixes an out-of-bounds read found by AddressSanitizer.
- Generated Autotools files were refreshed after their canonical inputs changed.
- Missing optional Cairo or Readline libraries now disable those features cleanly instead of aborting configuration or leaving stale linker flags.
- Aggregate extension builds now stop when a sub-build fails instead of printing a misleading success.

## Tests added

`pytests/bp5_writer.c` generates a small two-step BP5 dataset at test time. `pytests/TestBP5.sh` checks:

- descriptor and descriptor-free opening;
- parent-directory resolution;
- field aliases, title, descriptions, units, and coordinate-unit conversion;
- descriptor precedence over missing-value attributes;
- `_FillValue` and `missing_value` masking;
- float/double conversion and step selection;
- full 2-D statistics and shaded-contour output;
- invalid empty/ambiguous directories and invalid descriptor shapes;
- repeated open, close, and reinit cleanup.

The same test passed in a normal CPU-only build and under AddressSanitizer plus UndefinedBehaviorSanitizer. An ADIOS2-disabled build of `grads` and `libgxdummy.la` also passed.

## Real-data verification

The reader was exercised with `/raid/mog/rcemip_f_tc_bp5/vvm_output.bp`: 35 compatible fields, a 512 x 512 x 44 reference grid, and 21 steps were discovered. A full 512 x 512 `w` plane at T=21/Z=7 and its shaded contour were evaluated successfully.

The RCEMIP data is not part of this repository.

## Deferred work

- ADIOS2 local arrays, complex numbers, MPI reads, and streaming engines.
- BP5 templates and PDEF.
- Descriptor-free inference beyond compatible rank-2/rank-3 global arrays.
- General request-context/thread-safety refactoring.
- Native curvilinear and cubed-sphere topology.
- Reproducible cross-platform performance benchmarks.

See [BP5.md](BP5.md) for usage and [ARCHITECTURE.md](ARCHITECTURE.md) for the architecture and longer roadmap.
