#
# GNU makefile for sample Dynamically Linked UDFs.
#

export PODS=shape.pod

all %:
	@$(MAKE) -f ../gex.mk $@ \
        CSRC="shape.c"\
        EXTRAS="shp_print.o libshp/shpopen.o libshp/dbfopen.o"

