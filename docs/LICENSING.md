# Licensing and usage rules

This page explains what you may do with opengrads-hpc, what is asked of you in
return, and one unresolved question you should know about before you
redistribute the binary archives.

It is a plain-language summary for users. The authoritative texts are
[`COPYING`](../COPYING) (the GNU General Public License, version 2) and
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md). Where this page and
those files disagree, those files win. None of this is legal advice.

## The short version

opengrads-hpc is a fork of OpenGrADS 2.2.1.oga.1, which is a fork of GrADS.
GrADS is licensed under **GPL-2.0-only**, and so is this fork — the licence
cannot be changed by anyone except the copyright holders.

| If you want to… | What applies |
| --- | --- |
| Use it for research, teaching, or commercial work | Nothing. No obligations at all. |
| Modify it for yourself | Nothing. Private modification is unrestricted. |
| Share it inside your own group or institution | Generally internal use, not distribution. |
| Give source or binaries to people outside your organisation | GPLv2 obligations apply — see below. |
| Repackage it for Spack, conda-forge, Debian, etc. | Read **The situation** below first. |

## What you are free to do

The GPL is a permissive-in-practice licence for *users*. You may:

- run it for any purpose, including commercial and classified work, with no
  field-of-use restriction and no fee;
- read, study, and modify the source however you like;
- keep your modifications entirely private, forever, with no obligation to
  publish or contribute anything back;
- run it on as many machines as you want;
- publish research produced with it without any licensing implication.

**Private use triggers nothing.** The GPL's obligations attach only when you
distribute the software to others. If you build it, modify it, and use it
yourself or within your own organisation, nothing in this page applies to you.

## What is asked of you if you redistribute

If you pass the software on to someone outside your organisation — source or
binary, modified or not — GPLv2 asks five things:

1. **Keep the notices.** Copyright notices, the warranty disclaimer, and the
   licence text stay with the software.
2. **Include the licence.** Ship a copy of `COPYING`.
3. **Mark your changes.** Files you modify must say that you changed them.
4. **Pass on the same freedoms.** Your version must also be GPL-2.0-only. You
   may not add conditions of your own, and you may not relicense it.
5. **Offer the source.** If you distribute a binary, you must also provide the
   corresponding source, or a written offer to supply it. Our archives do this
   with the `SOURCE_OFFER` file they contain.

You *may* charge money for distribution. The GPL does not require that
software be free of cost, only that recipients get the same freedoms you did.

## The situation

**This affects binary archives only. It does not affect building from source,
and it does not affect using the software.**

The BP5 reader is built on [ADIOS2](https://github.com/ornladios/ADIOS2),
which is licensed under Apache-2.0. GrADS is GPL-2.0-only. These two licences
are **not compatible with each other**, and both the Apache Software
Foundation and the Free Software Foundation say so.

The mechanism is narrow and worth understanding:

- GPLv2 §6 says you *"may not impose any further restrictions on the
  recipients' exercise of the rights granted herein."*
- Apache-2.0 adds conditions GPLv2 does not have — a patent-retaliation
  clause, a NOTICE-propagation requirement, and an indemnification term.
- From GPLv2's point of view those count as further restrictions, so the two
  cannot be satisfied at once.

Nobody is at fault here. Apache-2.0 is permissive and raises no objection to
being combined with GPL code; the refusal comes entirely from the GPL side.
GPLv3 §7 was later written specifically to permit terms like Apache's — which
is why this problem would not exist if GrADS had used the customary "version 2
**or any later version**" wording. It did not, and only the copyright holders
(George Mason University / COLA, and Arlindo da Silva) can change that.

GNU Readline raises the same question independently: it is GPLv3, which is
also incompatible with GPLv2-only. Neither has a usable substitute. BP5 is
ADIOS2's own format with no independent implementation, and the BSD libedit
alternative to Readline was trialled and reverted because it degrades the
interactive prompt and Tab completion.

### What that means for you

**Using a binary archive:** no consequence. Licence compatibility constrains
*distribution*, not use. Download it and run it.

**Building from source:** no consequence, and no ambiguity of any kind. This
repository contains no ADIOS2 code — only a pinned URL and checksum. When you
build, *you* create the combined work, on your own machine, for yourself. That
is private use, which the GPL does not restrict. If you want to avoid the
question entirely, build from source.

**Redistributing a BP5-enabled binary further:** this is where the question
lands. Our archives ship with the conflict documented rather than concealed,
so that you can assess it yourself. Do that before mirroring them, bundling
them into another product, or submitting them to a package repository — most
distribution channels run a licence review that will flag this.

**Packaging for Spack, conda-forge, Debian, Homebrew, etc.:** please read the
above and make your own determination. A BP5-enabled build is likely to be
rejected on licence-compatibility grounds. A build configured with
`--without-adios2` has no such issue and is straightforward to package.

## Building without ADIOS2

If you need a build with no licence question attached, disable ADIOS2
explicitly. Note that `--with-adios2` defaults to `auto`, so simply omitting it
is **not** enough — a system ADIOS2 on `PATH` would still be detected and
linked:

```bash
./cola/configure --enable-openmp --enable-sdfopen --with-opengrads \
  --without-gadap --without-adios2
```

Confirm with `q config`, which should report `ADIOS2 BP5 interface DISABLED`.

You lose `bpopen` and `dtype bp5`. NetCDF, HDF5, GRIB, station data, OpenMP
threading, and all graphics are unaffected.

## Third-party components

Release archives bundle the libraries the program needs at runtime. All are
unmodified upstream builds, and every archive carries their licence texts in
`licenses/` plus a `licenses/BUNDLED-LIBRARIES.txt` manifest recording exactly
what it contains and where each file came from.
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md) lists each component,
its upstream, and its licence.

## Who holds the copyright

| Holder | Scope |
| --- | --- |
| George Mason University / COLA | the GrADS core |
| Arlindo da Silva | the OpenGrADS additions, including `grads.c` |
| Institute of Global Environment and Society | `gxshad2.c` |
| Aaron Hsieh | this fork's additions: BP5 reader, OpenMP engine, editing shim |

Every one of these is licensed to you under GPL-2.0-only. Contact details for
the GrADS copyright holders are in [`COPYRIGHT`](../COPYRIGHT).

## Reporting a problem with this

If you are a copyright holder and object to anything described here, please
open an issue or contact the maintainer. Binary publication is controlled by a
single switch and can be turned off immediately while any concern is
discussed; source distribution is unaffected either way.
