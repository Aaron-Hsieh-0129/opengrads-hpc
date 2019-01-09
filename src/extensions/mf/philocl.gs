function philocl(args)
*
*	plot H/L using the NCEP user-defined extension mfhilo 'cl' option
*
*	philo2.gs created by Mike Fiorino on 960214/200107
*
*	input args:
*
*	varo = GrADS expression to plot H/L
*	ptype = 
*		maxmin	- plot both H and L
*		max	- plot only H
*		min	- plot only H
*
*	fmt = fortran-style format for the H/L value
*	clint = contour level parameter of the clhilo.f alogorithm
*	clrad = radius parameter (km) of the clhilo.f alogorithm
*		as clrad decrease the number of H/L increases
*
*	current defaults:
*	
*	fmt=i4 or I4 output (e.g., slp is displayed as 1024)
*	clint = '' use the  GrADS contour interval
*	clrad = 1000 km or synotpic scale H/L
*
varo=subwrd(args,1)
clrad=subwrd(args,2)
clint=subwrd(args,3)
ptype=subwrd(args,4)
fmt=subwrd(args,5)
*
*	set defaults
*
if(ptype='') ; ptype=maxmin ; endif
if(fmt='') ; fmt=i4 ; endif
if(clrad='') ; clrad=1000 ;endif
*
*	run the clhilo function
*
rc=clhilo(varo,ptype,fmt,clint,clrad)

return

*
*-------------------------- clhilo ------------------

function clhilo(var,maxmin,fmt,cint,rad)

  if(maxmin = "maxmin" | maxmin = "MAXMIN")
    mm='b'
  endif

  if(maxmin = "max" | maxmin = "MAX")
    mm='h'
  endif

  if(maxmin = "min" | maxmin = "MIN")
    mm='l'
  endif
*
*	set gxout to stat to find the GrADS contour interval
*
  if(cint = -999 | cint='') 
    'set gxout stat'
    'd 'var
    dum=sublin(result,9)
    say "dum="dum
    cint=subwrd(dum,7)
    say "cint set to "cint
    'set gxout contour'
  endif

'mfhilo 'var' cl 'mm' 'rad' 'cint
clrc=result
i=1
card=sublin(clrc,i) ; i=i+1
np=subwrd(card,5)
print card' np: 'np

n=1
while(n<=np)

  card=sublin(clrc,i)

  maxormin=subwrd(card,1)
  lat=subwrd(card,2)
  lon=subwrd(card,3)
  val=subwrd(card,5)
  val=math_format('%4.0f',val)
  
  'q w2xy 'lon' 'lat
  x=subwrd(result,3)
  y=subwrd(result,6)
  if(maxormin = "H"); str="H"; 'set string 4 c 6'; endif
  if(maxormin = "L"); str="L"; 'set string 2 c 6'; endif
  'set font 5'
  'set strsiz 0.1'
  'draw string 'x' 'y' 'str
  'set font 0'
  'set strsiz 0.07'
  'set string 1 tc 4'
  ytop=y-0.1
  'draw string 'x' 'ytop' 'val
  
  n=n+1
  i=i+1
endwhile


return
*
*	parse the udfile 
*
  while (1)
    res=read(udfile)
    rc=sublin(res,1)
    if (rc != 0)
      if(rc = 1); say "open error for file "udfile; return; endif
      if(rc = 2); say "end of file for file "udfile; break; endif
      if(rc = 9); say "I/O error for file "udfile; return; endif
    endif
    dum=sublin(res,2)
    
  endwhile
  rc=close(udfile)

return





