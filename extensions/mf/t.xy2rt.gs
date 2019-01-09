function main(args)

rc=gsfallow(on)
rc=const()

'load udxt libmf.udxt'

'open /w21/dat/nwp2/w2flds/dat/gfs2/2010052500/gfs2.w2flds.2010052500.ctl'
'open /data/amb/hfip/fiorino/w21/dat/nwp2/w2flds/dat/gfs2/2015020100/gfs2.w2flds.2015020100.ctl'

#adeck: /w21/dat/tc/adeck/esrl/2010/w2flds/tctrk.atcf.2010052500.gfs2.txt
## g AVNO /w21/dat/tc/adeck/nhc/2010/aal902010.dat | g 052500
## AL, 90, 2010052500, 03, AVNO,   0, 289N,  722W,  31, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,   6, 287N,  723W,  26, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  12, 292N,  723W,  22, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  18, 297N,  719W,  19, 1007, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  24, 304N,  720W,  20, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  30, 314N,  721W,  21, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  36, 324N,  723W,  21, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  42, 331N,  727W,  20, 1007, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  48, 332N,  731W,  22, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  54, 323N,  743W,  22, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  60, 318N,  738W,  22, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  66, 315N,  715W,  24, 1006, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  72, 317N,  698W,  22, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  78, 316N,  684W,  24, 1005, XX,  34, NEQ,    0,    0,    0,    0, 
## AL, 90, 2010052500, 03, AVNO,  84, 319N,  665W,  22, 1006, XX,  34, NEQ,    0,    0,    0,    0, 

latc=28.9
lonc=287.8

dlat=5
dlon=5

lat1=latc-dlat
lat2=latc+dlat

lon1=lonc-dlon
lon2=lonc+dlon


'set lat 'lat1' 'lat2
'set lon 'lon1' 'lon2

'set lev 850'
'set gxout barb'
'u=ua*'_ms2kt
'v=va*'_ms2kt
#'d u;v'
'q w2xy 'lonc' 'latc
print result
xtc=subwrd(result,3)
ytc=subwrd(result,6)
'draw wxsym 41 'xtc' 'ytc' 0.25 1'

'vt=uv2trw(u,v,'latc','lonc',1)'
'uvt=uv2trw(u,v,'latc','lonc',2)'
'vvt=uv2trw(u,v,'latc','lonc',3)'

'vr=uv2trw(u,v,'latc','lonc',-1)'
'uvr=uv2trw(u,v,'latc','lonc',-2)'
'vvr=uv2trw(u,v,'latc','lonc',-3)'

'set gxout contour'
'set ccolor rainbow'
#'d vr'

'set gxout vector'
'set arrscl 0.5 30'
'set ccolor 2'
'd uvt;vvt'

'set ccolor 3'
'set arrscl 0.5 30'
'd uvr;vvr'

'set ccolor 1'
'set arrscl 0.5 30'
'd u;v'

return

#'d psl'
#'q pos'

'tcxy2rt psl 'latc' 'lonc' 30 45 252'
print result
return
