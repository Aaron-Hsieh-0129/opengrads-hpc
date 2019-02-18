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

ALIASES = opengrads$(EXEEXT) gradsgui$(EXEEXT) ncep$(EXEEXT) geos$(EXEEXT)

PLUGINS = libgxdCairo.dll libgxdX11.dll libgxdummy.dll libgxpCairo.dll libgxpGD.dll

CPPFLAGS += -I/usr/include/freetype2 -I/usr/include/cairo

#                  --------------------------------------------
#                  Create DLL for each GrADS Binary and Plugins
#                  --------------------------------------------

libgrads.a: $(grads_OBJECTS) $(grads_DEPENDENCIES) 
	@rm -f $@
	ar cr libgrads.a $(grads_OBJECTS) 
	ranlib libgrads.a

cyggrads.dll: libgrads.a
	@rm -f $@
	$(grads_LINK) -shared \
                      -Wl,--out-implib=libgrads.dll.a \
                      -Wl,--export-all-symbols \
                      -Wl,--enable-auto-import \
                      -Wl,--whole-archive libgrads.a \
                      -Wl,--no-whole-archive $(grads_LDADD) $(LIBS) -lz

libgrads.dll: $(grads_OBJECTS) $(grads_DEPENDENCIES) 
	@rm -f $@
	$(grads_LINK) -shared $(grads_OBJECTS) $(grads_LDADD) $(LIBS) -lz

libgxdCairo_OBJECTS = $(patsubst %.lo,.libs/%.o,$(libgxdCairo_la_OBJECTS))
libgxdCairo_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgxdCairo_la_DEPENDENCIES))
libgxdCairo.dll: $(libgxdCairo_la_OBJECTS) $(libgxdCairo_la_DEPENDENCIES) libgrads.dll
	@rm -f $@
	$(grads_LINK)  -shared $(libgxdCairo_OBJECTS) \
                               -lcairo -lfreetype libgrads.dll $(X11_ldadd) -lz

libgxdX11_OBJECTS = $(patsubst %.lo,.libs/%.o,$(libgxdX11_la_OBJECTS))
libgxdX11_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgxdX11_la_DEPENDENCIES))
libgxdX11.dll: $(libgxdX11_la_OBJECTS) $(libgxdX11_la_DEPENDENCIES) libgrads.dll
	@rm -f $@
	$(grads_LINK)  -shared $(libgxdX11_OBJECTS) libgrads.dll $(X11_ldadd) -lz

libgxdummy_OBJECTS = $(patsubst %.lo,.libs/%.o,$(libgxdummy_la_OBJECTS))
libgxdummy_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgxdummy_la_DEPENDENCIES))
libgxdummy.dll: $(libgxdummy_la_OBJECTS) $(libgxdummy_la_DEPENDENCIES) libgrads.dll
	@rm -f $@
	$(grads_LINK)  -shared $(libgxdummy_OBJECTS) libgrads.dll -lz

libgxpCairo_OBJECTS = .libs/libgxdCairo_la-gxX.o \
                       $(patsubst %.lo,.libs/%.o,$(libgxpCairo_la_OBJECTS))
libgxpCairo_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgxpCairo_la_DEPENDENCIES))
libgxpCairo.dll: $(libgxpCairo_la_OBJECTS) $(libgxpCairo_la_DEPENDENCIES) libgrads.dll
	@rm -f $@
	$(grads_LINK)  -shared $(libgxpCairo_OBJECTS)  libgrads.dll \
                               -lcairo -lfreetype $(X11_ldadd) -lz

libgxpGD_OBJECTS = $(patsubst %.lo,.libs/%.o,$(libgxpGD_la_OBJECTS))
libgxpGD_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgxpGD_la_DEPENDENCIES))
libgxpGD.dll: $(libgxpGD_la_OBJECTS) $(libgxpGD_la_DEPENDENCIES) libgrads.dll
	@rm -f $@
	$(grads_LINK)  -shared $(libgxpGD_OBJECTS) \
                               libgrads.dll -lgd  -lz

libgradspy_OBJECTS = $(patsubst %.lo,.libs/%.o,$(libgradspy_la_OBJECTS))
libgradspy_DEPENDENCIES = $(patsubst %.lo,.libs/%.o,$(libgradspy_la_DEPENDENCIES))
libgradspy.dll: $(libgradspy_la_OBJECTS) $(libgradspy_la_DEPENDENCIES) \
                $(EXTRA_libgradspy_la_DEPENDENCIES) 
	@rm -f $@
	$(grads_LINK) -shared $(libgradspy_OBJECTS) $(grads_LDADD) $(LIBS) -lz


#                  --------------------------------
#                  Redefine Rules for Main Binaries
#                  --------------------------------

main_win32.o : main_win32.c
	$(COMPILE) -c $<

grads$(EXEEXT) : libgrads.dll main_win32.o $(ALIASES)
	@rm -f $@
	$(grads_LINK) main_win32.o libgrads.dll

$(ALIASES) : libgrads.dll main_win32.o 
	@rm -f $@
	$(grads_LINK) main_win32.o $<

#                  --------------------------------
#                             Install DLLs
#                  --------------------------------

binPROGRAMS_INSTALL = install-win32
lib_LTLIBRARIES := 

win32_DLLS := libgrads.dll $(PLUGINS)

all-win32: $(win32_DLLS) $(bin_PROGRAMS)

install install-win32: $(win32_DLLS) $(bin_PROGRAMS)
	test -z "$(bindir)" || mkdir -p  "$(DESTDIR)$(bindir)"
	@list='$(ALIASES)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/.exe$$//'`; \
	  if test -f $$p \
	  ; then \
	    f=`echo "$$p1" | sed 's,^.*/,,;$(transform);s/$$/.exe/'`; \
	   echo " /usr/bin/install '$$p' '$(DESTDIR)$(bindir)/$$f'"; \
	   /usr/bin/install "$$p" "$(DESTDIR)$(bindir)" || exit 1; \
	  else :; fi; \
	done
	@list='$(win32_DLLS)'; for p in $$list; do \
	  p1=`echo $$p|sed 's/.dll$$//'`; \
	  if test -f $$p \
	  ; then \
	    f=`echo "$$p1" | sed 's,^.*/,,;$(transform);s/$$/.dll/'`; \
	   echo " $ /usr/bin/install '$$p' '$(DESTDIR)$(bindir)/$$f'"; \
	   /usr/bin/install "$$p" "$(DESTDIR)$(bindir)" || exit 1; \
	  else :; fi; \
	done


endif # if on cygwin

