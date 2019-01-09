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

    verb=0
    
    def setUp(self):
        utUDX.setUp(self,['libmf.udxt','mf/libmf.udxt'])


    def test_mfhilo_gr(self,verb=verb):
        self.ga("""set lat 20 90
        set lon 60 240
        set lev 200
        zga=zg-ave(zg,lon=0,lon=360)
        mfhilo zga gr b d 100 33.67 222.50""")
        if(verb): self._PrintResult()

        self.assertEqual(2,int(self.ga.rword(1,5)))
        self.assertEqual(33,int(float(self.ga.rword(2,2))))
        self.assertEqual(42,int(float(self.ga.rword(3,2))))

        
    def test_mfhilo_cl(self,verb=verb):
        self.ga("""set lat 20 90
        set lon 60 240
        set lev 200
        zga=zg-ave(zg,lon=0,lon=360)
        mfhilo zga cl b 300 10000 33.67 222.50""")
        if(verb): self._PrintResult()

        self.assertEqual(9,int(self.ga.rword(1,5)))
        self.assertEqual(33,int(float(self.ga.rword(2,2))))


    def test_tcprop(self,verb=verb):
        self.ga("""set lat 20 90
        set lon 60 240
        set lev 200
        zga=zg-ave(zg,lon=0,lon=360)
        tcprop zga 33.67 222.50 300""")
        if(verb): self._PrintResult()

        radmean=float(self.ga.rword(6,2))
        self.assertEqual(227,int(radmean))

    def test_re2(self,verb=verb):
        self.ga("set lev 200")
        self.ga("d re2(ua,1.0)")
        if(verb): self._PrintResult()
        self.assertEqual(360,int(self.ga.rword(3,6)))


    def test_smth2d(self,verb=verb):
        self.ga("set lev 200")
        self._CheckCint('smth2d(ua,10,0.5)',-5,50,5)


    def test_esmrf(self):
        self._CheckCint('esmrf(ta)',5,40,5)


    def test_linreg(self,verb=verb):
        self.ga("""set lat 0
        set lev 200
        d linreg(zg*1e-5)""")

        if(verb): self._PrintResult()
        self.assertEqual("0.1244",self.ga.rword(1,4))



    # not tested yet...........................
    
    #def test_grhist(self):
    #    sys.stdout.write("skipped ... ")
    #    pass


    #def test_uv2trw(self):
    #    sys.stdout.write("skipped ... ")
    #    pass

    #def test_mfhilo_tm(self,verb=0):
    #    sys.stdout.write("skipped ... ")
    #    pass



#--
#            Useful Internal Methods for Writing Tests

    def _PrintResult(self):
        
        for i in range(0,self.ga.nLines+1):
            card=self.ga.rline(i)
            print 'card ',i,card


    def _CheckCint(self,name,cmin,cmax,cint):
        """
        Check contour intervals during display.
        """
        self.ga('clear')
        self.ga('display %s'%name)
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))


    def _CompareFiles(self,fh1,fh2):
        vars1 = fh1.vars[:]
        vars2 = fh2.vars[:]
        self.assertEqual(vars1.sort(),vars2.sort())
        self.assertEqual(fh1.nt,fh2.nt)
        for i in range(len(fh1.vars)):
            var = fh1.vars[i]
            nz = fh1.var_levs[i]
            if nz==0:      nz=1
            if var=='hus': nz=5
            nt = fh1.nt
            for t in range(1,nt+1):
                for z in range(1,nz+1):
                    self.ga('clear')
                    self.ga('display %s.%d(z=%d,t=%d) - %s.%d(z=%d,t=%d)'\
                                %(var,fh1.fid,z,t,var,fh2.fid,z,t))
#                    print ">>> t=%d, z=%d, %s --- %s "%(t,z,var,self.ga.rline(1))
                    self.assertEqual(self.ga.rline(1), \
                                     'Constant field.  Value = 0')

#......................................................................

if __name__ == "__main__":
    run(ut)
