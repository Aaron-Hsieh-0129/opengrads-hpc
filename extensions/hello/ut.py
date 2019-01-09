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
        utUDX.setUp(self,['hello.udxt','hello/hello.udxt'])

#--
        
    def test_udc_hello(self):
        self.ga('hello Caetano Veloso')
        self.assertEqual("<hello>",self.ga.rword(2,3))
        self.assertEqual("<Caetano>",self.ga.rword(3,3))
        self.assertEqual("<Veloso>",self.ga.rword(4,3))

    def test_udf_hello(self):
        self.ga('d hello()')
        self.assertEqual("[1]",self.ga.rword(1,1))
        self.assertEqual("Hello,",self.ga.rword(1,2))
        self.assertEqual("GrADS",self.ga.rword(1,3))
        self.assertEqual("World!",self.ga.rword(1,5))

#......................................................................

if __name__ == "__main__":
    run(ut)
