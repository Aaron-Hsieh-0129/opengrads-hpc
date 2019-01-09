#!/usr/bin/perl 
#
#    Copyright (C) 2009 by Arlindo da Silva <dasilva@opengrads.org>
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
# ---
#
#   Generic wrapper script for running GrADS and utilities from the
#   the top Contents/ directory.
#

use Strict;
use Env qw(PATH); 
use FindBin;
use File::Basename;
use Getopt::Long;

# Platform Detection
# ------------------
  chomp($arch=`uname -s`);
  chomp($mach=`uname -m`);

# Command line options
# --------------------
  $batch = $prefix = $symlink = $upgrads = $replace = $force="";
  $print_version = $help = "";
  die unless 
  GetOptions("batch"          => \$batch,
             "prefix:s"       => \$prefix,
             "symlink:s"      => \$symlink,
             "upgrade"        => \$upgrade,
             "replace"        => \$replace,
             "version"        => \$print_version,
             "help"           => \$help);

# Key directories
# ---------------
  $garoot  = "$FindBin::RealBin/Contents";
  $gaddir  = "$garoot/Resources/SupportData";
  $gadset  = "$garoot/Resources/SampleDatasets";
  $gascrp  = "$garoot/Resources/Scripts";
 
# Special options
# ---------------
  $Name = basename($0) unless ($Alias=$asif);
  usage() if $help;

  die "$Name: work in progress, this script is not ready yet.\n   ---> ";

# On Linux, try i686 if build for $mach is not available
# ------------------------------------------------------
  if ( "$arch" eq "Linux" ) {
    if ( -e "$garoot/$arch/i686" ) {
         $mach = "i686" unless ( -e "$garoot/$arch/$mach" );
    }
  }

# Are we at the right place?
# --------------------------
  check_integrity();

# Versioning
# ----------
  $gabdir  = "$garoot/$arch/$mach";
  if ( -e "$gabdir/VERSION" ) { 
    chomp ( $version = `cat $gabdir/VERSION` );
  } else {
    die "$Name:  $gabdir\n <$dir>\n     ---> " 
  }
  if ( $print_version ) { print $version; exit; }

# Validate directories
# --------------------
  foreach $dir ( $gabdir ) {
      die "$Name:  $dir>\n     ---> " 
          unless -d $dir;
  }

# OK, we appear to have what we need
# ----------------------------------
  hello();
  $prefix  = get_prefix()  unless $prefix;
  $symlink = get_symlink() unless $symlink;

# Go for it
# ---------
  go4it();

# All done
# --------
  exit;

#......................................................................

sub hello {

   print <<"EOF";

              Welcome to the OpenGrADS Bundle Distribution
              --------------------------------------------

For additional information enter "$Alias --help".
$Msg
Starting "$cmd" ...

EOF
}

#......................................................................
sub get_prefix {

  if ( $batch ) {
       die "$Name: missing installation directory, must specify --prefix in batch mode\n   --->" 
          unless($prefix);
       return;
     }

   @dirs=();
   foreach $dir ( "/opt/opengrads", 
                  "/usr/local/opengrads", 
                  "/usr/opengrads", 
                  "/usr/share/opengrads", 
                  "Applications/OPenGrADS",
                  "$HOME/Applications/OpenGrADS",
                  "$HOME/opengrads",
                  "/share/$USER/opengrads") {

          push @dirs, $dir if (-w $dir||-w basename($dir)) ;

   }

  print <<EOF;

  Recommended installation directories:

EOF

  $i = 0;
  foreach $dir ( @Dirs ) {
       print "   $i.  $dir\n";
  }
       print "   9. Specify your own\n\n";
  print "Your choice? [1-9]";
  chomp($answer = lc <STDIN>);

  if ( "$answer" eq "9" ) {
     print "Enter the installation directory: ";
     chomp($prefix = lc <STDIN>);
   } else {
     if ( $answer < $#Dirs ) {
          $prefix = $Dirs[$answer];
     } else {
          return 1;
     }
   }

# Directory exists
# ----------------
  if ( -d $prefix ) {
    $answer = 
      Ask("Directory $prefix already exists. Do you want to remove it?","n");
    if ( "$answer" eq "y" ) {
      $rc = system("rm -ri $prefix");
      return $rc if $rc;
    } else {
      return 1;
    }
    
# Directory does not exist
# ------------------------
  } else {
    $answer = 
      Ask("Directory $prefix does not exist. Do you want to create it?","y");
    if ( "$answer" eq "y" ) {
      $rc = system("mkdir -p $prefix");
      return $rc if $rc;
    }

  }  

}

