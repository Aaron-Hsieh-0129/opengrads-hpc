#
# Top Makefile for building COLA GrADS, OpenGrADS extensions and
# assembling an OpenGrADS Bundle.
#

# System idenfication
# -------------------
  VERSION := $(shell cat cola/src/VERSION)
  SHELL = /bin/sh
  ROOTDIR := $(shell dirname `pwd`)
  SYSNAME := $(shell ./config.guess)
  ifeq (x$(OSTYPE),x)
        OSTYPE := $(shell uname -o | tr '[:upper:]' '[:lower:]' )
  endif

  ifeq (x$(OSTYPE),x)
        OSTYPE := $(shell uname -o | tr '[:upper:]' '[:lower:]' )
  endif

  ARCH := $(shell uname -s)
  MACH := $(shell uname -m)
  SITE := $(shell uname -n)
  export ARCH MACH SITE 

  HERE := $(shell pwd)
  prefix := /opt # where to install binaries
  distdir := $(HERE)/dist
  host_triplet := $(shell ./config.guess)

  GLIBC:=
  BUILD:=
  INSTALL:=install
  UDPT:=bin/gex/udpt
  ifeq ($(ARCH),Darwin)
     DLLEXT=dylib
  else
  ifeq ($(OSTYPE),cygwin)
     DLLEXT:=dll
     INSTALL:=install-win32
     BUILD:=all-win32
     UDPT:=bin/udpt
  ifeq ($(ARCH),Linux)
     DLLEXT=so
     GLIBC := $(word 4, $(shell ldd --version | head -1))
     host_triplet := $(host_triplet)-glibc_$(GLIBC)
  else
  ifeq ($(OSTYPE),cygwin)
     DLLEXT=dll
  endif
  endif
  endif
  endif

#
# Build core COLA binaries
#

cola-build: cola/Makefile
	@(cd cola/src; $(MAKE) $(BUILD) )
	@cat etc/udpt | sed s/\.so/\.$(DLLEXT)/g > $(UDPT)

cola-install:
	(cd cola/src; $(MAKE) $(INSTALL) )
	@cat etc/udpt | sed s/\.so/\.$(DLLEXT)/g > $(UDPT)

cola-check: cola-install
	(cd pytests; $(MAKE) check)


cola-clean cola-distclean:
	(cd cola; $(MAKE) $(subst cola-,,$@))
	@cat etc/udpt | sed s/\.so/\.$(DLLEXT)/g > $(UDPT)

cola/Makefile:
	@oga_configure

#
# OpenGRADS Extensions
#

gex-build:
	@/bin/mkdir -p bin/gex
	@(cd extensions; $(MAKE) --silent all)

gex-install gex-check: gex-build
	@/bin/mkdir -p bin/gex
	@(cd extensions; $(MAKE) --silent $(subst gex-,,$@))

gex-clean gex-distclean: 
	@(cd extensions; $(MAKE) --silent $(subst gex-,,$@))

#
# Bundle, docs
#

check bundle-check: cola-install gex-install bundle-create
	@echo "-------------------- Testing Core GrADS -------------------"
	@echo "Core Build ................................................"
	@(cd pytests; $(MAKE) --silent check)
	@echo "Inside Bundle ............................................."
	@(cd pytests; $(MAKE) --silent bundle-check)
	@echo "------------- Testing OpenGrADS Extensions  ---------------"
	@echo "Core Build ................................................"
	@(cd extensions; $(MAKE) --silent check)
	echo "Inside Bundle ............................................."
	@(cd extensions; $(MAKE) --silent bundle-check)

Documentation.html: Documentation.php
	php Documentation.php > Documentation.html

#
# Bundle installation under local dir
#
bundle-create:
	@bundle/bundle_create.sh 

pkg-create:
	@bundle/bundle_create.sh -macpkg

#
# Bundle installation under prefix
#
bundle-install: 
	@bundle/bundle_create.sh 
	@mkdir -p $(prefix)
	@rm -rf .opengrads
	@mv opengrads .opengrads
	@mv .opengrads/Contents $(prefix)/OpenGrADS
	@rm -rf .opengrads
	@echo "Make sure to put $(prefix)/OpenGrADS in your path"

clean:
	$(MAKE) cola-clean
	$(MAKE) gex-clean

distclean:
	$(MAKE) cola-distclean
	$(MAKE) gex-distclean
	/bin/rm -rf bin opengrads dist darwin/build darwin/build_debug

#
#                  Distribution Tarballs
#

all-dist: src-dist bundle-dist

SRCS_TAR = opengrads-$(VERSION)-bundle.tar.gz
src-dist:
	@echo 
	@echo "Creating OpenGrADS Source Tarball"
	@echo "---------------------------------"
	mkdir -p $(distdir)
	(git archive --prefix opengrads-$(VERSION)/ master | gzip > $(distdir)/$(SRCS_TAR))

BUNDLE_TAR = opengrads-$(VERSION)-bundle-$(host_triplet).tar.gz
bundle-dist: bundle-create
	@echo 
	@echo "Creating Binary OpenGrADS Tarball"
	@echo "---------------------------------"
	@mkdir -p $(distdir)
	@bundle/bundle_create.sh $(distdir)/opengrads-$(VERSION) 
	@(cd $(distdir); tar cvfz $(BUNDLE_TAR) opengrads-$(VERSION))
	@/bin/rm -rf  $(distdir)/opengrads-$(VERSION)

# ----------------------- No Longer Maintained ------------------- 
#
#
#                                J A V A
#                                -------
#
#
java-dist: all-am
	$(MAKE)
	(cd Java; $(MAKE) grads.jar)
	cp -p Java/grads.jar $(distdir).jar

java-clean:
	(cd Java; make clean)

java-distclean:
	(cd Java; make distclean)

reallyclean:
	$(MAKE) distclean
	$(MAKE) java-distclean


.PHONY: release
release:
	./release/build-release.sh
