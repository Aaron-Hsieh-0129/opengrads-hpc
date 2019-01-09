#!/bin/tcsh 
#
# w32_dist.csh - last update: 20 jan 2008
#
# Script for creating a Win32 distribution.
#
# Notice that it is important to preserve the following directory
# structure:
#
# $(TARGETDIR)/win32       binaries, e.g., gradsc.exe
# $(TARGETDIR)/dat         map database, fonts, etc.
# $(TARGETDIR)/lib         gs and gui scripts
# $(TARGETDIR)/doc         documentation
#
# where $(TARGETDIR) is wherever the user chooses for installation.
# Preserving this structure is important for the frontend mechanism.
#
# Send questions to Arlindo da Silva (dasilva@opengrads.org)
# 

# Directories referenced by this script
# -------------------------------------
    set  PREFIX="$PWD/../../PCGrADS"
  set  BIN=$PREFIX/win32

# Create directories if they are not there
# ----------------------------------------
  /bin/mkdir -p $PREFIX $BIN

# Make and install extensions for each type of binary
# ---------------------------------------------------
  foreach bintype ( c dods nc nc4 hdf )
    make clean   bintype=$bintype bindir=$BIN
    make all     bintype=$bintype bindir=$BIN
    make install bintype=$bintype bindir=$BIN
  end
