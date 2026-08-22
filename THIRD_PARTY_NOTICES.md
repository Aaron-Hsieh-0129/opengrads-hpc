# Third-party notices

OpenGrADS itself is distributed under GNU GPL version 2 only; see `COPYING` and `COPYRIGHT`. This file records optional libraries used by this development revision. It does not change the license of OpenGrADS.

No ADIOS2 or GNU Readline source or binary is committed in this repository.
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

## GNU Readline

- Project: GNU Readline
- Upstream: https://tiswww.case.edu/php/chet/readline/rltop.html
- License stated for current releases: GNU GPL version 3
- Purpose here: optional command history, editing, and Tab completion

Release archives include Readline; ordinary source configuration still treats it as optional.

## Redistribution warning

The OpenGrADS notice says “using version 2 of the License,” without an “or later” option, so this repository treats it as `GPL-2.0-only`.

The Apache Software Foundation and Free Software Foundation both describe Apache-2.0 as compatible with GPLv3, but not GPLv2-only. The Free Software Foundation also describes GPLv2-only and GPLv3-only as incompatible. Because the BP5-enabled executable links OpenGrADS to ADIOS2, and the verified interactive executable also links a modern GPLv3 Readline, redistribution of those linked binaries presents unresolved compatibility questions.

Accordingly:

- Publishing this source repository without vendored dependency code is the recommended scope.
- Do not distribute an ADIOS2-linked or modern-Readline-linked executable/bundle from this fork until permission, a linking exception, relicensing, or qualified legal advice resolves the combination.
- Do not change the repository to Apache-2.0 or GPLv3 unilaterally; only the relevant copyright holders can grant broader terms for existing code.
- If a third-party dependency is ever vendored, include the exact license and required copyright/notice files for that version.

Useful compatibility references:

- https://www.apache.org/licenses/GPL-compatibility
- https://www.gnu.org/licenses/license-list.en.html
- https://www.gnu.org/licenses/gpl-faq.en.html#v2v3Compatibility

This notice is a conservative summary for maintainers, not legal advice.
