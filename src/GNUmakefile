#
#    Copyright (C) 2008 by Arlindo da Silva <dasilva@opengrads.org>
#    All Rights Reserved.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; using version 2 of the License.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, please consult  
#              
#              http://www.gnu.org/licenses/licenses.html
#
#    or write to the Free Software Foundation, Inc., 59 Temple Place,
#    Suite 330, Boston, MA 02111-1307 USA
#
#--------------------------------------------------------------------------
#
# Makefile fragment for building DLL's on Win32. It overrides targets for
# the main GrADS binaries (gradsc, gradsnc, gradshdf, gradsdods).
# 
# This is a customization of the main build for building extensible binaries
# on Win32. Because DLLs cannot have any undefined symbol, one must be able
# to resolve all the symbols in the extension DLL. One way of ensuring this
# is to have all code of the main binaries (gradsc, gradsnc, etc) placed on
# individual DLLs (libgradsc.dll, libgradsnc.dll, etc), with a very thin
# main.c containing the actual main() function.
#
# A better approach would be to organize the GrADS code in sensible DLLs, 
# but because of circular dependencies (grads.c calls many other functions
# that in turn reference a signal function that resides in grads.c)
#
#
#
#--------------------------------------------------------------------------

include Makefile
# If not on cygwin, no need for cygwin nonsense
# ---------------------------------------------
ifeq ($(build_os),cygwin)

ALIASES = opengrads$(EXEEXT) gradsdap$(EXEEXT) gradsgui$(EXEEXT) merra$(EXEEXT) nomads$(EXEEXT) geos5$(EXEEXT)

#                  --------------------------------
#                  Create DLL for each GrADS Binary
#                  --------------------------------

libgrads_OBJECTS    = $(patsubst main.o,,$(grads_OBJECTS))

libgrads.dll: $(libgrads_OBJECTS) $(grads_DEPENDENCIES) 
	@rm -f libgrads.dll
	$(grads_LINK) -shared $(libgrads_OBJECTS) $(grads_LDADD) $(LIBS)

#                  --------------------------------
#                  Redefine Rules for Main Binaries
#                  --------------------------------

main_win32.o : main_win32.c
	$(COMPILE) -c $<

grads$(EXEEXT) : libgrads.dll main_win32.o $(ALIASES)
	@rm -f $@
	$(grads_LINK) main_win32.o $<

$(ALIASES) : libgrads.dll main_win32.o 
	@rm -f $@
	$(grads_LINK) main_win32.o $<

#                  --------------------------------
#                             Install DLLs
#                  --------------------------------

binPROGRAMS_INSTALL = install

win32_DLLS := libgrads.dll # libgradsdap.dll 

install install-win32: #install-binPROGRAMS
	test -z "$(bindir)" || mkdir -p  "$(DESTDIR)$(bindir)"
	@list='$(ALIASES)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/.exe$$//'`; \
	  if test -f $$p \
	  ; then \
	    f=`echo "$$p1" | sed 's,^.*/,,;$(transform);s/$$/.exe/'`; \
	   echo " $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) '$$p' '$(DESTDIR)$(bindir)/$$f'"; \
	   $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) "$$p" "$(DESTDIR)$(bindir)/$$f" || exit 1; \
	  else :; fi; \
	done
	@list='$(win32_DLLS)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/.dll$$//'`; \
	  if test -f $$p \
	  ; then \
	    f=`echo "$$p1" | sed 's,^.*/,,;$(transform);s/$$/.dll/'`; \
	   echo " $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) '$$p' '$(DESTDIR)$(bindir)/$$f'"; \
	   $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) "$$p" "$(DESTDIR)$(bindir)/$$f" || exit 1; \
	  else :; fi; \
	done

endif # if on cygwin

