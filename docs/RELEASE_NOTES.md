## opengrads-hpc 1.0.2

GrADS for modern simulation output: an ADIOS2/BP5 reader, OpenMP-threaded
calculations, and native archives for Linux and macOS.

### Fixed since 1.0.1

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
sha256sum -c opengrads-hpc-1.0.2-linux-x86_64.tar.gz.sha256
tar -xzf opengrads-hpc-1.0.2-linux-x86_64.tar.gz
cd opengrads-hpc-1.0.2-linux-x86_64
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
