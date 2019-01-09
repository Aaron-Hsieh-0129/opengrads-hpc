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

    def test_orb_1_set(self):
        self.ga("set_orb dt 30")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("-1",self.ga.rword(2,4))
        self.assertEqual("0", self.ga.rword(3,4))
        self.assertEqual("0", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('dt ... ')
        self.ga("set_orb mark 5")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("0", self.ga.rword(3,4))
        self.assertEqual("0", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('mark ... ')
        self.ga("set_orb ihalo 2 2")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("2", self.ga.rword(3,4))
        self.assertEqual("2", self.ga.rword(3,5))
        self.assertEqual("0", self.ga.rword(3,8))
        self.assertEqual("0", self.ga.rword(3,9))
        sys.stdout.write('ihalo ... ')
        self.ga("set_orb jhalo 3 3")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("2", self.ga.rword(3,4))
        self.assertEqual("2", self.ga.rword(3,5))
        self.assertEqual("3", self.ga.rword(3,8))
        self.assertEqual("3", self.ga.rword(3,9))
        sys.stdout.write('jhalo ... ')
        self.ga("set_orb halo 1")
        self.assertEqual("30",self.ga.rword(1,4))
        self.assertEqual("5",self.ga.rword(2,4))
        self.assertEqual("1", self.ga.rword(3,4))
        self.assertEqual("1", self.ga.rword(3,5))
        self.assertEqual("1", self.ga.rword(3,8))
        self.assertEqual("1", self.ga.rword(3,9))
        sys.stdout.write('halo ... ')

    def test_orb_2_track(self):
        self.ga("set_orb dt 3600")
        self.assertEqual("3600",self.ga.rword(1,4))

        self.ga("orb_track aqua")
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))

        self.assertEqual(31,  int(float(self.ga.rword(2,3))))
        self.assertEqual(-17, int(float(self.ga.rword(2,4))))
        self.assertEqual(110, int(float(self.ga.rword(9,3))))
        self.assertEqual(68,  int(float(self.ga.rword(9,4))))

    def test_orb_3_mask(self):
        self.ga("display orb_mask(ts,terra)")
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

        self.ga("display orb_mask(ts,terra,300)")
        self.assertEqual("19861231",self.ga.rword(1,7))
        self.assertEqual("120000,",self.ga.rword(1,8))
        self.assertEqual("19870101",self.ga.rword(1,11))
        self.assertEqual("120000",self.ga.rword(1,12))
        self.assertEqual("240",self.ga.rword(2,2))
        self.assertEqual("310",self.ga.rword(2,4))
        self.assertEqual("10", self.ga.rword(2,6))

        self.ga("display orb_mask(ts,terra,300,500,20)")
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
