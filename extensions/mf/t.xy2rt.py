#!/usr/bin/env python

from TC import *
import AD

import GA
from grads.gacore import GaCore,GrADSError
from GA import GradsEnv,polyCircle,title

class G1(GaCore,MFbase):

    Quiet=True
    #Quiet=0
    
    def cmd ( self, gacmd, Quiet=Quiet, Block=True ):
        """
        Sends a command to GrADS. When Block=True, the output is captured 
        and can be retrieved by methods rline() and rword(). On input,
        *gacmd* can be a single GrADS command or several commands separated
        by a new line ('\n').
        """
        Verb = self.Verb
        for cmd_ in gacmd.split('\n'):
            cmd = cmd_ + '\n'
            self.Writer.write(cmd)
            self.Writer.flush()
            if Block:
                rc = self._parseReader(Quiet)
                if rc != 0: 
                    if Verb==1:   print "rc = ", rc, ' for ' + cmd_
                    raise GrADSError, 'GrADS returned rc=%d for <%s>'%(rc,cmd_)
                else:
                    if Verb>1:    print "rc = ", rc, ' for ' + cmd_
        return
        
    __call__ = cmd


class TcDiag(MFbase):

    dlat=12
    dlon=15

    dr=5
    dtheta=45
    rmax=200
    
    radinfIn=200
    radinfOut=800
    radinfInWind=0
    radinfOutWind=500
    
    radinfInnm=radinfIn*km2nm
    radinfOutnm=radinfOut*km2nm

    radinfInWindnm=radinfInWind*km2nm
    radinfOutWindnm=radinfOutWind*km2nm

    areaIn=radinfIn*radinfIn
    areaOut=radinfOut*radinfOut
    areaAnnulus=areaOut-areaIn
        

    def __init__(self,ga,ge,latc,lonc,stmid,dtg,tau,
                 dr=dr,dtheta=dtheta,rmax=rmax):

        self.ga=ga
        
        self.latc=latc
        self.lonc=lonc
        
        self.stmid=stmid
        self.dtg=dtg
        self.tau=tau

        self.vdtg=mf.dtginc(dtg,tau)
        self.vgtime=mf.dtg2gtime(self.vdtg)

        self.dr=dr
        self.dtheta=dtheta
        self.rmax=rmax

        # units are km for dr and rmax
        self.drnm=dr*km2nm
        self.rmaxnm=rmax*km2nm
        
        ge.lat1=latc-self.dlat
        ge.lat2=latc+self.dlat
        ge.lon1=lonc-self.dlon
        ge.lon2=lonc+self.dlon

        ge.getLevs()
        # -- set the lat/lon/lev/time dims
        ge.lev1=ge.Levs[0]
        ge.setLevs()
        ge.setLatLon()
        ge.setTimebyDtgTau(dtg,tau,verb=1)

        self.ge=ge
        
        self.pT=title(ga,ge,scale=0.80)

    def initGA(self,xlint=10,ylint=5):
        
        self.ga('c')
        self.ga('set grads off')
        self.ga('set timelab on')
        self.ga('set mpdset mres')
        self.ga('set map 15 0 5')
        self.ga('set xlint %d'%(xlint))
        self.ga('set ylint %d'%(ylint))


    def drawHurr(self):

        (xp,yp)=self.getW2xy(self.latc,self.lonc)
        self.ga("draw wxsym 41 %s %s 0.25 1"%(xp,yp))

    def drawRadinf(self,radinf):

        pC=polyCircle(self.ga,self.ge,self.latc,self.lonc,radinf)
        pC.border()



    def drawMark(self,lat,lon,cmarkval,markcol=3):

        (xp,yp)=self.getW2xy(lat,lon)
        self.ga('set line %d'%(markcol))
        self.ga("draw mark 3 %s %s 0.20"%(xp,yp))
        self.ga('set strsiz 0.07')
        self.ga('set string 0 c 5')
        self.ga("draw string  %s %s %s"%(xp,yp,cmarkval))


    def getW2xy(self,lat,lon):
        
        self.ga("q w2xy %f %f"%(lon,lat))

        xp=self.ga.rword(1,3)
        yp=self.ga.rword(1,6)

        return(xp,yp)


    def getVmaxRmax4Tcxy2rt(self):
        

        self.initGA()

        ga=self.ga
        ge=self.ge

        latc=self.latc
        lonc=self.lonc
        
        fld='w'
        ga("%s=mag(uas,vas)*%f"%(fld,ms2knots))
        ga('d %s'%(fld))

        # -- convert from x,y -> r,theta and parse result
        #
        ga("tcxy2rt %s %f %f %f %f %f"%(fld,latc,lonc,self.drnm,self.dtheta,self.rmaxnm))

        nlines=ga.nLines

        R=[]
        T=[]
        V={}
        Lat={}
        Lon={}
        Vmax={}
        Rmax={}
        Latmax={}
        Lonmax={}

        nr=ga.rword(2,2)
        nt=ga.rword(2,4)

        for n in range(4,nlines+1):
             #17    0   255    0    31.19  287.80       100974.12

             i=ga.rword(n,1)
             j=ga.rword(n,2)
             r=float(ga.rword(n,3))
             t=float(ga.rword(n,4))
             lat=float(ga.rword(n,5))
             lon=float(ga.rword(n,6))
             val=float(ga.rword(n,7))

             V[r,t]=val
             Lat[r,t]=lat
             Lon[r,t]=lon

             R.append(r)
             T.append(t)


        R=MF.uniq(R)
        T=MF.uniq(T)

        finalVmax=-999
        finalRmax=0.0
        nRmax=0
        
        nR=len(R)
        for t in T:

            Vm=-1e20

            for j in range(0,nR):

                r0=R[j]
                v0=V[r0,t]

                if(j==nR-1):
                    rp1=r0
                    vp1=V[r0,t]
                else:
                    rp1=R[j+1]
                    vp1=V[rp1,t]

                if(v0>Vm):
                    Vm=v0
                    Vmax[t]=v0
                    Rmax[t]=r0
                    Latmax[t]=Lat[r0,t]
                    Lonmax[t]=Lon[r0,t]

                #print 't ',t,r0,v0

            if(Vmax[t] > finalVmax): finalVmax=Vmax[t]
            finalRmax=finalRmax+Rmax[t]
            nRmax=nRmax+1
            print 'FFFF ',t,Vmax[t],Rmax[t],Latmax[t],Lonmax[t],v0,r0,Lat[r0,t],Lon[r0,t]


        if(nRmax > 0):
            print 'FFFF ',nRmax,finalRmax/nRmax
            finalRmax=(finalRmax/nRmax)*nm2km


        print 'Final Vmax: ',finalVmax,' finalRmax: ',finalRmax
        
        # -- draw the search radius (nm)
        #
        self.drawRadinf(self.rmaxnm)

        # -- draw the max points
        #
        for t in T:
            (xp,yp)=self.getW2xy(Latmax[t],Lonmax[t])
            ga('set line 2')
            ga("draw mark 3 %s %s 0.20"%(xp,yp))
            ga('set strsiz 0.09')
            ga('set string 0 c 5')
            ga("draw string  %s %s %2.0f"%(xp,yp,Vmax[t]))


        ga("mfhilo %s gr l d 100 %f %f"%(fld,self.latc,self.lonc))
        latvmin=float(ga.rword(2,2))
        lonvmin=float(ga.rword(2,3))
        vmin=float(ga.rword(2,5))

        self.drawMark(latvmin,lonvmin,"%02.0f"%(vmin))
        self.drawHurr()

        t1="TCdiag MAX WIND/RMX: for: %s  dtg: %s  tau: %d R=0-%2.0f km"%(self.stmid,self.dtg,self.tau,self.rmax)
        t2="sfc wind speed [kt]  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim vmax.%03d.png"%(self.tau))

        return(T,Vmax,Rmax,Latmax,Lonmax)


    def getPrw(self):

        self.initGA()

        ga=self.ga
        ge=self.ge

        fld='tpw'
        ga("%s=prw"%(fld))

        ga("tcprop %s %f %f %f"%(fld,self.latc,self.lonc,self.rmaxnm))
        meantpw=float(ga.rword(5,2))
        
        ga('d %s'%(fld))

        ga("mfhilo %s gr h d 100 %f %f"%(fld,self.latc,self.lonc))
        lattpw=float(ga.rword(2,2))
        lontpw=float(ga.rword(2,3))
        maxtpw=float(ga.rword(2,5))

        self.drawMark(lattpw,lontpw,"%02.0f"%(maxtpw))
        self.drawHurr()
        self.drawRadinf(self.rmaxnm)

        t1="TCdiag TPW: %2.0f for: %s  dtg: %s  tau: %d R=0-%2.0f km"%(meantpw,self.stmid,self.dtg,self.tau,self.rmax)
        t2="slp [mb]  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim prw.%03d.png"%(self.tau))


        return(meantpw)



    def getPmin(self):

        self.initGA()

        ga=self.ga
        ge=self.ge
        
        fld='slp'
        ga("%s=psl*0.01"%(fld))
        ga("mfhilo %s gr l d 100 %f %f"%(fld,self.latc,self.lonc))

        latpmin=float(ga.rword(2,2))
        lonpmin=float(ga.rword(2,3))
        pmin=float(ga.rword(2,5))

        pinc=2
        if(pmin < 980): pinc=4
        self.plotFldContour(fld,pinc)

        self.drawMark(latpmin,lonpmin,"%02.0f"%(pmin-1000.0))
        self.drawHurr()
        
        t1="TCdiag MINSLP: %4.0f for: %s  dtg: %s  tau: %d"%(pmin,self.stmid,self.dtg,self.tau)
        t2="slp [mb]  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim pmin.%03d.png"%(self.tau))

        return(pmin)


    def getShear(self):

        self.initGA()

        ga=self.ga
        ge=self.ge

        radinf=500
        radinfnm=radinf*km2nm

        fld='magshr'
        ga("%s=mag((ua(lev=200)-ua(lev=850)),(va(lev=200)-va(lev=850)))*%f"%(fld,ms2knots))
        ga("%s=re2(%s,0.25)"%(fld,fld))
        ga("tcprop %s %f %f %f"%(fld,self.latc,self.lonc,radinfnm))
        shrmag=float(ga.rword(5,2))
        
        ufld='ushr'
        ga("%s=(ua(lev=200)-ua(lev=850))*%f"%(ufld,ms2knots))
        ga("%s=re2(%s,0.25)"%(ufld,ufld))
        ga("tcprop %s %f %f %f"%(ufld,self.latc,self.lonc,radinfnm))
        meanu=float(ga.rword(5,2))
        
        vfld='vshr'
        ga("%s=(va(lev=200)-va(lev=850))*%f"%(vfld,ms2knots))
        ga("%s=re2(%s,0.25)"%(vfld,vfld))
        ga("tcprop %s %f %f %f"%(vfld,self.latc,self.lonc,radinfnm))
        meanv=float(ga.rword(5,2))
        
        shrdir=270.0-atan2(meanv,meanu)*rad2deg 

        if(shrdir < 0.0): shrdir=shrdir+360.0
        if(shrdir > 360.0): shrdir=360.0-shrdir
        
        self.plotFldContour(fld)
        ga('set gxout barb')
        ga('d ushr;skip(vshr,5))')

        ga("mfhilo %s gr l d 100 %f %f"%(fld,self.latc,self.lonc))
        latmshr=float(ga.rword(2,2))
        lonmshr=float(ga.rword(2,3))
        minshr=float(ga.rword(2,5))

        self.drawMark(latmshr,lonmshr,"%02.0f"%(minshr))
        self.drawHurr()
        self.drawRadinf(radinfnm)

        t1="TCdiag SHRMAG: %3.0f SHRDIR: %3.0f for: %s  dtg: %s  tau: %d R=0-%2.0f km"%(shrmag,shrdir,self.stmid,self.dtg,self.tau,radinf)
        t2="shear [kt] latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim magshr.%03d.png"%(self.tau))

        return(shrmag)


    def getVort850(self):

        self.initGA()

        ga=self.ga
        ge=self.ge

        radinf=1000
        radinfnm=radinf*km2nm

        fld='vt8'
        ga("%s=hcurl(ua(lev=850),va(lev=850))*1e7"%(fld))
        ga("%s=re2(%s,0.25)"%(fld,fld))
        ga("tcprop %s %f %f %f"%(fld,self.latc,self.lonc,radinfnm))

        meanvt850=float(ga.rword(5,2))
        print 'MMMMMMMMMMMMMMMMMM ',meanvt850

        self.plotFldContour(fld)

        ga("mfhilo %s gr h d 100 %f %f"%(fld,self.latc,self.lonc))
        latvt8=float(ga.rword(2,2))
        lonvt8=float(ga.rword(2,3))
        maxvt8=float(ga.rword(2,5))*0.01

        self.drawMark(latvt8,lonvt8,"%02.0f"%(maxvt8))
        self.drawHurr()
        self.drawRadinf(radinfnm)
        
        t1="TCdiag 850VORT: %4.0f for: %s  dtg: %s  tau: %d  R=0-%2.0f km"%(meanvt850,self.stmid,self.dtg,self.tau,radinf)
        t2="10e-7 s`a-1`n  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim vt850.%03d.png"%(self.tau))

        return(meanvt850)


    def getDiv200(self):

        self.initGA()

        ga=self.ga
        ge=self.ge

        radinf=1000
        radinfnm=radinf*km2nm

        fld='vt8'
        ga("%s=hdivg(ua(lev=200),va(lev=200))*1e7"%(fld))
        ga("%s=re2(%s,0.25)"%(fld,fld))
        ga("tcprop %s %f %f %f"%(fld,self.latc,self.lonc,radinfnm))

        meandiv200=float(ga.rword(5,2))
        print 'MMMMMMMMMMMMMMMMMM ',meandiv200

        self.plotFldContour(fld)

        ga("mfhilo %s gr h d 100 %f %f"%(fld,self.latc,self.lonc))
        latdv2=float(ga.rword(2,2))
        londv2=float(ga.rword(2,3))
        maxdv2=float(ga.rword(2,5))*0.01

        self.drawMark(latdv2,londv2,"%02.0f"%(maxdv2))
        self.drawHurr()
        self.drawRadinf(radinfnm)

        t1="TCdiag 200DVRG: %4.0f for: %s  dtg: %s  tau: %d  R=0-%2.0f km"%(meandiv200,self.stmid,self.dtg,self.tau,radinf)
        t2="10e-7 s`a-1`n  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        
        self.pT.top(t1,t2)

        ga("printim div200.%03d.png"%(self.tau))

        return(meandiv200)



    def getTang850(self):

        self.initGA()

        ga=self.ga
        ge=self.ge

        radinf=600
        radinfnm=radinf*km2nm

        fld='tv8'
        ga("%s=uv2trw(ua(lev=850),va(lev=850),%f,%f,1)*10"%(fld,self.latc,self.lonc))
        ga("%s=re2(%s,0.25)"%(fld,fld))
        ga("tcprop %s %f %f %f"%(fld,self.latc,self.lonc,radinfnm))

        meantv850=float(ga.rword(5,2))

        ga("mfhilo %s gr l d 100 %f %f"%(fld,self.latc,self.lonc))
        lattv8=float(ga.rword(2,2))
        lontv8=float(ga.rword(2,3))
        mintv8=float(ga.rword(2,5))

        self.plotFldGrfill(fld)
        
        self.drawMark(lattv8,lontv8,"%02.0f"%(mintv8))
        self.drawHurr()
        self.drawRadinf(radinfnm)
        
        t1="TCdiag 850TANG: %2.0f for: %s  dtg: %s  tau: %d  R=0-%2.0f km"%(meantv850,self.stmid,self.dtg,self.tau,radinf)
        t2="10e1 kt  latc: %4.1f  lonc: %4.1f"%(self.latc,self.lonc)
        self.pT.top(t1,t2)

        ga("printim tv850.%03d.png"%(self.tau))

        return(meantv850)


    def fld2var(self,fld,level):

        fact='1.0'
        afact='0.0'
        cfact=fact
        
        format='%5.1f'
        if(fld == 'u' and level == 'sfc'):
            var='uas'
            fact=ms2knots
            cfact=''
        elif(fld == 'u' and level != 'sfc'):
            var='ua'
            fact=ms2knots
            cfact=''
        elif(fld == 'v' and level == 'sfc'):
            var='vas'
            fact=ms2knots
            cfact=''
        elif(fld == 'v' and level != 'sfc'):
            var='va'
            fact=ms2knots
            cfact=''
        elif(fld == 'r' and level == 'sfc'):
            var='hurs'
            format='%5.0f'
        elif(fld == 'r' and level != 'sfc'):
            var='hur'
            format='%5.0f'
        elif(fld == 't' and level == 'sfc'):
            var='tas'
            afact='-273.16'
        elif(fld == 't' and level != 'sfc'):
            var='ta'
            afact='-273.16'
        elif(fld == 'p' and level == 'sfc'):
            var='psl'
            fact='0.01'
            format='%5.0f'
            cfact=fact
        elif(fld == 'z' and level != 'sfc'):
            var='zg'
            fact='0.1'
            format='%5.0f'
            cfact=fact

        return(var,fact,afact,format,cfact)

            

    def getSounding(self,ifld,level):

        def printmean(ifld,level,meanfld):
            cmeanfld="%s"%(format)%(meanfld)
            print 'fld: %3s level: %4s  mean: %s'%(ifld,level,cmeanfld)


            
        self.initGA()

        ga=self.ga
        ge=self.ge

        (fld,fact,afact,format,cfact)=self.fld2var(ifld,level)

        try:
            if(level != 'sfc'):
                ga("set lev %s"%(level))
            ga("%s=re2(%s,0.25)*%s + %s"%(ifld,fld,fact,afact))
        except GrADSError:
            meanfld=9999.
            printmean(ifld,level,meanfld)
            return(meanfld)

        self.plotFldContour(ifld)
        self.drawHurr()

        if(ifld == 'u' or ifld == 'v'):
            ga("tcprop %s %f %f %f"%(ifld,self.latc,self.lonc,self.radinfOutWindnm))
            meanfld=float(ga.rword(5,2))

            self.drawRadinf(self.radinfOutWindnm)

            t1="TCdiag Var: %s Level: %s  Mean: %-2.1f for: %s  dtg: %s  tau: %d  R=0-%2.0f km"%\
                (ifld,level,meanfld,self.stmid,self.dtg,self.tau,self.radinfOutWind)

            t2="%s [kt] latc: %4.1f  lonc: %4.1f"%(cfact,self.latc,self.lonc)
        
        else:
            ga("tcprop %s %f %f %f"%(ifld,self.latc,self.lonc,self.radinfInnm))
            meanin=float(ga.rword(5,2))*self.areaIn
        
            ga("tcprop %s %f %f %f"%(ifld,self.latc,self.lonc,self.radinfOutnm))
            meanout=float(ga.rword(5,2))*self.areaOut
            meanfld=(meanout-meanin)/self.areaAnnulus

            self.drawRadinf(self.radinfInnm)
            self.drawRadinf(self.radinfOutnm)
        
            t1="TCdiag Var: %s Level: %s  Mean: %-2.1f for: %s  dtg: %s  tau: %d  R=%2.0f-%2.0f km"%\
                (ifld,level,meanfld,self.stmid,self.dtg,self.tau,self.radinfIn,self.radinfOut)
            t2="%s  latc: %4.1f  lonc: %4.1f"%(cfact,self.latc,self.lonc)

        self.pT.top(t1,t2)

        ga("printim %s_%s.%03d.png"%(ifld,level,self.tau))
        
        printmean(ifld,level,meanfld)

        if(ifld == 'u'):

            meanu=meanfld
            ufld=ifld
            vfld='v'
            (vvar,fact,afact,format,cfact)=self.fld2var(vfld,level)
            ga("%s=re2(%s,0.25)*%s + %s"%(vfld,vvar,fact,afact))
            ga("tcprop %s %f %f %f"%(vfld,self.latc,self.lonc,self.radinfOutWindnm))
            meanv=float(ga.rword(5,2))
            self.plotWindbarbs(ufld,vfld)
            self.drawHurr()
            self.drawRadinf(self.radinfOutWindnm)
            t1="TCdiag Winds Level: %s  U: %-2.1f V: %-2.1f for: %s dtg: %s tau: %d R=%2.0f-%2.0f km"%\
            (level,meanu,meanv,self.stmid,self.dtg,self.tau,self.radinfIn,self.radinfOut)
            t2="%s [kt] latc: %4.1f  lonc: %4.1f"%(cfact,self.latc,self.lonc)
            self.pT.top(t1,t2)
            ga("printim uv_%s.%03d.png"%(level,self.tau))

        return(meanfld)
        

    def plotFldGrfill(self,fld):

        ga('set gxout grfill')
        ga('d %s'%(fld))
        ga('cbarn')
        ga('set map 0 0 8')
        ga('draw map')
        ga('set map 1 0 4')
        ga('draw map')

    def plotWindbarbs(self,u,v):

        ga('c')
        ga('set gxout barb')
        ga('set clevs 10 25 35 50 65 120')
        ga('set ccols 0 7  3  2  8  4   5')
        ga('d %s;skip(%s,6);mag(%s,%s)'%(u,v,u,v))
        ga('cbarn')
        ga('set map 0 0 8')
        ga('draw map')
        ga('set map 1 0 4')
        ga('draw map')

    def plotFldContour(self,fld,cint=None):
        
        if(cint != None):
            ga('set cint %f'%(cint))
            
        ga('set gxout contour')
        ga('d %s'%(fld))




