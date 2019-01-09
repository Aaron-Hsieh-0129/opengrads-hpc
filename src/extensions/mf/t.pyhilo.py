import os,sys

import M2
import TCw2 as TC
import const
import grads


#llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
#
#  local
#

def plotPosit(p):
    tt=p.split()
    chrhl=tt[0]
    lathl=tt[1]
    lonhl=tt[2]
    valhl=tt[4]
    grdhl=tt[12]
    lplhl=tt[14]

    hlfmt='%3.1f'
    arg= "%s %s %s %s %s %s %s"%(chrhl,lathl,lonhl,valhl,grdhl,lplhl,hlfmt)
    argm= "(%s,%s,%s,%s,%s,%s,%s)"%(chrhl,lathl,lonhl,valhl,grdhl,lplhl,hlfmt)
    print 'ppp ',chrhl,lathl,lonhl,valhl,grdhl,lplhl
    print 'aaa ',arg
    gcmd="p1hilo %s"%(arg)
    ga(gcmd)
    rcp1=ga.Lines
    
    return(gcmd,rcp1)
    
def plotBt(lat,lon,mw,btszscl=1.0,btcol=1,btcolty=2):

    cmw="%3.0f"%(mw)
    arg= "%s %s %s %s %s %s"%(lat,lon,cmw,btszscl,btcol,btcolty)
    gcmd="drawtcbt %s"%(arg)
    ga(gcmd)
    rcp1=ga.Lines
    
    return(gcmd,rcp1)
    
def plotFt(p,fttype=1,ftszscl=1.0,ftbcol=3,ftfcol=2):

    tt=p.split()
    chrhl=tt[0]
    lathl=tt[1]
    lonhl=tt[2]
    valhl=float(tt[4])
    grdhl=float(tt[12])
    lplhl=float(tt[14])

    cval="%3.0f"%(valhl)
    
    arg= "%s %s %s %s %s %s"%(lathl,lonhl,fttype,ftszscl,ftbcol,ftfcol)
    gcmd="drawtcft %s"%(arg)
    ga(gcmd)
    rcp1=ga.Lines
    
    return(gcmd,rcp1)
    

def plotXpt(stmid,dtg,model,var,type,ptype,btlat,btlon,btmw,radinf,bearing,
            doplotvar=0,doclear=0,doqpos=0,docirc=1,dopng=1,tag=None):

    if(ptype == 'w'):
        ftbcol=4
        ftfcol=2
    elif(ptype == 'm'):
        ftbcol=3
        ftfcol=1
    else:
        ftbcol=3
        ftfcol=5
        
    if(doclear): ga('c')
    if(doplotvar): ga("d %s"%(var))
    (rc1,gacmd)=plotBt(btlat,btlon,btmw)
    cmd="mfhilo %s gr %s d 80 %f %f"%(var,type,btlat,btlon)
    ga(cmd)
    rc=ga.Lines
    p1=rc[2]
    print 'qqqqqqqqqqqqqq ',p1
#    (rcp1,gacmd)=plotPosit(p1)
    (rcp1,gacmd)=plotFt(p1,ftbcol=ftbcol,ftfcol=ftfcol)
    if(doqpos): ga('q pos')

    if(doclear and docirc):
        t1="stmid: %s  dtg: %s vmax: %3.0f model: %s var: %s"%(stmid,dtg,btmw,model,var)
        t2=plotC1tcprop(var,p1,radinf,bearing,docirc)
        ga('set strsiz 0.06')
        cmd="draw title %s\%s"%(t1,t2)
        ga(cmd)

    if(dopng):
        if(tag != None):
            pngfile="/w3/rapb/wxmap2/tmp/mf.%s.%s.%s.%s.png"%(tag,stmid,dtg,var)
        else:
            pngfile="/w3/rapb/wxmap2/tmp/mf.%s.%s.%s.png"%(stmid,dtg,var)
        cmd="gxyat -o %s -r -x 1024 -y 768"%(pngfile)
        ga(cmd)
    
    return(p1)
    

def plotC1tcprop(var,p,radinf,bearing,docirc=1):

    tt=p.split()
    chr=tt[0]
    lat=tt[1]
    lon=tt[2]
    val=float(tt[4])
    grd=float(tt[12])
    lpl=float(tt[14])

    cval="%3.0f"%(val)

    arg="%s %s %s %s"%(lat,lon,radinf,bearing)
    cmd="tcprop %s %s"%(var,arg)
    ga(cmd)
    cards=ga.Lines

    for card in cards:
        print 'cccc ',card

    meancard=cards[3]
    tt=meancard.split()
    mean=float(tt[1])
    meanh1=float(tt[3])
    meanh2=float(tt[5])
    t2="m: %3.1f mRH: %3.1f mLH: %3.1f  R: %3.0f [nm] B: %3.0f deg"%(mean,meanh1,meanh2,radinf,bearing)
    print 'tttt222 ',t2

    if(docirc):
        cmd="c1hilo %s %s %s %s"%(lat,lon,radinf,bearing)
        ga(cmd)
        
    return(t2)

    





