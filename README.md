# opengrads-hpc

opengrads-hpc is a maintained fork of OpenGrADS 2.2.1.oga.1, GrADS built for
modern simulation output. It keeps the interactive GrADS workflow — the same
descriptors, commands, and `.gs` scripts — while adding an ADIOS2/BP5 reader,
OpenMP-threaded calculations, and self-contained releases for Linux, macOS,
and Windows.

**"HPC" means the ADIOS2/BP5 engine plus OpenMP threading on a single node.**
There is no MPI or GPU support and none is planned; ADIOS2 is deliberately
built with `ADIOS2_USE_MPI=OFF`. Data written by a multi-rank MPI job is read
serially.

Releases are versioned independently of GrADS: archives are named
`opengrads-hpc-<version>`, while `grads` still reports the GrADS baseline it
matches (`2.2.1.oga.1`). Each archive carries a `VERSION` file with both. See
[release documentation](docs/RELEASES.md).

## What is GrADS?

The Grid Analysis and Display System (GrADS) is an interactive environment for
working with gridded and station earth-science data. It lets users open data,
select a longitude/latitude/level/time domain, evaluate expressions, and make
maps, contours, vectors, time series, and other scientific plots. GrADS also
has a scripting language for repeatable analysis and batch graphics.

This fork retains the familiar GrADS data model and commands, including support
for common binary, GRIB, NetCDF, HDF, and BUFR workflows when their optional
libraries are available.

## What this project adds

- **ADIOS2 BP5 reader.** Open supported BP5 output directly with
  `bpopen /path/to/data.bp`, without writing a CTL file, or use an explicit
  descriptor with `dtype bp5`. Coordinate, variable, unit, title, and
  missing-value metadata are discovered where available. See
  [BP5 documentation](docs/BP5.md).

- **OpenMP calculation engine.** Common grid arithmetic, functions, and
  reductions can use multiple CPU cores. The default is four calculation
  threads; control it with `set threads N`, `./opengrads -j N`, or
  `GA_NUM_THREADS=N`. See [performance notes](docs/PERFORMANCE.md).

- **Restored self-describing-file access.** Release builds require NetCDF and
  UDUNITS support, so `sdfopen` and `xdfopen` remain available for compatible
  NetCDF and HDF data instead of disappearing from the command set. Confirm
  enabled features with `q config`.

- **Advanced scripts and extensions.** The bundled OpenGrADS/Kodama script
  collection in [lib/scripts](lib/scripts) provides reusable plotting and
  analysis tools, including maps, panels, colour bars, meteorograms,
  trajectories, and interpolation helpers. Existing OpenGrADS extensions are
  retained under [extensions](extensions).

- **Modern interactive and release experience.** Optional BSD libedit adds
  command history and Tab completion. The release builders produce five
  self-contained archives — Linux x86_64 and aarch64, macOS arm64 and x86_64,
  and Windows x86_64 — carrying BP5, OpenMP, the graphics plug-ins available
  on that platform, and every required runtime library.

## Quick start

For a packaged release, follow [RELEASES.md](docs/RELEASES.md). For a
source build, follow [INSTALL.md](docs/INSTALL.md). Once started, a BP5 session
can look like this:

```text
ga-> bpopen /path/to/output.bp
ga-> q vars
ga-> set t 1
ga-> set z 1
ga-> set threads 8
ga-> set gxout shaded
ga-> display temperature
```

Use `q config` to see whether this build includes `adios2-bp5`, `openmp`,
`netcdf`, graphics devices, and other optional components.

## Documentation

- [Installation guide](docs/INSTALL.md)
- [Release and packaging guide](docs/RELEASES.md)
- [BP5 reader guide and limitations](docs/BP5.md)
- [OpenMP controls and benchmark notes](docs/PERFORMANCE.md)
- [Architecture and development roadmap](docs/ARCHITECTURE.md)
- [Detailed BP5 change record](docs/CHANGES-BP5.md)

## Legacy OpenGrADS documentation

The [pre-fork OpenGrADS README](https://github.com/Aaron-Hsieh-0129/opengrads-hpc/blob/3c0ea22b3c592ccfcc876da442da61a37bb1d23c/README)
is retained in the repository history for the original project overview and
older installation guidance. The historical [BUILD](BUILD) and [INSTALL](INSTALL)
files are also preserved, but the documentation above describes this fork's
supported build and release process.

## License and notices

OpenGrADS license and copyright terms are in [COPYING](COPYING) and
[COPYRIGHT](COPYRIGHT). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for dependency attribution and binary-redistribution cautions.
