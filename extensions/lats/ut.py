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
        utUDX.setUp(self,['lats.udxt','lats/lats.udxt'])
        self.ga("!mkdir -p %soutput"%self.udxdir)

    def tearDown(self):
#        self.ga("!/bin/rm -rf output")
        del self.ga

    def test_LATS(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/model -format stream -vars ps ta -func sqrt(@) -time = = 2 "%self.udxdir)

    def test_LATS_mean(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/model -format stream -mean"%self.udxdir)

    def test_LATS_NetCDF3(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf -o %soutput/model "%self.udxdir)
        fh = self.ga.open("%soutput/model.nc"%self.udxdir)
        self._CompareFiles(self.fh,fh)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_NetCDF4(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf4 -o %soutput/model "%self.udxdir)
        fh = self.ga.open("%soutput/model.nc4"%self.udxdir,"sdf")
        self._CompareFiles(self.fh,fh)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_Shave(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf4 -shave 12 -o %soutput/shave "%self.udxdir)
        fh = self.ga.open("%soutput/shave.nc4"%self.udxdir,"sdf",0.05)
        self._CompareFiles(self.fh,fh)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_GZIP_1(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf4 -gzip 1 -o %soutput/gzip "%self.udxdir)
        fh = self.ga.open("%soutput/gzip.nc4"%self.udxdir,"sdf")
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_GZIP_2(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf4 -gzip 2 -o %soutput/gzip "%self.udxdir)
        fh = self.ga.open("%soutput/gzip.nc4"%self.udxdir,"sdf")
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_GZIP_9(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format netcdf4 -gzip 9 -o %soutput/gzip "%self.udxdir)
        fh = self.ga.open("%soutput/gzip.nc4"%self.udxdir,"sdf")
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_HDF4(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format hdf4 -o %soutput/model "%self.udxdir)
        fh = self.ga.open("%soutput/model.hdf"%self.udxdir)
        self._CompareFiles(self.fh,fh)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_GaGrib(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/model -format grads_grib"%self.udxdir)
        fh = self.ga.open("%soutput/model.ctl"%self.udxdir)
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_GribOnly(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/model -format grib"%self.udxdir)

    def test_LATS_stream(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/stream -be -format stream"%self.udxdir)

    def test_LATS_stream_Read(self):
        fh = self.ga.open("%sstream.ctl"%self.udxdir)
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_sequential(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -o %soutput/sequential -le -format sequential"%self.udxdir)

    def test_LATS_sequential_Read(self):
        fh = self.ga.open("%ssequential.ctl"%self.udxdir)
        self._CompareFiles(self.fh,fh,0.0001)
        self.ga.cmd('close %d'%fh.fid)

    def test_LATS_stats(self):
        self.ga.cmd("set x 1 72")
        self.ga.cmd("lats4d -format stats")

    def _CompareFiles(self,fh1,fh2,tol=0.01):
        vars1 = fh1.vars[:]
        vars2 = fh2.vars[:]
        self.assertEqual(vars1.sort(),vars2.sort())
        self.assertEqual(fh1.nt,fh2.nt)
        for i in range(len(fh1.vars)):
            var = fh1.vars[i]
            sys.stdout.write(var+' ... ')
            nz = fh1.var_levs[i]
            if nz==0:      nz=1
            if var=='hus': nz=5
            nt = fh1.nt
            for t in range(1,nt+1):
                for z in range(1,nz+1):
                    self.ga.cmd('clear')
                    self.ga.cmd('display (%s.%d(z=%d,t=%d) - %s.%d(z=%d,t=%d))/aave(abs(%s.%d(z=%d,t=%d)),global)'\
                                %(var,fh1.fid,z,t,var,fh2.fid,z,t,var,fh1.fid,z,t))
                    if 'WARNING!' in self.ga.rline(1):
                        i = self.ga.nLines
                    else:
                        i = 1
                    if self.ga.rword(i,1) == 'Constant':
                        err = abs(float(self.ga.rword(i,5)))
                        self.assertTrue(err<tol)
                    else:
                        try:
                            cmin = abs(float(self.ga.rword(i,2)))
                            cmax = abs(float(self.ga.rword(i,4)))
                        except:
                            print ""
                            print "var=<%s>, t=%d, z=%d"%(var,t,z)
                            for i in range(self.ga.nLines):
                                print self.ga.rline(i+1)
                            print ""
                        err = max(cmin,cmax)
                        if ( err >= tol ):
                            sys.stdout.write(var+'='+str(err)+' ... ')
                        self.assertTrue(err<tol)

#......................................................................

if __name__ == "__main__":
    run(ut)
