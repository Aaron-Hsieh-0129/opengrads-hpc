#!/usr/bin/env python

"""
    Unit tests based on PyUnit.
"""

# Add parent directory to python search path
# ------------------------------------------
import sys
sys.path.insert(0,'..')

import os
import unittest

from utudx import utUDX, run

#......................................................................

class ut(utUDX):

    def setUp(self):
        utUDX.setUp(self,['shfilt.udxt','shfilt/shfilt.udxt'])
        self.ga('set lev 200')

#--

    def test_shfilt(self):
        self._CheckCint('ps',1,4,   880,1040, 20)
        self._CheckCint('ps',5,10, -250,100,50)
        self._CheckCint('ps',11,21,-150,120,30)

    def test_power(self):
        self.ga('set gxout stat')
        self.ga('d sh_power(ts)')
        self.assertEqual(-1,int(self.ga.rword(2,4)))
        self.assertEqual(-999,int(self.ga.rword(4,4)))
        self.assertEqual(46,int(self.ga.rword(7,8)))
        self.assertEqual(-20000,int(self.ga.rword(9,5)))
        self.assertEqual(200000,int(self.ga.rword(9,6)))
        self.assertEqual(20000,int(self.ga.rword(9,7)))

    def xtest_shfish(self):
        self.ga('d sh_fish(hcurl(ua,va))/1e7')
        self._CheckCint(-30,0,3)
        self.ga('d sh_fish(hdivg(ua,va))/1e6')
        self._CheckCint(-10,12,2)

    def test_shpsi(self):
        self.ga('d sh_psi(ua,va)/1e7')
        self._CheckCint2(-15,12,3)

    def test_shchi(self):
        self.ga('d sh_chi(ua,va)/1e6')
        self._CheckCint2(-10,10,2)

    def test_shvor(self):
        self.ga('d sh_vor(ua,va)*1e5')
        self._CheckCint2(-8,8,2)

    def test_shdiv(self):
        self.ga('d sh_div(ua,va)*1e6')
        self._CheckCint2(-15,15,5)


#--
#            Useful Internal Methods for Writing Tests

    def _CheckCint(self,name,n1,n2,cmin,cmax,cint):
        """
        Check contour intervals during display.
        """
        self.ga('clear')
        self.ga('display sh_filt(%s,%d,%d)'%(name,n1,n2))
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))

    def _CheckCint2(self,cmin,cmax,cint):
        """
        Check contour intervals during display.
        """
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))


#......................................................................

if __name__ == "__main__":
    run(ut)

