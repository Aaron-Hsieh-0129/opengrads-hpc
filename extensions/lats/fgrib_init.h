#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include "fgrib.h" 

/* static SITE_ID ; */

static user_var FGRIBAPI_vr = {

 "test",  /*           char name[20] ; */
 "test",  /*           char li[12] ;   */
      0,  /*            int lv;        */
      0,  /*  unsigned char cent;      */
      0,  /*  unsigned char yr;        */
      0,  /*  unsigned char mo;        */
      0,  /*  unsigned char da;        */
      0,  /*  unsigned char hr;        */
      0,  /* unsigned char mn;         */
"test",   /*           char ti[12];    */
      1,  /*            int tv;        */
#ifdef __GO32__  /* DJGPP: FLT_MAX is not a constant */
3.40282e+38  /*  float undef;       */
#else
FLT_MAX      /*  float undef;       */
#endif /* __GO32__ */
} ;

static user_grid FGRIBAPI_gd  = {

   -999,  /*          int ni */
   -999,  /*          int nj */
   -999,  /*        float dx */
   -999,  /*        float dy */
   -999,  /*        float latb */
   -999,  /*        float late */
   -999,  /*        float lonb */
   -999,  /*        float lone */
 "test",  /*           char gi[12] */
      0,  /*  unsigned char drt */
   NULL,  /*        float *lat */
   NULL   /*        float *lon */
} ;


