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
        utUDX.setUp(self,['fish.udxt','fish/fish.udxt'])
        if os.path.isfile('../bjt/bjt.udxt'):
            self.ga('load udxt ../bjt/bjt.udxt')
        else:
            self.ga('load udxt bjt/bjt.udxt')
        self.ga('set lev 200')
#--

    def test_fish(self):
        self.ga('d fish(hcurl(ua,va))/1e7')
        self._CheckCint(-30,0,3)
        self.ga('d fish(hdivg(ua,va))/1e6')
        self._CheckCint(-10,12,2)

    def test_psi(self):
        self.ga('d fish_psi(ua,va)/1e7')
        self._CheckCint(-30,0,3)

    def test_chi(self):
        self.ga('d fish_chi(ua,va)/1e6')
        self._CheckCint(-8,12,2)

    def test_vor(self):
        self.ga('d fish_vor(ua,va)*1e5')
        self._CheckCint(-8,8,2)

    def test_div(self):
        self.ga('d fish_div(ua,va)*1e6')
        self._CheckCint(-15,15,5)

#--
#            Useful Internal Methods for Writing Tests

    def _CheckCint(self,cmin,cmax,cint):
        """
        Check contour intervals during display.
        """
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))

#......................................................................

if __name__ == "__main__":
    run(ut)
