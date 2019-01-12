#
# GNU makefile for sample Dynamically Linked UDFs.
#

include ../supplibs.mk

# Try to locate cairo
# -------------------
cairo_libs = -L$(cairo_prefix)/lib -lcairo 
ifneq ($(wildcard $(SUPPLIBS)/include/cairo),)
   cairo_prefix = $(SUPPLIBS)
   cairo_libs = $(cairo_prefix)/lib/libcairo.a \
                 $(cairo_prefix)/lib/libfontconfig.a \
                 $(cairo_prefix)/lib/libfreetype.a \
                 $(cairo_prefix)/lib/libpixman-1.a \
                 $(cairo_prefix)/lib/libxml2.a \
                 $(cairo_prefix)/lib/libpng.a \
                 $(cairo_prefix)/lib/libz.a \
                 -lpthread -lm
   ifeq ($(OS),Cygwin)
       cairo_prefix = $(SUPPLIBS)
       cairo_libs = $(cairo_prefix)/lib/libcairo.a \
                 $(cairo_prefix)/lib/libfontconfig.a \
                 $(cairo_prefix)/lib/libfreetype.a \
                 $(cairo_prefix)/lib/libpixman-1.a \
                 $(cairo_prefix)/lib/libxml2.a \
                 $(cairo_prefix)/lib/libpng.a \
                 $(cairo_prefix)/lib/libz.a -liconv
   else
   ifeq ($(OS),FreeBSD)
       cairo_prefix = $(SUPPLIBS)
       cairo_libs = $(cairo_prefix)/lib/libcairo.so \
                 $(cairo_prefix)/lib/libfontconfig.so \
                 $(cairo_prefix)/lib/libfreetype.so \
                 $(cairo_prefix)/lib/libpixman-1.so
   else
   ifeq ($(OS),AIX)
      cairo_libs = $(cairo_prefix)/lib/libcairo.a \
                   $(cairo_prefix)/lib/libfreetype.a \
                   $(cairo_prefix)/lib/libpixman-1.a \
                   $(cairo_prefix)/lib/libxml2.a -lm
   else
      cairo_libs = $(cairo_prefix)/lib/libcairo$(DLLEXT) \
                   $(cairo_prefix)/lib/libfreetype.a \
                   $(cairo_prefix)/lib/libpixman-1.a \
                   $(cairo_prefix)/lib/libxml2.a 
   endif
   endif
   endif
else
ifneq ($(wildcard /usr/include/cairo),)
	cairo_prefix = /usr
else
ifneq ($(wildcard /usr/local/include/cairo),)
	cairo_prefix = /usr/local
else
ifneq ($(wildcard /opt/local/include/cairo),)
	cairo_prefix = /opt/local
else
ifneq ($(wildcard /sw/include/cairo),)
	cairo_prefix = /sw
else
ifneq ($(wildcard $(HOME)/local/include/cairo),)
	cairo_prefix = $(HOME)/local
endif
endif
endif
endif
endif
endif

ifneq ($(wildcard $(SUPPLIBS)/include/libpng),)
	HELPER = pngquant
        INC_PNG = -I$(SUPPLIBS)/include/libpng -I$(SUPPLIBS)/include/zlib
        LIB_PNG = $(SUPPLIBS)/lib/libpng.a $(SUPPLIBS)/lib/libz.a -lm
else
	HELPER = 
	INC_PNG = 
endif

ifneq ($(cairo_prefix),)

ifeq ($(ARCH),AIX)
	CC = gcc -maix64
endif

XCFLAGS = -I$(cairo_prefix)/include/cairo
XLDFLAGS = $(cairo_libs)
# XDLLS = $(cairo_prefix)/lib/libcairo

PNGQUANT = pngquant.d/pngquant.c pngquant.d/rwpng.c

FLIBS =

export XCFLAGS XLDFLAGS XDLLS

clean:
	@$(MAKE) -f ../gex.mk clean XBINS="gxyat $(HELPER)" 

%: gxyat $(HELPER)
	@$(MAKE) -f ../gex.mk $@ CSRC=gxyat.c XBINS="gxyat $(HELPER)" 

gxyat : gxyat.c
	$(CC) -o gxyat -O $(XCFLAGS) gxyat.c $(XLDFLAGS) 

pngquant : $(PNGQUANT)
	$(CC) -o pngquant -O -Ipngquant.d $(INC_PNG) $(PNGQUANT) $(LIB_PNG)
else

all lib install clean distclean:
	@echo 'Cannot find cairo installation. Will not build "gxyat"'

endif