static grib_var_table FGRIBAPI_vtpcmdi[] = {

  {   1 , "p"       , "Pressure"                                 , "Pa"        , -1    , -999 },
  {   2 , "psl"     , "Pressure reduced to MSL"                  , "Pa"        , -1    , -999 },
  {   3 , "ptend"   , "Pressure tendency"                        , "Pa/s"      , -5    , -999 },
  {   6 , "gp"      , "Geopotential"                             , "m^2/s^2"   , -1    , -999 },
  {   7 , "zg"      , "Geopotential height"                      , "m"         ,  0    , -999 },
  {   8 , "zt"      , "Geometric height"                         , "m"         ,  0    , -999 },
  {   9 , "hstdv"   , "Standard deviation of height"             , "m"         ,  0    , -999 },
  {  10 , "hvar"    , "Variance of height"                       , "m^2"       , -999  ,   16 },
  {  11 , "ta"      , "Temperature"                              , "degK"      ,  1    , -999 },
  {  12 , "tav"     , "Virtual temperature"                      , "degK"      ,  1    , -999 },
  {  13 , "pot"     , "Potential temperature"                    , "degK"      ,  1    , -999 },
  {  14 , "epot"    , "Pseudo-adiabatic potential temperature"   , "degK"      ,  1    , -999 },
  {  15 , "tmax"    , "Maximum temperature"                      , "degK"      ,  1    , -999 },
  {  16 , "tmin"    , "Minimum temperature"                      , "degK"      ,  1    , -999 },
  {  17 , "td"      , "Dew point temperature"                    , "degK"      ,  1    , -999 },
  {  18 , "tdd"     , "Dew point depression"                     , "degK"      ,  1    , -999 },
  {  19 , "dtdz"    , "Lapse rate"                               , "K/m"       ,  4    , -999 },
  {  20 , "vis"     , "Visibility"                               , "m"         ,  0    , -999 },
  {  25 , "tanom"   , "Temperature anomaly"                      , "degK"      ,  1    , -999 },
  {  26 , "panom"   , "Pressure anomaly"                         , "Pa"        , -1    , -999 },
  {  27 , "zga"     , "Geopotential height anomaly"              , "gpm"       ,  0    , -999 },
  {  31 , "dira"    , "Wind direction"                           , "deg"       ,  0    , -999 },
  {  32 , "spda"    , "Wind speed"                               , "m/s"       ,  1    , -999 },
  {  33 , "ua"      , "u wind"                                   , "m/s"       ,  1    , -999 },
  {  34 , "va"      , "v wind"                                   , "m/s"       ,  1    , -999 },
  {  35 , "strf"    , "Stream function"                          , "m^2/s"     , -4    , -999 },
  {  36 , "vpot"    , "Velocity potential"                       , "m^2/s"     , -4    , -999 },
  {  37 , "stfm"    , "Montgomery stream function"               , "m^2/s^2"   , -1    , -999 },
  {  38 , "was"     , "Sigma coord. vertical velocity"           , "/s"        ,  8    , -999 },
  {  39 , "wa"      , "Pressure vertical velocity"               , "Pa/s"      ,  3    , -999 },
  {  40 , "dzdt"    , "Geometric vertical velocity"              , "m/s"       ,  4    , -999 },
  {  41 , "vrta"    , "Absolute vorticity"                       , "/s"        ,  6    , -999 },
  {  42 , "div"     , "Absolute divergence"                      , "/s"        ,  6    , -999 },
  {  43 , "vrt"     , "Relative vorticity"                       , "/s"        ,  6    , -999 },
  {  44 , "divr"    , "Relative divergence"                      , "/s"        ,  6    , -999 },
  {  45 , "dudz"    , "Vertical u shear"                         , "/s"        ,  4    , -999 },
  {  46 , "dvdz"    , "Vertical v shear"                         , "/s"        ,  4    , -999 },
  {  47 , "diruos"  , "Direction of current"                     , "deg"       ,  0    , -999 },
  {  48 , "spduos"  , "Speed of current"                         , "m/s"       ,  2    , -999 },
  {  49 , "uos"     , "u of current"                             , "m/s"       ,  2    , -999 },
  {  50 , "vos"     , "v of current"                             , "m/s"       ,  2    , -999 },
  {  51 , "hus"     , "Specific humidity"                        , "kg/kg"     ,  4    , -999 },
  {  52 , "hur"     , "Relative humidity"                        , "%"         ,  0    , -999 },
  {  53 , "hum"     , "Humidity mixing ratio"                    , "kg/kg"     ,  4    , -999 },
  {  54 , "prw"     , "Precipitable water"                       , "kg/m^2"    ,  1    , -999 },
  {  55 , "huv"     , "Vapor pressure"                           , "Pa"        , -1    , -999 },
  {  56 , "satd"    , "Saturation deficit"                       , "Pa"        , -1    , -999 },
  {  57 , "evp"     , "Evaporation"                              , "kg/m^2"    ,  1    , -999 },
  {  58 , "cice"    , "Cloud Ice"                                , "kg/m^2"    ,  1    , -999 },
  {  59 , "prr"     , "Precipitation rate"                       , "kg/m^2/s"  ,  6    , -999 },
  {  60 , "tstm"    , "Thunderstorm probability"                 , "%"         ,  0    , -999 },
  {  61 , "pra"     , "Total precipitation"                      , "kg/m^2"    ,  1    , -999 },
  {  62 , "prla"    , "Large scale precipitation"                , "kg/m^2"    ,  1    , -999 },
  {  63 , "prla"    , "Convective precipitation"                 , "kg/m^2"    ,  1    , -999 },
  {  64 , "srweq"   , "Snowfall rate water equivalent"           , "kg/m^2/s"  ,  6    , -999 },
  {  65 , "weasd"   , "Water equiv. of accum. snow depth"        , "kg/m^2"    ,  0    , -999 },
  {  66 , "snod"    , "Snow depth"                               , "m"         ,  2    , -999 },
  {  67 , "mixht"   , "Mixed layer depth"                        , "m"         , -999  ,   12 },
  {  68 , "tthdp"   , "Transient thermocline depth"              , "m"         , -999  ,   12 },
  {  69 , "mthd"    , "Main thermocline depth"                   , "m"         , -999  ,   12 },
  {  70 , "mtha"    , "Main thermocline anomaly"                 , "m"         , -999  ,   12 },
  {  71 , "clt"     , "Total cloud cover"                        , "%"         ,  0    , -999 },
  {  72 , "cdcon"   , "Convective cloud cover"                   , "%"         ,  0    , -999 },
  {  73 , "cll"     , "Low level cloud cover"                    , "%"         ,  0    , -999 },
  {  74 , "clm"     , "Mid level cloud cover"                    , "%"         ,  0    , -999 },
  {  75 , "clh"     , "High level cloud cover"                   , "%"         ,  0    , -999 },
  {  76 , "cwat"    , "Cloud water"                              , "kg/m^2"    ,  1    , -999 },
  {  78 , "prsc"    , "Convective snow"                          , "kg/m^2"    ,  1    , -999 },
  {  79 , "prsl"    , "Large scale snow"                         , "kg/m^2"    ,  1    , -999 },
  {  80 , "sst"     , "Water temperature"                        , "degK"      ,  1    , -999 },
  {  81 , "land"    , "Land-sea mask (1=land; 0=sea)"            , "int"       ,  0    , -999 },
  {  82 , "dslm"    , "Deviation of sea level from mean"         , "m"         , -999  ,   12 },
  {  83 , "sfcr"    , "Surface roughness"                        , "m"         ,  5    , -999 },
  {  84 , "alb"     , "Albedo"                                   , "%"         ,  0    , -999 },
  {  85 , "tsoil"   , "Soil temperature"                         , "degK"      ,  1    , -999 },
  {  86 , "mrso"    , "Soil moisture content (mm)"               , "kg/m^2"    ,  0    , -999 },
  {  87 , "veg"     , "Vegetation"                               , "%"         ,  0    , -999 },
  {  88 , "salty"   , "Salinity"                                 , "kg/kg"     , -999  ,   12 },
  {  89 , "den"     , "Density"                                  , "kg/m^2"    ,  4    , -999 },
  {  90 , "mrroa"   , "Runoff accum (mm)"                        , "kg/m^2"    ,  1    , -999 },
  {  91 , "simask"  , "Ice concentration (ice=1; no ice=0)"      , "1/0"       , -999  ,   12 },
  {  92 , "sit"     , "Ice thickness"                            , "m"         ,  0    , -999 },
  {  93 , "dirid"   , "Direction of ice drift"                   , "deg"       , -999  ,   12 },
  {  94 , "spdid"   , "Speed of ice drift"                       , "m/s"       , -999  , -999 },
  {  95 , "ui"      , "u of ice drift"                           , "m/s"       , -999  , -999 },
  {  96 , "vi"      , "v of ice drift"                           , "m/s"       , -999  , -999 },
  {  97 , "igr"     , "Ice growth"                               , "m"         , -999  , -999 },
  {  98 , "idv"     , "Ice divergence"                           , "/s"        , -999  , -999 },
  {  99 , "snom"    , "Snow melt"                                , "kg/m^2"    ,  1    , -999 },
  { 100 , "htsgw"   , "Sig height of wind waves and swell"       , "m"         , -999  ,   12 },
  { 101 , "wvdir"   , "Direction of wind waves"                  , "deg"       ,  0    , -999 },
  { 102 , "wvhgt"   , "Significant height of wind waves"         , "m"         , -999  ,   12 },
  { 103 , "wvper"   , "Mean period of wind waves"                , "s"         , -999  ,   12 },
  { 104 , "swdir"   , "Direction of swell waves"                 , "deg"       ,  0    , -999 },
  { 105 , "swell"   , "Significant height of swell waves"        , "m"         , -999  ,   12 },
  { 106 , "swper"   , "Mean period of swell waves"               , "s"         , -999  ,   12 },
  { 107 , "dirpw"   , "Primary wave direction"                   , "deg"       ,  0    , -999 },
  { 108 , "perpw"   , "Primary wave mean period"                 , "s"         , -999  ,   12 },
  { 109 , "dirsw"   , "Secondary wave direction"                 , "deg"       ,  0    , -999 },
  { 110 , "persw"   , "Secondary wave mean period"               , "s"         , -999  ,   12 },
  { 111 , "rss"     , "Net short wave radiation (surface)"       , "W/m^2"     ,  0    , -999 },
  { 112 , "rls"     , "Net long wave radiation (surface)"        , "W/m^2"     ,  0    , -999 },
  { 113 , "rst"     , "Net short wave radiation (top)"           , "W/m^2"     ,  0    , -999 },
  { 114 , "rlt"     , "Net LW radiation (top) OLR"               , "W/m^2"     ,  0    , -999 },
  { 115 , "rl"      , "Long wave radiation"                      , "W/m^2"     ,  0    , -999 },
  { 116 , "rs"      , "Short wave radiation"                     , "W/m^2"     ,  0    , -999 },
  { 117 , "rglb"    , "Global radiation"                         , "W/m^2"     ,  0    , -999 },
  { 121 , "hfl"     , "Latent heat flux"                         , "W/m^2"     ,  0    , -999 },
  { 122 , "hfs"     , "Sensible heat flux"                       , "W/m^2"     ,  0    , -999 },
  { 123 , "hfbld"   , "Boundary layer dissipation"               , "W/m^2"     ,  0    , -999 },
  { 124 , "tauu"    , "Zonal component of momentum flux"         , "N/m^2"     ,  3    , -999 },
  { 125 , "tauv"    , "Meridional component of momentum flux"    , "N/m^2"     ,  3    , -999 },
  { 126 , "wmixe"   , "Wind mixing energy"                       , "J"         , -999  ,   12 },
  { 127 , "pix"     , "Image data"                               , "int"       , -999  ,   12 },

/*-------- amip 2 specific ------------ */

  { 128 , "tg"      , "Ground temperature"                       , "degK"      ,    1  ,   12 },
  { 129 , "pr"      , "Total precipitation"                      , "mm/day"    ,    2  , -999 },
  { 130 , "ps"      , "Surface pressure"                         , "Pa"        ,    0  , -999 },
  { 131 , "sic"     , "Sea-ice concentration"                    , "%"         ,    0  , -999 },
  { 133 , "mrss"    , "Surface soil moisture"                    , "mm"        ,    1  , -999 },    
  { 134 , "mrst"    , "Total  soil moisture"                     , "mm"        ,    1  , -999 },    
  { 135 , "mrsb"    , "Surface soil moisture beta (%FC)"         , "%"         ,    0  , -999 },    
  { 136 , "mrst"    , "Total soil moisture beta (%FC)"           , "%"         ,    0  , -999 },    
  { 137 , "mrros"   , "Surface Runoff"                           , "mm/day"    ,    2  , -999 },    
  { 138 , "mrrot"   , "Total Runoff"                             , "mm/day"    ,    2  , -999 },    
  { 139 , "tss"     , "Surface layer soil temperature"           , "degK"      ,    1  , -999 },    
  { 140 , "tsd"     , "Deep layer soil temperature"              , "degK"      ,    1  , -999 },    
  { 141 , "tst"     , "Lower boundary soil temperature"          , "degK"      ,    1  , -999 },    
  { 142 , "snm"     , "Snow depth water equivalent"              , "mm"        ,    0  , -999 },    
  { 143 , "snc"     , "Snow cover"                               , "%"         ,    0  , -999 },    
  { 144 , "prc"     , "Convective precitipation"                 , "mm/day"    ,    2  , -999 },    
  { 145 , "evs"     , "Evaporation"                              , "mm/day"    ,    2  , -999 },
  { 180 , "rsds"    , "SW radiation downward (sfc)"              , "W/m^2"     ,    0  , -999 },
  { 181 , "rsus"    , "SW radiation upward (sfc)"                , "W/m^2"     ,    0  , -999 },
  { 182 , "rsdt"    , "SW radiation downward (TOA)"              , "W/m^2"     ,    0  , -999 },
  { 183 , "rsut"    , "SW radiation upward (TOA)"                , "W/m^2"     ,    0  , -999 },
  { 184 , "rsdscs"  , "Clear sky SW radiation downward (sfc)"    , "W/m^2"     ,    0  , -999 },
  { 185 , "rsuscs"  , "Clear sky SW radiation upward (sfc)"      , "W/m^2"     ,    0  , -999 },
  { 186 , "rsutcs"  , "Clear sky SW radiation upward (TOA)"      , "W/m^2"     ,    0  , -999 },
  { 187 , "rlds"    , "LW radiation downward (sfc)"              , "W/m^2"     ,    0  , -999 },
  { 188 , "rlus"    , "LW radiation upward (sfc)"                , "W/m^2"     ,    0  , -999 },
  { 189 , "rldscs"  , "Clear sky SW radiation downward (sfc)"    , "W/m^2"     ,    0  , -999 },
  { 190 , "rluscs"  , "Clear sky SW radiation upward (sfc)"      , "W/m^2"     ,    0  , -999 },
  { 191 , "rlutcs"  , "Clear sky SW radiation upward (TOA)"      , "W/m^2"     ,    0  , -999 },
  { 200 , "cvzz"    , "Variance of geopotential height"          , "m^2"       ,  -999 ,   16 },
  { 201 , "cvuu"    , "Variance of u comp of wind"               , "m^2/s^2"   ,  -999 ,   16 },
  { 202 , "cvvv"    , "Variance of v comp of wind"               , "m^2/s^2"   ,  -999 ,   16 },
  { 203 , "cvww"    , "Variance of vertical wind velocity"       , "Pa^2/s^2"  ,  -999 ,   16 },
  { 204 , "cvtt"    , "Variance of temperature"                  , "degK^2"    ,  -999 ,   16 },
  { 205 , "cvqq"    , "Variance of specific humidity"            , "kg^2/kg^2" ,  -999 ,   16 },
  { 206 , "cvut"    , "Covariance of u and T"                    , "m*degK/s"  ,  -999 ,   16 },
  { 207 , "cvvt"    , "Covariance of v and T"                    , "m*degK/s"  ,  -999 ,   16 },
  { 208 , "cvuv"    , "Covariance of u and v"                    , "m^2/s^2"   ,  -999 ,   16 },
  { 209 , "cvwu"    , "Covariance of w and u"                    , "m*Pa/s^2"  ,  -999 ,   16 },
  { 210 , "cvwv"    , "Covariance of w and v"                    , "m*Pa/s^2"  ,  -999 ,   16 },
  { 211 , "cvuq"    , "Covariance of u and q"                    , "kg/kg*m/s" ,  -999 ,   16 },
  { 212 , "cvvq"    , "Covariance of v and q"                    , "kg/kg*m/s" ,  -999 ,   16 },
  { 213 , "cvwt"    , "Covariance of w and t"                    , "degK*Pa/s" ,  -999 ,   16 },
  { 214 , "cvwq"    , "Covariance of w and q"                    , "kg/kg*Pa/s",  -999 ,   16 },
  { 0 , "" },

} ;