dowindow=0
dtg='2009050312'
dtg='2009050412'
model='gfs2'
model='fim8'

tt=TC.GetStmidsByDtg(dtg)
stmids=tt[0]
stmid=stmids[0]
bts=TC.GetBtLatLonVmax(stmid)
bt=bts[dtg]

btlat=bt[0]
btlon=bt[1]
btmw=bt[2]
btdir=bt[4]
btspd=bt[5]

print 'qqqqqqqqqqq ',btdir,btspd

radinf=200.0
bearing=btdir

dlat=4
dlon=dlat*(4.0/3.0)

blat=btlat-dlat
elat=blat+2*dlat

blon=btlon-dlon
elon=blon+2*dlon

ga=grads.GaNum(Bin='../../bin/grads',Window=dowindow)

ga('load udxt libmf.udxt')
ga('load udxt ../re/re.udxt')
ga('load udxt ../fish/fish.udxt')
ga('load udxt ../gxyat/gxyat.udxt')

ga('set mpdset hires')
ga('set map 15 0 8')


g1=M2.Model2(model).DataPath(dtg)
dpgfs=g1.dpaths[0]

print 'qqqq ',dpgfs
ga.open(dpgfs)

ga('set lev 850')
ga('vt=hcurl(ua,va)*1e5')
print 'btlat ',btlat,btlon,btdir

#ga('vt=smth2d(vt,2)')
ga('set csmooth on')
if(btlat < 0):  ga('vt=-vt')
ga('psi=fish(vt*1e-5))*1e-6')
ga('psl=((psl*0.01)-1000.0)')
dorest=0
if(dorest):
    ga('tup=zg(lev=200)-zg(lev=500)')
    ga('tup=smth2d(tup,10,0.5)')
    ga('z8=zg(lev=850)')
    ga('z7=zg(lev=700)')
    ga('was=mag(uas,vas)*%f'%(const.ms2knots))

    
ga('set lat %f %f'%(blat,elat))
ga('set lon %f %f'%(blon,elon))

ga('tcprop psl %f %f 200 0'%(btlat,btlon))

sys.exit()

var='psl'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1,dopng=0)
var='vt'
p1=plotXpt(stmid,dtg,model,var,'h','w',btlat,btlon,btmw,radinf,bearing,doclear=0,doplotvar=0,docirc=0,dopng=0)
var='psi'
p1=plotXpt(stmid,dtg,model,var,'l','w',btlat,btlon,btmw,radinf,bearing,doclear=0,doplotvar=0,docirc=0,dopng=0)
var='z8'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=0,doplotvar=0,docirc=0,dopng=0)
var='z7'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=0,doplotvar=0,docirc=0,dopng=0)
var='was'
p1=plotXpt(stmid,dtg,model,var,'l','w',btlat,btlon,btmw,radinf,bearing,doclear=0,doplotvar=0,docirc=0,dopng=1,tag='circs')

var='psl'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='vt'
p1=plotXpt(stmid,dtg,model,var,'h','w',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='psi'
p1=plotXpt(stmid,dtg,model,var,'l','w',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='z8'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='z7'
p1=plotXpt(stmid,dtg,model,var,'l','m',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='was'
p1=plotXpt(stmid,dtg,model,var,'l','w',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
var='tup'
p1=plotXpt(stmid,dtg,model,var,'h','m',btlat,btlon,btmw,radinf,bearing,doclear=1,doplotvar=1,docirc=1)
sys.exit()


sys.exit()

rc=plotXpt('psi','l','w',btlat,btlon,btmw,doclear=0)
rc=plotXpt('psl','l','m',btlat,btlon,btmw,doclear=0)
rc=plotXpt('was','l','w',btlat,btlon,btmw)
rc=plotXpt('tup','h','m',btlat,btlon,btmw)
rc=plotXpt('z8','l','m',btlat,btlon,btmw)
rc=plotXpt('z7','l','m',btlat,btlon,btmw)





    

    

#ga('mfhilo vt gr h d 80 %f %f'%(btlat,btlon))
#rcvt=ga.Lines
#ga('mfhilo psl gr l d 80 %f %f'%(btlat,btlon))
#rcpsl=ga.Lines
#print rcvty








