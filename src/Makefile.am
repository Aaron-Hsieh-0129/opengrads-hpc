##
##          Makefile.am  for GrADS
##
######################################################################

######################################################################
#
# Targets
#

# Targets that will built by "make all"
bin_PROGRAMS	       = grads sgrads\
		         gribmap stnmap wgrib gribscan bufrscan \
			 grib2scan

# Targets that will not always be built
EXTRA_PROGRAMS	       = # 

# Convenience target
utils: bufrscan gribscan gribmap stnmap wgrib $(extra_utils)

######################################################################
#
# Compiler and linker settings
#

# Paths to external headers and libraries
supp_include_dir	= $(SUPPLIBS)/include
supp_lib_dir		= $(SUPPLIBS)/lib

# Settings used for all binaries
LDADD			= -L$(supp_lib_dir) 
AM_CPPFLAGS		= -DOPENGRADS \
                          $(readline_inc) $(printim_inc) $(grib2_inc) $(gui_inc) $(nc_inc) \
			  $(hdf_inc) $(hdf5_inc) $(geotiff_inc) $(shp_inc) $(gadap_inc) \
                          $(X_CFLAGS) $(XAW_CFLAGS) $(GD_CFLAGS) $(HDF4_CFLAGS) $(cairo_inc)

# Settings used for all GrADS binaries
common_ldadd            = $(LDADD) $(X_LIBS) $(host_ldadd) -lX11

######################################################################
#
# Headers and data
#

# Headers must be listed here to be included in the distribution.  The
# "noinst_" prefix prevents "make install" from trying to do anything
# with them
hdr_core            = gagmap.h grads.h gs.h gatypes.h \
		      gvt.h gx.h gxmap.h wx.h
hdr_bufr	    = gabufr.h
hdr_x11		    = gx.h bitmaps.h
hdr_sdf             = gasdf.h gasdf_std_time.h 
hdr_gui		    = gagui.h
hdr_png             = mtables.c
hdr_cairo           = gxC.h
noinst_HEADERS	    = $(hdr_core) $(hdr_x11) $(hdr_sdf) $(hdr_png) $(hdr_gui) $(hdr_bufr) $(hdr_cairo)

# Get rid of buildinfo.h on "make distclean"
DISTCLEANFILES	       = buildinfo.h


######################################################################
#
# Configuration-dependent sources & headers
#

if USEGADAP
src_gadap 	    = dodstn.c
obj_gadap           = dodstn.o
endif

if USEGUI
src_gui             = gagui.c gsgui.c
endif


##############################################################
#
# Sources for GrADS binary

# Source files to be compiled
grads_SOURCES          = main.c grads.c gxsubs.c gxmeta.c gxchpl.c gxcntr.c gxstrm.c \
			 gxwmap.c gxshad.c gxshad2.c gaexpr.c gafunc.c gautil.c gagx.c \
			 gscrpt.c gamach.c bufrstn.c gabufr.c gabufrtbl.c \
			 gxX.c gxdxwd.c galloc.c $(src_gui) $(src_gadap) \
                         gaddes.c gaio.c gacfg.c gauser.c gasdf.c gatxt.c gaudx.c \
                         gxC.c gxprint.c gxdb.c

# Sources that will not always be compiled
# Since gradsc is the only GrADS binary that is not conditionally compiled,
# any source file that we want to go in the distribution must be listed.
#EXTRA_grads_SOURCES    = gagmap.c gagui.c gsgui.c gxhpng.c dodstn.c
EXTRA_grads_SOURCES    = gagmap.c gagui.c gsgui.c dodstn.c


# libraries needed 
grads_LDADD	       = $(common_ldadd) $(readline_libs) $(printim_libs) \
			 $(grib2_libs) $(hdf_libs) $(hdf5_libs) $(nc_libs) \
			 $(gui_libs) $(geotiff_libs) $(shp_libs) $(dap_libs) $(cairo_libs)

# Custom compilation for object files specific to this GrADS binary
COMPILE_C = $(COMPILE) 

##############################################################
#
# Sources for Secure GrADS binary

# Source files to be compiled
sgrads_SOURCES         = smain.c grads.c gxsubs.c gxmeta.c gxchpl.c gxcntr.c gxstrm.c \
			 gxwmap.c gxshad.c gxshad2.c gaexpr.c gafunc.c gautil.c gagx.c \
			 gscrpt.c gamach.c bufrstn.c gabufr.c gabufrtbl.c \
			 gxX-stubs.c galloc.c $(src_gui) $(src_gadap) \
                         gaddes.c gaio.c gacfg.c gauser.c gasdf.c gatxt.c gaudx.c \
                         gxC.c gxprint.c gxdb.c

# Sources that will not always be compiled
# Since gradsc is the only GrADS binary that is not conditionally compiled,
# any source file that we want to go in the distribution must be listed.
#EXTRA_grads_SOURCES    = gagmap.c gagui.c gsgui.c gxhpng.c dodstn.c
EXTRA_sgrads_SOURCES    = gagmap.c gagui.c gsgui.c dodstn.c


# libraries needed 
sgrads_LDADD	       = $(common_ldadd) $(readline_libs) $(printim_libs) \
			 $(grib2_libs) $(hdf_libs) $(hdf5_libs) $(nc_libs) \
			 $(gui_libs) $(geotiff_libs) $(shp_libs) $(dap_libs) $(cairo_libs)

##############################################################
#
# gradsdap
#

# If gadap is enabled, use C++ linker instead of C linker
if USEGADAP
grads_LINK          = $(CXX) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
sgrads_LINK         = $(CXX) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
else
grads_LINK          = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
sgrads_LINK         = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
endif

# Object files that should not be reused by the other GrADS binaries,
# plus libraries needed by this GrADS binary


# Custom compilation for object files specific to this GrADS binary
COMPILE_DAP = $(COMPILE) 


##############################################################
#
# Utilities
#

# Object files that need to be recompiled for standalone utilities
gaddes.sa.o: $(srcdir)/gaddes.c
	$(COMPILE) -DSTNDALN -o gaddes.sa.o -c $(srcdir)/gaddes.c

gautil.sa.o: $(srcdir)/gautil.c
	$(COMPILE) -DSTNDALN -o gautil.sa.o -c $(srcdir)/gautil.c

##############################################################
#
# stnmap
#
stnmap_SOURCES	       = stnmap.c gamach.c galloc.c
stnmap_LDADD	       = gaddes.sa.o gautil.sa.o gatxt.o $(LDADD) $(readline_libs)

##############################################################
#
# gribmap
#
gribmap_SOURCES	       = gribmap.c gagmap.c gamach.c galloc.c
gribmap_LDADD	       = gaddes.sa.o gautil.sa.o gatxt.o $(LDADD) $(readline_libs) $(grib2_libs)

##############################################################
#
# wgrib
#
wgrib_SOURCES	       = wgrib.c

##############################################################
#
# gribscan
#
gribscan_SOURCES       = gribscan.c gamach.c galloc.c
gribscan_LDADD	       = gautil.sa.o gatxt.o $(LDADD) $(readline_libs)

##############################################################
#
# grib2scan
#
grib2scan_SOURCES      = grib2scan.c gamach.c galloc.c
grib2scan_LDADD	       = gautil.sa.o gatxt.o $(LDADD) $(grib2_libs) $(readline_libs)

##############################################################
#
# bufrscan
#
bufrscan_SOURCES       = bufrscan.c gabufr.c gabufrtbl.c gamach.c

