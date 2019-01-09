#!/usr/bin/env python

"""
Unit tests based oy PyUnit.
"""

# Add parent directory to python search path
# ------------------------------------------
import os
import sys

import unittest

sys.path.insert(0,'../../pytests/lib')
from grads import GaCore

#......................................................................

class utUDX(unittest.TestCase):

    def setUp(self,udxt,BinDir=None,DataDir=None):

#       Search for a reasonable default for binary dir
#       ----------------------------------------------
        if BinDir is not None:
            bin = BinDir + 'grads'
        else:
            if os.path.isfile('../../src/grads.c'):
                BinDir = '../../src/'
            elif os.path.isfile('../../opengrads/Contents/grads'):
                BinDir = '../../opengrads/Contents/'
            elif os.path.isfile('../opengrads/Contents/grads'):
                BinDir = '../opengrads/Contents/'
            else:
                rc = os.system('which grads')
                if rc: 
                    raise GrADSError, "cannot find grads"
                else:
                    BinDir = ''
                    
        Binary = BinDir + 'grads' # grads binary to be used for testing extension
###        print "- Testing with GrADS binary " + Binary

#       Search for a reasonable default for data files
#       ----------------------------------------------
        if DataDir is None:
            sample = 'model.grb'
            for dir in ( '.', '../pytests/data', '../../pytests/data'):
                if os.path.exists(dir+'/'+sample):
                    DataDir = dir + '/'
                    break
                
        DataFile = DataDir + 'model.ctl' # to be opened during test setup
###        print "- Testing with data file " + DataFile
###        print ""

        self.ga = GaCore(Bin=Binary, Echo=False, Window=False)
        self.fh = self.ga.open(DataFile)
        if os.path.isfile(udxt[0]):
            self.ga('load udxt ' + udxt[0])
            self.udxdir = os.path.dirname(udxt[0])
        else:
            self.ga('load udxt ' + udxt[1])
            self.udxdir = os.path.dirname(udxt[1])

        if self.udxdir != '':
            self.udxdir = self.udxdir + '/'

        # TLE Data
        # ---------
        for sat in ( 'aqua', 'terra' ):
            self.__dict__[sat] = None
            for dir in ( '.', '../data', '../../data'):
                if os.path.exists(dir+'/'+sat+'.tle'):
                    self.__dict__[sat] = dir + '/' + sat + '.tle'
                    break
                
    def tearDown(self):
        del self.ga
        

#......................................................................

def run(Class,verb=2,BinDir=None,DataDir=None):
    """
    Runs all tests based on the standard *model* testing file.
    """
        
    #   Assemble and run the test suite
    #   -------------------------------
    load = unittest.TestLoader().loadTestsFromTestCase
    TestSuite = [ load(Class), ]
    all = unittest.TestSuite(TestSuite)
    Results = unittest.TextTestRunner(verbosity=verb).run(all)
    
    #   Return number of errors+failures: skipped binaries do not count
    #   ---------------------------------------------------------------
    if not Results.wasSuccessful(): 
        raise IOError, 'UDX unit tests failed'
    else:
        print "Passed all UDX unit tests"
        
