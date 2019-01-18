#
# GNU makefile fragment for making dynamic libraries for GrADS
#

SHELL = /bin/bash

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

bintype=#        On windows this will be c, nc, nc4, dods, hdf
bindir = ../../bin

ifneq ($(wildcard ../include),)
       bindir = ../bin
       GAINC = ../include
else
ifneq ($(wildcard ../../include),)
       GAINC = ../../include
else
       GAINC = ../../cola/src
endif
endif

gexdir = $(bindir)/gex/$(bintype)

#
#                          C  Compiler Check
#                          -----------------

hintCC = gcc    # user can suggest something different
CC = $(hintCC)

# Look for a C compiler
# ---------------------
ifeq ($(shell which $(CC)),)
   CC_ = $(CC)
   override CC := gcc
   ifeq ($(shell which $(CC)),)
         $(warning Cannot find CC = [$(CC_)], not even gcc can be found )
         HAS_CC = no#
   else
         $(warning Cannot find CC = [$(CC)], using gcc instead )
         HAS_CC = yes#   
         CC = gcc
   endif
else
   HAS_CC = yes   
endif

# Must have a C compiler
# ----------------------
ifeq ($(HAS_CC),no)
      $(error Cannot proceed without a C compiler )
endif

CPPFLAGS := -D___GAUDX___ -I. -I$(GAINC)
CFLAGS   := $(LDFLAGS) -O -fno-common -fPIC 

# C compiler library
# ------------------
CLIBS    := $(shell gcc -print-file-name=libgcc.a )

#
#                       Fortran Compiler Check
#                       ----------------------

hintF90 = gfortran#   user can suggest something different
hintF77 = gfortran#        user can suggest something different

F77 = $(hintF77)
F90 = $(hintF90)
FC  = $(F77)
FFLAGS = $(FOPT) -I. -I$(GAINC)

# FOPT = -g -fbounds-check

ifeq ($(ARCH),Darwin)
   FOPT = -O0  # turn off optimization for now
else
   FOPT = -O
endif


# Look for F77
# ------------
ifeq ($(shell which $(F90)),)
   HAS_F90 = no#
else
   HAS_F90 = yes#
endif 

# Look for F77
# ------------
ifeq ($(shell which $(F77)),)
   HAS_F77 = no#
   ifeq ($(HAS_F90),yes)
#           $(warning Cannot find F77 = [$(F77)] --- using [$(F90)] instead )
           F77 = $(F90)
           HAS_F77 = yes#
   else
           $(warning Cannot find F77 = $(F77) )
   endif 
else
   HAS_F77 = yes#
endif 

# Make sure we have what we need: F77
# -----------------------------------
ifeq ($(NEED_F77),yes)
  ifeq ($(HAS_F77),yes)
        FC = $(F77)
  else 
     $(error Cannot proceed without a F77 compiler )
 endif 
endif 

# Make sure we have what we need: F90
# -----------------------------------
ifeq ($(NEED_F90),yes)
  ifeq ($(HAS_F90),yes)
        FC = $(F90)
  else
     $(error Cannot proceed without a F90 compiler )
  endif
endif 

# Fortran compiler libraries
# --------------------------
ifeq ($(FC),g77)
# FLIBS := $(shell g77 -print-file-name=libg2c.a )
  FLIBS := -L$(shell dirname `g77 -print-file-name=libg2c.a` ) \
           -lg2c
else
ifeq ($(FC),g95) # does not yet work on Mac
  FLIBS := -L$(shell dirname `g95 -print-file-name=libf95.a` ) -lf95
  EXTENDED_SOURCE := -ffixed-line-length-132
else
ifeq ($(patsubst gfortran%,gfortran,$(FC)),gfortran)
  FLIBS := -L$(shell dirname `$(FC) -print-file-name=libgfortran.a` ) \
           -lgfortran
  EXTENDED_SOURCE := -ffixed-line-length-132
else
ifeq ($(ARCH),AIX)
  EXTENDED_SOURCE = -qfixed=132
else
  EXTENDED_SOURCE = -extend_source # ifort, osf1, 
endif
endif
endif
endif

#
#                          Libraries, etc
#                          --------------

LD := $(CC)
LDFLAGS := -shared 
DLLEXT:=so

LIBS += $(CLIBS) $(FLIBS) 

#                            -----------------------
#                            Platform Specific Stuff
#                            -----------------------

