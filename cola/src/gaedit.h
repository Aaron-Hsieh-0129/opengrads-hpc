/*

    Copyright (C) 2026 Aaron Hsieh <b08209006@ntu.edu.tw>
    All Rights Reserved.

    Added in 2026 as part of opengrads-hpc, a fork of OpenGrADS.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; using version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see file COPYING.

*/

/* Command line editing comes from either BSD libedit or GNU Readline.
   libedit is preferred and is what release archives link: Readline is
   GPLv3, which is incompatible with this GPLv2-only program, so a binary
   linking it cannot be redistributed. See THIRD_PARTY_NOTICES.md.

   libedit's readline emulation supplies every symbol GrADS uses, so the
   call sites are identical either way and only the header differs. */

#ifndef GAEDIT_H
#define GAEDIT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_EDITLINE
#define USE_EDITLINE 0
#endif

#if USE_EDITLINE == 1
#include <editline/readline.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

#endif /* GAEDIT_H */
