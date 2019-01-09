#!/bin/bash
#
#   Simple script to create an OpenGrADS Bundle.
#

if [ $#0 -lt 1 ] ; then
     prefix=./opengrads
else
     prefix=$1
fi

#............................................................

function init {

arch=`uname -s`
mach=`uname -m`

preserve=no

copy="rsync -Cax"

std_files="ChangeLog COPYRIGHT INSTALL NEWS README src/VERSION"

version=`cat src/VERSION`

}

#............................................................

function liberate {

  xlist=""
  for file in bin/* bin/gex/*
  do
     if test -f $file -a -x $file ; then
         xlist="$xlist $file"
     fi 
  done

  if test "$arch" = Darwin; then

      export DYLD_LIBRARY_PATH="../supplibs/lib:$DYLD_LIBRARY_PATH"
      dep_libs=`otool -L $xlist 2>&1 | grep '/' | grep -v ':' | awk '{print $1}' | sort | uniq`  
      int_libs=`otool -L $xlist 2>&1 | grep '/' | grep -v -e ':' -e 'X11' -e System -e libgcc_s | awk '{print $1}' | sort | uniq`  

  else

     export LD_LIBRARY_PATH="../supplibs/lib:$LD_LIBRARY_PATH"
     dep_libs=`ldd $xlist 2>&1 | grep '/' | grep -v ':' | awk '{print $3}' | sort | uniq`  
     int_libs=`ldd $xlist 2>&1 | grep '/' | grep -v -e ':' -e 'X11' -e System | awk '{print $3}' | sort | uniq`  

  fi

  ext_libs=""
  for lib in $dep_libs
  do
      lib_=$lib
      for ilib in $int_libs
      do
          if test $lib = $ilib ; then
             lib_=""
          fi
      done
      ext_libs="$ext_libs $lib_"
   done

  echo 
  echo "Included Shared Libraries"
  echo "-------------------------"
  if [[ "x$int_libs" != x ]] ; then
         ls -1 $int_libs
  fi

  echo 
  echo "External Shared Libraries"
  echo "-------------------------"
  if [[ "x$ext_libs" != x ]] ; then
        ls -1 $ext_libs
  fi

}

#............................................................
 
 if init && liberate
 echo
 then
    echo $0: All done.
 else 
    echo $0: did not complete
    exit 1
 fi

 
