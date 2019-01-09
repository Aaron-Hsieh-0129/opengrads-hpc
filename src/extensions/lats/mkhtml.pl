#!/usr/bin/env perl
#
# Simple script for basic HTML formatting of built-in man page.
#

use Getopt::Std;         # command line options

getopts('t');

# table of contents
if ( $opt_t ) {

# Make index
$man = 0; # where NAME has beeing found yet
$hasName = 0;
HLINE: while (<>) {

   $line = $_;
   chomp $line;
   @tokens = split " ", "$line";

   if ( "$tokens[0]" eq "NAME" ) {
     $man = 1;
     next HLINE;
   }

   $man = 0 if ( "$tokens[0]" eq "<RC>" );

   next HLINE unless($man);

   print <<'EOF';
<?xml version="1.0" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/\
xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<link rel="stylesheet" href="/pod.css" type="text/css" />
</head>
EOF

   if ( length($line) ) {

     unless ( $hasName ) {
       print "<h1> $line </h1>\n";
       print "<ul>\n";
       print "<li> <a href=\#NAME > NAME </a></li>\n";
       $hasName=1;
       next HLINE;
     }

#    Must be a header
#    ----------------
     if ( substr($line,1,1) ne " " ) {    
       print "<li> <a href=\#$tokens[0]$tokens[1] > $line </a></li>\n";
       next HLINE;
     }
   } # not blank
} # while

print "</ul>\n";

# Do body
# -------
  } else {

  $man = 0; # where NAME has beeing found yet
  LINE: while (<>) {

      $line = $_;
      chomp $line;
      @tokens = split " ", "$line";
      
      if ( "$tokens[0]" eq "NAME" ) {
        $man = 1;
        print "<h2> <a name=$tokens[0] </a> $line </h2>\n";
        print "<pre>\n";
        next LINE;
   }
      
      $man = 0 if ( "$tokens[0]" eq "<RC>" );
      
      next LINE unless($man);
      
      #  Must be a header
      #  ----------------
      if ( substr($line,1,1) ne " " ) {    
        print "</pre>\n";
        print "<h2> <a name=$tokens[0]$tokens[1] </a> $line </h2>\n";
        print "<pre>\n";
        next LINE;
      }
      print "$line\n";
      
    }
  print "</pre>\n";
  
}
