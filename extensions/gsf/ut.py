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
        utUDX.setUp(self,['gsf.udxt','gsf/gsf.udxt'])

#--

    def test_shear(self):
        self.ga("d shear(ua(lev=500),va(lev=500),ua(lev=200),va(lev=200))")
        self._CheckCint(5,40,5)

    def test_plcl(self):
        self.ga("d plcl(ts,80,ps)")
        self._CheckCint(500,1000,50)

    def test_tlcl(self):
        self.ga("d tlcl(ts,80)")
        self._CheckCint(230,310,10)

    def test_dewpt(self):
        self.ga("d dewpt(ts,80)")
        self._CheckCint(230,310,10)

    def test_epi(self):
        self.ga("d epi(ta(lev=850),80,ta(lev=500),50)")
        self._CheckCint(-35,35,5)

    def test_ept(self):
        self.ga("d ept(ta(lev=500),75,500)) ")
        self._CheckCint(280,340,5)

    def test_wchill(self):
        self.ga("d wchill(ts,ua,va)")
        self._CheckCint(230,300,10)

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
