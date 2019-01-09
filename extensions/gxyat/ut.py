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
import platform

from utudx import utUDX, run

#......................................................................

class ut(utUDX):

    def setUp(self):
        utUDX.setUp(self,['gxyat.udxt','gxyat/gxyat.udxt'])

#--
        
    def test_gxyat_png(self):
        self.ga("gxyat -v /tmp/ps.png")
        self.assertEqual("(memory",self.ga.rword(4,3))
        self.assertEqual("buffer)",self.ga.rword(4,4))
        self.assertEqual("/tmp/ps.png",self.ga.rword(5,3))
        self.assertEqual("800",self.ga.rword(6,3))
        self.assertEqual("600",self.ga.rword(6,5))
        self.assertEqual("PNG",self.ga.rword(7,3))
        self.ga("gxyat -v -x 400 -y 300 /tmp/ps.png")
        self.assertEqual("400",self.ga.rword(6,3))
        self.assertEqual("300",self.ga.rword(6,5))

    def test_gxyat_ps(self):
        if platform.platform()[0:7] == "xxxFreeBSD":
            sys.stdout.write("skipped on FreeBSD... ")
            pass
        else:
            self.ga("gxyat -v /tmp/ps.ps")
            self.assertEqual("(memory",self.ga.rword(4,3))
            self.assertEqual("buffer)",self.ga.rword(4,4))
            self.assertEqual("/tmp/ps.ps",self.ga.rword(5,3))
            self.assertEqual("792",self.ga.rword(6,3))
            self.assertEqual("612",self.ga.rword(6,5))
            self.assertEqual("(11.0",self.ga.rword(6,7))
            self.assertEqual("8.5",self.ga.rword(6,9))
            self.ga("gxyat -v -x 400 -y 300 /tmp/ps.ps")
            self.assertEqual("400",self.ga.rword(6,3))
            self.assertEqual("300",self.ga.rword(6,5))

    def test_gxyat_pdf(self):
        self.ga("gxyat -v /tmp/ps.pdf")
        self.assertEqual("(memory",self.ga.rword(4,3))
        self.assertEqual("buffer)",self.ga.rword(4,4))
        self.assertEqual("/tmp/ps.pdf",self.ga.rword(5,3))
        self.assertEqual("792",self.ga.rword(6,3))
        self.assertEqual("612",self.ga.rword(6,5))
        self.assertEqual("(11.0",self.ga.rword(6,7))
        self.assertEqual("8.5",self.ga.rword(6,9))
        self.ga("gxyat -v -x 400 -y 300 /tmp/ps.pdf")
        self.assertEqual("400",self.ga.rword(6,3))
        self.assertEqual("300",self.ga.rword(6,5))

    def test_gxyat_svg(self):
        if platform.platform()[0:7] == "xxxFreeBSD":
            sys.stdout.write("skipped on FreeBSD... ")
            pass
        else:
            self.ga("gxyat -v /tmp/ps.svg")
            self.assertEqual("(memory",self.ga.rword(4,3))
            self.assertEqual("buffer)",self.ga.rword(4,4))
            self.assertEqual("/tmp/ps.svg",self.ga.rword(5,3))
            self.assertEqual("800",self.ga.rword(6,3))
            self.assertEqual("600",self.ga.rword(6,5))
            self.ga("gxyat -v -x 400 -y 300 /tmp/ps.svg")
            self.assertEqual("400",self.ga.rword(6,3))
            self.assertEqual("300",self.ga.rword(6,5))
            
    def test_set_rgba(self):
        self.ga("set_rgba 60 125 125 125 0 1")
        self.ga("gxyat -v /tmp/ps.png")
        self.assertEqual("(memory",self.ga.rword(4,3))
        self.assertEqual("buffer)",self.ga.rword(4,4))
        self.assertEqual("/tmp/ps.png",self.ga.rword(5,3))
        self.assertEqual("800",self.ga.rword(6,3))
        self.assertEqual("600",self.ga.rword(6,5))
        self.assertEqual("PNG",self.ga.rword(7,3))

#......................................................................

if __name__ == "__main__":
    run(ut)
