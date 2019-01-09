"""
Test Basic GrADS Operations on the standard "model" test file.
"""

# Add parent directory to python search path
# ------------------------------------------
import os
import sys
sys.path.insert(0,'..')
sys.path.insert(0,'lib')

import unittest
from grads import GrADS

class TestModelFile(unittest.TestCase):

    def tearDown(self):
        del self.ga
#        os.system("/bin/rm -rf output")

    def test_01_Open(self):
        """
        Check whether file was opened correctly.
        """
        type = 'Gridded'
        vars = ['ps', 'ts', 'pr', 'ua', 'va', 'zg', 'ta', 'hus']
        var_levs = [0, 0, 0, 7, 7, 7, 7, 7]
        nx, ny, nz, nt = (72, 46, 7, 5)

        fh = self.fh
        self.assertEqual(type,fh.type)
        self.assertEqual(nx,fh.nx)
        self.assertEqual(ny,fh.ny)
        self.assertEqual(nz,fh.nz)
        self.assertEqual(nt,fh.nt)

        vars2 = fh.vars[:]
        var_levs2 = fh.var_levs[:]
        self.assertEqual(vars.sort(),vars2.sort())
        self.assertEqual(var_levs.sort(),var_levs2.sort())


    def test_02_Execs(self):
        """
        Exercises the exec command using both Unix and DOS text files.
        """
        self.ga("exec Exec.ga")
        self.ga("exec Exec_dos.ga")

    def test_02_Prints(self):
        """
        Exercises print/print file.eps/printim but does verify results.
        This is not really a test as it does not check the outcome.
        """
        self.ga("display ps")
        #self.ga("print   output/ps.eps")
        #self.ga("enable print output/ps.gx")
        self.ga("gxprint   output/ps.eps")
        self.ga("gxprint   output/ps.pdf")
        self.ga("gxprint   output/ps.png")
        #self.ga("gxprint   output/ps.jpg")
        self.ga("gxprint   output/ps.svg")
        #self.ga("gxprint   output/ps.gif")
        #self.ga("disable print")

    def test_02_Printim(self):
        """
        Exercises printim - not really a test as it does not check outcome.
        """
        self.ga("q config")
        cf = self.ga.rline(1)
        if 'printim' in cf:
            self.ga("printim output/ps.png")

    def test_03_Display(self):
        """
        Displays several variables and checks contour intervals
        """
        self._CheckCint('ps',500,1000,50,z=1,t=1)
        self._CheckCint('ts',240,310,10,z=1,t=1)
        self._CheckCint('10000*pr',0,8,1,z=1,t=1)

        self._CheckCint('ps',500,1000,50,z=1,t=5)
        self._CheckCint('ts',240,310,10,z=1,t=5)
        self._CheckCint('10000*pr',0,10,1,z=1,t=5)

        self._CheckCint('ua',-12,18,3,z=1,t=1)
        self._CheckCint('va',-15,15,3,z=1,t=1)
        self._CheckCint('zg',-100,300,50,z=1,t=1)
        self._CheckCint('ta',245,300,5,z=1,t=1)
        self._CheckCint('1000*hus',2,20,2,z=1,t=5)

        self._CheckCint('ua',-15,50,5,z=7,t=5)
        self._CheckCint('va',-20,20,5,z=7,t=5)
        self._CheckCint('zg',14800,16400,200,z=7,t=5)
        self._CheckCint('ta',195,225,5,z=7,t=5)
        self._CheckCint('10000*hus',1,9,1,z=5,t=5)

    def test_04_Write_generic(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/sequential -format sequential -vars ps ta -func sqrt(@) -time = = 2 ")

    def test_04_Write_stream(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/stream -be -format stream")

    def test_05_Read_stream(self):
        fh = self.ga.open("stream.ctl")
        self._CompareFiles(self.fh,fh)
        self.ga('close %d'%fh.fid)

    def test_04_Write_sequential(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/sequential -le -format sequential")

    def test_05_Read_sequential(self):
        fh = self.ga.open("sequential.ctl")
        self._CompareFiles(self.fh,fh)
        self.ga('close %d'%fh.fid)

    def test_04_stats(self):
        self.ga("set x 1 72")
        self.ga("lats4d -format stats")

    def test_04_Write_mean(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/mean -format stream -mean")

    def xtest_06_regriding_linear(self):
        self._CheckCint2("re(ts,2,2,bl)",240,310,10)
        self._CheckCint2("re(ts,2,1,bl)",240,310,10)

    def xtest_06_regriding_bessel(self):
        self._CheckCint2("re(ts,2,2,bs)",240,310,10)
        self._CheckCint2("re(ts,2,1,bs)",240,310,10)

    def xtest_06_regriding_box_average(self):
        self._CheckCint2("re(ts,2,2,ba)",240,310,10)
        self._CheckCint2("re(ts,2,1,ba)",240,310,10)

    def xtest_06_regriding_box_voting(self):
        self._CheckCint2("1e4*re(pr,8,8,vt,0.6,0.2)",0,8,1)

    def _CheckCint(self,name,cmin,cmax,cint,z=1,t=1):
        """
        Check contour intervals during display.
        """
        self.ga('clear')
        self.ga('display %s(z=%d,t=%d)'%(name,z,t))
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))

    def _CheckCint2(self,name,cmin,cmax,cint):
        """
        Check contour intervals during display.
        """
        self.ga.cmd('clear')
        self.ga.cmd('display %s'%name)
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

    def _GenericSetUp(self,bin,dat):
        global GrADSTestFiles
        global GrADSBinaryFiles
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga.open(GrADSTestFiles[dat])

    def notest_04_LATS_Coards_1(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/model ")

    def notest_04_LATS_Coards_2(self):
        fh = self.ga.open("output/model.nc")
        self._CompareFiles(self.fh,fh)
        self.ga('close %d'%fh.fid)

    def notest_04_LATS_GaGrib_1(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/model -format grads_grib")

    def notest_04_LATS_GaGrib_2(self):
        fh = self.ga.open("output/model.ctl")
        self._CompareFiles(self.fh,fh)
        self.ga('close %d'%fh.fid)

    def notest_04_LATS_grib(self):
        self.ga("set x 1 72")
        self.ga("lats4d -o output/model -format grib")


#......................................................................

class TestModelUrl(unittest.TestCase):

    def tearDown(self):
        del self.ga
#        os.system("/bin/rm -rf output")

    def test_01_Open(self):
        """
        Check whether file was opened correctly.
        """
        type = 'Gridded'
        vars = ['ps', 'ts', 'pr', 'ua', 'va', 'zg', 'ta', 'hus']
        var_levs = [0, 0, 0, 7, 7, 7, 7, 7]
        nx, ny, nz, nt = (72, 46, 7, 5)

        fh = self.fh
        self.assertEqual(type,fh.type)
        self.assertEqual(nx,fh.nx)
        self.assertEqual(ny,fh.ny)
        self.assertEqual(nz,fh.nz)
        self.assertEqual(nt,fh.nt)

        vars2 = fh.vars[:]
        var_levs2 = fh.var_levs[:]
        self.assertEqual(vars.sort(),vars2.sort())
        self.assertEqual(var_levs.sort(),var_levs2.sort())

    def test_03_Display(self):
        """
        Displays several variables and checks contour intervals
        """
        self._CheckCint('ps',500,1000,50,z=1,t=1)
        self._CheckCint('ts',240,310,10,z=1,t=1)
        self._CheckCint('10000*pr',0,8,1,z=1,t=1)

        self._CheckCint('ps',500,1000,50,z=1,t=5)
        self._CheckCint('ts',240,310,10,z=1,t=5)
        self._CheckCint('10000*pr',0,10,1,z=1,t=5)

        self._CheckCint('ua',-12,18,3,z=1,t=1)
        self._CheckCint('va',-15,15,3,z=1,t=1)
        self._CheckCint('zg',-100,300,50,z=1,t=1)
        self._CheckCint('ta',245,300,5,z=1,t=1)
        self._CheckCint('1000*hus',2,20,2,z=1,t=5)

        self._CheckCint('ua',-15,50,5,z=7,t=5)
        self._CheckCint('va',-20,20,5,z=7,t=5)
        self._CheckCint('zg',14800,16400,200,z=7,t=5)
        self._CheckCint('ta',195,225,5,z=7,t=5)
        self._CheckCint('10000*hus',1,9,1,z=5,t=5)

    def _CheckCint(self,name,cmin,cmax,cint,z=1,t=1):
        """
        Check contour intervals during display.
        """
        sys.stdout.write(name+' ... ')
        self.ga('clear')
        self.ga('display %s(z=%d,t=%d)'%(name,z,t))
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))

    def _GenericSetUp(self,bin,dat):
        global GrADSTestFiles
        global GrADSBinaryFiles
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga.open(GrADSTestFiles[dat])

#......................................................................

class TestStnUrl(unittest.TestCase):

    def _GenericSetUp(self,bin,url):
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga('open ' + url)

    def tearDown(self):
        del self.ga

    def test_01_Open(self):
        ga = self.ga
        ga('q file')

        Binary = ga.rword(3,2)
        Type   = ga.rword(4,3) + ' ' + ga.rword(4,4)
        Tsize  = ga.rword(5,3)
        nvars  = ga.rword(6,5)

        self.assertEqual(Binary,'http://monsoondata.org:9090/dods/stn/metar/2013/mon02')
        self.assertEqual(Type,'Station Data')
        self.assertEqual(Tsize,'672')
        self.assertEqual(nvars,'10')
        vars = ['cld','ds','filt','ptype','slp','ts','us','vis','vs','wx']
        for i in range(10):
            self.assertEqual(vars[i],ga.rword(7+i,1))

    def test_01_Stats(self):
        ga = self.ga
        ga('set gxout stat')
#                    var  count  undef    nu   min   max cmin cmax cint
#        self._stats(  'cld','2842','-9.99e+08',  '0', '20','25','20','25','0.5')
#        self._stats(   'ds', '2842','-9.99e+08','133','-49','27','-40','20','10')
#        self._stats( 'filt', '2842','-9.99e+08','0','0','6','0','6','0.5')

        self._stats(  'slp', '3476','-9.99e+08','1988','954.7','1045.7','960','1040','10')
        self._stats(   'ts', '3476','-9.99e+08','29','-44','99','-40','90','10')
        self._stats(   'us', '3476','-9.99e+08','14','-504','434.744','-500','400','100')
        self._stats(   'vs', '3476','-9.99e+08','14','-251.001','34.7687','-240','30','30')

    def _stats(self,var,count,v_undef,n_undef,min,max,cmin,cmax,cint):
        ga = self.ga
        ga('set gxout stat')
        ga('display '+var)

        sys.stdout.write(var+' ... ')
        self.assertEqual(count,ga.rword(6,4))
        self.assertEqual(v_undef,ga.rword(7,4))
        self.assertEqual(n_undef,ga.rword(8,4))

#        self.assertEqual(min,ga.rword(9,4))
#        self.assertEqual(max,ga.rword(9,5))

        self.assertEqual(cmin,ga.rword(10,5))
        self.assertEqual(cmax,ga.rword(10,6))
        self.assertEqual(cint,ga.rword(10,7))

#......................................................................

class TestPdefFile(unittest.TestCase):

    def tearDown(self):
        del self.ga

    def test_01_Open(self):
        """
        Check whether file was opened correctly.
        """

        type = 'Gridded'
        vars = ['pslv']
        var_levs = [ 0 ]
        nx, ny, nz, nt = (333,182,20,1)

        fh = self.fh
        self.assertEqual(type,fh.type)
        self.assertEqual(nx,fh.nx)
        self.assertEqual(ny,fh.ny)
        self.assertEqual(nz,fh.nz)
        self.assertEqual(nt,fh.nt)

        vars2 = fh.vars[:]
        var_levs2 = fh.var_levs[:]
        self.assertEqual(vars.sort(),vars2.sort())
        self.assertEqual(var_levs.sort(),var_levs2.sort())

    def test_03_Display(self):
        """
        Displays several variables and checks contour intervals
        """
        self._CheckCint('pslv',1004,1026,2,z=1,t=1)

    def _CheckCint(self,name,cmin,cmax,cint,z=1,t=1):
        """
        Check contour intervals during display.
        """
        self.ga('clear')
        self.ga('display %s(z=%d,t=%d)'%(name,z,t))
        self.assertEqual(cmin,int(self.ga.rword(2,2)))
        self.assertEqual(cmax,int(self.ga.rword(2,4)))
        self.assertEqual(cint,int(self.ga.rword(2,6)))

    def _GenericSetUp(self,bin):
        global GrADSTestFiles
        global GrADSBinaryFiles
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga.open(GrADSTestFiles['pdef'])

#......................................................................

class TestGrb2File(unittest.TestCase):

    def tearDown(self):
        del self.ga

    def _GenericSetUp(self,bin):
        global GrADSTestFiles
        global GrADSBinaryFiles
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga.open(GrADSTestFiles['grb2'])

    def test_01_Open(self):
        """
        Check whether file was opened correctly.
        """

        type = 'Gridded'
        vars = ['apcpsfc', 'hgtprs', 'prmslmsl', 'rhprs', 'tmpprs', 'ugrdprs', 'vgrdprs']
        var_levs = [0, 3, 0, 3, 3, 3, 3]
        nx, ny, nz, nt = (144,73,3,4)  # no E size yet

        fh = self.fh
        self.assertEqual(type,fh.type)
        self.assertEqual(nx,fh.nx)
        self.assertEqual(ny,fh.ny)
        self.assertEqual(nz,fh.nz)
        self.assertEqual(nt,fh.nt)

        vars2 = fh.vars[:]
        var_levs2 = fh.var_levs[:]
        self.assertEqual(vars.sort(),vars2.sort())
        self.assertEqual(var_levs.sort(),var_levs2.sort())

    def test_03_Display(self):
        """
        Displays several variables and checks contour intervals
        """
        self._CheckCint('apcpsfc',0,140,10,z=1,t=2)
        self._CheckCint('0.01*prmslmsl',950,1050,10,z=1,t=1)
        self._CheckCint('hgtprs',900, 1600,100,z=1,t=1)
        self._CheckCint('rhprs',0,100,10,z=1,t=1)
        self._CheckCint('tmpprs',235,300,5,z=1,t=1)
        self._CheckCint('ugrdprs',-30,30,10,z=1,t=1)
        self._CheckCint('vgrdprs',-25,35,5,z=1,t=1)

        self._CheckCint('apcpsfc',0,80,10,z=1,t=4)
        self._CheckCint('0.01*prmslmsl',950,1040,10,z=1,t=4)
        self._CheckCint('hgtprs',900, 1600,100,z=1,t=4)
        self._CheckCint('rhprs',0,100,10,z=1,t=4)
        self._CheckCint('tmpprs',240,300,5,z=1,t=4)
        self._CheckCint('ugrdprs',-25,30,5,z=1,t=4)
        self._CheckCint('vgrdprs',-25,30,5,z=1,t=4)

        self._CheckCint('hgtprs',10800,12400,200,z=3,t=4)
        self._CheckCint('rhprs',0,100,10,z=3,t=4)
        self._CheckCint('tmpprs',200,235,5,z=3,t=4)
        self._CheckCint('ugrdprs',-40,100,10,z=3,t=4)
        self._CheckCint('vgrdprs',-50,50,10,z=3,t=4)

    def _CheckCint(self,name,cmin,cmax,cint,z=1,t=1):
        """
        Check contour intervals during display.
        """
        self.ga('clear')
        self.ga('display %s(z=%d,t=%d)'%(name,z,t))
        self.assertEqual(cmin,int(self.ga.rword(1,2)))
        self.assertEqual(cmax,int(self.ga.rword(1,4)))
        self.assertEqual(cint,int(self.ga.rword(1,6)))

    def _GenericSetUp(self,bin):
        global GrADSTestFiles
        global GrADSBinaryFiles
        self.ga = GrADS(Bin=GrADSBinaryFiles[bin], Echo=False, Window=False)
        self.fh = self.ga.open(GrADSTestFiles['grb2'])

#......................................................................

#                          -----
#                          grads
#                          -----

class grads_grb(TestModelFile):
    def setUp(self):
        self._GenericSetUp('grads','grb')
    def test_05_Read_stream(self): pass
    def test_05_Read_sequential(self): pass

class grads_grb2(TestGrb2File):
    """Grib-2 specific tests"""
    def setUp(self):
        self._GenericSetUp('grads')

class grads_nc(TestModelFile):
    def setUp(self):
        self._GenericSetUp('grads','nc')

class grads_ctlnc(TestModelFile):
    def setUp(self):
        self._GenericSetUp('grads','ctlnc')

class grads_url(TestModelUrl):
    def setUp(self):
        self._GenericSetUp('grads','url')

class grads_stn(TestStnUrl):
    def setUp(self):
        self._GenericSetUp('grads','http://monsoondata.org:9090/dods/stn/metar/2013/mon02')

class grads_hdf(TestModelFile):
    def setUp(self):
        self._GenericSetUp('grads','hdf')

class grads_ctlhdf(TestModelFile):
    def setUp(self):
        self._GenericSetUp('grads','ctlhdf')

class grads_pdef(TestPdefFile):
    def setUp(self):
        self._GenericSetUp('grads')


#                          --------
#                          gradsdap
#                          --------

class gradsdap_grb(TestModelFile):
    def setUp(self):
        self._GenericSetUp('gradsdap','grb')
    def test_05_Read_stream(self):
        pass
    def test_05_Read_sequential(self):
        pass

class gradsdap_grb2(TestGrb2File):
    """Grib-2 specific tests"""
    def setUp(self):
        self._GenericSetUp('gradsdap')

class gradsdap_nc(TestModelUrl):
    def setUp(self):
        self._GenericSetUp('gradsdap','nc')

class gradsdap_ctlnc(TestModelUrl):
    def setUp(self):
        self._GenericSetUp('gradsdap','ctlnc')

class gradsdap_url(TestModelFile):
    def setUp(self):
        self._GenericSetUp('gradsdap','url')


class gradsdap_ctlhdf(TestModelFile):
    def setUp(self):
        self._GenericSetUp('gradsdap','ctlhdf')

class gradsdap_hdf(TestModelFile):
    def setUp(self):
        self._GenericSetUp('gradsdap','hdf')

class gradsdap_pdef(TestPdefFile):
    def setUp(self):
        self._GenericSetUp('gradsdap')

#......................................................................

def run_all_tests(verb=2,BinDir=None,DataDir=None):
    """
    Runs all tests based on the standard *model* testing file.
    """

#   Search for a reasonable default for binary dir
#   ----------------------------------------------
    if BinDir is None:
        BinDir = ''
        if os.path.exists('../src/grad.c'):
            BinDir = '../src/'

#   Search for a reasonable default for data files
#   ----------------------------------------------
    if DataDir is None:
        sample = 'model.grb'
        for dir in ( '.', 'data', '../data', '../../../data'):
            if os.path.exists(dir+'/'+sample):
                DataDir = dir + '/'
                break

#   File names
#   ----------
    global GrADSTestFiles
    GrADSTestFiles = { 'grb' : DataDir+'model.ctl',      \
                       'grb2': DataDir+'model_25.ctl',   \
                       'nc'  : DataDir+'model.nc',       \
                       'ctlnc'  : DataDir+'model_nc.ctl',       \
                       'url' : 'http://monsoondata.org:9090/dods/model', \
                       'hdf' : DataDir+'model.hdf',      \
                       'ctlhdf' : DataDir+'model_sds.ctl',      \
                       'pdef': DataDir+'pdef.ctl',       \
                       'dap' : DataDir+'model_dap.ddf'   }

    global GrADSBinaryFiles
    GrADSBinaryFiles = { 'grads'    : BinDir+'grads',    \
                         'gradsdap' : BinDir+'gradsdap'  }


    print ""
    print "Testing with GrADS Data Files from " + DataDir
    if BinDir is '':
        print "Testing with GrADS binaries from PATH"
        rc = os.system('which grads')
        if rc: 
           raise GrADSError, "cannot find grads"
    else:
        print "Testing with GrADS binaries from " + BinDir
    print ""


#   Assemble the test suite
#   -----------------------
    load = unittest.TestLoader().loadTestsFromTestCase
    TestSuite = []
    npass = 0
    Failed = []
    
#   grads
#   -----
    bin = 'grads'
    if os.path.exists(GrADSBinaryFiles[bin]):
        TestSuite.append(load(grads_grb))
        TestSuite.append(load(grads_grb2))
        TestSuite.append(load(grads_nc))
        TestSuite.append(load(grads_ctlnc))
        TestSuite.append(load(grads_url))
        TestSuite.append(load(grads_stn))
        TestSuite.append(load(grads_hdf))
        TestSuite.append(load(grads_ctlhdf))
        TestSuite.append(load(grads_pdef))
        print '+ Will test GrADS binary <%s>'%GrADSBinaryFiles[bin]
        npass += 1
    else:
        print '- Not testing GrADS binary <%s>, file missing'%GrADSBinaryFiles[bin]
        Failed.append(GrADSBinaryFiles[bin])

    print ""
    all = unittest.TestSuite(TestSuite)

#   gradsdap
#   --------
    bin = 'gradsdap'
    if os.path.exists(GrADSBinaryFiles[bin]):
        TestSuite.append(load(gradsdap_grb))
        TestSuite.append(load(gradsdap_grb2))
        TestSuite.append(load(gradsdap_nc))
        TestSuite.append(load(gradsdap_ctlnc))
        TestSuite.append(load(gradsdap_url))
        TestSuite.append(load(gradsdap_hdf))
        TestSuite.append(load(gradsdap_ctlhdf))
        TestSuite.append(load(gradsdap_pdef))
        print '+ Will test GrADS binary <%s>'%GrADSBinaryFiles[bin]
        npass += 1
    else:
        print '- Not testing GrADS binary <%s>, file missing'%GrADSBinaryFiles[bin]
        Failed.append(GrADSBinaryFiles[bin])

    print ""
    all = unittest.TestSuite(TestSuite)

    if not npass:
        print "Could not find a single binary to test..."
        return 1

#   Go for it
#   ---------
    os.system("/bin/rm   -rf output")
    os.system("/bin/mkdir -p output")

    Results = unittest.TextTestRunner(verbosity=verb).run(all)

    if len(Failed)>0:
        print "Could NOT test %s GrADS binaries: "%len(Failed), Failed 

#   Return number of errors+failures: skipped binaries do not count
#   ---------------------------------------------------------------
    if not Results.wasSuccessful(): 
        raise IOError, 'GrADS tests failed'
    else:
        os.system("/bin/rm   -rf output .grads.lats.table")



