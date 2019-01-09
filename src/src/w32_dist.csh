#!/bin/tcsh 
#
#    Copyright (C) 2008 by Arlindo da Silva <dasilva@opengrads.org>
#    All Rights Reserved.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; using version 2 of the License.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, please consult  
#              
#              http://www.gnu.org/licenses/licenses.html
#
#    or write to the Free Software Foundation, Inc., 59 Temple Place,
#    Suite 330, Boston, MA 02111-1307 USA
#
#--------------------------------------------------------------------------
#
# w32_dist.csh 
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
# Preserving this structure is important for the frontend mechanism;
# see "main_win32.c" for details.
#
# If you are bundling the OpenGrADS extensions with the binaries
# (recommended), you need to manually run the w32_dist.csh script
# in the ../extensions directory.
#
#------------------------------------------------------------------------- 

# Directories referenced by this script
# -------------------------------------
  set   mach = win32
  set    BASE="../../PCGrADS_base"
  set  PREFIX="../../PCGrADS"
  set    BIN=$PREFIX/$mach
# set   FUTIL=../win32/bin
#  set    XDIR=/usr/X11R6
  set    XDIR=/usr
  set     SRC=..  

# Make sure all binaries/DLLs are current
# ---------------------------------------
  make all
  if ( $status ) then
    echo "ERROR: 'make all' failed, so Win32 distribution tree did not complete"
    exit 1
  endif

# Init the distribution from base files
# -------------------------------------
  if ( ! (-d $PREFIX/Xming) ) then

    echo "Initializing Win32 GrADS distribution under $PREFIX"

    if ( -d $BASE/Xming ) then

#       Create fresh distribution directory from base
#       ---------------------------------------------
	/bin/rm -rf $PREFIX
	foreach dir ( $BIN $BIN/gex $PREFIX/dat $PREFIX/data \
                      $PREFIX/doc/gadoc )  
	    /bin/mkdir -p $dir
	end
	/bin/cp -r $BASE/* $PREFIX

#       Fix extension: from *.ex_ to *.exe
#       ----------------------------------
	foreach file ( $PREFIX/Xming/*.ex_ $PREFIX/win32/*.ex_ )
	   /bin/mv $file $file:r.exe
        end

#       Next data and documentation
#       ---------------------------
        /bin/cp -r $SRC/doc/*           $PREFIX/doc/gadoc
        /bin/cp -r $SRC/data/*          $PREFIX/dat
        /bin/cp -r $SRC/pytests/data/*  $PREFIX/data

#       GNU sh, tcsh and basic GNU file utils
#       -------------------------------------
        foreach exe ( tcsh sh mount umount unix2dos dos2unix \
                      awk sed ls ln cp rm mv dir pwd cat date mkdir \
                      grep tar gzip )
          /bin/cp /bin/$exe.exe $BIN
        end

#   Must have a base distribution
#   -----------------------------
    else
        echo "Cannot find Win32 BASE distribution under <$BASE>"
        echo "Please check out or download PCGrADS base and install it"
        echo " as $BASE"
	exit 1

    endif
  endif   # init distribution tree

# Copy over information files
# ---------------------------
  /bin/cp $BASE/*.html $PREFIX 
  foreach file ( README NEWS ChangeLog COPYRIGHT )
       /bin/cp $SRC/$file $PREFIX/$file.txt
  end
  /bin/cp $SRC/INSTALL.win32 $PREFIX/INSTALL.txt

# Now copy GrADS binaries and DLLs
# --------------------------------
  echo "Copying GrADS binaries from here..."
  /bin/cp *.exe $BIN
  /bin/cp *.dll $BIN

# Make sure all needed DLLs are included in the distro
# ----------------------------------------------------
  echo ""
  set path = ( $BIN $path . )
  foreach file ( $BIN/*.exe )
   set ok = 1
   set fbase = `basename $file`
   echo "Checking DLLs for $fbase"
   foreach DLL ( `cygcheck $file |& grep -v WINDOWS | sed -e 's=\\=/=g'`  )
      set dirn = `dirname $DLL`
      set base = `basename $DLL`
      if ( "$dirn" != "$BIN" ) then
         if ( -e $DLL ) then
            echo "     adding DLL: $base"
            /bin/cp $DLL $BIN
         else
            echo "     cannot find  DLL: $base"
            set ok = 0
         endif
      endif
   end
  end
 
# All done
# --------
  if ( $ok ) then
     echo "Win32 GrADS distribution tree completed."  
     exit 0
  else
     echo "ERROR: DLLs missing from distribution"
     exit 1
  endif



