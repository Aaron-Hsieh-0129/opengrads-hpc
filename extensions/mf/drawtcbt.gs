function drawtcbt(args)

'set rgb 98   1   1   1'
'set rgb 99 254 254 254'
btc=99
i=1
btlat=subwrd(args,i); i=i+1
btlon=subwrd(args,i); i=i+1
btmw=subwrd(args,i); i=i+1
btszscl=subwrd(args,i); i=i+1
btcol=subwrd(args,i); i=i+1
btcolty=subwrd(args,i); i=i+1

if(btmw = 'btmw' | btmw = ''); btmw=30 ; endif

if(_ntcbt = 0) ; return ; endif

nbt=1
n=1
while(n<=nbt) 

  btsizmx=0.25
  btsizmn=0.15
  btsiz=btsizmx*(btmw/135)
  btsiz=0.50
  if(btsiz<btsizmn) ; btsiz=btsizmn ; endif
  btsym=41

  if(btszscl != '' & btszscl != 'btszscl' & btszscl != 'reset' )
    if(btszscl > 0)
       btsiz=btsiz*btszscl
    else
       btsiz=-1*btszscl
    endif
  endif

  btstrc=1

  if(btmw <= 65)
    btc=2
    if(btcolty != 'btcolty' & btcol != '' ) ; btc=btcolty ; endif
    btsym=41
    btstrc=btc
  endif

  if(btmw <= 35)
    btc=6
    if(btcol != 'btcol' & btcol != '' ) ; btc=btcol ; endif
    btsym=40
    btstrc=btc
  endif

  if(btmw <= 30)
    btc=6
    if(btcol != 'btcol' & btcol != '' ) ; btc=btcol ; endif
    btsym=40
    btstrc=btc
  endif


#
# check if lon setting is deg w
#
  if(_lon1 < 0)
    btlon=btlon-360.0
  endif

  'q w2xy 'btlon' 'btlat

  x=subwrd(result,3)
  y=subwrd(result,6)
#
# test if a plot has been made...
#
drawtest=substr(result,1,10)
if(drawtest = 'No scaling'); return; endif

  xs=x+0.015
  ys=y-0.015
  'draw wxsym 'btsym' 'xs' 'ys' 'btsiz' 98 8'
  'draw wxsym 'btsym' 'x' 'y' 'btsiz' 'btc' 6'

  btstrsiz=btsiz*0.15
  if(btmw >= 100) ;  btstrsiz=btsiz*0.125 ; endif

  'set strsiz 'btstrsiz

  xso=x+0.0
  yso=y+0.0

  'set string 0 c 10'
  'draw string 'xso' 'yso' 'btmw
  'set string 'btstrc' c 5'
  'draw string 'xso' 'yso' 'btmw

  n=n+1
endwhile

return

