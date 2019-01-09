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
        utUDX.setUp(self,['shape.udxt','shape/shape.udxt'])
        self.ga("set lon -180 180")
        self.ga("draw map")
        self.ga("clear")
#--
        
    def test_lines(self):
        self.ga("shp_lines admin98")
        self.assertEqual("shapefile",self.ga.rword(1,2))
        self.assertEqual("Polygon",self.ga.rword(2,3))
        self.assertEqual("2604",self.ga.rword(2,7))
        self.assertEqual("-180.000,",self.ga.rword(3,4))
        self.assertEqual("-90.000",self.ga.rword(3,5))
        self.assertEqual("180.000,",self.ga.rword(3,9))
        self.assertEqual("83.624",self.ga.rword(3,10))

    def test_polyf(self):
        self.ga("shp_polyf admin98")
        self.assertEqual("shapefile",self.ga.rword(1,2))
        self.assertEqual("Polygon",self.ga.rword(2,3))
        self.assertEqual("2604",self.ga.rword(2,7))
        self.assertEqual("-180.000,",self.ga.rword(3,4))
        self.assertEqual("-90.000",self.ga.rword(3,5))
        self.assertEqual("180.000,",self.ga.rword(3,9))
        self.assertEqual("83.624",self.ga.rword(3,10))

    def test_object(self):
        self.ga("shp_lines admin98 1024")
        self.assertEqual("shapefile",self.ga.rword(1,2))
        self.assertEqual("Polygon",self.ga.rword(2,3))
        self.assertEqual("2604",self.ga.rword(2,7))
        self.assertEqual("-180.000,",self.ga.rword(3,4))
        self.assertEqual("-90.000",self.ga.rword(3,5))
        self.assertEqual("180.000,",self.ga.rword(3,9))
        self.assertEqual("83.624",self.ga.rword(3,10))
        self.assertEqual("1024",self.ga.rword(4,3))

#......................................................................

if __name__ == "__main__":
    run(ut)