class MyCmdLine(CmdLine):

    def __init__(self,argv=sys.argv):

        if(argv == None): argv=sys.argv
        
        self.argv=argv
        self.argopts={
            1:['dtgopt',  'run dtgs'],
            2:['modelopt',    'model'],
            }

        self.defaults={
            'doupdate':0,
            }

        self.options={
            'override':['O',0,1,'override'],
            'verb':['V',0,1,'verb=1 is verbose'],
            'quiet':['q',0,1,' run GA in quiet mode'],
            'ropt':['N','','norun',' norun is norun'],
            'dowindow':['W',0,1,'1 - dowindow in GA.setGA()'],
            'doxv':['X',0,1,'1 - xv the plot'],
            }

        self.purpose='''
purpose -- generate TC large-scale 'diag' file for lgem/ships/stips intensity models
 '''
        self.examples='''
%s 2010052500 gfs2
'''
        


#mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
#
# main
#

argstr="pyfile 2010052500 gfs2"
argv=argstr.split()
#argv=sys.argv
CL=MyCmdLine(argv=argv)
CL.CmdLine()
exec(CL.estr)
if(verb): print CL.estr

gaopt='-g 1024x768'
ddir="%s/nwp2/w2flds/dat"%(W2BaseDirDat)

dtgs=mf.dtg_dtgopt_prc(dtgopt)
dtg=dtgs[0]

