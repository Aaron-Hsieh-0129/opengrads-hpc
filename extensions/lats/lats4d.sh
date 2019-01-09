#!/bin/sh
#
# Simple sh(1) script to start GrADS and LATS4D, exiting from GrADS
# upon completion.
#
# Revision history:
#
# 29dec1999   da Silva  First crack. 
# 05jan1999   da Silva  Minor mod.
# 04oct2005   Todling   Added pointer for lats4d.gs in the build
# 10may2006   da Silva  Auto detection of install dir
# 15Nov2008   da Silva  Added gradsdap, simplified; renamed 
#                       lats4d --> lats4d.sh
#
#-------------------------------------------------------------------------

# Use full patch names if you wish

gradsbin="../../opengrads/Contents/opengrads"
lats4dgs="./lats4d.gs"

if [ $#0 -lt 1 ]; then
	echo "          "
	echo "NAME"
	echo "     lats4d - file conversion and subsetting utility"
	echo "          "
        echo "SYNOPSIS"
        echo "     lats4d [-nc] [-hdf]  option(s)"
	echo "          "
	echo "DESCRIPTION"
	echo "     lats4d is a command line interface to GrADS"
        echo "     and the lats4d.gs script. It starts either"
        echo "     gradshdf or gradsnc depending on the -hdf/-nc"
        echo "     option specified, runs lats4d.gs, and exits"
	echo "     from GrADS upon completion.     "
	echo "          "
	echo "     For additional information on LATS4D enter: lats4d -h"
	echo "          "
	echo "OPTIONS"
	echo " -nc        for producing GRIB or NetCDF  files"
	echo " -hdf       for producing GRIB or HDF-SDS files (default)"
	echo " -dods      for reading OPeNDAP URLs with gradsdods (v1.9)"
	echo " -dap       for reading OPeNDAP URLs with gradsdap (v2.0)"
	echo " option(s)  for a list of lats4d options enter: lats4d -h"
	echo "          "
	echo "IMPORTANT"
	echo "     You must specify the input file name with "
        echo "     the \"-i\" option."
	echo "          "
        echo "SEE ALSO  "
        echo "     http://opengrads.org/wiki/index.php?title=LATS4D"
	echo "          "
	exit 1
fi

if [ "$1" = "-nc" ];  then
	gradsbin="gradsnc"
	shift
elif [ "$1" = "-hdf" ]; then
	gradsbin="gradshdf"
	shift
elif [ "$1" = "-dods" ]; then
	gradsbin="gradsdods"
	shift
elif [ "$1" = "-dap" ]; then
	gradsbin="gradsdap"
	shift
fi

echo $gradsbin -blc \'run $lats4dgs -q $@ \'
eval $gradsbin -blc \'run $lats4dgs -q $@ \'