static grib_grid_table FGRIBAPI_gtpcmdi[]={
    {"gg",4 },
    {"ull",0 },
    {"gll",220 },
    {""}
  };

static grib_lev_table FGRIBAPI_ltpcmdi[]={
    {"sfc",1 ,0,0,0 },         /* sfc of earth */
    {"lands",112 ,0,10,0 },    /* below ground 0 10 cm */
    {"landd",112 ,10,200,0 },  /* below ground 10 200 cm */
    {"sfc10m",105 ,0,0,10 },   /* sfc wind */
    {"sfc2m",105 ,0,0,2 },     /* sfc air temp */
    {"msl",102 ,0,0,0 },       /* mean sea level */
    {"plev",100 ,0,0,0 },      /* pressure level */
    {"toa",8,0,0,0  },         /* nominal top of the atmosphere */
    {"atm",200,0,0,0 },        /* entire atmosphere */
    {"sky_cvr",200 ,0,0,0 },   /* below ground 0 10 cm */
    {"hi_cld",234 ,0,0,0 },    /* below ground 0 10 cm */
    {"mid_cld",224 ,0,0,0 },   /* below ground 0 10 cm */
    {"low_cld",214 ,0,0,0 },   /* below ground 0 10 cm */
    {"ht_sfc",105 ,0,0,0 },    /* mean sea level */
    {"toa",8,0,0,0  },         /* nominal top of the atmosphere */
    {"atm",200,0,0,0 },        /* entire atmosphere */
    {""}        /* NULL terminator */
};

