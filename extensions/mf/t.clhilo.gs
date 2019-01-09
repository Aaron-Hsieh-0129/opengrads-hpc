function main(args)

rc=gsfallow(on)
rc=const()

'load udxt libmf.udxt'

'open psl.ctl'

area=global
area=nhem
area=ca

'set lat 0 70'
'set lon -180 -0'

'set lat 15 50'
'set lon -120 -80'

if(area=global)
  'set lat -90 90'
  'set lon 0 360'
endif

if(area=nhem)
  'set lat 0 90'
  'set lon 0 360'
  'set mproj nps'
endif

var=slp

if(var=slp)
  'set cint 0.5'
  'slp=psl*0.01'
  varo=slp
  clrad=300
  clint=100
endif

'd 'varo
'philocl 'varo' 'clrad' 'clint
'draw title test of philocl.gs for psl analysis of 2001071812\mfhilo slp cl b 'clrad' 'clint
'printim slp_clhilo.png x1024 y768'

#'quit'

return

