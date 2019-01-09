#!/usr/bin/env python

import sys
import os
from optparse      import OptionParser   # Command-line args

import unittest
from TestModelFile import *
#from TestPdefFile import *
     
#........................................................................

print ""
print "  Welcome to the PyUnit-based GrADS Unit Tests"
print ""

# Parse command line options
# --------------------------
parser = OptionParser(usage="Usage: %prog [options]", version='1.0.0' )

parser.add_option("-b", "--bindir", dest="bindir", default=None,
                  help="Directory for GrADS binaries " )
                                                                                
parser.add_option("-d", "--datadir", dest="datadir", default=None,
                  help="Directory for GrADS binaries " )
                                                                                
parser.add_option("-v", "--verbose", dest="level", default=2,
                  help="verbose level (default=%default)" )

(o, a) = parser.parse_args()

# Run the "model" based tests
# ---------------------------
run_all_tests(verb=o.level, BinDir=o.bindir, DataDir=o.datadir)



