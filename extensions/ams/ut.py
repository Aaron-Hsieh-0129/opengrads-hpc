#!/usr/bin/env python

"""
Unit tests based oy PyUnit.
"""

# Add parent directory to python search path
# ------------------------------------------
import sys
sys.path.insert(0,'..')

import os
from utudx import utUDX, run

#......................................................................

class ut(utUDX):

    def setUp(self):
        utUDX.setUp(self,['ams.udxt','ams/ams.udxt'])

    def test_getenv(self):
        self.ga('getenv USER HOME')
        self.assertEqual("USER",self.ga.rword(1,3))
        self.assertEqual("HOME",self.ga.rword(2,3))
        self.assertEqual(os.getenv("USER"),self.ga.rword(1,5))
        self.assertEqual(os.getenv("HOME"),self.ga.rword(2,5))
        try:
            self.ga('getenv DA4B928248580C274E')
        except:
            self.assertEqual("DA4B928248580C274E",self.ga.rword(1,3))
            self.assertEqual("<undef>",           self.ga.rword(1,5))

    def test_setenv(self):
        self.ga('setenv XXXX "moqueca de peixe"')
        self.assertEqual("XXXX",self.ga.rword(1,3))
        self.assertEqual("moqueca",self.ga.rword(1,5))
        self.assertEqual("de",self.ga.rword(1,6))
        self.assertEqual("peixe",self.ga.rword(1,7))
        self.ga('getenv XXXX')
        self.assertEqual("moqueca",self.ga.rword(1,5))
        self.assertEqual("de",self.ga.rword(1,6))
        self.assertEqual("peixe",self.ga.rword(1,7))
        
    def test_printenv(self):
        self.ga('setenv XXXX "moqueca de peixe"')
        self.ga('printenv $XXXX')
        self.assertEqual("moqueca",self.ga.rword(1,1))
        self.assertEqual("de",self.ga.rword(1,2))
        self.assertEqual("peixe",self.ga.rword(1,3))

    def test_pid(self):
        self.ga('printenv $$')
        self.assertEqual(os.getpid()<self.ga.rword(1,1),True)

    def test_runenv(self):
        self.ga('setenv EXPR ua;va;sqrt(ua*ua+va*va)')
        self.ga('runenv d $EXPR')
        self.ga('setenv EXPR ts')
        self.ga('@ d $EXPR')
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

    def test_chdir(self):
        self.ga('chdir /usr/bin')

    def test_abspath(self):
        self.ga('@ chdir $HOME')
        path = self.ga.rword(2,3)
        self.ga('abspath .')
        self.assertEqual(path, self.ga.rword(1,1))

    def test_basename(self):
        self.ga('basename /path/to/somewhere/somefile.nc4')
        self.assertEqual('somefile.nc4', self.ga.rword(1,1))
        
    def test_dirname(self):
        self.ga('dirname /path/to/somewhere/somefile.nc4')
        self.assertEqual('/path/to/somewhere', self.ga.rword(1,1))
        
    def test_exists(self):
        self.ga('exists /')
        self.assertEqual('yes', self.ga.rword(1,1))
        
    def test_getatime(self):
        self.ga('getatime /') # do not know how to verify

    def test_getctime(self):
        self.ga('getctime /') # do not know how to verify

    def test_getmtime(self):
        self.ga('getmtime /') # do not know how to verify

    def test_glob(self):
        self.ga('glob /')
        self.assertEqual('/', self.ga.rword(2,1))
        self.ga('glob /*') # do not know how to verify
        self.ga('printenv $$')
        pid = self.ga.rword(1,1)
        self._createFake(pid,5)
        self.ga('glob /tmp/opengrads-%s.*.png'%pid)
        five = self.ga.rword(1,2)
        self._cleanFake(pid,5)
        self.assertEqual('5', five)
        
    def test_isfile(self):
        self.ga('isfile .')
        self.assertEqual('no', self.ga.rword(1,1))
        self.ga('printenv $$')
        pid = self.ga.rword(1,1)
        self.ga('! touch /tmp/opengrads.%s'%pid)
        self.ga('isfile /tmp/opengrads.%s'%pid)
        self.assertEqual('yes', self.ga.rword(1,1))
        self.ga('! /bin/rm -rf /tmp/opengrads.%s'%pid)
        
    def test_isdir(self):
        self.ga('isdir .')
        self.assertEqual('yes', self.ga.rword(1,1))
        self.ga('printenv $$')
        pid = self.ga.rword(1,1)
        self.ga('! touch /tmp/opengrads.%s'%pid)
        self.ga('isdir /tmp/opengrads.%s'%pid)
        self.assertEqual('no', self.ga.rword(1,1))
        self.ga('! /bin/rm -rf /tmp/opengrads.%s'%pid)
        
    def test_match(self):
        self.ga('match /open/ "OpenGrADS"')
        self.assertEqual('no', self.ga.rword(1,1))
        self.ga('match /open/i "OpenGrADS"')
        self.assertEqual('yes', self.ga.rword(1,1))
        self.ga('match /Open$/ "OpenGrADS"')
        self.assertEqual('no', self.ga.rword(1,1))
        self.ga('match /^Open/ "OpenGrADS"')
        self.assertEqual('yes', self.ga.rword(1,1))
        self.ga('match /grads$/i "OpenGrADS"')
        self.assertEqual('yes', self.ga.rword(1,1))
        self.ga('match /^GrADS/ "OpenGrADS"')
        self.assertEqual('no', self.ga.rword(1,1))

    def test_sed(self):
        self.ga('sed s/grads/GrADS/ig "Opengrads provides GRADS extensions"')
        self.assertEqual('OpenGrADS', self.ga.rword(2,1))
        self.assertEqual('GrADS', self.ga.rword(2,3))

    def test_wordexp(self):
        self.ga('wordexp ~')
        home = self.ga.rword(2,1)
        self.ga('getenv HOME')
        self.assertEqual(home, self.ga.rword(1,5))
        self.ga('printenv $$')
        pid = self.ga.rword(1,1)
        self._createFake(pid,5)
        self.ga('wordexp /tmp/opengrads-%s.*.png'%pid)
        five = self.ga.rword(1,2)
        self._cleanFake(pid,5)
        self.assertEqual('5', five)
        
    def _createFake(self,pid,n):
        for i in range(n):
            self.ga('! touch /tmp/opengrads-%s.%d.png'%(pid,i))

    def _cleanFake(self,pid,n):
        return
        for i in range(n):
            self.ga('! /bin/rm -rf /tmp/opengrads-%s.%d.png'%(pid,i))
            
#......................................................................

if __name__ == "__main__":
    run(ut)
