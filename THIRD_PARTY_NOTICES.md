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

## Other bundled runtime libraries

Release archives bundle the transitive runtime closure of the GrADS
executable and its graphics plug-ins. These are unmodified upstream builds,
taken from the distribution's own packages (Ubuntu), Homebrew (macOS), or
MSYS2 (Windows). Each archive records exactly what it carries in
`licenses/BUNDLED-LIBRARIES.txt`, with license texts in `licenses/`.

| Component | Upstream | License family |
| --- | --- | --- |
| Cairo | https://www.cairographics.org/ | LGPL-2.1 or MPL-1.1 |
| NetCDF-C | https://www.unidata.ucar.edu/software/netcdf/ | BSD-3-Clause style (UCAR) |
| HDF5 | https://www.hdfgroup.org/ | BSD-3-Clause style (HDF Group) |
| UDUNITS 1 | https://www.unidata.ucar.edu/software/udunits/ | Permissive (UCAR) |
| libgeotiff / libtiff | https://github.com/OSGeo/libgeotiff | MIT/BSD style |
| zlib | https://zlib.net/ | zlib license |
| libjpeg | https://www.ijg.org/ | IJG permissive |
| freetype / fontconfig / pixman | see archive notices | FTL or BSD/MIT style |
| libgomp (OpenMP runtime) | https://gcc.gnu.org/ | GPL-3.0 with GCC Runtime Library Exception |

All of the above are GPLv2-compatible as used here. libgomp is GPLv3, but the
GCC Runtime Library Exception explicitly permits linking it into a program
under any license when that program is compiled by GCC, which is how these
archives are built.

Linux archives additionally copy the Debian/Ubuntu `copyright` file for each
system package they bundle into `licenses/system/`.

## License compatibility

This section records the licensing position of the binary archives so that
recipients can assess it for themselves.

The OpenGrADS notice says "using version 2 of the License," without an "or
later" option, so this repository treats GrADS as `GPL-2.0-only`.

ADIOS2 is Apache-2.0. The Apache Software Foundation and the Free Software
Foundation both describe Apache-2.0 as compatible with GPLv3 but **not** with
GPLv2-only. Because the BP5-enabled executable links GrADS to ADIOS2, the
combination raises a license-compatibility question that is **unresolved**.

Release archives published from this repository contain that combination.
This is a deliberate decision by the maintainer, taken with the conflict
documented rather than concealed, while a compatibility grant is sought from
the GrADS copyright holders (COLA / George Mason University) or from the
ADIOS2 copyright holders. Anyone redistributing these archives further, or
packaging them for a distribution channel that performs license review,
should evaluate the question independently first.

The Readline half of this problem was removed in 2026 by replacing GNU
Readline (GPLv3) with BSD libedit. That substitution is not available for
ADIOS2: BP5 is ADIOS2's own format and no independently licensed
implementation exists.

Everything else the GPL requires of these archives is satisfied: changed
files carry modification notices, fork-authored files carry the same license
grant, the license texts ship inside every archive, and each archive carries
a written offer for complete corresponding source (see `SOURCE_OFFER`).

Building from source has no such question attached. The repository commits no
ADIOS2 code, so a source checkout is not a combined work, and the GPL places
no restriction whatsoever on building or using the result privately.

- Do not change the repository license to Apache-2.0 or GPLv3 unilaterally;
  only the relevant copyright holders can grant broader terms for existing
  code.
- If a third-party dependency is ever vendored, include the exact license and
  required copyright/notice files for that version.

Useful compatibility references:

- https://www.apache.org/licenses/GPL-compatibility
- https://www.gnu.org/licenses/license-list.en.html
- https://www.gnu.org/licenses/gpl-faq.en.html#v2v3Compatibility

This notice is a conservative summary for maintainers, not legal advice.