static grib_time_table FGRIBAPI_ttpcmdi[]={
    {"moave", 3 ,0,0,0,0 },         /* monthly average */
    {"6haccm", 1 ,0,6,0,4 },        /* 6h accum */
    {"6h", 1 ,0,0,0,10 },            /* 6h instantaneous */
    {"fh", 1 ,0,0,0,10 },           /* forecast hour */
    {""}        /* entire atmosphere */
};

static grib_center_table FGRIBAPI_ctpcmdi[]={
    {"NCEP", 7 },      /* NCEP T0 J:1,3 */
    {"ECMWF", 98 },    /* ECMWF T0 J:1,3*/
    {"FNMOC", 58 },    /* FNMOC T0 J:1,3  */
    {"PCMDI", 100 },   /* PCMDI (given) T0 J:1,3 */
    {"",0}            /* termlinating null */
};

static grib_subcenter_table FGRIBAPI_stpcmdi[]={
    {"AMIP1", 1 },         /* monthly mean */
    {"AMIP2", 2 },         /* monthly mean */
    {"AODS", 3 },         /* monthly mean */
    {"",0}         /* monthly mean */
};

static grib_proc_table FGRIBAPI_pt_aods_pcmdi[]={
    {"ncep_rnl", 1 },         /* monthly mean */
    {"ecmwf_rnl", 2 },         /* monthly mean */
    {"",0},         /* monthly mean */
};