stmid='90L.2010'

models=modelopt.split(',')
model=models[0]

tD=TcData()

apath="%s/%s/w2flds/tctrk.atcf.%s.%s.txt"%(TcAdecksEsrlDir,dtg[0:4],dtg,model)
apath="%s/%s/tracks.all.%s.txt"%(TcAdecksNcepDir,dtg[0:4],dtg)

aid='avno'
aD=AD.Adeck(apath)
aT=aD.GetAidTrks(aid,stm1id=stmid)

try:
    trk=aT.atrks[dtg]
except:
    print 'EEE no traks for aid: ',aid,' stmid: ',stmid
    sys.exit()

#stmids=tD.getStmidDtg(dtg)
#print stmids

tau=0
(latc,lonc,vmaxc,pminc)=trk[tau]
print 'TTTT tau: ',tau,' latc: ',latc,' lonc: ',lonc,' vmaxc: ',vmaxc,' pminc: ',pminc

ctlpath='%s/%s/%s/%s.w2flds.%s.ctl'%(ddir,model,dtg,model,dtg)
ga=G1(Bin='grads2',Opts=gaopt,Window=dowindow)

#ga=GA.setGA(window=dowindow,opt=gaopt,type='gacore')
ga.fh=ga.open(ctlpath)
ge=GA.GradsEnv(ga)
ga('load udxt libmf.udxt')


tG=TcDiag(ga,ge,latc,lonc,stmid,dtg,tau)

levels=['sfc']+ge.Levs

fldssfc=['t','r','p','u','v']
fldua=['t','r','z','u','v']
for level in levels:

    if(level == 'sfc'):
        flds=fldssfc
    else:
        flds=fldua
        level="%2.0f"%(level)

    for fld in flds:
        rc=tG.getSounding(fld,level)
        

rc=tG.getVmaxRmax4Tcxy2rt()
rc=tG.getTang850()
rc=tG.getPmin()
rc=tG.getPrw()
rc=tG.getShear()
rc=tG.getVort850()
rc=tG.getDiv200()

sys.exit()
