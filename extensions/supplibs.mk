#
# Locate supplibs
#

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

ifeq ($(OS),MacOSX)
   DLLEXT:=.dylib
else 
ifeq ($(OS),Cygwin)
   DLLEXT=.dll
else
   DLLEXT:=.so
endif
endif

ifndef SUPPLIBS
   SUPPLIBS = $(wildcard ../../../supplibs ../../supplibs)
   ifneq ($(SUPPLIBS),)
      SUPPLIBS := $(shell cd $(SUPPLIBS); pwd)
   endif
endif

