# Third-party notices

OpenGrADS itself is distributed under GNU GPL version 2 only; see `COPYING` and `COPYRIGHT`. This file records optional libraries used by this development revision. It does not change the license of OpenGrADS.

No ADIOS2 or libedit source or binary is committed in this repository.
The release builder downloads checksum-pinned sources into ignored build
directories and copies their license texts into each generated archive.

## ADIOS2

- Project: ADIOS2, the Adaptable Input Output System version 2
- Upstream: https://github.com/ornladios/ADIOS2
- Documentation: https://adios2.readthedocs.io/
- Version used for verification: 2.11.0
- License stated by upstream: Apache License 2.0
- Upstream license: https://github.com/ornladios/ADIOS2/blob/v2.11.0/LICENSE
- Upstream copyright: https://github.com/ornladios/ADIOS2/blob/v2.11.0/Copyright.txt

The BP5 backend uses ADIOS2's serial C API. Release users receive the required
runtime libraries inside the archive; source builders may use automatic
detection or the pinned release bootstrap. Redistributors must follow the
upstream license and notices.

## BSD libedit (editline)

- Project: libedit, the NetBSD command line editor library
- Upstream: https://thrysoee.dk/editline/
- Version used for verification: 20240808-3.1
- License stated by upstream: BSD 3-Clause (Regents of the University of California)
- Purpose here: command history, line editing, and Tab completion

libedit replaced GNU Readline in 2026. Readline is GPLv3, which is
incompatible with this GPLv2-only program, so a binary linking it cannot be
redistributed. libedit is BSD licensed and therefore GPLv2-compatible, and its
Readline emulation supplies every symbol GrADS uses, so the feature set is
unchanged.

Source builds still accept GNU Readline if libedit is absent; `configure`
prefers libedit when both are present. Anyone who builds against Readline
should not redistribute the resulting binary.

## GNU ncurses

- Project: GNU ncurses
- Upstream: https://ftp.gnu.org/gnu/ncurses/
- Version used for verification: 6.5
- License stated by upstream: MIT-style permissive (X11-like)
- Purpose here: terminal capability database required by libedit

## Redistribution warning

The OpenGrADS notice says “using version 2 of the License,” without an “or later” option, so this repository treats it as `GPL-2.0-only`.

The Apache Software Foundation and Free Software Foundation both describe Apache-2.0 as compatible with GPLv3, but not GPLv2-only. Because the BP5-enabled executable links OpenGrADS to ADIOS2, redistribution of that linked binary presents unresolved compatibility questions.

The Readline half of this problem was resolved in 2026 by replacing it with BSD libedit. The ADIOS2 half cannot be resolved by substitution: BP5 is ADIOS2's own format and no independently licensed implementation exists. It requires either a GPLv2-compatible grant from the GrADS copyright holders (COLA / George Mason University), whose "version 2 of the License" wording carries no "or later" option, or a GPLv2-compatible grant for ADIOS2 from its copyright holders.

Accordingly:

- Publishing this source repository without vendored dependency code is the recommended scope.
- Do not distribute an ADIOS2-linked executable/bundle from this fork until permission, a linking exception, relicensing, or qualified legal advice resolves the combination. Binary publication is gated behind the `BINARY_REDISTRIBUTION_APPROVED` repository variable for this reason.
- Do not change the repository to Apache-2.0 or GPLv3 unilaterally; only the relevant copyright holders can grant broader terms for existing code.
- If a third-party dependency is ever vendored, include the exact license and required copyright/notice files for that version.

Useful compatibility references:

- https://www.apache.org/licenses/GPL-compatibility
- https://www.gnu.org/licenses/license-list.en.html
- https://www.gnu.org/licenses/gpl-faq.en.html#v2v3Compatibility

This notice is a conservative summary for maintainers, not legal advice.
