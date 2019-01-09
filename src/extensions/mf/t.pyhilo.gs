function main(args)
rc=gsfallow(on)
rc=const()

'load udxt libmf.udxt'
'load udxt ../re/re.udxt'
'load udxt ../fish/fish.udxt'
'load udxt ../gxyat/gxyat.udxt'

# 01b - 45 kt at 041612
#btlat=18.4
#btlon=86.6
#'open /w21/dat/nwp2/ncep/gfs2/2009041612/gfs2.2009041612.ctl'

#
# TC 27S - 40 kt at 2009042712
#

#'open /w21/dat/nwp2/ncep/gfs2/2009042712/gfs2.2009042712.ctl'
#'open fim8.tracker.grb.2009042712.ctl'
#'open /w21/dat/nwp2/esrl/fim8/2009050412/fim8.2009050412.ctl'

'open  /w21/dat/nwp2/tctrk/2009050412/gfs2.tctrk.2009050412.ctl'

btlat=16.8
btlon=129.7
radinf=200
bearing=60

dlat=5
dlon=dlat*(4.0/3.0)

blat=btlat-dlat
elat=blat+2*dlat

blon=btlon-dlon
elon=blon+2*dlon

'load udxt libmf.udxt'
'load udxt ../re/re.udxt'

'set lev 850'
'set lat 'blat' 'elat
'set lon 'blon' 'elon

'vt=hcurl(ua,va)*1e5'
if(btlat < 0) ; 'vt=-(vt)' ; endif
#'d vt'

'psl=psl*0.01'
'set cint 1'
'd psl'


radinf=175
cmd='tcprop psl 'btlat' 'btlon' 'radinf' 'bearing
print cmd
cmd
'drawtcbt 'btlat' 'btlon
'c1hilo 'btlat' 'btlon' 'radinf' 'bearing

radinf=200
cmd='tcprop psl 'btlat' 'btlon' 'radinf' 'bearing
print cmd
cmd
'drawtcbt 'btlat' 'btlon
'c1hilo 'btlat' 'btlon' 'radinf' 'bearing

radinf=225
cmd='tcprop psl 'btlat' 'btlon' 'radinf' 'bearing
print cmd
cmd
'drawtcbt 'btlat' 'btlon
'c1hilo 'btlat' 'btlon' 'radinf' 'bearing

radinf=250
cmd='tcprop psl 'btlat' 'btlon' 'radinf' 'bearing
print cmd
cmd
'drawtcbt 'btlat' 'btlon
'c1hilo 'btlat' 'btlon' 'radinf' 'bearing


return

cmd='mfhilo vt gr h d 80 'btlat' 'btlon
print cmd
cmd
cmd='mfhilo vt tm 'btlat' 'btlon
print cmd
cmd

return


print r1
rc=getposit(r1)
chrhl=subwrd(rc,1)
lathl=subwrd(rc,2)
lonhl=subwrd(rc,3)
valhl=subwrd(rc,4)
grdhl=subwrd(rc,5)
lplhl=subwrd(rc,6)

arg=chrhl' 'lonhl' 'lathl' 'valhl' 'grdhl' 'lplhl' 'hlfmt
print arg
'p1hilo 'arg
rcp1=result
print rcp1

radinf=200.0
bearing=15.0

arg=radinf' 'bearing' 'rcp1
print 'ccccccccccccccc 'arg
'c1hilo 'arg


arg=lathl' 'lonhl' 'radinf' 'bearing
print arg
'tcprop vt 'arg



return




'mfhilo psl gr l d 80 'btlat' 'btlon
rc=getposit(result)
return

'd smth2d(psl)'
'c'
'vts=smth2d(vt,3,0.5)'
'set gxout stat'
'd vts'
#print result
'mfhilo vts gr h d 50 'btlat' 'btlon
#print result
return
#'tcprop vts 'btlat' 'btlon

print 'd xy2rt(vts,'btlat','btlon')'

'c'
'd psl'
'd xy2rt(psl,'btlat','btlon')'

return


function getposit(r1)
head=sublin(r1,1)
posit=sublin(r1,2)
print 'hhh   'head
print 'posit 'posit

chrhl=subwrd(posit,1)
lathl=subwrd(posit,2)
lonhl=subwrd(posit,3)
valhl=subwrd(posit,5)
grdhl=subwrd(posit,13)
lplhl=subwrd(posit,15)

rc=chrhl' 'lathl' 'lonhl' 'valhl' 'grdhl' 'lplhl

return(rc)

