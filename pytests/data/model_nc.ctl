dset ^model.nc
title CTL for model.nc
undef 1e+20
dtype netcdf
xdef 72 linear 0 5
ydef 46 linear -90 4
zdef 7 levels 1000 850 700 500 300 200 100
tdef 5 linear 00Z01JAN1987 1440mn
vars 8
ps 0 t,y,x Surface pressure [hPa]
ts 0 t,y,x Surface (2m) air temperature [K]
pr 0 t,y,x Total precipitation rate [kg/(m^2*s)]
ua 7 t,z,y,x Eastward wind [m/s]
va 7 t,z,y,x Northward wind [m/s]
zg 7 t,z,y,x Geopotential height [m]
ta 7 t,z,y,x Air Temperature [K]
hus 7 t,z,y,x Specific humidity [kg/kg]
endvars
