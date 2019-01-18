#
# GNU makefile for the Poisson solver UDF
# This is an implementation based on dynamically linked UDFs
# available in GrADS at least later than v1.9b4.
#

ARCH := $(shell uname -s)

#export FOPT=-g -fcheck-bounds
export FOPT=-O0 # this is needed with gfortran or else a coredump may result
export PODS=tle.pod
export EXTRAS=tle_mod.o sgp4_mod.o ftn_tle.o

%: 
ifeq ($(ARCH),AIX)
	@$(MAKE) -f ../gex.mk $@ CSRC=tle.c FOPT=$(FOPT) LD=f95 
else
	@$(MAKE) -f ../gex.mk $@ CSRC=tle.c  
endif

