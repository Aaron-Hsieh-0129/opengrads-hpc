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
        utUDX.setUp(self,['orb.udxt','orb/orb.udxt'])

    def test_tle_1_set(self):

        self.ga("set_tle dt 30")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("-1",self.ga.rword(2,4))
        self.assertEqual("0", self.ga.rword(3,4))
        self.assertEqual("0", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('dt ... ')
        self.ga("set_tle mark 5")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("0", self.ga.rword(3,4))
        self.assertEqual("0", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('mark ... ')
        self.ga("set_tle ihalo 2 2")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("2", self.ga.rword(3,4))
        self.assertEqual("2", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('ihalo ... ')
        self.ga("set_tle jhalo 3 3")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("2", self.ga.rword(3,4))
        self.assertEqual("2", self.ga.rword(3,5))
        self.assertEqual("3", self.ga.rword(3,8))
        self.assertEqual("3", self.ga.rword(3,9))
        sys.stdout.write('jhalo ... ')
        self.ga("set_tle halo 1")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("1", self.ga.rword(3,4))
        self.assertEqual("1", self.ga.rword(3,5))
        self.assertEqual("1", self.ga.rword(3,8))
        self.assertEqual("1", self.ga.rword(3,9))
        sys.stdout.write('halo ... ')

    def test_tle_2_track(self):

        self.ga("set_tle dt 3600")
        self.assertEqual("3600",self.ga.rword(1,4))

        self.ga("tle_track %s"%self.aqua)
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))

        self.assertEqual(139,  int(float(self.ga.rword(2,3))))
        self.assertEqual(-69, int(float(self.ga.rword(2,4))))
        self.assertEqual(10, int(float(self.ga.rword(9,3))))
        self.assertEqual(6,  int(float(self.ga.rword(9,4))))

    def test_tle_3_mask(self):
        
        self.ga("display tle_mask(ts,%s)"%self.aqua)
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

        self.ga("display tle_mask(ts,%s,300)"%self.terra)
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

        self.ga("display tle_mask(ts,%s,300,500,20)"%self.terra)
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

#......................................................................

if __name__ == "__main__":
    run(ut)
