*
* Demonstrates the calculation of streamfunction and velocity
* potential using the following OpenGrADS extensions:
*
*     - libbjt (advection functions)
*     - fish (poison solver)
*
* This version computes vorticity/divergence using the GrADS intrinsic 
* function hcurl() and hdivg().
*
* This script is in the public domain.
*

function utfish3(lev)

  if ( lev='' )
       lev = 300
  endif

* NCEP R2
* -------
  say 'utFish3: openning NCEP-R2 OPeNDAP monthly mean dataset'
  url = 'http://nomad2.ncep.noaa.gov:9090/dods/reanalyses/reanalysis-2/month/pgb/pgb'

  'sdfopen ' url
  'set time jul1982'
  'set lev ' lev
  'set gxout shaded'

* Define these to minimize OPeNDAP access
* --------------------------------------- 
  say 'utFish3: reading u-winds'
  'u = ugrdprs'
  say 'utFish3: reading v-winds'
  'v = vgrdprs'
  say 'utFish3: reading heights'
  'h = hgtprs'
   say 'utFish3: Done reading winds'

* Vorticity and divervenge: enlarge x-domain to avoid grid undefs
* ---------------------------------------------------------------
  'vort = hcurl(u,v)'
  'div  = hdivg(u,v)'

* Display vorticity/divergence
* ----------------------------
  'd vort'; 'draw title Vorticity'
  clear()
  'd div';  'draw title Divergence'
  clear()
  
* Stream function
* ---------------
  'psi = fish(vort)'
  'd psi'
  'set gxout contour'
  'd h'
  'draw title Stream Function and Geopotential Height'
  'printim psi-3.png'
  clear()

* Velocity Potential
* ------------------
  'chi = fish(div)'
  'set gxout shaded'
  'd chi'
  'draw title Velocity Potential'
  'printim chi-3.png'

return

function clear()
  say 'Hit <CR> to continue...'; pull ans; 'clear'
return

