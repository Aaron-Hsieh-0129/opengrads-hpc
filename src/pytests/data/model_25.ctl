dset ^model_25.grb2
index ^model_25.idx
undef 9.999E+20
title model_25.grb2
*  produced by g2ctl v0.0.3d
* griddef=1:0:(144 x 73):grid_template=0: lat-lon grid:(144 x 73) units 1e-06 input WE:NS output WE:SN res 48 lat 90.000000 to -90.000000 by 2.500000 lon 0.000000 to 357.500000 by 2.500000 #points=10512

dtype grib2
ydef 73 linear -90.000000 2.5
xdef 144 linear 0.000000 2.500000
tdef 4 linear 12Z13feb2008 1dy
*  z has 3 levels -prs
zdef 3 levels 85000 50000 20000
options pascals
vars 7
APCPsfc  0,1,0   0,1,8,1 ** surface acc Total Precipitation [kg/m^2]
HGTprs   3,100  0,3,5 ** (850 500 200) none Geopotential Height [gpm]
PRMSLmsl  0,101,0   0,3,1 ** mean sea level none Pressure Reduced to MSL [Pa]
RHprs   3,100  0,1,1 ** (850 500 200) none Relative Humidity [%]
TMPprs   3,100  0,0,0 ** (850 500 200) none Temperature [K]
UGRDprs   3,100  0,2,2 ** (850 500 200) none U-Component of Wind [m/s]
VGRDprs   3,100  0,2,3 ** (850 500 200) none V-Component of Wind [m/s]
ENDVARS
