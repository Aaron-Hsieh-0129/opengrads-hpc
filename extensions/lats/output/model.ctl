dset ^model.grb
title File
undef 1e+20
dtype grib
index ^model.gmp
xdef 72 linear 0.000000 5.000000
ydef 46 linear -90.000000 4.000000
zdef 7 levels
1000 850 700 500 300 200 100 
tdef 5 linear 0Z1jan1987  24hr
vars 8
ps        0    1,  1,  0,  0 Surface pressure [hPa] []
ts        0    2,  1,  0,  0 Surface (2m) air temperature [K] []
pr        0    3,  1,  0,  0 Total precipitation rate [kg/(m^2*s)] []
ua        7    4,100 Eastward wind [m/s] []
va        7    5,100 Northward wind [m/s] []
zg        7    6,100 Geopotential height [m] []
ta        7    7,100 Air Temperature [K] []
hus       7    8,100 Specific humidity [kg/kg] []
endvars
