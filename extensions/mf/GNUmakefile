#
# GNU makefile for sample Dynamically Linked UDFs.
#

export NEED_F90=yes
#export XFFLAGS = -ffixed-line-length-132
export PODS=libmf.pod
export EXTRAS=libmf_modules.o gettrk_modules.o \
              ftn_libmf.o gettrk_routines.o \
              ftn_clhilo.o ftn_grhilo.o ftn_tmhilo.o ftn_smth2d.o \
              ftn_uv2trw.o ftn_tcprop.o ftn_xy2rt.o ftn_tcxy2rt.o \
              ftn_re2.o ftn_esmrf.o ftn_linreg.o ftn_grhist.o


%: $(EXTRAS)
	@$(MAKE) -f ../gex.mk $@  CSRC=libmf.c 

$(EXTRAS):
	@$(MAKE) -f ../gex.mk $@  


