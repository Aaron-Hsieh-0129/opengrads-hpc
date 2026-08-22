# Self-contained binary releases

The modern release path produces archives that users unpack and run directly.
ADIOS2, ncurses, Readline, Cairo/X11, OpenMP runtime support, and every other
non-glibc shared-library dependency discovered at build time are included.
Users do not install ADIOS2 or set library paths.

## End-user workflow

Download the archive matching the machine architecture, verify its adjacent
`.sha256` file, and run:

```bash
sha256sum -c opengrads-2.2.1.oga.1-linux-ARCH.tar.gz.sha256
tar -xzf opengrads-2.2.1.oga.1-linux-ARCH.tar.gz
cd opengrads-2.2.1.oga.1-linux-ARCH
./opengrads
```

With an X display, the launcher selects the Cairo graphical plug-ins. Without
an X display it uses the bundled noninteractive device. `./opengrads -j N`
selects the calculation CPU count; the default remains four.

The initial release matrix is:

- Linux x86_64, built natively on Ubuntu 22.04;
- Linux aarch64, built natively on Ubuntu 22.04 ARM64.

Using Ubuntu 22.04 establishes an older glibc baseline than newer runner
images. glibc itself and the dynamic loader are deliberately not bundled;
release archives therefore require a glibc-based Linux system at least as new
as the build baseline. macOS and Windows need separate native packagers and
are not yet claimed by this workflow.

## Maintainer one-command build

On Linux, install a compiler, CMake, curl, standard Autotools, and the Cairo,
X11, GeoTIFF, and optional HDF5 development headers. Then run:

```bash
./release/build-release.sh
```

The builder downloads checksum-pinned ADIOS2 2.11.0, ncurses 6.5, and Readline
8.2 source archives; builds them into `.release-work`; builds OpenGrADS with
ADIOS2 and OpenMP required; runs the BP5 and OpenMP regressions; assembles the
runtime closure; and writes the archive and checksum to `release-dist`.
Nothing is installed system-wide.

For offline validation with existing private prefixes:

```bash
OPENGRADS_RELEASE_ADIOS2_ROOT=/path/to/adios2 \
OPENGRADS_RELEASE_DEPS_ROOT=/path/to/readline-and-ncurses \
  ./release/build-release.sh
```

`release/versions.env` is the dependency lock file. Update a version, URL, and
SHA-256 together and re-run both architecture jobs before accepting a change.

## GitHub Actions and publication gate

`.github/workflows/release.yml` builds and tests native x86_64 and aarch64
archives on a version tag or manual dispatch. Binary artifact upload and
GitHub Release creation occur only when the repository variable
`BINARY_REDISTRIBUTION_APPROVED` is exactly `true`.

Do not set that variable merely because the build passes. OpenGrADS is treated
as GPL-2.0-only, while ADIOS2 is Apache-2.0 and current Readline is GPLv3. The
current project notices identify unresolved linked-binary compatibility. The
gate may be enabled only after permission, a valid relicensing/exception, or
qualified legal advice establishes a distributable combination. Local builds
and tests do not themselves authorize public redistribution.
