#
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
export PYTHONPATH := $(HERE)/lib
export GADSET  := $(shell cd $(HERE)/../pytests/data; pwd)# where test datsets are
export GADDIR  := $(shell cd $(HERE)/../cola/data; pwd)
export GABDIR  := $(shell cd $(HERE)/../bin/; pwd)# where binaries for testing are
export GA2UDXT := $(GABDIR)/gex/udxt
export GAUDPT  := $(GABDIR)/gex/udpt
export GASCRP  := $(GABDIR)/gex/
export LD_LIBRARY_PATH := $(shell cd $(HERE)/../bin/gex; pwd)

SUBDIRS = ams bjt fish hello ipc re shfilt gsf mf lats orb tle # gxyat shape

#---------------------------  t a r g e ts   --------------------------------

% :
	@ t=$@; argv="$(SUBDIRS)" ;\
          for d in $$argv; do                    \
            ( cd $$d                            ;\
              echo -n "Making $$t for extension <$$d> ... ";\
              $(MAKE) -e  $$t ;\
              echo ok ) \
          done

# Note: it may be better to have a top ut.py for this
thisbin : install
	$(GABDIR)/grads -bl

check : install
	$(PYTHON) ut.all.py 

install : force
	@/bin/mkdir -p $(bindir) $(gexdir)
	@ t=$@; argv="$(SUBDIRS)" ;\
          for d in $$argv; do                    \
            ( cd $$d                            ;\
              echo -n "Making $$t for extension <$$d> ... ";\
              $(MAKE) -e  $$t ;\
              echo ok ) \
          done
	@/bin/rm -rf $(gexdir)/udxt
	@/bin/cat $(gexdir)/*.udxt > $(gexdir)/udxt 

force : ;
