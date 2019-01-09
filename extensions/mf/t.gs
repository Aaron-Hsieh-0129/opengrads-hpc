function main(args)
rc=gsfallow('on')
nwp1='/storage/dat/nwp/ncep/gfs/200811/gfs.10.2008110400.ctl'
nwp2='/storage3/nwp2/ncep/gfs2/2008110400/gfs2.2008110400.ctl'
f1=ofile(nwp1)
f2=ofile(nwp2)

print 'fffffffffffffffffffffffff 'f1' 'f2
dops=0
dovt=1
dotc=1

latc=9.4
lonc=-115.0
if(lonc < 0); lonc=360+lonc ; endif


dlat=7.5 ; dlon=10 ; dbuff=2.0

#dlat=3.0 ; dlon=4.0 ; dbuff=1.0

olatb=latc-dlat-dbuff
olate=olatb+2*dlat+2*dbuff

olonb=lonc-dlon-dbuff
olone=olonb+2*dlon+2*dbuff

ilatb=olatb+dbuff
ilate=olate-dbuff

ilonb=olonb+dbuff
ilone=olone-dbuff


'set lat 'olatb' 'olate
'set lon 'olonb' 'olone
'set lev 850'

'ut=uv2trw(ua,va)'

'vt=hcurl(ua,va)*1e5'
'vt2=hcurl(ua.2,va.2)*1e5'
'p=psl'
'p2=psl.2*0.01'

'set lat 'ilatb' 'ilate
'set lon 'ilonb' 'ilone
#
# tm center...
#
cmd='mfhilo vt2 tm h 'lonc' 'latc
print cmd ; cmd
card=sublin(result,3)
tmlonc=subwrd(card,2)
tmlatc=subwrd(card,4)

'set cint 0.5'
'd vt'

'cylprms vt2 'tmlonc' 'tmlatc

'vtan=uv2trw(ua.2,va.2)'

'cylprms vtan 'tmlonc' 'tmlatc



return


cmd='cylprms p2 'lonc' 'latc
print cmd ; cmd
return

'quit'
'rc=findctr(p2,'lonc','latc',min,pmsl,hires)'

cmd='mfhilo vt tm h 'lonc' 'latc
print cmd ;cmd
'rc=findctr(vt,'lonc','latc',min,pmsl,lores)'
return


#'vt=const(vt,0,-u)'
print 'rc=findctr(vt,'lonc','latc',max,zeta,hires)'
'rc=findctr(vt,'lonc','latc',max,zeta,lores)'
return

#'vth=regrid2(vt,0.25,0.25,bs)'
'd vt'

if(dotc)
'set lat 'ilatb' 'ilate
'set lon 'ilonb' 'ilone
'vti=vt'
#'vth=regrid2(vt,0.25,0.25,bs)'
#'vtl=regrid2(vt,0.25,0.25,bl)'
endif

'rc=findctr(vti,'latc','lonc',max,zeta,lores)'

return


'quit'


'd smth2d(vt)'

'mfhilo vth cl'
'mfhilo vth gr'

return



if(dovt)
'set cint 0.25'
'vts=smth9(smth9(vt))'
'd vt'
'mfhilo vt'
endif

if(dops)
'set cint 0.10'
'd ps'
'mfhilo ps'
endif

print 'RRRRRRRRRRRRRRRRRRRRR'
print result
print 'RRRRRRRRRRRRRRRRRRRRR'

cards=result
card=sublin(cards,1)
print card
np=subwrd(card,10)
print 'nnnnnnnnn 'np
np=np+1
n=2
while(n<=np)
  card=sublin(cards,n)
  print 'nnnn 'n' 'card
  clon=subwrd(card,3)
  clat=subwrd(card,4)
  'q w2xy 'clon' 'clat
  print result
  cx=subwrd(result,3)
  cy=subwrd(result,6)
  'draw mark 1 'cx' 'cy' 0.25'
  n=n+1
endwhile



return

