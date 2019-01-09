#
# GNU makefile for sample Dynamically Linked UDFs.
#

include ../supplibs.mk

export PODS=liblats.pod
export XCFLAGS=-DUSELATS=1 -DGOT_NETCDF -I$(SUPPLIBS)/include/hdf -I$(SUPPLIBS)/include/netcdf
export EXTRAS=galats.o lats.o  latsgrib.o  latsgribmap.o  latsint.o  \
              latsnc.o  latssd.o latsstat.o  latstime.o \
              fgutil.o fgbds.o ShaveMantissa.o
export GSFS=lats4d.gs

ifeq ($(ARCH),AIX)
export XLDFLAGS = $(SUPPLIBS)/lib/libnetcdf.a $(SUPPLIBS)/lib/libmfhdf.a 
endif

all %:
	@$(MAKE) -f ../gex.mk $@ CSRC="liblats.c"

html:
	./lats4d.sh -h | ./mkhtml.pl -t >  lats4d.html
	./lats4d.sh -h | ./mkhtml.pl    >> lats4d.html
	@$(MAKE) -f ../gex.mk html CSRC="liblats.c"

VERSION:
	./lats4d.sh -h | grep '(Version' | cut -d' ' -f12 > VERSION

BASEN = lats4d-$(shell cat VERSION)

dist: html VERSION
	/bin/rm -rf $(BASEN)
	mkdir -p $(BASEN)
	cp ChangeLog README NEWS lats4d.html \
           lats4d.gs lats4d.sh $(BASEN)
	cp INSTALL.txt $(BASEN)/INSTALL # so that make install works
	tar cvfz $(BASEN).tar.gz $(BASEN)
 
