# Publishing this fork

This checklist prepares the source tree for a GitHub repository. It does not upload, push, tag, or publish anything.

## Recommended publication scope

Publish the modified source tree, tests, documentation, and existing OpenGrADS license files. Do not publish binaries linked with ADIOS2 or a modern GNU Readline release until a qualified review or permission from the relevant copyright holders resolves the license-compatibility questions described in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

The repository itself does not vendor ADIOS2 or Readline. Users obtain those optional libraries separately.

This is a conservative engineering checklist, not legal advice.

## Files to keep

- `COPYING` and `COPYRIGHT`: the existing GPL version 2-only terms and notices.
- `THIRD_PARTY_NOTICES.md`: optional dependency attribution and compatibility warning.
- `AGENTS.md`: architecture, verified build record, and remaining roadmap.
- `docs/BP5.md`: portable BP5 build, usage, testing, and limitations.
- `docs/CHANGES-BP5.md`: reviewer-facing summary of this development revision.
- Source and generated Autotools changes under `cola/`.
- `pytests/bp5_writer.c`, `pytests/TestBP5.sh`, and `pytests/data/bp5_fixture.ctl`.

Do not commit local ADIOS2 installations, `/tmp` build trees, generated BP fixture directories, RCEMIP data, personal environment files, or built libraries/executables.

## Before creating the GitHub repository

Run from the repository root:

```bash
git status --short
git diff --check
git diff --stat
git diff -- COPYING COPYRIGHT
find . -type f -size +50M -not -path './.git/*' -print
```

Review every untracked file. Confirm that no credentials, tokens, SSH keys, private URLs, machine-specific data, or restricted scientific datasets are present.

The generated `cola/configure`, `cola/src/Makefile.in`, and `cola/src/config.h.in` are intentionally included so a user does not need Autoconf just to configure the checkout. Their diff is large because they were regenerated with Autoconf 2.71/Automake after the canonical `configure.ac` and `Makefile.am` changes.

## Reproduce the checks

Build and test the normal BP5 configuration as described in [BP5.md](BP5.md), then run:

```bash
cd /path/to/opengrads-Grads/pytests
OPENGRADS_BUILD_ROOT=/tmp/opengrads-build-cpu \
OPENGRADS_ADIOS2_ROOT=/opt/adios2-cpu \
  ./TestBP5.sh
```

Also build the reduced ADIOS2-disabled configuration:

```bash
mkdir -p /tmp/opengrads-build-noadios/src /tmp/opengrads-build-noadios/lib
cd /tmp/opengrads-build-noadios
/path/to/opengrads-Grads/cola/configure \
  --disable-dyn-supplibs --with-opengrads --without-gadap --without-gui
make -C src -j4 grads libgxdummy.la
```

For a release candidate, repeat the sanitizer run in [BP5.md](BP5.md). Record the compiler, ADIOS2 version, configure command, test output, and platforms in the GitHub release notes.

## Suggested commit structure

A single development commit is acceptable, but these review units are easier to understand:

1. `build: add optional serial ADIOS2 detection`
2. `bp5: add descriptor and descriptor-free reader`
3. `bp5: add attributes and bulk XY reads`
4. `cli: add Readline completion and colored launcher`
5. `test: add generated BP5 regression fixture`
6. `docs: document architecture, BP5 use, and publication constraints`

Do not rewrite upstream copyright statements. Add your own authorship or copyright notice only when you know the correct person or organization that owns the contribution.

## Suggested repository description

> OpenGrADS 2.2.1 development fork with optional serial ADIOS2 BP5 input, descriptor-free `bpopen`, metadata/missing-value support, bulk XY reads, and regression tests.

## Suggested release notes

> This source development revision adds optional CPU-only ADIOS2 BP5 input to OpenGrADS. BP5 datasets can be opened directly with `bpopen` or through a `dtype bp5` descriptor. The reader supports global real numeric arrays, ADIOS2 step selection, coordinate discovery, descriptive and missing-value attributes, and bulk in-bounds XY selections. It also adds Readline command completion, a color-enabled development launcher, and generated BP5 integration tests. See `docs/BP5.md` for prerequisites and limitations. Source publication only is recommended pending resolution of optional-library license compatibility for redistributed linked binaries.

## After publishing

- Put the license identifier `GPL-2.0-only` in the GitHub description or metadata; do not label the entire repository Apache-2.0.
- Link the README to `COPYING`, `docs/BP5.md`, and `THIRD_PARTY_NOTICES.md`.
- State that ADIOS2 and Readline are optional, separately installed dependencies.
- Use GitHub Issues for unsupported BP shapes, including `bpls -la` output and a minimal non-sensitive reproducer.
- Never attach the RCEMIP dataset unless its data owner and license explicitly allow redistribution.
