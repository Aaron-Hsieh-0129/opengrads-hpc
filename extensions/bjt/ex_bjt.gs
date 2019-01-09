'reinit'
'open model.ctl'
***********
*'set mproj off'
'set t 1'
'set lev 850'
page=1
while (page<=10)
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
*******************************minv******************
* d minv(exp1,exp2)
* display the minimum value between exp1 amd exp2  
if (page=1)
  if (fig=1)  
  'set clevs -12 -9 -6 -3 0 3 6 9 12'
  'd u'
  'run cbarn.gs'
  'draw title d u'
  endif
  if (fig=2)  
  'set clevs -12 -9 -6 -3 0 3 6 9 12'
  'd minv(u,0)'
  'run cbarn.gs'
  'draw title d minv(u,0)'
  endif
endif
*******************************maxv******************
* d maxv(exp1,exp2)
* display the maximum value between exp1 amd exp2  
if (page=2)
  if (fig=1)  
  'set clevs -12 -9 -6 -3 0 3 6 9 12'
  'd u'
  'run cbarn.gs'
  'draw title d u'
  endif
  if (fig=2)  
  'set clevs -12 -9 -6 -3 0 3 6 9 12'
  'd maxv(u,0)'
  'run cbarn.gs'
  'draw title d maxv(u,0)'
  endif
endif
*******************************if******************
* d if(exp1,relation,exp2,true_exp,false_exp)'
*
* If the relation beween exp1 and exp2 is true, it displays true-exp. Otherwise,
*   it displays false_exp.  
* The relation operator can be: "==", "!=", ">=", "<=", ">" and "<". They are:
*   equal to, not equal to, larger than or equal to, less than or equal to,
*   larger than, and less than, respectively.
*
if (page=3)
  if (fig=1)
*  'set cmin 270'
*  'set cmax 300'  
  'd t'
  'run cbarn.gs'
  'draw title d t'
  endif
  if (fig=2)  
*  'set cmin 270'
*  'set cmax 300'  
#  'd if(u,>,0,t,-2.56E33)'
  'd if(u,>,0,t,-u)'
  'run cbarn.gs'
  'draw title if(u,>,0,t,-u)'
* note that -2.56E33 is the missing value of the first argument "u"
  endif
endif
*******************************muadv, mvadv, mwadv******************
* d muadv(u,T):   calc -u*dT/dx, where dx has the unit of m.
* d mvadv(v,T):   calc -v*dT/dy, where dy has the unit of m
* d mwadv(w,T):   calc -w*dT/dz, where dz has the same unit as level in its .ctl file
* muadv and mvadv are calculated according to 4-level Bott's (1989b) scheme 
* ref: Bott, A., 1989a: A positive definite advection schme obtained by nonlinear renormalization
*        of the advection flux. Monthly Weather Review, 117:1006-1015. 
*      Bott, A., 1989b: Notes and correspondance. Monthly Weather Review, 117:2633-2636.
* mwadv is calculated according to upwind finite difference scheme

if (page=4)
  if (fig=1)  
  'd muadv(u,t)'
  'run cbarn.gs'
  'draw title -u*dT/dx (K s`a-1`n)'
  endif
  if (fig=2)  
  'd mvadv(v,t)'
  'run cbarn.gs'
  'draw title -v*dT/dy (K s`a-1`n)'
  endif
endif
*******************************cosz******************
* d cosz(exp,time_scale):   display cosine zenith angle
* time_scal can be "h": hourly
*                  "d": daily mean
*                  "m": monthly mean
*
if (page=5)
  if (fig=1)  
  'd cosz(lat,h)'
  'run cbarn.gs'
  'draw title hourly cosine zenith angle'
  endif
  if (fig=2)  
  'd cosz(lat,d)'
  'run cbarn.gs'
  'draw title daily mean cosine zenith angle'
  endif
endif
*******************************cosz, lt******************
* d cosz(exp,time_scale):   display cosine zenith angle
* d lt(exp):   diaplay local time in hour
*
if (page=6)
  if (fig=1)  
  'd cosz(lat,m)'
  'run cbarn.gs'
  'draw title monthly mean cosine zenith angle'
  endif
  if (fig=2)  
  'd lt(lat)'
  'run cbarn.gs'
  'draw title local time (h)\d lt(lat)'
  endif
endif
****************************** jd, dayratio ******************
* d jd(exp):   display julian date from 1 January in year 1
* d dayratio(exp):  display daytime ratio in a day
*
if (page=7)
  if (fig=1)  
  'd jd(lat)'
  'run cbarn.gs'
  'draw title julian date from 1 January 0001 (jd=1)\d jd(lat)'
  endif
  if (fig=2)  
  'd dayratio(lat)'
  'run cbarn.gs'
  'draw title daytime ratiod dayratio(lat)'
  endif
endif
****************************** dew, which ******************
* d dew(vap):   where vap : vapor pressure in Pa, dew: dew point in K
*
* d which(exp,con1,exp1,con2,exp2, ..., else_exp):
* If exp == cond1, exp1 is displayed.
* else if exp == cond2, exp2 is displayed
* else if .....................
* else else_exp is displayed.
*
if (page=8)
  if (fig=1)  
  'd dew(q/0.622*lev*100)'
  'run cbarn.gs'
  'draw title dew point (K)\d dew(q/0.622*lev*100)'
  endif
  if (fig=2)
  'set clevs 0 1 2'  
  'd which(minv(maxv(u,0),10),0,0,10,2,1)'
*  'd which(minv(u,10),0,0,10,2,1)'
  'run cbarn.gs'
  'draw title u<=0: 0; 0<u<10: 1; u>=10: 2\d which(minv(u,10),0,0,10,2,1)'
  endif
endif
****************************** line, which ******************
* d dew(vap):   where vap : vapor pressure in Pa, dew: dew point in K

****************************** satvap, ??? ******************
* d satvap(T):   where T: temperature in K, satvap : saturated vapor pressure in Pa, 
*   'd satvap(t)'
*
if (page=9)
  if (fig=1)  
  'd satvap(t)'
  'run cbarn.gs'
  'draw title saturated temperature (Pa)'
  endif
  if (fig=2)
  'set clevs 0 1 2'  
  'd which(minv(maxv(u,0),10),0,0,10,2,1)'
*  'd which(minv(u,10),0,0,10,2,1)'
  'run cbarn.gs'
  'draw title u<=0: 0; 0<u<10: 1; u>=10: 2'
  endif
endif

if (page=10)
  if (fig=1)  
  'd line(u,30,60,200,-40,-r)'
  'run cbarn.gs'
  'draw title draw line\d line(u,lon0,lat0,lon1,lat1,-r)'
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

say "All done"

******************************
*'disable print'
*'quit'


*'print'

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
