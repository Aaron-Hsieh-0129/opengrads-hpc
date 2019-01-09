function main(args)
rc=gsfallow(on)
rc=const()

'open psl.ctl'

area=global
area=nhem
area=ca

_hiloc=grhilo
#_hiloc=clhilo

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

'set lev 500'
'set cint 60'
#'!rm udf.'_hiloc'.out'
var=z
var=psl

if(var=z) 
  'set cint 60'
  varo=z
  clrad=2000
  clint=60
endif

if(var=psl)
  'set cint 0.5'
  'slp=psl*0.01'
  'psl=regrid2(slp,0.5,0.5,bs)'
  varo=psl
  clrad=300
  clint=100
endif

'd smth2d(psl,2,0.5,-0.5,0.5,-0.5)-psl'
return


'd 'varo
if(_hiloc='clhilo')
  'run philo2.gs 'varo' 'clrad' 'clint
  'draw title test of clhilo for psl analysis of 2001071812'
 
'mfhilo 'varo' cl l 'clrad' 'clint

#print result
else
  rc=philo(varo)
  'draw title test of grhilo for psl analysis of 2001071812'
'mfhilo 'varo' gr a'
endif

return

function philo(var,hlfmt,hlpcntl)
*
*	defaults to philo
*
if(hlfmt='' | hlfmt='hlfmt') ; hlfmt='"f5.0"' ; endif
if(hlpcntl='' | hlpcntl = 'hlpcntl') ; hlpcntl='0.25' ; endif
hlfmt='i5'

*
*	plotting params
*

hlmk=1
hlmksiz=0.05
hlmkthk=0.05
hlmkcol=1

hlchoffx=0.0
hlchoffy=0.10

hlchsizl=0.10
hlchcoll=2
hlchthkl=6
hlchfntl=5

hlchsizh=0.10
hlchcolh=1
hlchthkh=6
hlchfnth=5

hlvlsiz=0.06
hlvlcol=1
hlvlthk=6

pcnth=1
pcntl=1

print 'd '_hiloc'('var','hlfmt','hlpcntl')'
'd '_hiloc'('var','hlfmt','hlpcntl')'
uofile='udf.'_hiloc'.out'
rc=read(uofile)
card=sublin(rc,2)
iok=sublin(rc,1)
if(iok!=0) ; return; endif
nh=subwrd(card,1)
nl=subwrd(card,2)
rmgh=subwrd(card,3)
rmlh=subwrd(card,4)
rmgl=subwrd(card,5)
rmll=subwrd(card,6)

*
*	read and plot H's
*

i=1
while(i<=nh)
  rc=read(uofile)
  card=sublin(rc,2)
  iok=sublin(rc,1)
  chrhl.i=subwrd(card,1)
  lonhl.i=subwrd(card,2)
  lathl.i=subwrd(card,3)
  valhl.i=subwrd(card,4)
  grdhl.i=subwrd(card,5)
  lplhl.i=subwrd(card,6)
  i=i+1
endwhile

nhp=nint(nh*pcnth)

i=1
while(i<=nhp)

  if(lplhl.i > rmlh)
  'q w2xy 'lonhl.i' 'lathl.i
  xhl=subwrd(result,3)
  yhl=subwrd(result,6)

  'set string 'hlmkcol' c 'hlmkthk
  'draw mark 'hlmk' 'xhl' 'yhl' 'hlmksiz
  xhlc=xhl+hlchoffx
  yhlc=yhl+hlchoffy
  'set string 'hlchcolh' c 'hlchthkh
  'set strsiz 'hlchsizh
  'draw string 'xhlc' 'yhlc' `'hlchfnth%chrhl.i

  xhlv=xhl-hlchoffx
  yhlv=yhl-hlchoffy
  'set string 'hlvlcol' c 'hlvlthk
  'set strsiz 'hlvlsiz
  'draw string  'xhlv' 'yhlv' 'valhl.i
 
  endif

  i=i+1

endwhile

*
*	read and plot L's
*

i=1
while(i<=nl)
  rc=read(uofile)
  card=sublin(rc,2)
  iok=sublin(rc,1)
  chrhl.i=subwrd(card,1)
  lonhl.i=subwrd(card,2)
  lathl.i=subwrd(card,3)
  valhl.i=subwrd(card,4)
  grdhl.i=subwrd(card,5)
  lplhl.i=subwrd(card,6)
  i=i+1
endwhile


nlp=nint(nl*pcntl)
i=1
while(i<=nlp)

  if(lplhl.i > rmll) 

  'q w2xy 'lonhl.i' 'lathl.i

  xhl=subwrd(result,3)
  yhl=subwrd(result,6)
  'draw mark 'hlmk' 'xhl' 'yhl' 'hlmksiz

  xhlc=xhl+hlchoffx
  yhlc=yhl+hlchoffy
  'set string 'hlchcoll' c 'hlchthkl
  'set strsiz 'hlchsizl
  'draw string 'xhlc' 'yhlc' `'hlchfntl%chrhl.i

  xhlv=xhl-hlchoffx
  yhlv=yhl-hlchoffy
  'set string 'hlvlcol' c 'hlvlthk
  'set strsiz 'hlvlsiz
  'draw string  'xhlv' 'yhlv' 'valhl.i

  endif

  i=i+1

endwhile


return
*
*-------------------------- nint ------------------
*
function nint(i0)
  i0=i0+0.5
  i=0
  while(i<12)
    i=i+1
    if(substr(i0,i,1)='.')
      i0=substr(i0,1,i-1)
      break
    endif
  endwhile
return(i0)
