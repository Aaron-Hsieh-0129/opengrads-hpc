# Third-party notices

OpenGrADS itself is distributed under GNU GPL version 2 only; see `COPYING` and `COPYRIGHT`. This file records optional libraries used by this development revision. It does not change the license of OpenGrADS.

No ADIOS2 or GNU Readline source, binary, or license text is vendored in this repository.

## ADIOS2

- Project: ADIOS2, the Adaptable Input Output System version 2
- Upstream: https://github.com/ornladios/ADIOS2
- Documentation: https://adios2.readthedocs.io/
- Version used for verification: 2.11.0
- License stated by upstream: Apache License 2.0
- Upstream license: https://github.com/ornladios/ADIOS2/blob/v2.11.0/LICENSE
- Upstream copyright: https://github.com/ornladios/ADIOS2/blob/v2.11.0/Copyright.txt

The BP5 backend uses ADIOS2's separately installed serial C API. Users who obtain or redistribute ADIOS2 must follow its own license and notices.

## GNU Readline

- Project: GNU Readline
- Upstream: https://tiswww.case.edu/php/chet/readline/rltop.html
- License stated for current releases: GNU GPL version 3
- Purpose here: optional command history, editing, and Tab completion

Readline is separately installed and remains optional at configure time.

## Redistribution warning

The OpenGrADS notice says “using version 2 of the License,” without an “or later” option, so this repository treats it as `GPL-2.0-only`.

The Apache Software Foundation and Free Software Foundation both describe Apache-2.0 as compatible with GPLv3, but not GPLv2-only. The Free Software Foundation also describes GPLv2-only and GPLv3-only as incompatible. Because the optional BP5 executable links OpenGrADS to ADIOS2, and the verified interactive executable also links a modern GPLv3 Readline, redistribution of those linked binaries presents unresolved compatibility questions.

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