static grib_proc_table FGRIBAPI_pt_amip_pcmdi[]={
    {"BMRC", 1 },      
    {"CCC", 2 },       
    {"CNRM", 3 },      
    {"COLA", 4 },      
    {"CSIRO", 5 },     
    {"CSU", 6 },       
    {"DNM", 7 },       
    {"ECMWF", 8 },     
    {"GFDL", 9 },      
    {"GFDL/DERF", 10 },
    {"GISS", 11 },     
    {"GLA", 12},       
    {"GSFC", 13},      
    {"IAP", 14},       
    {"JMA", 15},       
    {"LMD", 16},       
    {"MGO", 17},       
    {"MPI", 18},       
    {"MRI", 19},       
    {"NCAR", 20},       
    {"NCEP", 21},       
    {"NRL", 22},       
    {"RPN", 23},       
    {"SUNYA", 24},       
    {"SUNYA/NCAR", 25},       
    {"UCLA", 26},       
    {"UGAMP", 27},       
    {"UILL", 28},       
    {"UKMO", 29},       
    {"YONU", 30},       
    {""},         /* monthly mean */
};

static grib_varver_table FGRIBAPI_varver[]={
    {"PCMDI", 128 },         /* monthly mean */
    {"NCEP", 2 },         /* monthly mean */
    {"FNMOC", 3 },         /* monthly mean */
    {"",0},         /* monthly mean */
};


