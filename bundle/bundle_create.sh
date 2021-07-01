#!/bin/bash
#
#   This script creates an OpenGrADS Bundle. On Windows, the option "-super" supersizes
# the bundle creating the so-called "superpack".
#

   if test "x$SUPPLIBS" = "x"; then   
       echo "$0: environment variable SUPPLIBS not set"
       exit 1
   fi
   if test -d "$SUPPLIBS"; then
       echo "$0: Using $SUPPLIBS for SUPPLIBS"
   else
       echo "$0: $SUPPLIBS not a valid SUPPLIBS directory."
       exit 1
   fi


#  Command line arguments
#  ----------------------
   superpack=no
   macpkg=no
   if test "$1" = "-super" ; then
      superpack=yes
      shift 
   elif test "$1" = "-macpkg" ; then
       macpkg=yes
       shift
   fi

   if [ $#0 -lt 1 ] ; then
       prefix=./opengrads
   else
       prefix=$1
   fi

   function init {

#  Identify system
#  ---------------
   arch=`uname -s`
   mach=`uname -m`
   case $arch in
    CYGWIN*)
	  arch=Cygwin
	  ;;
    Darwin*)
	  mach=`uname -p`
	  ;;
    AIX*)
	  mach=`uname -p`
	  ;;
   esac

   preserve=no
   #copy="rsync -Cax"
   copy="rsync -ax"
   std_files="ChangeLog COPYRIGHT INSTALL NEWS README"
   version=`cat cola/src/VERSION`
}

#............................................................

function prepare_dirs {

  echo "$0: Preparing directories"

# Bundle directories
# ------------------
  root=$prefix
  contents=$root/Contents

  b_exec=$contents/$arch/Versions/$version/$mach  # where binaries really are

  b_data=$contents/Resources/SupportData            # fonts & maps datasets
  b_dset=$contents/Resources/SampleDatasets         # sample datasets
  b_scrp=$contents/Resources/Scripts                # support grads scripts    
  b_docs=$contents/Resources/Documentation          # HTML documentation
  x_ming=$contents/Resources/Xming                  # Xserver (Win32 only)
  b_icon=$contents/Resources/Icons
  
  if test "$arch" = Cygwin ; then
      b_temp=$contents/tmp                          # temp dir for windows only (last resort) 
      b_wbin=$contents/$arch/Versions/$version/bin  # without cygwin, /bin
      b_wtmp=$contents/$arch/Versions/$version/tmp  # without cygwin, /tmp
  else
      b_temp=""
      b_wbin=""
      b_wtmp=""
  fi
  
  r_exec=Contents/$arch/Versions/$version/$mach   # where binaries really are
  r_data=Contents/Resources/SupportData           # fonts & maps datasets
  r_scrp=Contents/Resources/Scripts               # support grads scripts
  r_dset=Contents/Resources/SampleDatasets        # support grads scripts
  
# Classic directories
# ------------------
  classic=$root/Classic

# Start with a clean slate unless -p is specified
# -----------------------------------------------
  if test "$preserve" = yes; then
     if [[ -d $root ]] ; then
        echo "$0: Using existing directory $root"
     fi
  else
     if [[ -d $root ]] ; then
        echo "$0: Cleaning up pre-existing directory $root" 
        if ! rm -rf $root; then return 1; fi 
     else
        echo "$0: Creating new directory $root"
     fi
  fi

# Create directory structure
# --------------------------
  if ! mkdir -p $b_exec $b_temp $b_wbin $b_wtmp $b_data $b_dset $b_scrp $b_docs $b_icon; then return 1; fi

  rm -rf $contents/$arch/Versions/Current@ $contents/$arch/$mach'@'

# Note: symlinks are not portable - windows/dos file systems do not
#       understand it.
###  (cd $contents/$arch/Versions; ln -sf $version Current)
###  (cd $contents/$arch;          ln -sf Versions/Current/$mach .)

# Fake symlinks: create a text file ending in "@" with the true 
# location of the file directory inside
# ---------------------------------------------------------------          
  (cd $contents/$arch/Versions; echo $version > Current@ )
  (cd $contents/$arch;          echo Versions/Current@/$mach > ./$mach'@')

# Classic symlinks
# ---------------
  rm -rf   $classic
  mkdir -p $classic
  (cd $classic; ln -sf ../$r_exec bin)    
  (cd $classic; ln -sf ../$r_data data)
  (cd $classic; ln -sf ../$r_scrp scripts) 
  (cd $classic; ln -sf ../$r_dset test_data)	 

  }
