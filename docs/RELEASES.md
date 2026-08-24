# Self-contained binary releases

The modern release path produces archives that users unpack and run directly.
ADIOS2, ncurses, Readline, Cairo/X11, OpenMP runtime support, and every other
non-glibc shared-library dependency discovered at build time are included.
Users do not install ADIOS2 or set library paths.

## End-user workflow

Download the archive matching the machine architecture, verify its adjacent
`.sha256` file, and run:

```bash
sha256sum -c opengrads-hpc-1.0.0-linux-ARCH.tar.gz.sha256
tar -xzf opengrads-hpc-1.0.0-linux-ARCH.tar.gz
cd opengrads-hpc-1.0.0-linux-ARCH
./opengrads
```

With an X display, the launcher selects the Cairo graphical plug-ins. Without
an X display it uses the bundled noninteractive device. `./opengrads -j N`
selects the calculation CPU count; the default remains four.

The release matrix is:


- Linux x86_64, built natively on Ubuntu 22.04;
- Linux aarch64, built natively on Ubuntu 22.04 ARM64;
- macOS arm64, built natively on macOS 15;
- macOS x86_64, built natively on macOS 15 Intel;


### glibc baseline

Linux archives are built inside `manylinux_2_28` (AlmaLinux 8, **glibc 2.28**)
rather than on the Ubuntu 22.04 runner (glibc 2.35). glibc is forward
compatible only: a binary built against 2.35 will not start on anything older,
and the RHEL 8 family that HPC clusters still run is on 2.28. Building on the
runner produced archives that failed on exactly the machines this project
targets.

glibc, the dynamic loader, and the UDUNITS unit database are bundled too, so
the archive depends on nothing but the kernel. The launcher compares the
host's glibc against `glibc-required.txt` and starts through the bundled
loader only when the host is older; otherwise it uses the host's glibc.

That preference matters. glibc's NSS `dlopen`s the host's `libnss_*` modules
at runtime, and a much newer bundled glibc cannot load them, which breaks
network name resolution and therefore OPeNDAP. Local data is unaffected.
Tested on glibc 2.28, 2.27, and 2.24: everything works on all three except
OPeNDAP on 2.24.

The bundled glibc is never exported through `LD_LIBRARY_PATH`; it is passed to
the loader as `--library-path` so it cannot leak into subprocesses such as
shell escapes, which would fail to relocate against the host loader.

macOS archives are native builds, not cross-compiled Linux archives, and
start with `./opengrads`.

### Graphics drivers per platform

Only Linux ships an interactive display driver. GrADS graphics plug-ins are
loaded with `dlopen()` and call back into symbols defined in the `grads`
executable, which constrains what each platform can carry:

| Platform | Display (`-d`) | Hardcopy (`-h`) |
| --- | --- | --- |
| Linux | `Cairo`, `X11`, `gxdummy` | `Cairo`, `gxdummy` |
| macOS | `gxdummy` | `Cairo`, `gxdummy` |

macOS archives therefore run headless but keep the full Cairo hardcopy path,
so `printim` and `print` produce PNG, PS, PDF, and SVG output without
XQuartz. Both launchers pass the drivers their archive actually carries, so
`./opengrads` and `opengrads.cmd` work without extra flags; anything the
caller passes still wins.

### Windows status

Windows is **not built by CI and not published**. The builder scripts
(`release/build-release-windows.sh`, `release/package-windows.sh`) are kept
because the work is mostly done, but two things remain:

- The MSYS2 package is named `mingw-w64-x86_64-libgeotiff`, not
  `mingw-w64-x86_64-geotiff`. The scripts are correct; the CI install list was
  not, which is what stopped the first run.
- A native MinGW build of GrADS is unproven. There are no `_WIN32` guards
  anywhere in `cola/src`, and the historical port went through Cygwin, so
  portability errors beyond the package name are likely.

