#
# GNU makefile for the Poisson solver UDF
# This is an implementation based on dynamically linked UDFs
# available in GrADS at least later than v1.9b4.
#

export PODS=shfilt.pod
export EXTRAS=sphere.o ftn_shf.o

ARCH := $(shell uname -s)
ifeq ($(ARCH),Darwin)
   FOPT = -O0  # turn off optimization for now
else
   FOPT = -O
endif

%: 
	@$(MAKE) -f ../gex.mk $@ CSRC=shfilt.c FOPT=$(FOPT)