static grib_pds FGRIBAPI_pds = {

    NULL,  /* pds->pds = (unsigned char *)malloc( pds->len); */
      28,  /* len of PDS pds->len =*/
       0,  /* pds->ver  parameter table version # 128 for PCMDI J:1,1*/
       0,  /* pds->ctr center # 100 = PCMDI T0 J:1,3 */
       0,  /* pds->proc # 58 = NOGAPS (J:1,4) */
     255,  /* pds->grid grid id 255 means defined by GDS J:1,5-8 */
       1,  /* pds->gflg  B1 1 = GDS included 0 = not T1 J:1,26 */
       0,  /* pds->bflg  B2 1 = BMS included 0 = not T1 J:1,26 */
       0,  /* pds->parm = 0    parameter T2 J:1,27-28 */
       0,  /* pds->ltyp = 0    level type indicator T3 T3a J:1,33-36 */
       0,  /* pds->l1 = 0    level 1 T3 J:1,33-34 */
       0,  /* pds->l2 = 0    level 2 T3 J:1,33-34 */
       0,  /* pds->l12 = 0    level 1 and 2 T3 J:1,33-34 */
       0,  /* pds->yr = 0    year of century */
       0,  /* pds->mo = 0    month */
       0,  /* pds->da = 0    day */
       0,  /* pds->hr = 0    hour */
       0,  /* pds->mn = 0    min */
       0,  /* pds->ftu = 0    forecast time unit T4 J:1,36 */
       0,  /* pds->p1 = 0    period 1 */
       0,  /* pds->p2 = 0    period 2 */
       0,  /* pds->p12 = 0    period 1 and 2 */
       0,  /* pds->tri = 0    time range indicator T5 J:1,37 */
       0,  /* pds->nave = 0    number of points in the average */
       0,  /* pds->nmis = 0    number missing from average */
       0,  /* pds->cent = 20   century 20=1900 21=2000 */
       0,  /* pds->sctr       sub center T0 J:1,3-4 */
    -999,  /* pds->dsf    decimal scale factor */

} ;