#............................................................

function compile_stuff {

  if test "$arch" = FreeBSD ; then
     make="gmake -e"
  else
     make=make
  fi 

# Build and install under bin/
# ----------------------------
  echo $0: Building GrADS
  if ! $make cola-install          > /dev/null; then return 1; fi
  echo $0: Building Extensions
  if ! $make -C extensions install > /dev/null; then return 1; fi

}

#............................................................

function populate {

# Populating bundle
# ----------------
  echo $0: Copying Files with Version `cat cola/src/VERSION`
  if ! $copy cola/src/VERSION          $b_exec; then return 1; fi

  if test "$arch" = Cygwin; then
     for file in $std_files
     do
         if ! $copy $file	       $root/$file.txt  ; then return 1; fi 
	 if ! unix2dos                 $root/$file.txt  ; then return 1; fi
     done
     if ! $copy Getting_Started.html   $root;   then return 1; fi
     if ! $copy win32/*.ico            $b_icon; then return 1; fi         
     if ! $copy win32/icons/*.gif      $b_icon; then return 1; fi         
     if ! cp bin/*.exe                 $b_exec; then return 1; fi
     if ! $copy bin/*                  $b_exec; then return 1; fi
  else
     if test "$macpkg" = yes ; then
        if ! $copy $std_files          $contents  ; then return 1; fi 
     else
        if ! $copy $std_files          $root  ; then return 1; fi 
     fi
     if ! $copy bin/*                  $b_exec; then return 1; fi
  fi

  if ! $copy lib/scripts/*             $b_scrp; then return 1; fi
  if ! $copy data/* cola/data/*        $b_data; then return 1; fi
  if ! $copy pytests/data/model.ctl \
             pytests/data/model.gmp \
             pytests/data/model.grb \
                                       $b_dset; then return 1; fi

# Documentation
# -------------
  if ! $copy doc/*         $b_docs ; then return 1; fi
  if ! $copy cola/doc/*    $b_docs ; then return 1; fi
# php Documentation.php >  $contents/Documentation.html
  cp Documentation.html    $contents/Documentation.html

# Move gsUDFs to Scripts directory
# --------------------------------
  if ! mv $b_exec/gex/*.gs*            $b_scrp; then return 1; fi

}

#...................................................................

function supersize_it {

   if test "$superpack" != yes    ; then return; fi
   if test "$arch"      != Cygwin ; then return; fi

   echo $0: Supersizing it
   
# Handy posix utilities
# ---------------------
  for exe in tcsh sh uname test mount umount unix2dos dos2unix \
              awk sed ls ln cp rm mv dir pwd cat date mkdir \
              grep tar wget gzip cut less diff head tail tee \
              rxvt touch wc sort uniq sleep 
  do			  
       if ! /bin/cp /usr/bin/$exe $b_exec/$exe.exe ; then return 1; fi
  done

# Put shells under /bin so that system() work
# -------------------------------------------
  if ! /bin/cp $b_exec/sh.exe   $b_wbin ; then return 1; fi
  if ! /bin/cp $b_exec/tcsh.exe $b_wbin ; then return 1; fi
   
# X server (Xming)
# ----------------
  if ! $copy win32/Xming/* $x_ming ; then return 1; fi
  
# Other utilities (gv, etc.)
# --------------------------
  if ! $copy win32/utils/* $b_exec ; then return 1; fi
 
# Fix .ex_ extensions (this funny extension is to trick CVS into
# checking in these files)
# --------------------------------------------------------------
  for file in $x_ming/*.ex_ $b_exec/*.ex_ 
  do
    if ! mv $file ${file%.ex_}.exe ; then return 1; fi 
  done
  
  }

#...........................................................................

function liberate_unix {

  echo $0: Copying Dependencies
  xlist=""
  for file in bin/* bin/gex/*
  do
     if test -f $file -a -x $file ; then
         xlist="$xlist $file"
     fi 
  done

# Make a list of DLLs to include in the distro
# --------------------------------------------
  if test "$arch" = Darwin; then

      dllext=dylib
      export DYLD_LIBRARY_PATH="$SUPPLIBS/lib:$DYLD_LIBRARY_PATH"
      dep_libs=`otool -L $xlist 2>&1 | grep '/' | grep -v -e ':' -e System  | awk '{print $1}' | sort | sed -e s@/usr/lib/.\*\b@@g | uniq`
      for lib in $dep_libs
      do
	  echo "---> $lib"
      done
      
   elif test "$arch" = Cygwin; then
      dllext=dll
      export PATH="$SUPPLIBS/bin:$PATH"
      dep_libs=`cygcheck $xlist 2>&1 | grep '/' | grep -v -e 'WINDOWS' | awk '{print $1}' | sort | uniq`  

  elif test "$arch" = AIX; then
      dllext=so
      echo "----------- skipping -------------"
      return
  else

     dllext=so

     # export LD_LIBRARY_PATH="$SUPLLIBS/lib:$LD_LIBRARY_PATH"
     dep_libs=`ldd $xlist 2>&1 | grep '/' | grep -v -e ':' -e System -e $SUPPLIBS | awk '{print $3}' | sort | uniq`  

  fi

# Copy most libraries to the libs/ subdirectory as gex/ will be under
# LD_LIBRARY_PATH/DYLD_LIBRARY_PATH and may cause conflict
# -------------------------------------------------------------
  if ! mkdir -p $b_exec/libs $b_exec/gex ; then return 1; fi
  if ! cp -f  $dep_libs $b_exec/libs; then return 1; fi
  
# Now put under gex/ the ones that are likely not to cause conflict
# -----------------------------------------------------------------
  echo "$0: Adding supplibs shared libraries to gex/"
  for Lib in $SUPPLIBS/lib/*.$dllext*
  do
     if ! cp -pr  $Lib  $b_exec/gex; then return 1; fi
     lib=`basename $Lib | sed -e 's/^lib//' -e "s/\.$dllext//"`
     echo  "$0:   - $lib "
  done
  echo "$0: Adding X shared libraries to gex/"
  for Lib in $b_exec/libs/lib[xX]* $b_exec/libs/libICE* \
             $b_exec/libs/libSM* 
  do
      if ! mv  $Lib $b_exec/gex; then return 1; fi
      lib=`basename $Lib | sed -e 's/lib//' -e "s/\.$dllext//"`
      echo "$0:   - ${lib%.*}"
  done
  candidates="bz2 c++ crypto gfortran iconv lzma quadmath stdc++ uuid" 
  echo "$0: Adding system shared libraries to gex/"
  for Lib in $b_exec/libs/* 
  do
      lib=`basename $Lib | sed -e 's/lib//' -e "s/\.$dllext//"`
      for pat in $candidates; do
	  if expr $lib : ^$pat >/dev/null ; then
	      if ! mv  $Lib $b_exec/gex; then return 1; fi
	      echo "$0:   - ${lib%.*}"
	  fi
      done
  done
  echo "$0: Adding backup shared libraries to libs/"
  for Lib in $b_exec/libs/*
  do
      libfn=`basename $Lib`
      if test -e $b_exec/gex/$libfn; then
          /bin/rm $Lib # avoid duplication
      else
          lib=`basename $Lib | sed -e 's/lib//' -e "s/\.$dllext//"`
          echo "$0:   - ${lib%.*}"
      fi
  done


}

#...........................................................................

function liberate_win32 {

  echo $0: Copying Dependencies
  xlist=""
  for file in $b_exec/*.exe $b_exec/*.dll $b_exec/gex/*.gex
  do
     if test -f $file -a -x $file ; then
         xlist="$xlist $file"
     fi 
  done

# Make a list of DLLs to include in the distro
# --------------------------------------------
  export PATH="../supplibs/bin:$PATH"
  dep_libs=`cygcheck $xlist 2>&1 | grep -i -v -e '\.exe' -e '\.gex' -e 'WINDOWS' -e libgrads | awk '{print $1}' | sort | uniq`  

# I believe rxvt does a dlopen on libW11
# --------------------------------------
  dep_libs="$dep_libs /bin/libW11.dll"

# Copy dependency libraries alongside the binaries since Windows
# look for DLLs in PATH
# -------------------------------------------------------------
  if ! cp -pr $dep_libs $b_exec; then return 1; fi

}

#...........................................................................

function liberate {

# Windows is so different that it has its own liberate
# ----------------------------------------------------
  if test "$arch" = Cygwin; then
      if ! liberate_win32 ; then return 1; fi
  else
      if ! liberate_unix  ; then return 1; fi
  fi

}

#.....................................................................................

function add_wrappers {

  if test "$arch" = Cygwin ; then
      exe=".exe"
  else
      exe=""
  fi
  
# Add wrappers for each binary added to bundle
# --------------------------------------------
  echo $0: Adding wrappers

# Windows superpack gets a few VBScript wrappers at the very top
# --------------------------------------------------------------
 if test "$superpack" = yes; then
    if ! $copy bundle/bundle_wrapper.vbs $root/grads.vbs;     then return 1;fi
    if ! $copy bundle/bundle_wrapper.vbs $root/opengrads.vbs; then return 1;fi
    #if ! $copy bundle/bundle_wrapper.vbs $root/gradsdap.vbs;  then return 1;fi
    if ! $copy bundle/bundle_wrapper.vbs $root/gradsgui.vbs;  then return 1;fi
    #if ! $copy bundle/bundle_wrapper.vbs $root/merra.vbs;     then return 1;fi
    if ! $copy bundle/bundle_wrapper.vbs $root/geos.vbs;     then return 1;fi
    if ! $copy bundle/bundle_wrapper.vbs $root/ncep.vbs;    then return 1;fi
    #if ! $copy bundle/bundle_wrapper.vbs $root/gv32.vbs;      then return 1;fi
 fi

#  Add the regular Perl wrappers under Contents
# ---------------------------------------------
  for file in $b_exec/*$exe
  do
      if [[ -x $file && ! -d $file ]]; then
          base=`basename $file`
          if ! $copy bundle/bundle_wrapper.pl $contents/${base%$exe}; then return 1;fi
      fi
  done
  
# Special handling of lats4d.sh
# -----------------------------
  if ! $copy bundle/lats4d.sh $contents; then return 1;fi
  if test "$arch" = Cygwin ; then
     if ! $copy bundle/lats4d.bat $b_exec; then return 1;fi
  fi

# Cygwin already got these
# -------------------------  
  if test "$arch" != Cygwin ; then
      if ! $copy bundle/bundle_wrapper.pl $contents/opengrads; then return 1;fi
      # if ! $copy bundle/bundle_wrapper.pl $contents/gradsdap;  then return 1;fi
      # if ! $copy bundle/bundle_wrapper.pl $contents/merra;     then return 1;fi
      if ! $copy bundle/bundle_wrapper.pl $contents/geos;     then return 1;fi
      if ! $copy bundle/bundle_wrapper.pl $contents/ncep;      then return 1;fi
  fi
    
}

#.....................................................................

 if init && prepare_dirs && compile_stuff && populate  \
         && supersize_it && liberate      && add_wrappers  
 then
    if test "$macpkg" = yes; then
       mv $root/Contents $root/OpenGrADS
    fi
    echo $0: All done.
 else 
    echo $0: did not complete
    exit 1
 fi
 










