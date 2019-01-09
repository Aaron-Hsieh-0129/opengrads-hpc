#!/bin/csh -f


# Make a local file
# -----------------
  rm -rf output
  mkdir -p output
  lats4d.sh -v -shave -ntimes 8 -vars tskin \
          -i http://opendap.gsfc.nasa.gov:9090/dods/GEOS-5/fp/0.5_deg/assim/inst2d_met_x \
         -o output/tskin 


exit

#  Created masked files
#  --------------------
   set birds = (aqua terra aura cloudsat calipso)
   set terra = (modis_terra misr mopitt aster)
   set aqua = (modis_aqua airs amsu_a hsb amsr_e)
   set aura = (hirdls omi tes)
   set eos = ( $birds $terra $aqua $aura )

   foreach sat ( $eos )
     lats4d.sh -v -shave -i output/tskin.nc4 -mask $sat -o output/$sat
   end
