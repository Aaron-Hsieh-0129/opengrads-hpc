
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


/* info for LATS */
struct galats {

/*=== lats_parmtab ===*/
  char ptname[256];         /* parameter file name */

/*=== lats_create ===*/
  int convention;           /* data convention (enum) */
  int gzip;                 /*ams level of gzip compression (nc only) ams*/
  int shave;                /*ams number of mantissa bits to shave ams*/
  int frequency;            /* data frequency (enum) e.g., hourly */
  int calendar;             /* calendar of the time model (enum) e.g., standard */
  int deltat;               /* number of time units between outputs, e.g., 12 for every 12 h */
  char model[256];          /* model COMMENT */
  char center[32];          /* data source which is the GRIB PROCESS ! NOT the GRIB CENTER */
  char comment[256];        /* comment on data set which will go into the title of the .ctl file */
  char oname[512];          /* name of the output file (dset) */

/*=== lats_basetime ===*/
  int lyr;                 /* lats basetime year */
  int lmo;                 /* lats basetime month */
  int lda;                 /* lats basetime day */
  int lhr;                 /* lats basetime hour */
  int lmn;                 /* lats basetime min */

/*=== lats_vertdim ===*/
  char vertdimname[32];     /* name of the vertial dimension */
  double levels[128];       /* vertical levels MAX IS 128!!!*/
  int nlev;                 /* number of levels */

/*=== lats_grid ===*/
  int gridtype;             /* grid type (enum), calculated in GrADS except when LATS_GAUSSIAN */
  char gridname[32];        /* name of the vertial dimension */
  int ilinear;              /* linearity of the grid in x */
  int jlinear;              /* linearity of the grid in y */
  int nlon;                 /* # of points in x */
  int nlat;                 /* # of points in y */
  float lon_1;              /* starting longitide */
  float lat_1;              /* starting latitude */
  float lon_nlon;           /* ending longitude */
  float lat_nlat;           /* ending latitude */

/*=== lats_var ===*/
  char var[32];             /* variable name */
  char var_comment[256];    /* variable name */
  int timestat;             /* variable time statistic (enum) */

/*=== lats_write ===*/
  double varlev;            /* level value of the variable  for lats_write*/

/*--- time options ---*/
  int time_opt;             /* 0 - grid relative ; 1 - dimension environment relative ; 2 - set time using baset time ; 3 - forecast hourly */
  int fhour;                /* forecast hour if using lats forecast_hourly */
  int fmin;                  /* forecast min if using lats forecast_hourly */

/*--- internal id's to pass to the lats routines ---*/
  int id_file;
  int id_lev;
  int id_grid;
  int id_var;
  int id_user_file;
  int id_user_var;
  int id_user_grid;
  int id_user_lev;
  int id_user_write;
};


