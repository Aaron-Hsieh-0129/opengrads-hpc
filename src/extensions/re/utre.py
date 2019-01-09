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

from grads import GrADS
from utudx import utUDX, run

#......................................................................

class ut(utUDX):

    def setUp(self):
        utUDX.setUp(self,['re.udxt','re/re.udxt'])

#--

    def test_re(self):
        self._CheckCint('d re(ts,2.5)',240,310,10)
        self._CheckCint('d re(ts,2.5,2.5)',240,310,10)
        self._CheckCint('d re(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,ba)',
                         240,310,10)
        self._CheckCint('d re(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,bl)',
                         240,310,10)
        self._CheckCint('d re(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,bs)',
                         240,310,10)
        self._CheckCint('d re(ts,50,linear,1.25,2.5,20,gaus,5,40,ig,94)',
                         265,315,5,3)
        self._CheckCint('d re(ts,48,linear,0,7.5,40,gaus,1,40,vt,0.60,0.20)',
                         240,310,10,5)
        self._CheckCint('d re(ts,48,linear,0,7.5,40,gaus,1,40,ma,0.5)',
                         240,310,10,5)

    def test_re_(self):
        self._CheckCint('d re_(ts,2.5)',240,310,10,9)
        self._CheckCint('d re_(ts,2.5,2.5)',240,310,10,9)
        self._CheckCint('d re_(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,ba)',
                         240,310,10,9)
        self._CheckCint('d re_(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,bl)',
                         240,310,10,9)
        self._CheckCint('d re_(ts,144,linear,1.25,2.5,72,linear,-88.75,2.5,bs)',
                         240,310,10,9)
        self._CheckCint('d re_(ts,50,linear,1.25,2.5,20,gaus,5,40,ig,94)',
                         265,315,5,13)
        self._CheckCint('d re_(ts,48,linear,0,7.5,40,gaus,1,40,vt,0.60,0.20)',
                         240,310,10,15)
        self._CheckCint('d re_(ts,48,linear,0,7.5,40,gaus,1,40,ma,0.5)',
                         240,310,10,14)

    def test_geos(self):
        self._CheckCint('d geos(ts,144,91)',240,310,10)
        self._CheckCint('d geos(ts,b)',240,310,10)
        self._CheckCint('d geos(ts,288,181,bs)',240,310,10)
        self._CheckCint('d geos(ts,540,361,,1)',240,310,10,9)

    def test_reimg(self):
        self._CheckCint('d reimg(ts,800,600)',240,310,10)
        self._CheckCint('d reimg(ts,800,600,bs)',240,310,10)
        self._CheckCint('d reimg(ts,800,600,,1)',240,310,10,9)

#--
#            Useful Internal Methods for Writing Tests

    def _CheckCint(self,cmd,cmin,cmax,cint,skip=0):
        """
        Check contour intervals during display.
        """
        self.ga(cmd)
        self.assertEqual(cmin,int(self.ga.rword(1+skip,2)))
        self.assertEqual(cmax,int(self.ga.rword(1+skip,4)))
        self.assertEqual(cint,int(self.ga.rword(1+skip,6)))

#......................................................................

if __name__ == "__main__":
    run(ut)

