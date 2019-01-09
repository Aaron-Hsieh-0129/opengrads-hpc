#
# GNU makefile for B.-J. Tsuang contributed GrADS functions.
# This is an implementation based on dynamically linked UDFs
# available in GrADS at least later than v1.9b4.
#

export PODS=libbjt.pod
export EXTRAS=nr.o ir.o

all %: 
	@$(MAKE) -f ../gex.mk $@ CSRC=libbjt.c