Even once it builds, Windows would be compute-and-query only. On PE targets a
DLL cannot leave symbols undefined at link time, so the Cairo plug-ins — which
call `gxdb*` routines that live in `grads.exe` — do not link natively under
MinGW; only the self-contained `gxdummy` driver does. Adding Windows hardcopy
means exporting the executable's symbols (`-Wl,--export-all-symbols
-Wl,--out-implib`) and linking the plug-ins against that import library, or
returning to a Cygwin port as the historical OpenGrADS Windows builds did.

## Two version numbers

Archives are named for the distribution, not for GrADS:

- `release/VERSION` holds the **distribution version** (`1.0.0`). It names the
  archives and must match the release tag — the publish job fails if
  `vX.Y.Z` and `release/VERSION` disagree.
- `AC_INIT` in `cola/configure.ac` holds the **GrADS baseline**
  (`2.2.1.oga.1`). It feeds `GRADS_VERSION`, so `grads` keeps reporting the
  GrADS release its behaviour matches. Do not repurpose it as a fork version:
  a bare `1.0.0` there would make the banner read "GrADS Version 1.0.0", and
  GrADS 1.x genuinely existed.

Every archive carries a `VERSION` file recording both. To cut a release, bump
`release/VERSION`, update `docs/RELEASE_NOTES.md`, commit, then tag `vX.Y.Z`.

## Maintainer one-command build

On Linux, install a compiler, CMake, curl, standard Autotools, and the Cairo,
X11, GeoTIFF, and optional HDF5 development headers. Then run:

```bash
./release/build-release.sh
```

The builder downloads checksum-pinned ADIOS2 2.11.0, ncurses 6.5, and Readline
8.2 source archives; builds them into `.release-work`; builds opengrads-hpc with
ADIOS2, OpenMP, and NetCDF/UDUNITS support required; runs the BP5, SDF, and
OpenMP regressions; assembles the runtime closure; and writes the archive and
checksum to `release-dist`.
Nothing is installed system-wide.

For offline validation with existing private prefixes:

```bash
OPENGRADS_RELEASE_ADIOS2_ROOT=/path/to/adios2 \
OPENGRADS_RELEASE_DEPS_ROOT=/path/to/readline-and-ncurses \
  ./release/build-release.sh
```

`release/versions.env` is the dependency lock file. Update a version, URL, and
SHA-256 together and re-run both architecture jobs before accepting a change.

## Native macOS build

The macOS builder uses Homebrew dependencies and must run on a Mac:

```bash
brew install adios2 autoconf automake cairo coreutils gcc libgeotiff hdf5 \
  libomp libtool netcdf pkgconf readline udunits
./release/build-release-macos.sh
```

The Windows builder is not currently exercised; see **Windows status** above.

`sdfopen` and `xdfopen` need the legacy UDUNITS 1 API (`utInit`, `utScan`).
UDUNITS-2 ships that API and its `udunits.h` compatibility header, so every
platform uses UDUNITS-2 — `libudunits2-dev` on Linux, the `udunits` formula on
macOS. An earlier attempt to build UDUNITS 1.12.11 from source was dropped: it
fails against modern bison on Linux and on the Fortran probe on macOS, and it
was never necessary. The builders run the BP5, SDF, and OpenMP regressions
before packaging their archive.

The macOS packager rewrites every bundled Mach-O install name to `@rpath` and
re-signs the result, because editing a Mach-O header invalidates the ad-hoc
signature that arm64 macOS requires. Its smoke test runs the archive under
`env -i` and asserts that Cairo wrote a real PNG, which proves the bundle does
not reach back into the Homebrew prefix it was built from.

## GitHub Actions and publication gate

`.github/workflows/release.yml` builds and tests all four native archives on
a version tag or manual dispatch. Binary artifact upload and GitHub Release
creation occur only when the repository variable
`BINARY_REDISTRIBUTION_APPROVED` is exactly `true`.

That variable is the deliberate control over binary publication. GrADS is
treated as GPL-2.0-only while ADIOS2 is Apache-2.0, a combination the FSF and
ASF both describe as incompatible, and this project publishes those binaries
with the question documented rather than resolved. GNU Readline is GPLv3 and
raises the same question independently. See
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for the full position.

Turning the gate off is how you pause binary publication -- for example if a
copyright holder objects, or while a compatibility grant is pending. Builds
and tests still run on every platform when it is off; only upload and release
creation stop.
