'load udxt gsf.udxt'
'reinit'
'sdfopen http://nomads.ncep.noaa.gov:9090/dods/gfs_hd/gfs_hd20090331/gfs_hd_18z'
'set t 2'

nordic();
colors()

dew()  
chill()
ept()
pi()
lcl()
lls()
srh1()
srh3()
lls()
dls()
tsindex()

return

* NORDIC dewpoint -----------------------------------------------------------
function dew()
'set gxout shaded'
'set clevs -30 -28 -26 -24 -22 -20 -18 -16 -14 -12 -10  -8  -6  -4  -2   0   2   4   6   8  10  12  14  16  18  20  22  24  26  28  30  32  34  '
'set ccols  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54'
'd dewpt(tmp2m,rh2m)-273.16'
'draw title Dew Point Temperature at 2m [C]'
'cbarn'
'gxyat -x 400 -y 300 dewpt.png'
return

* NORDIC wind chill -----------------------------------------------------------
function chill()
'clear'
'set gxout shaded'
'set clevs -45 -42 -39 -36 -33 -30 -27 -24 -21 -18 -15 -12 -9 -6 -3 -1 1 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48 51'
'set ccols  22  23  24  25  26  27  28  29  30  31  32  33  34  35 36 38 0 40  42  44  45  46  47  48  49  50  51  52  53  54 55 56 57 58 59'
'd wchill(tmp2m,ugrd10m,vgrd10m)-273.16'
'draw title Wind Chill [C]'
'cbarn'
'gxyat -x 400 -y 300 chill.png'
return

* NORDIC EPT 850 hPa -------------------------------------------------------
function ept()
'clear'
'set gxout shaded'
'set clevs  -9  -6  -3   0   3   6   9  12  15  18  21  24  27  30  33  36  39  42  45  48  51  54  57  60  63  66  69  72  75  78  81    '
'set ccols  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54'
'd ept(tmpprs(lev=850),rhprs(lev=850),850)-273.16'
'cbarn'
'draw title Equivalent Potential Temperature at 850 hPa [K]'
'gxyat -x 400 -y 300 ept850.png'
return

* NORDIC PI -----------------------------------------------------------------
function pi()
'clear'
'set gxout shaded'
'set clevs -30 -28 -26 -24 -22 -20 -18 -16 -14 -12 -10  -8  -6  -4  -2   0   2   4   6   8  10  12  14  16  18  20  22  24  26  28  30  32  34  '
'set ccols  54  53  52  51  50  49  48  47  46  45  44  43  42  41  40  39  38  37  36  35  34  33  32  31  30  29  28  27  26  25  24  23  22 99'
'set cint 1'
'd epi(tmpprs(lev=850),rhprs(lev=850),tmpprs(lev=500),rhprs(lev=500))'
'cbarn'
'draw title Potential Instability [K]'
'gxyat -x 400 -y 300 epi.png'
return

* NORDIC LCL -----------------------------------------------------------------
function lcl()
'clear'
'set gxout shaded'
'set clevs 500  520  540  560  580  600  620  640  660  680  700 720  740  760  780  800  820  840  860  880  900  920 940  960  980 1000 1020'
'set ccols  54  53  52  51  50  49  48  47  46  45  44  43  42  41  40  39  38  37  36  35  34  33  32  31  30  29  28  27  26  25  24  23  22 99'
'd plcl(tmp2m,rh2m,pressfc/100)'
'cbarn'
'draw title Lifiting Condensation Level [hPa]'
'gxyat -x 400 -y 300 plcl.png'
return

* NORDIC Total Totals Index -------------------------------------------------
function tti()
'clear'
'set gxout shaded'
'set clevs 0 5 10 15 20 25 30 32 34 36 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 58 60 64'
'set ccols  54  53  52  51  50  49  48  47  46  45  44  43  42  41  40  39  38  37  36  35  34  33  32  31  30  29  28  27  26  25  24  23  22 99'
'd ttindex(tmpprs(lev=850),rhprs(lev=850),tmpprs(lev=500))'
'cbarn'
'draw title Total Totals Index'
'gxyat -x 400 -y 300 tti.png'
return