static grib_bds FGRIBAPI_bds = {
  NULL, /* unsigned char *bds; */
     0, /*   unsigned int len;     length of BDS  */
     0, /*  unsigned char dt;      B1 data type 0 = grid point 1 = spec coef T11 J:4,6 */
     0, /* unsigned char pt;      B2 packing 0 = simple 1 = complex T11 J:4,6 */
     0, /*  unsigned char od;      B3 orig data 0 = floats 1 = int T11 J:4,6 */
     0, /*  unsigned char af;      B4 additional flags in octet 14 0 = no 1 = yes  T11 J:4,6 */
     0, /*  unsigned char nub;     B6-5 number of unused bits J:4,1*/
     0, /*       int bsf;     binary scale factor  J:4,1 */
    "", /*       char ref[4];  reference value float IBM format J:4,1 */
     0  /*   unsigned char nb;      # bits / grid point T11 J:4,6 */
} ;

static grib_bms FGRIBAPI_bms = {
   NULL, /* unsigned char *bms; */
      0, /* unsigned int len;     length of BMS  */
      0, /* unsigned char nub;     number of unused bits */
      0 /* unsigned int nu;      numeric = 0 of bitmap otherwise # of a predefined bitmap */
} ;

static grib_gds_ll FGRIBAPI_gds = {
  NULL, /* unsigned char *gds;
     0, /* unsigned int len;    length of GDS (32) */
     0, /* unsigned char nv;     num vert coor parameter */
     0, /* unsigned char pv;     location (octet E) of vert or num of points in each row or 255 */
     0, /* unsigned char drt;    data representation type (T6 J:2,2) */
     0, /* unsigned int ni;     num points in i */
     0, /* unsigned int nj;     num points in j */ 
     0, /*        int lat1;   lat of first point (deg*1000) */
     0, /*       int lon1;   lon of first point (deg*1000) */
     0, /* unsigned char rcdi;   B1 res comp flag -- direction increments 1 = given (T7 J:2,11) */
     0, /* unsigned char rcre;   B2 res comp flag -- earth 0=sphere r=6357.4km 1=oblate spheriod) (T7) */
     0, /* unsigned char rcuv;   B5 res comp flag -- u,v 0 = earth relative 1 = grid relative (T7) */
     0, /*     int lat2;   lat of last point (J:2,3) */
     0, /*      int lon2;   lon of last point (J:2,3) */
     0, /* unsigned int dx;     dlon, if not given all bits 1 (J:2,3) */
     0, /* unsigned int dy;     dlat if reg lon/lat grid, num of points eq-pole for gauss grid (J:2,3) */
     0, /* unsigned char smi;    scan mode in i 0 = +i 1 = -i (T8 J:2,12) */ 
     0, /* unsigned char smj;    scan mode in j 0 = -j 1 = +i (T8 J:2,12) */ 
     0  /* unsigned char smdir;  order 0 = consecutive points in i 1= cons in j */
} ;

static grib_is FGRIBAPI_is = {
           8,  /*       int len; */
  "GRIB    ",  /* unsigned char is[8]; */
           1,  /* unsigned char ver ;  */
} ; 

static grib_es FGRIBAPI_es = {
           4,  /* unsigned char len ;  */
      "7777",  /*     char es[4] ; */
} ; 
