'reinit'
'open ../../test_data/model.ctl'
***********
*'set mproj off'
'set t 1'
'set lev 850'
page=1
while (page<=2)
fig=1
* Page size
*
pagex = 8.5
pagey = 11.0
*
* Size of one picture
*
xwidth = 8.2
ywidth = 4.1
*
* Margins
*
lmarg = pagex/2-xwidth/2
rmarg = 0.1
tmarg = pagey - 0.7
xdiff = 0.1
ydiff = 0.1
*

* Number of pictures
while(fig<=2)
  if (fig = 1) 
     xl = lmarg 
     xr = xl + xwidth
     yt = tmarg 
     yb = yt - ywidth
  endif
  if (fig = 2) 
     xl = lmarg
     xr = xl + xwidth
     yt = tmarg - ywidth - ydiff
     yb = yt - ywidth
  endif
  'set vpage 'xl' 'xr' 'yb' 'yt
  'set grads off'
  'set gxout grfill'

if (page=1)
  if (fig=1)
  'set lon 0 360'
  'set lat -90 90'
  'd re(ps,5,5)'
* A short hand for regrid
  'run cbarn.gs'
  'draw title regrid to 5 deg resolution\re(ps,5,5)'
  endif
endif

****************************** re ******************
* d re(exp,nx,linear,x0,dx,ny,linear,y0,dy):  a regrig function modified after popular 
*   Mike Fiorino's regrid2. More details of "re" can be found in an attatched document.
*
if (page=2)
  if (fig=1)  
  'set lon 0 360'
  'set lat -90 90'
* the input domain listed above should be larger than the output domain after regid 
  'd re(ps,360,linear,0.5,1,180,linear,-89.5,1)'
* the output domain is lon: (0.5 359.5), lat: (-89.5, 89.5)
  'run cbarn.gs'
  'draw title regrid to 1 deg resolution\re(ps,360,linear,0.5,1,180,linear,-89.5,1)'
  endif
  if (fig=2)  
  'set lon 0 360'
  'set lat -90 90'
  'd re(ps,1,1)'
* A short hand for regrid
  'run cbarn.gs'
  'draw title A shorthand to regrid to 1 deg resolution\re(ps,1,1)'
  endif

endif

********************************************************

 fig = fig + 1
endwhile

say pause
pull continue
*'printim newFunctions.jpg white'
'c'
page = page + 1
endwhile
******************************
*'disable print'
*'quit'
*'print'

say "all done"

* Modify existing functions
****************************** vint ******************
* d vint(bottom, exp, top)
* where bottom: bottom layer pressure in hPa, top: top layer pressure in hPa
*    Now, both bottom and top can be either constant or expression
*
if (page=11)
  if (fig=1)  
  'd vint(1000,dtdt,300)'
  'run cbarn.gs'
  'draw title Integrate[dTdt,{dp,1000,300}] (K/s*kg/m2)'
  endif
  if (fig=2)
  'd vint(pressfc/100,dTdt,presML/100)'
  'run cbarn.gs'
  'draw title (K/s*kg/m2)'
  endif
endif
