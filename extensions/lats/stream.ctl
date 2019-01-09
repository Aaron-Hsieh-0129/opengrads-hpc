dset ^output/stream.bin
title "Sample Model Data for lats4d Tutorial"
undef 1.0e+20
options big_endian
xdef 72 linear 0.000000 5.000000
ydef 46 linear -90.000000 4.000000
zdef 7 levels
1000 850 700 500 300 200 100 
tdef 5 linear 0Z1jan1987 1dy
vars 8
ps        0   0  Surface pressure [hPa]
ts        0   0  Surface (2m) air temperature [K]
pr        0   0  Total precipitation rate [kg/(m^2*s)]
ua        7   0  Eastward wind [m/s]
va        7   0  Northward wind [m/s]
zg        7   0  Geopotential height [m]
ta        7   0  Air Temperature [K]
hus       7   0  Specific humidity [kg/kg]
endvars
