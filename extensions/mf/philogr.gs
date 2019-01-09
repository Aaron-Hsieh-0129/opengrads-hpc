function philogr(args)

i=1
var=subwrd(args,i) ; i=i+1
hlpcntl=subwrd(args,i) ; i=i+1
hlfmt=subwrd(args,i) ; i=i+1

*
*	defaults to philo
*
if(hlfmt='' | hlfmt='hlfmt') ; hlfmt='%4.0f' ; endif
if(hlpcntl='' | hlpcntl = 'hlpcntl') ; hlpcntl='50' ; endif

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
hlchcolh=4
hlchthkh=6
hlchfnth=5

hlvlsiz=0.06
hlvlcol=1
hlvlthk=6

pcnth=1
pcntl=1

sortby='m'

#
# settting nsigdig outside [2,8] disables precision checking in ftn_grhilo.F
# default is 6
#
nsigdig='-1'
nsigdig=''

#
# get highs
#
'mfhilo 'var' gr h 'sortby' 'hlpcntl' 30.0 140.0 'nsigdig

grrc=result
i=1
card=sublin(grrc,i) ; i=i+1
np=subwrd(card,5)
print card' np: 'np

n=1
while(n<=np)

  card=sublin(grrc,i)
  print 'HHHHHHHHHHHHHHHHHH 'i' 'card
# H    34.25  -85.17  M:    1018.36  G:   23.7  L:   53.0  D:  749.91  GV:   0.223634  LV:   0.499963 
  chrhl=subwrd(card,1)
  lathl=subwrd(card,2)
  lonhl=subwrd(card,3)
  valhl=subwrd(card,5)
  grdhl=subwrd(card,7)
  lplhl=subwrd(card,9)

  valhl=math_format(hlfmt,valhl)

  'q w2xy 'lonhl' 'lathl
  xhl=subwrd(result,3)
  yhl=subwrd(result,6)

  'set string 'hlmkcol' c 'hlmkthk
  'draw mark 'hlmk' 'xhl' 'yhl' 'hlmksiz
  xhlc=xhl+hlchoffx
  yhlc=yhl+hlchoffy
  'set string 'hlchcolh' c 'hlchthkh
  'set strsiz 'hlchsizh
  'draw string 'xhlc' 'yhlc' `'hlchfnth%chrhl

  xhlv=xhl-hlchoffx
  yhlv=yhl-hlchoffy
  'set string 'hlvlcol' c 'hlvlthk
  'set strsiz 'hlvlsiz
  'draw string  'xhlv' 'yhlv' 'valhl

  n=n+1
  i=i+1

endwhile

#
# get lows
#
'mfhilo 'var' gr l 'sortby' 'hlpcntl' 30.0 140.0 'nsigdig

grrc=result
i=1
card=sublin(grrc,i) ; i=i+1
np=subwrd(card,5)
print card' np: 'np

n=1
while(n<=np)

  card=sublin(grrc,i)
  print 'LLLLLLLLLLLL 'i' 'card
# H    34.25  -85.17  M:    1018.36  G:   23.7  L:   53.0  D:  749.91  GV:   0.223634  LV:   0.499963 
  chrhl=subwrd(card,1)
  lathl=subwrd(card,2)
  lonhl=subwrd(card,3)
  valhl=subwrd(card,5)
  grdhl=subwrd(card,7)
  lplhl=subwrd(card,9)

  valhl=math_format(hlfmt,valhl)

  'q w2xy 'lonhl' 'lathl
  xhl=subwrd(result,3)
  yhl=subwrd(result,6)

  'set string 'hlmkcol' c 'hlmkthk
  'draw mark 'hlmk' 'xhl' 'yhl' 'hlmksiz
  xhlc=xhl+hlchoffx
  yhlc=yhl+hlchoffy
  'set string 'hlchcoll' c 'hlchthkl
  'set strsiz 'hlchsizl
  'draw string 'xhlc' 'yhlc' `'hlchfntl%chrhl

  xhlv=xhl-hlchoffx
  yhlv=yhl-hlchoffy
  'set string 'hlvlcol' c 'hlvlthk
  'set strsiz 'hlvlsiz
  'draw string  'xhlv' 'yhlv' 'valhl

  n=n+1
  i=i+1

endwhile

return

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

nhp=math_nint(nh*pcnth)

i=1
while(i<=nhp)

  chrhl.i=subwrd(card,1)
  lonhl.i=subwrd(card,2)
  lathl.i=subwrd(card,3)
  valhl.i=subwrd(card,4)
  grdhl.i=subwrd(card,5)
  lplhl.i=subwrd(card,6)

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