# Mac OS
# ------
ifeq ($(ARCH),Darwin)
	CLIBS = 
        LD = /usr/bin/libtool
        CFLAGS := -O -fno-common -fPIC -mmacosx-version-min=10.6 
        FFLAGS += -mmacosx-version-min=10.6 
        DLLEXT=dylib
        LDFLAGS = -dynamic -flat_namespace -undefined suppress -macosx_version_min 10.6 
        LIBS += #-lSystemStubs 

# AIX
# ---
else
ifeq ($(ARCH),AIX)
	CC = gcc -maix64
	FC = f95 -qextname
        FLIBS = 
	EXTENDED_SOURCE = -qfixed=132
        LD = ld
        CFLAGS := -O -fno-common -fPIC 
        DLLEXT=so
        LDFLAGS = -G -bnoentry -bexpall
        LIBS = -lc $(shell gcc -maix64 -print-file-name=libgcc_s.a ) -lxlf90 -lm

# Linux
# -----
else
ifeq ($(ARCH),Linux)
	FFLAGS += -fPIC
	DLLEXT=so
	LD = $(CC)
	LDFLAGS += -nostartfiles
#        CLIBS=
#        FLIBS=
else
ifeq ($(ARCH),FreeBSD)
	FFLAGS += -fPIC
endif
endif
endif
endif

# Cygwin
# ------
ifeq ($(OS),Cygwin)
	bintype=#dods
	LIBGRADS = ../../src/libgrads$(bintype).dll
	CFLAGS := $(LDFLAGS) -O -fno-common
	DLLEXT=dll
	LD = $(CC)
	LDFLAGS += -nostartfiles $(LIBGRADS)
endif

CFLAGS   += $(XCFLAGS)
CPPFLAGS += $(XCPPFLAGS)
LDFLAGS  += $(XLDFLAGS)
FFLAGS   += $(XFFLAGS)

#                            -----------------------
#                                 Building Rules
#                            -----------------------

CSRC := $(wildcard *.c) 
UDXS := $(addsuffix .gex,$(basename $(CSRC)))
HTMS := $(addsuffix .html,$(basename $(PODS)))
UDXT := $(wildcard *.udxt)

ifneq ($(XDLLS),)
	XDLLS_ = $(wildcard $(XDLLS).$(DLLEXT))
endif

all : $(UDXS) $(UDXT) $(PODS)

install : $(UDXS) $(UDXT) $(PODS)
	@/bin/mkdir -p $(bindir) $(gexdir) 
	/bin/cp -p $(UDXS) $(UDXT) $(XDLLS_) $(gexdir)
        ifneq ($(XBINS), )
	    /bin/cp -p $(XBINS)              $(bindir)
        endif
        ifneq ($(PODS), )
	    /bin/cp -p $(PODS)               $(bindir)
        endif
        ifneq ($(GSFS), )
	    /bin/cp -p $(GSFS)               $(gexdir)
        endif

html : $(HTMS)

clean distclean:
	@/bin/rm -rf $(UDXS) $(XBINS) \
                     *~ *.o *.pyc *.tmp *.pod *.html *.wiki *.[Mm][Oo][Dd]\
                     .grads.lats.table output/

% : ;
#	@echo Target $@ not defined in `pwd`

%.o : %.F
	$(FC) -c $(EXTENDED_SOURCE) $(FFLAGS) $(XFLAGS) $*.F

%.o : %.f
	$(FC) -c $(EXTENDED_SOURCE) $(FFLAGS) $*.f

%.o : %.f90
	$(FC) -c $(FFLAGS) $*.f90

%.o : %.F90
	$(FC) -c $(FFLAGS) $*.F90

%.pod : %.pod_
	cpp -ansi -DPOD $*.pod_ >$*.pod

%.pod : %.c
	cpp -DPOD $*.c >$*.pod

%.html : %.pod_
	cpp -ansi -DPOD $*.pod_ | pod2html --css=/pod.css --header > $*.html

%.html : %.pod
	cpp -ansi -DPOD $*.c | pod2html --css=/pod.css --header > $*.html

%.html : %.pl
	pod2html --css=/pod.css --header $*.pl > $*.html

%.wiki : %.c
	cpp -ansi -DPOD $*.c | pod2wiki --style mediawiki > $*.wiki

%.gex : %.o $(EXTRAS)
	$(LD) $(LDFLAGS) -o $@ $*.o $(EXTRAS) $(LIBS) $(LDFLAGS)

%.x : %.o
	$(FC) $(FFLAGS) -o $@ $*.f


