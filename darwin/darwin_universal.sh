#!/bin/sh 
#
# Build universal Mac OS X binaries. It uses the cross compilation
# feature of gcc.
#
# This script is in the public domain.
# Contributed by: "Arlindo da Silva"<dasilva@opengrads.org>
#
#-------------------------------------------------------------------


# Location of each supplib
# ------------------------
  I386_SUPPLIBS=`(cd ../../supplibs-2.1.0/i686-apple-darwin9.6.0; pwd)`
  PPC_SUPPLIBS=`(cd ../../supplibs-2.1.0/powerpc-apple-darwin9.6.0; pwd)`

  arch=`arch`
  triplet=`./config.guess | sed -e "s/i386/powerpc/g" -e "s/i686/powerpc/g"`
  build_info="Built `date` for ${triplet} on a `arch` host"

# This section remains the same for any type of cross-compilation.
# ----------------------------------------------------------------
  export HOST_CC="gcc"
  export HOST_CXX="g++"
  export RANLIB=ranlib
  export AR=ar
  export AS=$CC
  export LD=ld
  export STRIP="strip -x -S"
  export CROSS_COMPILE=1

# Loop over architectures...
# --------------------------
  for arch in "ppc" "i386"
  do

      export CC="gcc  -arch $arch"
      export CXX="g++ -arch $arch"
 
      case $arch in
      ppc)
###          export SUPPLIBS=${PPC_SUPPLIBS}
             rm -rf ../supplibs
             ln -s ${PPC_SUPPLIBS} ../supplibs
      ;;
      i386)
###          export SUPPLIBS=${I386_SUPPLIBS}
             rm -rf ../supplibs
             ln -s ${I386_SUPPLIBS} ../supplibs
      ;;
      esac

      make distclean
      if ./configure --prefix=`pwd`/$arch --target $arch 
      then echo "Configured OK for $arch"
      else 
	  echo "Failed to configure for $arch"
	  exit 1
      fi
      echo "static char *buildinfo = \"${build_info}\";" > src/buildinfo.h

      if make
      then echo "Built OK for $arch"
      else 
	  echo "Failed to build on $arch"
	  exit 1
      fi

      if make install
      then echo "Installation successful for $arch"
      else 
	  echo "Failed installation for $arch"
	  exit 1
      fi
      
  done

# Use lipo to make fat universal binaries; place binaries under src/
# so that we can use make dist-bin next
# -----------------------------------------------------------------
  for file in ppc/bin/*
  do
      f=`basename $file`
      lipo -create -output src/$f ppc/bin/$f i386/bin/$f
  done 

# Now make binary distro with universal binaries
# ----------------------------------------------
  make bin-dist host_triplet=${triplet} prefix=`pwd`

  
