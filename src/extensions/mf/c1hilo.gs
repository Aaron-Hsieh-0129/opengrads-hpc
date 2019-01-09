function c1hilo(args)

rc=gsfallow(on)
rc=const()

i=1
ftlat=subwrd(args,i); i=i+1
ftlon=subwrd(args,i); i=i+1
radinf=subwrd(args,i) ; i=i+1
bearing=subwrd(args,i) ; i=i+1

mfact=math_cos(ftlat*_d2r)

ccol=1

ft1=ftlat+1.0
'set line 'ccol
'q w2xy 'ftlon' 'ftlat
if(subwrd(result,1) = 'No')
  return
endif
x1=subwrd(result,3)
y1=subwrd(result,6)

'q w2xy 'ftlon' 'ft1
if(subwrd(result,1) = 'No')
  return
endif
x2=subwrd(result,3)
y2=subwrd(result,6)

dy=y2-y1


circsiz=((radinf*2*mfact)/60.0)*dy
'draw mark 2 'x1' 'y1' 'circsiz

b1=bearing*_d2r
b2=(bearing+180)*_d2r

xh1=math_sin(b1)*circsiz*0.5+x1
yh1=math_cos(b1)*circsiz*0.5+y1
xh2=math_sin(b2)*circsiz*0.5+x1
yh2=math_cos(b2)*circsiz*0.5+y1

'draw line 'xh1' 'yh1' 'xh2' 'yh2

return