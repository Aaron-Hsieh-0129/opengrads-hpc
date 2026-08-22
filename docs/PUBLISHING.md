# Publishing this fork

This checklist prepares the source tree for a GitHub repository. It does not upload, push, tag, or publish anything.

## Recommended publication scope

Publish the modified source tree, tests, documentation, and existing OpenGrADS license files, together with the binary archives produced by the release workflow.

Binary archives contain ADIOS2, whose Apache-2.0 license raises an unresolved compatibility question against GrADS's GPL-2.0-only terms. That question is documented in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) and shipped inside every archive rather than concealed; a compatibility grant is being sought from the relevant copyright holders. Publish with that documentation intact. Command line editing uses BSD libedit, so GNU Readline is no longer part of the question.

Every archive must carry: `COPYING`, `COPYRIGHT`, `THIRD_PARTY_NOTICES.md`, a `SOURCE_OFFER` naming the exact commit, and a `licenses/` directory covering the bundled runtime libraries. Do not remove any of these to make the archive tidier -- the source offer is required by GPLv2 section 3, and the attribution notices are required by the Apache-2.0 and BSD licenses of the bundled libraries.

The repository does not commit ADIOS2 or libedit source. The release builder
downloads checksum-pinned source archives and packages their runtime libraries.

This is a conservative engineering checklist, not legal advice.

## Files to keep

- `COPYING` and `COPYRIGHT`: the existing GPL version 2-only terms and notices.
- `THIRD_PARTY_NOTICES.md`: optional dependency attribution and compatibility warning.
- `docs/RELEASES.md`: self-contained archive construction, CI matrix, and publication gate.
- `docs/INSTALL.md`: source installation, verification, and direct-use workflow.
- `docs/ARCHITECTURE.md`: architecture, contributor constraints, and remaining roadmap.
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

Build and test the self-contained release configuration first:

```bash
./release/build-release.sh
```

For focused development checks, run:

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
  --disable-dyn-supplibs --with-opengrads --without-gadap
make -C src -j4 grads libgxdummy.la
```

For a release candidate, repeat the sanitizer run in [BP5.md](BP5.md). Record the compiler, ADIOS2 version, configure command, test output, and platforms in the GitHub release notes.

## Suggested commit structure

A single development commit is acceptable, but these review units are easier to understand:

1. `build: add automatic serial ADIOS2 detection and release bootstrap`
2. `bp5: add descriptor and descriptor-free reader`
3. `bp5: add attributes and bulk XY reads`
4. `cli: add Readline completion and colored launcher`
5. `test: add generated BP5 regression fixture`
6. `docs: document architecture, BP5 use, and publication constraints`

Do not rewrite upstream copyright statements. Add your own authorship or copyright notice only when you know the correct person or organization that owns the contribution.

## Suggested repository description

> OpenGrADS 2.2.1 development fork with default serial ADIOS2 BP5 input, descriptor-free `bpopen`, metadata/missing-value support, bulk XY reads, and regression tests.

## Suggested release notes

> This source development revision adds CPU-only ADIOS2 BP5 input to OpenGrADS. BP5 datasets can be opened directly with `bpopen` or through a `dtype bp5` descriptor. The reader supports global real numeric arrays, ADIOS2 step selection, coordinate discovery, descriptive and missing-value attributes, and bulk in-bounds XY selections. It also adds libedit command completion, a color-enabled development launcher, and generated BP5 integration tests. See `docs/BP5.md` for prerequisites and limitations. Binary publication remains gated pending resolution of linked-library license compatibility.

## After publishing

- Put the license identifier `GPL-2.0-only` in the GitHub description or metadata; do not label the entire repository Apache-2.0.
- Link the README to `COPYING`, `docs/BP5.md`, and `THIRD_PARTY_NOTICES.md`.
- State that release archives contain ADIOS2 and libedit runtimes and retain their notices.
- Use GitHub Issues for unsupported BP shapes, including `bpls -la` output and a minimal non-sensitive reproducer.
- Never attach the RCEMIP dataset unless its data owner and license explicitly allow redistribution.
