*
* Demonstrates the calculation of streamfunction and velocity
* potential using the following OpenGrADS extensions:
*
*     - libbjt (advection functions)
*     - fish (poison solver)
*
* It also computes rotationl/divergent wind components.
*
* This script is in the public domain.
*

function utfish(lev)

  say 'utFish: openning NCEP-R2 OPeNDAP monthly mean dataset'
  url = 'http://nomad2.ncep.noaa.gov:9090/dods/reanalyses/reanalysis-2/month/pgb/pgb'

  if ( lev='' )
       lev = 200
  endif

* NCEP Reanalysis 2
* -----------------
 'sdfopen ' url
 'set lev ' lev
 'set time jan1980'

* Define these to minimize OPeNDAP access
* --------------------------------------- 
  say 'utFish: reading u-winds'
  'u = ugrdprs'
  say 'utFish: reading v-winds'
  'v = vgrdprs'
   say 'utFish: Done reading winds'
   
* Auxiliaty fields
* ----------------
  'one    = 1 + 0 * lat '

*  Solve poisson equation
*  ----------------------
  'psi = fish_psi(u,v)'
  'chi = fish_chi(u,v)'

*  Standardize arbritary constant by forcing psi/chi
*  to have zero global mean
*  -------------------------------------------------
  'psi = psi - aave(psi,global)'
  'chi = chi - aave(chi,global)'

* Rotational and divergent winds
* ------------------------------
  'upsi =   mvadv(one,psi)'
  'vpsi = - muadv(one,psi)'
  'uchi = - muadv(one,chi)'
  'vchi = - mvadv(one,chi)'

* Plotting
* --------
  'set gxout shaded'
  'display psi/1e7'
  'set gxout stream'
  'display upsi;vpsi'
  'draw title NCEP Reanalysis 2 - January 1980 \ Streamfunction/Rotational Wind - 'lev' hPa'
  'cbarn'
  'printim psi-1.png'
  pause()
  'clear'

  'display chi/1e6'
  'set gxout vector'
  'set cmin  2'
  'set cmax 20'
  'display skip(uchi,3,3);vchi'
  'draw title NCEP Reanalysis 2 - January 1980 \ Velocity Potential/Divergent Wind - 'lev' hPa'
  'cbarn'
  'printim chi-1.png'

return

function pause(msg)
*  say msg
  say 'Hit <CR> to continue...'; pull ans; 'clear'
return
