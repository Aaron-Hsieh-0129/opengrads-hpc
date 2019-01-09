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
        utUDX.setUp(self,['ipc.udxt','ipc/ipc.udxt'])

#--
        
    def test_verb(self):
        self.ga('ipc_verb')
        self.assertEqual("ON",self.ga.rword(1,4))
        self.ga('ipc_verb')
        self.assertEqual("OFF",self.ga.rword(1,4))
        self.ga('ipc_verb OFF')
        self.assertEqual("OFF",self.ga.rword(1,4))
        self.ga('ipc_verb ON')
        self.assertEqual("ON",self.ga.rword(1,4))

    def test_OpenClose(self):
        self.ga('ipc_verb ON')
        self.ga('ipc_open /tmp/ipc.bin w')
        self.assertEqual("/tmp/ipc.bin",self.ga.rword(1,4))
        self.ga('ipc_close')
        self.ga('ipc_open /tmp/ipc.bin r')
        self.assertEqual("/tmp/ipc.bin",self.ga.rword(1,4))
        self.ga('ipc_close')
        self.ga('ipc_verb OFF')

    def test_save(self):
        self.ga('ipc_verb ON')
        self.ga('ipc_save ts /tmp/ts.bin')
        self.assertEqual("20+3358+73+46",self.ga.rword(2,2))
        self.assertEqual("/tmp/ts.bin",self.ga.rword(3,5))
        self.ga('ipc_open /tmp/ps.bin w')
        self.assertEqual("/tmp/ps.bin",self.ga.rword(1,4))
        self.ga('ipc_save ps /tmp/ps.bin')
        self.assertEqual("20+3358+73+46",self.ga.rword(2,2))
        self.ga('ipc_verb OFF')

    def test_load(self):
        self.ga('ipc_verb ON')
        self.ga('ipc_save ts /tmp/ts.bin')
        self.ga('define tsload = ipc_load("/tmp/ts.bin")')
        self.ga('d ts - tsload')
        cmin = abs(float(self.ga.rword(1,2)))
        assert cmin<1e-4, 'large load/save discrepancy: cmin=%f'%cmin 
        cmax = abs(float(self.ga.rword(1,4)))
        assert cmax<1e-4, 'large load/save discrepancy: cmax=%f'%cmax 

    def test_error(self):
        try:
            self.ga('ipc_open /tmp/t354f_CXCFCGAW__ r')
        except:
            self.assertEqual("'/tmp/t354f_CXCFCGAW__'",self.ga.rword(1,8))
            line1 = self.ga.rline(1)
        try:
            self.ga('ipc_error')
        except:
            self.assertEqual("'/tmp/t354f_CXCFCGAW__'",self.ga.rword(1,8))
            line2 = self.ga.rline(1)
            self.assertEqual(line1,line2)

    def test_udf_save(self):
        self.ga('ipc_verb ON')
        self.ga('d ipc_save(ts,/tmp/ts.bin)')
        self.assertEqual("20+3358+73+46",self.ga.rword(2,2))
        self.ga('ipc_verb OFF')

#......................................................................

if __name__ == "__main__":
    run(ut)
