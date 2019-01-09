*
* Demonstrates the calculation of streamfunction and velocity
* potential using the following OpenGrADS extensions:
*
*     - libbjt (advection functions)
*     - fish (poison solver)
*
* This version does not use fish_psi()/fish_chi but does the calculation
* more explicitly using libbjt. 
*
* This script is in the public domain.

function utfish2(lev)

  if ( lev='' )
       lev = 700
  endif

* NCEP R2
* -------
  say 'utFish2: openning NCEP-R2 OPeNDAP monthly mean dataset'
  url = 'http://nomad2.ncep.noaa.gov:9090/dods/reanalyses/reanalysis-2/month/pgb/pgb'
  'sdfopen ' url
  'set time jul1982'
  'set lev ' lev

* Define these to minimize OPeNDAP access
* --------------------------------------- 
  say 'utFish2: reading u-winds'
  'u = ugrdprs'
  say 'utFish2: reading v-winds'
  'v = vgrdprs'
   say 'utFish2: Done reading winds'
   
* Auxiliaty fields
* ----------------
  'one    = 1 + 0 * lat '
  'cosphi = cos(0.0174532925199433 * lat)'

* B.-J.'s advection functions: intrinsic functions hcurl/hdivg
* do not handle boundary conditions properly
* ------------------------------------------------------------
  'vor = - ( madvu(v,one) - madvv(u,cosphi) / cosphi )'
  'div = - ( madvu(u,one) + madvv(v,cosphi) / cosphi )'

*  Solve poisson equation
*  ----------------------
  'psi = fish(vor) '
  'chi = fish(div) '

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
  'draw title Streamfunction/Rotational Wind - 'lev' hPa'
  'cbarn'
  'printim psi-2.png'
  pause()

  'display chi/1e6'
  'set gxout vector'
  'set cmin  2'
  'set cmax 20'
  'display skip(uchi,3,3);vchi'
  'draw title Velocity Potential/Divergent Wind - 'lev' hPa'
  'cbarn'
  'printim chi-2.png'

return

function pause(msg)
*  say msg
  say 'Hit <CR> to continue...'; pull ans; 'clear'
return
