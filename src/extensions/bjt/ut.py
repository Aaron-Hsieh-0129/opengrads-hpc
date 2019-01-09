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
        utUDX.setUp(self,['bjt.udxt','bjt/bjt.udxt'])

#--

    def test_lt(self):
        self.ga("d lt(ts)")
        self._CheckCint(0,22,2)

    def test_jd(self):
        self.ga("d jd(ts)")
        self.assertEqual(725373,int(self.ga.rword(1,2)))
        self.assertEqual(725374,int(self.ga.rword(1,4)))

    def test_cosz(self):
        self.ga("d 10*cosz(ts,h)")
        self._CheckCint(0,9,1)
        self.ga("d 100*cosz(ts,d)")
        self._CheckCint(0,35,5)
        self.ga("d 100*cosz(ts,m)")
        self._CheckCint(0,35,5)

    def test_dayratio (self):
        self.ga("d 10*dayratio(ts)")
        self._CheckCint(0,10,1)

    def test_if (self):
        self.ga("d if(ts,<,273.16,ts,-u)")
        self._CheckCint(235,270,5)
        self.ga("d if(ua,<,0,-ua,ua) - abs(ua)")
        self.assertEqual("Constant",self.ga.rword(1,1))
        self.assertEqual(0,int(self.ga.rword(1,5)))

    def test_maxv (self):
        self.ga("d maxv(ua(lev=500),ua(lev=200))")
        self._CheckCint(-10,80,10)

    def test_minv (self):
        self.ga("d minv(ua(lev=500),ua(lev=200))")
        self._CheckCint(-20,50,10)

    def test_which (self):
        self.ga("d 10*which(minv(maxv(ua(lev=200),0),10),0,0,10,2,1)")
        self._CheckCint(0,20,2)

    def test_ftest(self):
        self.ga("d 10000*ftest(0.8,100,200)")
        self.assertEqual(1056,int(float(self.ga.rword(1,4))))

    def test_ttest(self):
        self.ga("d 100*ttest(0.5,100)")
        self.assertEqual(38,int(float(self.ga.rword(1,4))))

    def test_tfit(self):
        sys.stdout.write("skipped ... ")
        pass

    def test_fit (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_tcorr2 (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_tregr2 (self):
        sys.stdout.write("skipped ... ")
        pass

    def xtest_tregr2 (self):
        self.ga("""set lat 0
                   set lon -180
                   define pseq = ps
                   set lon -180 180
                   set lat -90 90""")
        self.ga("d tregr2(pseq,ts,t=1,t=5)-tregr(pseq,ts,t=1,t=5)")
        self.assertEqual("Constant",self.ga.rword(3,1))
        self.ga("d 10*tregr2(pseq,ts,t=1,t=5)")
        self._CheckCint(-12,18,3,skip=1)

    def test_tmave2 (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_madvu (self):
        self.ga("set lev 200")
        self.ga("d 1000*madvu(ua,ta)")
        self._CheckCint(-8,10,2)

    def test_madvv (self):
        self.ga("set lev 200")
        self.ga("d 1000*madvv(va,ta)")
        self._CheckCint(-7,7,1)

    def test_madvw (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_muuadv (self):
        self.ga("set lev 200")
        self.ga("d 10000*muadv(ua,ta)")
        self._CheckCint(-5,4,1)

    def test_mvvadv (self):
        self.ga("set lev 200")
        self.ga("d 100000*mvadv(va,ta)")
        self._CheckCint(-15,25,5)

    def test_mwadv (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_satvap (self):
        self.ga("d satvap(ts)")
        self._CheckCint(1000,9000,1000)
 
    def test_dew (self):
        self.ga("d dew(satvap(ts))")
        self._CheckCint(240,310,10)
 
    def test_lw (self):
        self.ga("set lev 500")
        self.ga("d lw(lev,ta,hus(lev=1000),0,0,ps,ts,0.98,ta(lev=1000),hus(lev=1000)/(1+hus(lev=1000)))")
        self._CheckCint(-35,25,5)
 
    def xtest_lw2 (self):
        sys.stdout.write("skipped ... ")
        pass

    def test_pinterp (self):
        self.ga("d pinterp(ta,lev,450)")
        self._CheckCint(220,255,5)

    def test_zinterp (self):
        self.ga("d zinterp(ta,zg,5000)")
        self._CheckCint(230,270,5)

    def test_line (self):
        self.ga("d 10*line(ts,180,0,270,45)")
        self._CheckCint(0,10,1)

    def test_vint2 (self):
        self.ga("d vint2(ps,ta,100)/100000")
        self._CheckCint(10,24,2)

#--
#            Useful Internal Methods for Writing Tests

    def _CheckCint(self,cmin,cmax,cint,skip=0):
        """
        Check contour intervals during display.
        """
        self.assertEqual(cmin,int(self.ga.rword(1+skip,2)))
        self.assertEqual(cmax,int(self.ga.rword(1+skip,4)))
        self.assertEqual(cint,int(self.ga.rword(1+skip,6)))

#......................................................................

if __name__ == "__main__":
    run(ut)

