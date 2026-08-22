## opengrads-hpc 1.0.0

GrADS for modern simulation output: an ADIOS2/BP5 reader, OpenMP-threaded
calculations, and native archives for Linux, macOS, and Windows.

This is the first release under the `opengrads-hpc` name and its own version
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
- **Five native archives**, each self-contained: Linux x86_64 and aarch64,
  macOS arm64 and x86_64, Windows x86_64. No dependency installation and no
  library paths to set.

### Graphics drivers per platform

Plug-ins are loaded with `dlopen()` and call back into the `grads` executable,
which constrains what each platform can carry:

| Platform | Display (`-d`) | Hardcopy (`-h`) |
| --- | --- | --- |
| Linux | `Cairo`, `X11`, `gxdummy` | `Cairo`, `gxdummy` |
| macOS | `gxdummy` | `Cairo`, `gxdummy` |
| Windows | `gxdummy` | `gxdummy` |

macOS runs headless but keeps the full Cairo hardcopy path, so `printim` and
`print` produce PNG, PS, PDF, and SVG without XQuartz. Windows is
compute-and-query only this release; see `docs/RELEASES.md` for why and what
enabling it would take.

### Verifying and running

```bash
sha256sum -c opengrads-hpc-1.0.0-linux-x86_64.tar.gz.sha256
tar -xzf opengrads-hpc-1.0.0-linux-x86_64.tar.gz
cd opengrads-hpc-1.0.0-linux-x86_64
./opengrads
```

On macOS start `./opengrads`; on Windows run `opengrads.cmd` after extracting
the ZIP. Both launchers select the drivers their archive actually ships, so no
extra flags are needed.

### Known limitations

- No Windows graphics output.
- Linux archives need a glibc at least as new as the Ubuntu 22.04 build
  baseline; glibc and the dynamic loader are deliberately not bundled.
- Reading BP5 written by a multi-rank MPI job is supported by ADIOS2's format
  but is not yet covered by the regression suite.