* NORDIC  Storm Relative Helicity ---------------------------------------------
function srh1()
'clear'
'set gxout shaded'
'set clevs -200 -150 -100 100 150 200 250 300 350 400 450 500 550 650 700 750 800 850 900 950 1000'
'set ccols  30 34 36 0 38 40 42 44 46 47 48 49 50 51 52 53 54 55 56 57 58 59'
'd srh1km(ugrdprs,vgrdprs)'
'cbarn'
'draw title Storm Relative Helicity 0-1km'
'gxyat -x 400 -y 300 srh1.png'
return

* NORDIC  Storm Relative Helicity ---------------------------------------------
function srh3()
'clear'
'set gxout shaded'
'set clevs -200 -150 -100 100 150 200 250 300 350 400 450 500 550 650 700 750 800 850 900 950 1000'
'set ccols  30 34 36 0 38 40 42 44 46 47 48 49 50 51 52 53 54 55 56 57 58 59'
'd srh3km(ugrdprs,vgrdprs)'
'cbarn'
'draw title Storm Relative Helicity 0-3km'
'gxyat -x 400 -y 300 srh3.png'
return

* NORDIC Low Level Shear -------------------------------------------------------
function lls()
'clear'
'set gxout shaded'
'set clevs  0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 22 24 26 28 30'
'set ccols  22 24  26  28 30  32  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54'
'set cint 1'
'd shear(ugrdprs(lev=900),vgrdprs(lev=900),ugrd10m,vgrd10m)'
'cbarn'
'draw title Low Level Wind Shear: 900 hPa - 10m'
'gxyat -x 400 -y 300 lls.png'
return


* NORDIC Schlenczek Index ----------------------------------------------------
function tsindex()
'clear'
'set gxout shaded'
'set clevs    -3.0 -2.5 -2.0 -1.5 -1.0 -0.5 0.0 0.5 1.0 1.5 2.0 2.5 3.0'
'set ccols 99   31   32   33   34   38  39   40  41  45  46  47  48  52  54  48  49'
'd tsindex(cape180_0mb,cin180_0mb)'
'cbarn'
'draw title Likelihood of T-Storm Initiation'
'gxyat -x 400 -y 300 tsi.png'
return


* NORDIC Deep Layer Shear -------------------------------------------------------
function dls()
'clear'
'set gxout shaded'
'set clevs  0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50'
'set ccols  22 24  26  28  30 32 34 36  38  40  42 43 44 45 46 47 48 49 50  51  52  53  54 55 56 57 58 '
'd shear(ugrdprs(lev=500),vgrdprs(lev=500),ugrd10m,vgrd10m)'
'cbarn'
'draw title Deep Layer Wind Shear: 500 hPa - 10m'
'gxyat -x 400 -y 300 dls.png'
return

function nordic
 'clear'
 'set grads off'
 'set mproj nps'
 'set mpvals -10 51 48 80'
 'set lon -60 100'
 'set lat 43 90'
 'set mpdset hires'
return

function colors()

'set rgb 21  50   0  50'
'set rgb 22 100   0 100'
'set rgb 23 150   0 150'
'set rgb 24 200   0 200'
'set rgb 25 250   0 254'
'set rgb 26 200   0 254'
'set rgb 27 150   0 254'
'set rgb 28 100   0 254'
'set rgb 29  50   0 254'
'set rgb 30   0  50 254'
'set rgb 31   0 100 254'
'set rgb 32   0 150 254'
'set rgb 33   0 200 254'
'set rgb 34   0 230 240'
'set rgb 35   0 230 160'
'set rgb 36   0 230 120'
'set rgb 37   0 230  80'
'set rgb 38   0 240  40'
'set rgb 39   0 250   0'
'set rgb 40 254 254   0'
'set rgb 41 254 225   0'
'set rgb 42 254 200   0'
'set rgb 43 254 175   0'
'set rgb 44 254 150   0'
'set rgb 45 230 125   0'
'set rgb 46 230 100   0'
'set rgb 47 220  75  30'
'set rgb 48 200  50  30'
'set rgb 49 180  25  30'
'set rgb 50 170   0  30'
'set rgb 51 180   0  50'
'set rgb 52 200   0 100'
'set rgb 53 254   0 150'
'set rgb 54 254   0 200'
'set rgb 55 254   0 254'
'set rgb 56 200   0 200'
'set rgb 57 150   0 150'
'set rgb 58 100   0 100'
'set rgb 59  75   0  75'
'set rgb 97 128 128 128'
'set rgb 98   0   0   0'
'set rgb 99 255 255 255'
*'set background 99'
*'set line 98'
return
