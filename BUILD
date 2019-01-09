                    Building GrADS from Sources
                    ===========================


Quick Instructions
------------------

Here is the quickest way to build OpenGrADS from sources:

    * Choose a working directory and make sure you have GNU make.
    * Download the Supplemental Libraries (Supplibs)
      preferably pre-compiled, and untar them in this working directory
    * If you download the /supplibs/ sources instead, untar them in the
      same location and build: 

 % ln -s supplibs-2.2.1 supplibs
 % cd supplibs/src
 % gmake install     (do not configure or issue a simple make)

    * Download the grads sources, untar them in this same working
      directory, configure and make 

 % cd grads-2.0.x
 % ./configure
 % gmake 
 % gmake check
 % gmake bundle-dist   (to create a bundle tarball)

See file INSTALL for installing this bundle.

Read on if you need more information.

Introduction
------------

There are three different methods for having GrADS installed on your
computer:

   1. If it is included in your Linux distribution, for example, recent
      versions of Red Hat and Fedora distributions, install it directly
      from your CD or from the net with update tools such as yum, apt or
      urpmi.
   2. Install one of the binaries distributions from the GrADS download
      page <http://grads.iges.org/grads/download> at COLA. This usually
      works for a large number of platforms and gives you binaries
      loaded with all the features.
   3. If 1) or 2) fail, or for security reasons you prefer to build from
      sources, this is a viable option as well. The information in this
      page will guide you through the necessary steps in buiding GrADS
      from sources: from a fully featured build to a customized build
      that fits your needs. 

Like most applications of this nature, many of GrADS capabilities are
implemented by means of external libraries: NetCDF, HDF, OPeNDAP, to
name a few. These dependencies are fairly complex packages themselves,
and porting them to a new platform require experience with this sort of
activity. The good news is, these packages build fairly easily in
popular platforms such as Linux, Mac OS X, Unices and even Windows. The
GrADS code itself is very compact and easily portable. If you are going
to have a problem, it will most likely be building one of the external
packages, not GrADS.

There are a few options to satisfy GrADS external dependencies:

   1. The GrADS developers maintain a central repository with source
      code for these packages, with binaries available for the supported
      platforms. See Supplemental Libraries (Supplibs)
      </wiki/index.php?title=Supplemental_Libraries_%28Supplibs%29> for
      more information.
   2. Your OS distributions may provide development version for a number
      of these libraries. For Linux in particular, you would want to
      install packages with names having /-devel/ in it as these will
      include the header files necessary to compile GrADS.
   3. Or else, you download the sources from the package website and do
      the build yourself. Compared to using sources from the /supplibs/
      in 1) the only risk of this method is that you may end up using a
      version of the package that is not quite compatible with GrADS.
      You have been advised. 

However, except for the X11 client libraries, you do not need to resolve
all the external dependences. If the GrADS configure script cannot find
the necessary package, it will disable the GrADS feature that required
that package. The OpenGrADS Wiki will help identifying which packages are
relevant to build GrADS with the features that you need.

     http://opengrads.org/wiki/index.php?title=OpenGrADS_Documentation
