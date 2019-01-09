function drawtcft(args)

i=1

ftlat=subwrd(args,i); i=i+1
ftlon=subwrd(args,i); i=i+1
fttype=subwrd(args,i); i=i+1
ftszscl=subwrd(args,i); i=i+1
ftbcol=subwrd(args,i); i=i+1
ftfcol=subwrd(args,i); i=i+1


ftsiz=0.115
ftsizs=ftsiz+0.025
ftsizi=ftsiz-0.050
if(ftszscl != '' & ftszscl!='ftszscl') ; ftsiz=ftsiz*ftszscl ; endif

nall=1
n=1
while(n<=nall) 

#
# check if lon setting is deg w
#
  if(_lon1 < 0)
    ftlon=ftlon-360.0
  endif

  'q w2xy 'ftlon' 'ftlat
if(subwrd(result,1) = 'No')
  return
endif
  x=subwrd(result,3)
  y=subwrd(result,6)
  xs=x-0.015
  ys=y+0.015

  ftc=3
  ftci=2
  if(fttype = -1); ftci=4; endif

  if(ftbcol != '' & ftbcol != 'ftbcol') ; ftc=ftbcol ; endif
  if(ftfcol != '' & ftfcol != 'ftfcol') ; ftci=ftfcol ; endif

  ftm=3

  'set line 0'
  'draw mark 'ftm' 'x' 'y' 'ftsizs

  'set line 'ftc
  'draw mark 'ftm' 'x' 'y' 'ftsiz

  'set line 'ftci
  'draw mark 'ftm' 'x' 'y' 'ftsizi

  n=n+1
endwhile

return
