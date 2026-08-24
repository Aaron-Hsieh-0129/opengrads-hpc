## opengrads-hpc 1.0.7

GrADS for modern simulation output: an ADIOS2/BP5 reader, OpenMP-threaded
calculations, and native archives for Linux and macOS.

### Fixed since 1.0.6

- **1.0.6 did not start on modern Linux.** It bundled glibc alongside the
  other libraries, so any host with a glibc newer than 2.28 loaded the bundled
  2.28 `libc.so.6` with its own loader and died with SIGILL before printing
  anything. The bundled glibc now lives in its own directory that is never
  placed on `LD_LIBRARY_PATH`; it is reached only through the bundled loader,
  where loader and libc match. Verified on glibc 2.39, 2.28, 2.27, and 2.24.

### Fixed in 1.0.6

- **The Linux archive now carries everything but the kernel.** glibc, the
  dynamic loader, and the UDUNITS unit database are bundled alongside the 65
  libraries already included. The launcher uses the host's glibc when it is
  new enough and the bundled one only when it is not, so archives now start on
  systems older than the build machine. Verified on glibc 2.28, 2.27, and
  2.24; the previous archive failed to start on the latter two.
- **`sdfopen` no longer needs udunits2 installed on the host.** The archive
  shipped `libudunits2.so` without its unit database, which UDUNITS-2 reads
  from a path compiled into the library. On a machine without udunits2 that
  path does not exist and `sdfopen` failed with `UDUNITS package
  initialization failure`. This affected 1.0.5 and earlier.

### Fixed in 1.0.5

- **Line editing no longer corrupts the display.** The coloured prompt handed
  readline 15 bytes for something 5 columns wide, so it counted the ANSI
  escapes as visible width and every cursor calculation on a wrapped line was
  off by ten columns, leaving stray spaces and fragments of the previous
  command. The escapes are now marked non-printing with readline's
  `RL_PROMPT_START_IGNORE`/`RL_PROMPT_END_IGNORE`.

### Fixed in 1.0.4

- **`dtype hdf5_grid` now reads data.** 1.0.3 could open an HDF5 file but
  failed on every variable with `invalid object ID` and `H5Dopen2 failed`.
  GrADS stored HDF5 object ids in a 32-bit `gaint`, but HDF5 widened `hid_t`
  to 64 bits in version 1.10, so every id was truncated. The ids are now held
  in a 64-bit type. Verified against hdf5-1.10.5.

### Fixed in 1.0.3

- **Linux archives now run on RHEL 8 era systems, including HPC clusters.**
  Earlier releases were built on Ubuntu 22.04 (glibc 2.35) and failed to start
  on anything older with `version 'GLIBC_2.xx' not found`. Linux archives are
  now built in AlmaLinux 8 (glibc 2.28), so they require glibc 2.28 or newer.
- **Restored `-ldl` and `-lpthread` at link time.** A missing automake
  substitution meant `host_runtime_libs` expanded to nothing, so these were
  silently dropped. It went unnoticed because glibc 2.34 merged both into
  libc; on older systems the link fails outright.
- **`sdfopen` on RHEL-family builds.** UDUNITS headers live in
  `/usr/include/udunits2/` there rather than `/usr/include`, so the probe
  missed them.
- **HDF5 was disabled in every earlier Linux archive.** Debian hides its
  headers under `/usr/include/hdf5/serial/`, which the configure probe does
  not search, so `USEHDF5` was 0 and the descriptor
  keyword was rejected with `Data file type invalid`. The AlmaLinux build
  finds HDF5 in the standard location, and source builds on Debian now add
  the multiarch path explicitly.

### Fixed in 1.0.2

- **Prompt colour and Tab completion.** 1.0.0 and 1.0.1 linked BSD libedit,
  which mangles the ANSI escape sequences in the coloured prompt (printing
  `[32mga-> [39m` as literal text) and appends a space after ambiguous
  completions. GNU Readline is restored, so the green prompt and Tab
  behaviour match the earlier `bp5.*` releases again.

### Fixed in 1.0.1

- **Terminal handling at the `ga->` prompt.** The bundled ncurses carried the
  build machine's terminfo path compiled into it. That directory does not
  exist on your machine, so terminal lookup failed and the prompt fell back to
  dumb terminal settings, printing `No entry for terminal type ...`. The
  launcher now points `TERMINFO_DIRS` at the usual system locations. An
  explicit `TERMINFO` or `TERMINFO_DIRS` you set yourself still wins.

1.0.0 was the first release under the `opengrads-hpc` name and its own version
line. Earlier `v2.2.1.oga.1-bp5.*` tags remain available and are unaffected.

**Based on GrADS 2.2.1 / OpenGrADS 2.2.1.oga.1.** Descriptor files, `.gs`
scripts, and command syntax are unchanged, and `grads` still reports the GrADS
baseline it was built from. Each archive carries a `VERSION` file naming both
the distribution version and that baseline.

"HPC" here means the ADIOS2/BP5 engine plus OpenMP threading on a single node.
There is no MPI or GPU support, and none is planned — ADIOS2 is deliberately
built with `ADIOS2_USE_MPI=OFF`.

### What's in it

- **ADIOS2 BP5 input.** Open BP5 datasets through a GrADS descriptor. Handles
  partial `TDEF` from a run that did not finish, dataset attributes, and
  descriptor precedence.
- **OpenMP-threaded calculations.** Defaults to 4 threads; `-j N` or
  `GA_NUM_THREADS` override it, and `q threads` reports the active count.
- **`sdfopen` / `xdfopen`** against NetCDF-4 and HDF5.
- **Four native archives**, each self-contained: Linux x86_64 and aarch64,
  macOS arm64 and x86_64. No dependency installation and no library paths to
  set.

### Graphics drivers per platform

Plug-ins are loaded with `dlopen()` and call back into the `grads` executable,
which constrains what each platform can carry:

| Platform | Display (`-d`) | Hardcopy (`-h`) |
| --- | --- | --- |
| Linux | `Cairo`, `X11`, `gxdummy` | `Cairo`, `gxdummy` |
| macOS | `gxdummy` | `Cairo`, `gxdummy` |

macOS runs headless but keeps the full Cairo hardcopy path, so `printim` and
`print` produce PNG, PS, PDF, and SVG without XQuartz.

A native Windows build is not published yet; see `docs/RELEASES.md` for its
status.

### Verifying and running

```bash
sha256sum -c opengrads-hpc-1.0.7-linux-x86_64.tar.gz.sha256
tar -xzf opengrads-hpc-1.0.7-linux-x86_64.tar.gz
cd opengrads-hpc-1.0.7-linux-x86_64
./opengrads
```

On macOS start `./opengrads`. The launcher selects the drivers its archive
actually ships, so no extra flags are needed.

### Known limitations

- No Windows build in this release.
- Linux archives need a glibc at least as new as the Ubuntu 22.04 build
  baseline; glibc and the dynamic loader are deliberately not bundled.
- Reading BP5 written by a multi-rank MPI job is supported by ADIOS2's format
  but is not yet covered by the regression suite.
