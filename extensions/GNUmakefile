#
# Modified in 2026; see ../COPYING.
# Driver Makefile
#

.SILENT:

SHELL = /bin/bash
PYTHON = python

ARCH := $(shell uname -s)
MACH := $(shell uname -m)
ifeq ($(ARCH),Darwin)
   OS := MacOSX
else
ifeq ($(ARCH),FreeBSD)
   OS := FreeBSD
else
   OS := $(shell uname -o)
endif
endif

bintype=# On windows this will be set to c, nc, nc4, dods, hdf
# Default installation directory depends on whether this is a standalone
# module or not
ifneq ($(wildcard ./include),)
   bindir :=  $(shell cd ./bin; pwd)
else
   bindir :=  $(shell cd ../bin; pwd)
endif
gexdir = $(bindir)/gex/$(bintype)

ifeq ($(OS),Cygwin)
	bintype=
endif

export bintype bindir gexdir

# For pytests
HERE := $(shell pwd)

PYTHONPATH := $(HERE)/lib
GADSET  := $(shell cd $(HERE)/../pytests/data; pwd)# where test datsets are
GADDIR  := $(shell cd $(HERE)/../cola/data; pwd)
GABDIR  := $(shell cd $(HERE)/../bin/; pwd)# where binaries for testing are
GABnDIR  := $(shell cd $(HERE)/../opengrads/Contents/; pwd)# where binaries for testing are
GA2UDXT := $(GABDIR)/gex/udxt
GAUDPT  := $(GABDIR)/gex/udpt
GASCRP  := $(GABDIR)/gex/

ifeq ($(ARCH),Darwin)
   DYLD_LIBRARY_PATH := $(shell cd $(HERE)/../bin/gex; pwd):$(SUPPLIBS)/lib:$(DYLD_LIBRARY_PATH)
else
   LD_LIBRARY_PATH := $(shell cd $(HERE)/../bin/gex; pwd):$(SUPPLIBS)/lib:$(LD_LIBRARY_PATH)
endif

SUBDIRS = ams bjt fish hello ipc re shfilt gsf mf lats orb tle # gxyat shape

#---------------------------  t a r g e ts   --------------------------------

% :
	@ t=$@; argv="$(SUBDIRS)" ;\
          for d in $$argv; do                    \
            ( cd $$d; \
              echo -n "Making $$t for extension <$$d> ... "; \
              if $(MAKE) -e $$t; then \
                echo ok; \
              else \
                echo failed; exit 1; \
              fi ) || exit 1; \
          done

# Note: it may be better to have a top ut.py for this
thisbin : install
	$(GABDIR)/grads -bl

check : install
	/usr/bin/env PYTHONPATH=$(PYTHONPATH) \
	             GADSET=$(GADSET) \
	             GADDIR=$(GADDIR) \
	             GABDIR=$(GABDIR) \
	             GA2UDXT=$(GA2UDXT) \
	             GAUDPT=$(GAUDPT) \
	             GASCRP=$(GASCRP) \
	             LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) \
	             $(PYTHON) ut.all.py 

bundle-check : install
	/usr/bin/env PYTHONPATH=$(PYTHONPATH) \
	             GADSET=$(GADSET) \
	             GABDIR=$(GABnDIR) \
	             $(PYTHON) ut.all.py 

install : force
	@/bin/mkdir -p $(bindir) $(gexdir)
	@ t=$@; argv="$(SUBDIRS)" ;\
          for d in $$argv; do                    \
            ( cd $$d; \
              echo -n "Making $$t for extension <$$d> ... "; \
              if $(MAKE) -e $$t; then \
                echo ok; \
              else \
                echo failed; exit 1; \
              fi ) || exit 1; \
          done
	@/bin/rm -rf $(gexdir)/udxt
	@/bin/cat $(gexdir)/*.udxt > $(gexdir)/udxt 

force : ;
