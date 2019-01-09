#
# GNU makefile for the Poisson solver UDF
# This is an implementation based on dynamically linked UDFs
# available in GrADS at least later than v1.9b4.
#

ARCH := $(shell uname -s)

#export FOPT=-g -fcheck-bounds
export FOPT=-O0 # this is needed with gfortran or else a coredump may result
export PODS=orb.pod
export EXTRAS=orbits_mod.o ftn_orb.o

all %: 
ifeq ($(ARCH),AIX)
	@$(MAKE) -f ../gex.mk $@ CSRC=orb.c FOPT=$(FOPT) LD=f95 
else
	@$(MAKE) -f ../gex.mk $@ CSRC=orb.c FOPT=$(FOPT) 
endif

