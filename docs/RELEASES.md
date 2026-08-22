# Self-contained binary releases

The modern release path produces archives that users unpack and run directly.
ADIOS2, ncurses, libedit, Cairo/X11, OpenMP runtime support, and every other
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
- Windows x86_64, built natively with MSYS2/MinGW64.


Using Ubuntu 22.04 establishes an older glibc baseline than newer runner
images. glibc itself and the dynamic loader are deliberately not bundled;
release archives therefore require a glibc-based Linux system at least as new
as the build baseline. macOS and Windows are native builds, not cross-compiled
Linux archives. The macOS archive starts with `./opengrads`; Windows users
start `opengrads.cmd` after extracting the ZIP.

### Graphics drivers per platform

Only Linux ships an interactive display driver. GrADS graphics plug-ins are
loaded with `dlopen()` and call back into symbols defined in the `grads`
executable, which constrains what each platform can carry:

| Platform | Display (`-d`) | Hardcopy (`-h`) |
| --- | --- | --- |
| Linux | `Cairo`, `X11`, `gxdummy` | `Cairo`, `gxdummy` |
| macOS | `gxdummy` | `Cairo`, `gxdummy` |
| Windows | `gxdummy` | `gxdummy` |

macOS archives therefore run headless but keep the full Cairo hardcopy path,
so `printim` and `print` produce PNG, PS, PDF, and SVG output without
XQuartz. Both launchers pass the drivers their archive actually carries, so
`./opengrads` and `opengrads.cmd` work without extra flags; anything the
caller passes still wins.

Windows is currently compute-and-query only. On PE targets a DLL cannot leave
symbols undefined at link time, so the Cairo plug-ins — which call `gxdb*`
routines that live in `grads.exe` — do not link natively under MinGW. Only
the self-contained `gxdummy` driver does. Adding Windows hardcopy means
exporting the executable's symbols (`-Wl,--export-all-symbols
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

The builder downloads checksum-pinned ADIOS2 2.11.0, ncurses 6.5, and libedit
20240808-3.1 source archives; builds them into `.release-work`; builds OpenGrADS with
ADIOS2, OpenMP, and NetCDF/UDUNITS support required; runs the BP5, SDF, and
OpenMP regressions; assembles the runtime closure; and writes the archive and
checksum to `release-dist`.
Nothing is installed system-wide.

For offline validation with existing private prefixes:

```bash
OPENGRADS_RELEASE_ADIOS2_ROOT=/path/to/adios2 \
OPENGRADS_RELEASE_DEPS_ROOT=/path/to/libedit-and-ncurses \
  ./release/build-release.sh
```

`release/versions.env` is the dependency lock file. Update a version, URL, and
SHA-256 together and re-run both architecture jobs before accepting a change.

## Native macOS and Windows builds

The macOS builder uses Homebrew dependencies and must run on a Mac:

```bash
brew install adios2 autoconf automake cairo coreutils gcc geotiff hdf5 \
  libomp libtool netcdf pkgconf
./release/build-release-macos.sh
```

The Windows builder runs from an MSYS2 **MINGW64** shell:

```bash
./release/build-release-windows.sh
```

Both builders compile a private, checksum-pinned UDUNITS 1.12.11 dependency so
`sdfopen` and `xdfopen` retain the legacy GrADS API. They run the BP5, SDF, and
OpenMP regressions before packaging their archive.

The macOS packager rewrites every bundled Mach-O install name to `@rpath` and
re-signs the result, because editing a Mach-O header invalidates the ad-hoc
signature that arm64 macOS requires. Its smoke test runs the archive under
`env -i` and asserts that Cairo wrote a real PNG, which proves the bundle does
not reach back into the Homebrew prefix it was built from.

## GitHub Actions and publication gate

`.github/workflows/release.yml` builds and tests all five native archives on
a version tag or manual dispatch. Binary artifact upload and GitHub Release
creation occur only when the repository variable
`BINARY_REDISTRIBUTION_APPROVED` is exactly `true`.

Do not set that variable merely because the build passes. OpenGrADS is treated
as GPL-2.0-only, while ADIOS2 is Apache-2.0. (The Readline half of this problem
was removed in 2026 by switching to BSD libedit.) The current project notices
identify unresolved linked-binary compatibility for ADIOS2. The
gate may be enabled only after permission, a valid relicensing/exception, or
qualified legal advice establishes a distributable combination. Local builds
and tests do not themselves authorize public redistribution.