#......................................................................
sub Ask {
    my $msg = shift;
    my $def = shift;
    my $answer;
    print "$msg [$def] ";
    chomp($answer = lc <STDIN>);
    $answer = $def unless ($answer);
    $answer = "y" if $answer eq "yes";
    $answer = "n" if $answer eq "no";
    return $answer;
}

#......................................................................
sub check_integrity {
  
# Check key directories
# ---------------------
  foreach $dir ("$garoot", "$garoot/Resources",
                "$garoot/Resources/SupportData",
                "$garoot/Resources/Scripts" ) {

      die "$Name:  $dirt>\n     ---> " 
       unless -d $dir;

  }
  

# Architecture
# ------------
  unless ($force) {

    die "$Name:  $arch>; specify --force if you want to install it anyway.\n     ---> " 
       unless -d "$garoot/$arch";

    die "$Name:  $march>; specify --force to install it anyway\n     ---> " 
       unless -d "$garoot/$arch/$mach";

  }

}

#......................................................................

sub usage {

   print <<"EOF";

NAME
     $Name - Installs an OpenGraDAS Bundle 
          
SYNOPSIS
     $Name [OPTIONS] 
          
DESCRIPTION
     This script installs an OpenGrADS Bundle, either interactively or
     in batch mode.
     
     The "OpenGrADS Bundle" (OB) is simply a directory structure that 
     organizes GrADS, utilities, extension libraries as well as 
     supporting resources (fonts, map datasets, scripts). The main
     goal is to provide a relocatable, minimum configuration installation.
     Additional information about OBs can be found at

      http://opengrads.org/wiki/index.php?title=The_OpenGrADS_Bundle

     An OB allows for multiple-version, multi-platform installations on
     a single directory structure. This is convenient for shipping
     multi-platform software with data DVD-ROMs, or to have a GrADS
     installation on a network disk cross mounted on diverse platforms
     (Mac OS X, Linux, FreeBSD, Windows, etc.) By allowing concurrent
     installation of multiple versions users can choose when to upgrade
     to a specific version, while keeping stable production software
     running with a frozen release.

OPTIONS
     The user will be prompt for each of the parameters below that are
     are specified on the command line:

     --batch        runs unattended; no questions will be asked, aborting
                    if required information is missing.

     --force        install even if the binary type does not match your
                    architetcure

     --prefix       destination directory, e.g., /opt/opengrads

     --replace      entirely replaces an exisitng installation with this one;
                    ALL PREVIOUS DATA WILL BE DELETED.

     --symlink dir  creates symlinks under *dir* for all the executables

     --upgrade      if another version has already been installed, 
                    install this version alongside; Resources will be
                    overwritten.

     The following options are informational:

     --help         print this page

     --version      print OpenGrADS Bundle version 

SEE ALSO
     http://opengrads.org --- OpenGrADS Home Page
     http://cookbooks.opengrads.org --- OpenGrADS Cookbooks
     
AUTHOR
     Arlindo da Silva <dasilva@opengrads.org>

COPYRIGHT
     Copyright (c) 2009 Arlindo da Silva
     This is free software released under the GNU General Public
     License; see the source for copying conditions.
     There is NO  warranty;  not even for MERCHANTABILITY or FITNESS 
     FOR A PARTICULAR PURPOSE.

EOF

exit(1)

}
