/*

  This GrADS extension has been placed in the public domain. See individual
C source files for PCMDI licensing terms.

                               ---
                        About POD Documentation 

The following documentation uses the "Perl On-line Documentation"
(POD) mark up syntax. It is a simple and yet adequate mark up language
for creating basic man pages, and there are converters to html,
MediaWiki, etc. In adittion, the perldoc utility can be used to
display this documentation on the screen by entering:

% perldoc lats

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < lats.c > lats.pod

and place lats.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < lats.pod > lats.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < lats.pod > lats.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make lats.html
% make lats.wiki

*/

#ifndef POD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grads.h"
#include "gex.h"
#include "galats.h"
#include "lats.h"

#ifdef GOT_NETCDF
# include "netcdf.h"
#endif

static char pout[256];   /* Build error msgs here */
static int  Initialized = 0;

struct galats glats ;      /* LATS option struct */

int  galats (struct gacmn *, int, int); /*mf --- GrADS-lats interface mf*/

/* ..................................................................... */

static void init_() {

  strcpy(glats.ptname,"");

/*
   initialize to bad values for force failure on create
   that is, require specification of frequency
*/

  glats.convention=LATS_GRADS_GRIB;
  glats.calendar=LATS_STANDARD;
  glats.frequency=-1;
  glats.deltat=0;

  glats.gzip = -1;
  glats.shave = -1;

  strcpy(glats.model,"Unknown1 LATS model");
  strcpy(glats.center,"PCMDI");
  strcpy(glats.comment,"Written using the GrADS-LATS interface");
  strcpy(glats.oname,"grads.lats");

  glats.gridtype=LATS_LINEAR;
  strcpy(glats.gridname,"grid1");

  strcpy(glats.vertdimname,"plev");

  strcpy(glats.var,"unknown1");
  glats.timestat=LATS_INSTANT;
  strcpy(glats.var_comment,"Unknown1 variable comment");
  glats.varlev=-1e20;

  glats.lyr=-1;
  glats.lmo=-1;
  glats.lda=-1;
  glats.lhr=-1;
  glats.lmn=-1;

  glats.fhour=-1;
  glats.fmin=-1;

  glats.time_opt=1;

  glats.id_file=-1;
  glats.id_lev=0;
  glats.nlev=-1;
  glats.id_grid=-1;
  glats.id_var=-1;

  Initialized = 1;

}

static int do_lats ( int argc, char **argv, struct gacmn *pcm, int opt) {

  char *name = argv[0];
  int i, rc;
  struct gastat *pst;
  struct gagrid *pgr;

  rc = 0;
  if (argc<2) {
    sprintf(pout,"\nError from %s: missing expression\n", name);
    gaprnt(0,pout);
    return(1);
  }

  /* Evaluate expression */
  pst = getpst(pcm);
  if (pst==NULL) return(1);

  /* Note: gapars() no longer work from extensions since 2.0.a9 */
  rc = gaexpr(argv[1],pst);
  if (rc) goto retrn;
  pcm->type[0] = pst->type;
  pcm->result[0] = pst->result;
  pcm->numgrd = 1;
  pcm->relnum = 1;

  /* Make sure UNDEFs are set as in v1 */
  pgr = pcm->result[0].pgr;
  for (i=0; i<(pgr->isiz*pgr->jsiz); i++ ) 
    if ( ! pgr->umask[i] ) pgr->grid[i] = pgr->undef;

  /* Do the LATS thing */
  rc=galats(pcm,opt,0);

  retrn:
    gafree(pst);
    return(rc);

}

/* ..................................................................... */

int c_lats_reinit ( int argc, char **argv, struct gacmn *pcm) {
  init_();
  return 0;
}
int c_lats_grid ( int argc, char **argv, struct gacmn *pcm) {
  if ( ! Initialized ) init_();
  return do_lats(argc,argv,pcm,3);
}

int c_lats_data ( int argc, char **argv, struct gacmn *pcm) {
  if ( ! Initialized ) init_();
  return do_lats(argc,argv,pcm,5);
}

int c_lats_set ( int argc, char **argv, struct gacmn *pcm) {

    int i, rc, kwrd = 98;
    gadouble v1, v2;

    rc = 0;

    if ( ! Initialized ) init_();

    if ( argc<2 ) {
      gaprnt (0,"SET_LATS error:  No arguments....\n");
      gaprnt (0,"  valid arguments are: \n");
      gaprnt (0,"  parmtab [FILENAME] (e.g., set_lats parmtab lats.ncep.MRFtable)\n");
      gaprnt (0,"  convention [grads_grib|grib_only|coards] (e.g., set_lats convention grib)\n");
      gaprnt (0,"  gzip level (e.g., set_lats gzip 2)\n");
      gaprnt (0,"  shave nbits (e.g., set_lats shave 12)\n");
      gaprnt (0,"  calendar [standard|noleap|clim|climleap] (e.g., set_lats calendar standard)\n");
      gaprnt (0,"  frequency [yearly|monthly|monthly_table_comp|weekly|daily|hourly|minutes|forecast_hourly|forecast_minutes|fixed] (e.g., set_lats frequency hourly)\n");
      gaprnt (0,"  deltat [N] (integer number of freq units per time output, e.g., set_lats deltat 6)\n");
      gaprnt (0,"  fhour [N] (integer forecast hour,  e.g., set_lats fhour 120)\n");
      gaprnt (0,"  model [MODEL_NAME] (e.g., set_lats model MRF)\n");
      gaprnt (0,"  center [CENTER_NAME] (e.g., set_lats center NCEP)\n");
      gaprnt (0,"  create [FILENAME] (e.g., set_lats create MRF.EXP1)\n");
      gaprnt (0,"  comment [COMMENT (e.g., set_lats comment \"R1.6.1 of MRF with convection update\")\n");
      gaprnt (0,"  gridtype [linear|gaussian|generic] (e.g., set_lats gridtype gaussian\n");
      gaprnt (0,"  vertdim [DIMNAME LEV_TYPE lev_1 ... lev_N] (e.g., set_lats vertdim plev 1000 850 500 200)\n");
      gaprnt (0,"  var [VARNAME average|accum|instant LEVEL_ID)] (e.g., set_lats var u instant 1)\n");
      gaprnt (0,"  timeoption [grid|dim_env|settime (e.g., set_lats v timeoption dim_env (use the GrADS dimension environment)\n");
      gaprnt (0,"  write [VAR_ID LEVEL] (e.g., set_lats write 1 500 (return from t lats var)\n");
      gaprnt (0,"  close (e.g., set_lats close)\n\n");
      return(1);

    }

/*-------------------
  lats_parmtab interface
-------------------*/

    if(cmpwrd("parmtab",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS PARMTAB error:  No arguments....\n");
	return(1);
      } else strcpy(glats.ptname,argv[2]);
      rc=galats(pcm,0,0);
      sprintf(pout,"LATS PARMTAB ID = %d\n",rc);
      gaprnt(2,pout);
    }

/*----------
  lats_create interface
---------*/

    else if (cmpwrd("convention",argv[1])||cmpwrd("format",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS CONVENTION error:  No arguments....\n");
	return(1);
      } else if( cmpwrd("grads_grib",argv[2]) ) glats.convention=LATS_GRADS_GRIB;
      else if( cmpwrd("coards",argv[2]) )    glats.convention=LATS_NC3;
      else if( cmpwrd("netcdf",argv[2]) )    glats.convention=LATS_NC3;
      else if( cmpwrd("nc3",argv[2]) )       glats.convention=LATS_NC3;
      else if( cmpwrd("nc4",argv[2]) )       glats.convention=LATS_NC4;
      else if( cmpwrd("netcdf4",argv[2]) )   glats.convention=LATS_NC4;
      else if( cmpwrd("hdf",argv[2]) )       glats.convention=LATS_HDF4;
      else if( cmpwrd("hdf4",argv[2]) )      glats.convention=LATS_HDF4;
      else if( cmpwrd("grib_only",argv[2]) ) glats.convention=LATS_GRIB_ONLY;
      else {
	gaprnt (0,"SET_LATS CONVENTION error:  invalid argument using the default...\n");
	return(1);
      }
    }

    else if (cmpwrd("gzip",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS GZIP error:  No arguments....\n");
	return(1);
      } 
#ifndef NC_NETCDF4
	gaprnt (0,"SET_LATS GZIP warning:  ignored since NetCDF-4 is not present\n");
	return(0);
#else
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS GZIP error:  invalid or missing gzip level\n");
	return(1);
      } else if (i<1||i>9) {
	gaprnt (0,"SET_LATS GZIP error:  level must be between 1 and 9.\n");
	return(1);
      } else {
	glats.gzip=i;
      }
#endif
    }

    else if (cmpwrd("shave",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS SHAVE error:  No arguments....\n");
	return(1);
      } 
#ifndef NC_NETCDF4
	gaprnt (0,"SET_LATS SHAVE warning:  ignored since NetCDF-4 is not present\n");
	return(0);
#else
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS SHAVE error:  invalid or missing number of bits to shave level\n");
	return(1);
      } else if (i>23) {
	gaprnt (0,"SET_LATS SHAVE error:  nbits must be at most 23.\n");
	return(1);
      } else {
	glats.shave=i;
        if ( glats.gzip < 0 ) glats.gzip = 2; /* no point in shaving without compression */
      }
#endif
    }

    else if (cmpwrd("calendar",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS CALENDAR error:  No arguments....\n");
	return(1);
      }
      else if( cmpwrd("standard",argv[2]) ) glats.calendar=LATS_STANDARD;
      else if( cmpwrd("noleap",argv[2]) ) glats.calendar=LATS_NOLEAP;
      else if( cmpwrd("clim",argv[2]) ) glats.calendar=LATS_CLIM;
      else if( cmpwrd("climleap",argv[2]) ) glats.calendar=LATS_CLIMLEAP;
      else {
	gaprnt (0,"SET_LATS CALENDAR error:  Invalid argument using the default...\n");
	return(1);
      }
    }

    else if(cmpwrd("frequency",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS FREQUENCY error:  No arguments....\n");
	return(1);
      } else if( cmpwrd("yearly",argv[2]) ) glats.frequency=LATS_YEARLY;
      else if( cmpwrd("monthly",argv[2]) ) glats.frequency=LATS_MONTHLY;
      else if( cmpwrd("monthly_table_comp",argv[2]) ) glats.frequency=LATS_MONTHLY_TABLE_COMP;
      else if( cmpwrd("weekly",argv[2]) ) glats.frequency=LATS_WEEKLY;
      else if( cmpwrd("daily",argv[2]) ) glats.frequency=LATS_DAILY;
      else if( cmpwrd("hourly",argv[2]) ) glats.frequency=LATS_HOURLY;
      else if( cmpwrd("minutes",argv[2]) ) glats.frequency=LATS_MINUTES;
      else if( cmpwrd("forecast_hourly",argv[2]) ) {
	glats.frequency=LATS_FORECAST_HOURLY;
	glats.time_opt=3;
      }
      else if( cmpwrd("forecast_minutes",argv[2]) ) {
	glats.frequency=LATS_FORECAST_MINUTES; /*ams Mike: check this! ams*/
	glats.time_opt=3;
      }
      else if( cmpwrd("fixed",argv[2]) ) glats.frequency=LATS_FIXED;
      else {
	gaprnt (0,"SET_LATS FREQUENCY error:  Invalid arguments LATS_CREATE WILL FAIL....\n");
	return(1);
      }
    }

    else if(cmpwrd("deltat",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS DELTAT error:  No arguments....\n");
	return(1);
      }
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS DELTAT error:  invalid or missing increment\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS DELTAT error:  < 0 LATS_CREATE WILL FAIL...\n");
	return(1);
      } else {
	glats.deltat=i;
      }
    }

    else if(cmpwrd("fhour",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS FHOUR error:  No arguments....\n");
	return(1);
      }
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS FHOUR error:  invalid or missing increment\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS FHOUR error:  < 0 LATS WILL FAIL...\n");
	return(1);
      } else {
	glats.fmin=0; /* mf set default fmin to 0 if hourly */
	glats.fhour=i; 
      }
    }

    else if(cmpwrd("fminute",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS FMINUTE error:  No arguments....\n");
	return(1);
      }
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS FMINUTE error:  invalid or missing increment\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS FMINUTE error:  < 0 LATS WILL FAIL...\n");
	return(1);
      } else {
	glats.fmin=i;
	glats.fhour=0; /* mf set default fhour to 0 if minutes */
      }
    }

    else if(cmpwrd("basetime",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS BASETIME error:  No arguments....\n");
	return(1);
      }
      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME ID_FILE error:  missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME ID_FILE error:  id_file < 0\n");
	return(1);
      } else {
	if(i >= 0 && i <= glats.id_file) {
	  glats.id_user_file=i;
	} else {
	  sprintf(pout,"SET_LATS BASETIME error:  FILE ID is %d but the max FILE ID is %d\n",
		  i,glats.id_file);
	  gaprnt(0,pout);
	  return(1);
	}
      }

      if ( argc < 4 ) {
	gaprnt (0,"SET_LATS BASETIME YEAR error:  missing\n");
	return(1);
      }
      if ( intprs(argv[3],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME YEAR error:  invalid or missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME YEAR error:  year < 0\n");
	return(1);
      } else {
	glats.lyr=i;
      }

      if ( argc < 5 ) {
	gaprnt (0,"SET_LATS BASETIME MONTH error:  missing\n");
	return(1);
      }
      if ( intprs(argv[4],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME MONTH error:  invalid or missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME MONTH error:  year < 0\n");
	return(1);
      } else {
	glats.lmo=i;
      }

      if ( argc < 6 ) {
	gaprnt (0,"SET_LATS BASETIME DAY error:  missing\n");
	return(1);
      }
      if ( intprs(argv[5],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME DAY error:  invalid or missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME DAY error:  year < 0\n");
	return(1);
      } else {
	glats.lda=i;
      }

      if ( argc < 7 ) {
	gaprnt (0,"SET_LATS BASETIME HOUR error:  missing\n");
	return(1);
      }
      if ( intprs(argv[6],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME HOUR error:  invalid or missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME HOUR error:  year < 0\n");
	return(1);
      } else {
	glats.lhr=i;
      }

      if ( argc < 9 ) {
	gaprnt (0,"SET_LATS BASETIME MIN error:  missing\n");
	return(1);
      }
      if ( intprs(argv[7],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS BASETIME MIN error:  invalid or missing\n");
	return(1);
      } else if (i<0) {
	gaprnt (0,"SET_LATS BASETIME MIN error:  year < 0\n");
	return(1);
      } else {
	glats.lmn=i;
      }

      rc=galats(pcm,10,0);  /* set the basetime */

    }

    else if(cmpwrd("model",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS MODEL error:  No arguments....\n");
	return(1);
      } else strcpy(glats.model,argv[2]);
    }

    else if(cmpwrd("center",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS CENTER error:  No arguments....\n");
	return(1);
      } else strcpy(glats.center,argv[2]);
    }

    else if(cmpwrd("comment",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS COMMENT error:  No arguments....\n");
	return(1);
      } else strcpy(glats.comment,argv[2]);
    }

    else if(cmpwrd("varcomment",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS VARCOMMENT error:  No arguments....\n");
	return(1);
      } else strcpy(glats.var_comment,argv[2]);
    }

    else if(cmpwrd("create",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS CREATE error:  Missing or invalid arguments");
	return (1);
      } else strcpy(glats.oname,argv[2]);
      rc=galats(pcm,1,0);  /* open the lats file */
      sprintf(pout,"LATS FILE ID = %d\n",rc);
      gaprnt(2,pout);
    }


/*-----------------
  lats_grid interface
-----------------*/

    else if (cmpwrd("gridtype",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS GRIDTYPE error:  No arguments....\n");
	return(1);
      } else if( cmpwrd("linear",argv[2]) ) glats.gridtype=LATS_LINEAR;
        else if( cmpwrd("gaussian",argv[2]) ) glats.gridtype=LATS_GAUSSIAN;
        else if( cmpwrd("generic",argv[2]) ) glats.gridtype=LATS_GENERIC;
        else {
         gaprnt (0,"SET error:  Missing or invalid arguments for GRIDTYPE ");
         return (1);
      }
    }

    else if(cmpwrd("gridname",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS GRIDNAME error:  No arguments....\n");
	return(1);
      } else strcpy(glats.gridname,argv[2]);
    }

/*------------
  lats_vertdim interface
-------------*/

    else if(cmpwrd("vertdim",argv[1])) {
      glats.nlev=0;
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS VERTDIM error:  No arguments....\n");
	return(1);
      } else {
        strcpy(glats.vertdimname,argv[2]);
      }

      if ( argc < 4 ) {
	gaprnt (0,"SET_LATS VERTDIM error:  no levels given....\n");
	return(1);
      }

      for ( i=3; i<argc; i++ ) {
        getdbl(argv[i],&(v1));
	glats.levels[glats.nlev]=(double)v1;
	glats.nlev++;
      }

      rc=galats(pcm,2,0);  /* set the vertical dimension */
      sprintf(pout,"LATS VERTDIM ID = %d\n",rc);
      if(rc==0) gaprnt(0,pout);
      else      gaprnt(2,pout);

    }

/*------
  lats_var interface
------*/

    else if(cmpwrd("var",argv[1])) {

      if ( argc < 3 ) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR error:  No arguments! args: fileid varname datatype gridid levid\n");
	return(1);

      }

      /* --- file id */

      if ( intprs(argv[2],&(i))==NULL) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR FILE ID:  invalid FILE ID value given\n");
	return(1);
      }
      if(i >= 0 && i <= glats.id_file) {
	glats.id_user_file=i;
      } else {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	sprintf(pout,
		"SET_LATS VAR error:  FILE ID is %d but the max FILE ID is %d\n",
		i,glats.id_file);
	gaprnt(0,pout);
	return(1);

      }

      /* --- var name */
      if ( argc < 4 ) {
	gaprnt (0,"SET_LATS VAR error:  variable name not given...\n");
	return(1);
      } else { strcpy(glats.var,argv[3]); }

      /* --- stat type */
      if ( argc < 5 ) {
        if( cmpwrd("average",argv[4]) ) glats.timestat=LATS_AVERAGE;
        else if( cmpwrd("accum",argv[4]) ) glats.timestat=LATS_ACCUM;
        else if( cmpwrd("instant",argv[4]) ) glats.timestat=LATS_INSTANT;
        else {
	 gaprnt (0,"SET_LATS VAR error:  invalid variable statistic type given...\n");
	 return(1);
        }
      }

      /* --- level grid id */

      if ( argc < 6 ) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR error:  missing GRID ID\n");
	return(1);
      }

      if ( intprs(argv[5],&(i)) == NULL ) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR error:  invalid GRID ID value given\n");
	return(1);
      } else if(i >= 0 && i <= glats.id_grid) {
	glats.id_user_grid=i;
      } else {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	sprintf(pout,
		"SET_LATS VAR error:  GRID ID is %d but the max GRID ID is %d\n",
		i,glats.id_user_grid);
	gaprnt(0,pout);
	return(1);

      }

      /* --- level id */

      if ( argc < 7 ) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR error:  missing LEVEL ID\n");
	gaprnt (0,"                     use a value of 0 for surface variables\n");
	return(1);
      }

      if ( intprs(argv[6],&(i)) == NULL ) {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	gaprnt (0,"SET_LATS VAR error:  invalid LEVEL ID value given\n");
	gaprnt (0,"                     use a value of 0 for surface variable\n");
	return(1);
      } else if(i >= 0 && i <= glats.id_lev) {
	glats.id_user_lev=i;
      } else {
	sprintf(pout,"LATS VAR ID = -1\n");
	gaprnt(2,pout);
	sprintf(pout,
		"SET_LATS VAR error:  LEVEL ID is %d but the max LEVEL ID is %d\n",
		i,glats.id_user_lev);
	gaprnt(0,pout);
	gaprnt(0,"              use a value of 0 for surface variables...\n");
	return(1);

      }

      /* --- call lats_var */

      rc=galats(pcm,4,0);
      sprintf(pout,"LATS VAR ID = %d\n",rc);
      gaprnt(2,pout);

    }


/*-------------------
  lats_write interface
-------------------*/

    else if(cmpwrd("write",argv[1])) {

      glats.id_user_write=0;

      /* --- file id */

      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS WRITE error:  No arguments (fileid varid)\n");
	return(1);
      }

      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS WRITE error:  missing or invalid FILE ID given...\n");
	return(1);

      } else {

	glats.id_user_file=i;
	if(glats.id_user_file<0 || (glats.id_user_file > glats.id_file) ) {
	  sprintf(pout,
		  "SET_LATS WRITE error: FILE ID is outside the valid range of 1 - %d\n",
		  glats.id_file);
	  gaprnt (0,pout);
	  return(1);
	}
      }

      /* --- var id */

      if ( argc < 4 ) {
	gaprnt (0,"SET_LATS WRITE error:  No arguments...\n");
	return(1);
      }

      if ( intprs(argv[3],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS WRITE error:  missing or invalid VAR ID  given...\n");
	return(1);

      } else {

	glats.id_user_var=i;
	if(glats.id_user_var<0 || (glats.id_user_var > glats.id_var) ) {
	  sprintf(pout,
		  "SET_LATS WRITE error: VAR ID is outside the valid range of 1 - %d\n",
		  glats.id_var);
	  gaprnt (0,pout);
	  return(1);
	}
      }

      if ( argc >= 5 ) {
	getdbl(argv[4],&(v1)) ;
	glats.varlev=(double)v1;
      } else {
	glats.varlev=0.0;
      }
      glats.id_user_write=1;

      sprintf(pout,"LATS WRITE ID = %d\n",glats.id_user_write);
      gaprnt(2,pout);

    }

    else if (cmpwrd("timeoption",argv[1])) {
      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS TIMEOPTION error:  No arguments....\n");
	return(1);
      } else if ( cmpwrd("grid",argv[2]) ) glats.time_opt=0;
        else if( cmpwrd("dim_env",argv[2]) ) glats.time_opt=1;
        else if( cmpwrd("settime",argv[2]) ) glats.time_opt=2;
        else {
	  gaprnt (0,"SET_LATS TIMEOPTION error:  invalid option ....\n");
	  return(1);
      }
    }

/*-------------------
  lats_close interface
-------------------*/

    else if(cmpwrd("close",argv[1])) {

      if ( argc < 3 ) {
	gaprnt (0,"SET_LATS CLOSE error:  No arguments, specify a file id...\n");
	return(1);
      }

      if ( intprs(argv[2],&(i)) == NULL ) {
	gaprnt (0,"SET_LATS CLOSE error:  missing or invalid FILE ID given...\n");
	return(1);

      } else {
	
	if(i<0 || i>glats.id_file){
	  sprintf(pout,
		  "SET_LATS CLOSE error: FILE ID is outside the valid range of 1 - %d\n",
		  glats.id_file);
	  gaprnt (0,pout);
	  return(1);
	}

      }

      rc=galats(pcm,6,i);  /* close the lats file */

    }

    else if(cmpwrd("reset",argv[1])) {
      rc=galats(pcm,7,0);  /* reset the lats state  */
    }


    else {

      gaprnt (0,"SET_LATS error:  Missing or invalid arguments:\n ");
      sprintf (pout,"for %s option\n",argv[1]);
      gaprnt (0,pout);
      return (1);

    }

    /* Consistency checks */
    if ( glats.gzip >= 0 && glats.convention==LATS_NC3 ) {
      glats.convention = LATS_NC4;
      gaprnt(0,"SET_LATS Warning: GZIP compression requires NetCDF-4 --- resetting NetCDF to version 4\n");
    }

    return(abs(rc));

}

/* .................................................................. */

int c_lats_query ( int argc, char **argv, struct gacmn *pcm) {

  int i;
  gadouble v1, v2;

  if ( ! Initialized ) init_();

  /* LATS state */

    sprintf(pout,"GrADS-LATS Interface State:\n");
    gaprnt (2,pout);

    /*=== lats_parmtab ===*/
    sprintf(pout,"%s \t:parameter table file (by \"set_lats parmtab \")\n",glats.ptname);
    gaprnt (2,pout);

    /*=== lats_create ===*/
    sprintf(pout,"%d \t: data CONVENTION parameter (by \"set_lats convention\")\n",glats.convention);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: data CALENDAR parameter (by \"set_lats calendar\")\n",glats.calendar);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: data time FREQEUENCY  parameter (by \"set_lats frequency\")\n",glats.frequency);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: TIMEOPTION parameter (by \"set_lats timeoption\")\n",glats.time_opt);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: DELTAT time (# of intervals) (by \"set_lats deltat\")\n",glats.deltat);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: BASETIME year (# of intervals) (by \"set_lats basetime id_file yr mo da hr\")\n",glats.lyr);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: BASETIME month (# of intervals) (e.g., \"set_lats basetime 1 1997 5 31 00\")\n",glats.lmo);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: BASETIME day (# of intervals) (e.g., \"set_lats basetime 1 1997 5 31 00\")\n",glats.lda);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: BASETIME hour (# of intervals) (e.g., \"set_lats basetime 1 1997 5 31 00\")\n",glats.lhr);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: BASETIME minute (# of intervals) (e.g., \"set_lats basetime 1 1997 5 31 00 0\")\n",glats.lmn);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: FHOUR hour (# of intervals) (e.g., \"set_lats fhour 12\")\n",glats.fhour);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: FMIN hour (# of intervals) (e.g., \"set_lats fmin 12\")\n",glats.fmin);
    gaprnt (2,pout);

    sprintf(pout,"%s \t: MODEL (by \"set_lats model\")\n",glats.model);
    gaprnt (2,pout);

    sprintf(pout,"%s \t: data CENTER (by \"set_lats center\")\n",glats.center);
    gaprnt (2,pout);

    sprintf(pout,"%s \t: COMMENT (by \"set_lats comment\")\n",glats.comment);
    gaprnt (2,pout);

    sprintf(pout,"%s \t: output file (by \"set_lats open \")\n",glats.oname);
    gaprnt (2,pout);

/*=== lats compression ===*/
    sprintf(pout,"%d \t: GZIP level (by \"set_lats gzip\")\n",glats.gzip);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: SHAVE nbits (by \"set_lats shave\")\n",glats.shave);
    gaprnt (2,pout);


/*=== lats_grid ===*/
    sprintf(pout,"%d \t: GRIDTYPE grid type parameter(by \"set_lats gridtype\")\n",glats.gridtype);
    gaprnt (2,pout);

    sprintf(pout,"%s \t: GRIDNAME grid type parameter(by \"set_lats gridname\")\n",glats.gridname);
    gaprnt (2,pout);

    sprintf(pout,"%4d \t: linear in longitude (0=no,1=yes)\n",glats.ilinear);
    gaprnt (2,pout);
    sprintf(pout,"%4d \t: linear in latitude  (0=no,1=yes)\n",glats.jlinear);
    gaprnt (2,pout);
    sprintf(pout,"%4d \t: # longitude points (nlon) \n",glats.nlon);
    gaprnt (2,pout);
    sprintf(pout,"%4d \t: # latitude  points (nlat) \n",glats.nlat);
    gaprnt (2,pout);

    sprintf(pout,"    ( %10.4f , %10.4f ) \t: (   1,   1) lon lat\n",glats.lon_1,glats.lat_1);
    gaprnt (2,pout);
    sprintf(pout,"    ( %10.4f , %10.4f ) \t: (nlat,nlon) lon lat\n",glats.lon_nlon,glats.lat_nlat);
    gaprnt (2,pout);

/*=== lats_vertdim ===*/
    sprintf(pout,"%s \t: vertical dimension by \"set_lats levels (lev1 ... levN) vertdimname\")\n",glats.vertdimname);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: # of vertical levels (by \"set_lats levels (lev1 ... levN) vertdimname\")\n",glats.nlev);
    gaprnt (2,pout);

    for(i=0; i<(glats.nlev) ; i++) {
      sprintf(pout," %g",glats.levels[i]);
      gaprnt (2,pout);
    }

    sprintf(pout,"vertical levels\n");
    gaprnt (2,pout);


/*=== lats_var ===*/

    sprintf(pout,"%s \t: VAR output parameter (by \"set_lats var (name timestat [level])\")\n",glats.var);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: VAR TIMESTAT time statistic parameter\n",glats.timestat);
    gaprnt (2,pout);

/*==== LATS internal id's ===*/

    sprintf(pout,"%d \t: id_file LATS internal ID\n",glats.id_file);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_grid LATS internal ID\n",glats.id_grid);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_lev LATS internal ID\n",glats.id_lev);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_var LATS internal ID\n",glats.id_var);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_user_file LATS internal ID\n",glats.id_user_file);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_user_grid LATS internal ID\n",glats.id_user_grid);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_user_var LATS internal ID\n",glats.id_user_var);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_user_lev LATS internal ID\n",glats.id_user_lev);
    gaprnt (2,pout);

    sprintf(pout,"%d \t: id_user_write LATS internal ID\n",glats.id_user_write);
    gaprnt (2,pout);

    return(0);

}

/* .................................................................... */


  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

liblats.gex - GrADS Extension Library for File Subsetting and Reformatting

=head1 SYNOPSIS

=head3 GrADS Commands:

=over 4

=item 

run B<set_lats> I<PARAMETER> I<VALUE> - Set internal parameters

=item 

run B<query_lats> - Print internal paramaters

=item 

run B<lats_grid> I<EXPR> - Set horizontal grid

=item 

run B<lats_data> I<EXPR> - Write data to file

=back

=head1 DESCRIPTION 

This library provides GrADS extensions (I<gex>) for interfacing to
LATS (Library of AMIP II Transmission Standards), a collection of I/O
functions for creating lon/lat gridded datasets in the GRIB, NetCDF-3,
NetCDF-4/HDF-5 and HDF-4 formats. This is the low level LATS interface
in GrADS. Usually, one uses the wrapper B<lats4d.gs> script
(L<http://opengrads.org/doc/scripts/lats4d/>) for a more user friendly
interface which has reasonable defaults for most internal parameters.

The GrADS interface to LATS is implemented by means of 4 I<User
Defined Commands> (UDCs). Most parameters defining the output file are
set by means of the B<set_lats> command; the current LATS state can
queried with command B<query_lats>. Command B<lats_grid> is used to
define the horizontal longitude/lattude grid (vertical levels are set
with the B<set_lats> command.) Command B<lats_data> is used to write a
2-dimensional (longitude x latitude) slice of a variable, for a given time
and level.

The following sub-section describes the main attributes of the LATS
library which this extension interfaces to. It has been adapted from the
original LATS manual page.

=head2 Overview of the LATS Library

LATS is a subroutine library developed by the Program for Climate
Model Diagnosis and Intercomparison (PCMDI) to output lon/lat gridded
data for the AMIP II (Atmospheric Model Intercomparison Project II)
and other intercomparison projects.  In addition to outputting data,
LATS optionally performs basic quality control on the data
written. LATS outputs data in the GRIB and/or netCDF formats and
provides an interface to GrADS and VCS.  The LATS library is no longer
supported by the original developers at PCMDI but remains as a viable
mechanism for producing GRID-1, NetCDF and HDF output from GrADS.

The main features of LATS are:

=over 4

=item 1

LATS is designed to output rectilinear, generally global, gridded,
spatio-temporal data.  The amount of data written with a single
function call is a horizontal longitude-latitude slice of a variable.

=item 2

Data may be output in the machine-independent formats GRIB and/or
netCDF and are directly readable by GrADS.

=item 3

Acceptable variable names are prescribed.  The units, datatype, and
basic structure (e.g., single-level or multi-level) are inferred from
the variable name. This information is tabled in an external, ASCII
I<parameter table>. If no parameter table is specified, a default list of
AMIP II parameters is used.

=item 4

More than one LATS file may be open simultaneously  for
output. In GrADS, one usually write one file at a time.

=item 5

Data must be written in increasing time sequence.  All variables in a
file share a common time frequency (e.g., hourly, monthly,
etc.). Originally LATS did not support minutes, but a patch has ben
developed in version used by GrADS to allow minutes are a valid time
sequence.

=item 6

For a given timepoint, variables, and  multiple	levels for a variable, may be written in any order.

=item 7

Although GrADS Version 2 uses doubles (64 bits) to store variable
data, all data written to file by this interface are floating-point or
integer.  INTEGER*8), and C int data can be written.

=item 8

Data written to GRIB files are packed to a predefined bit width or
numerical precision depending on the variable. The precision and bit
width is specified in the I<parameter table> file.  Floating-point data written
to netCDF files are saved as 32-bit IEEE floating-point values;
integer data are written as 32-bit 2s complement integers.

=back


=head1 ANATOMY OF A LATS BASED GRADS SCRIPT

The skeleton of a GrADS script using the LATS interface is as follows:

=over 4

=item 1.  

Optionally, specify an external parameter file, with
B<set_lats> I<parmtab>.

=item 2.  

Define the horizontal grid with B<lats_grid>.  Define all vertical
dimensions (e.g., pressure level) with B<set_lats> I<vertdim>. If a default
surface dimension is defined for a variable, it does not
have to be redefined with B<set_lats> I<vertdim>.  Grids and vertical
dimensions are shared across variables.  NOTE: At
present, only one grid may be defined for a GrADS/GRIB file.

=item 3.  

Create a LATS file, with B<set_lats> I<create>.

=item 4. 

Optionally, set the basetime, with B<set_lats> I<basetime>.

=item 5. 

For EACH AND EVERY variable to be written, declare the
variable with B<set_lats> I<var>. The LATS requirement that ALL variables
be declared up front (before writing) is necessitated by the netCDF
interface.

=item 6.  

For each time-point, in increasing time order and, for each
horizontal level of each variable, write the data for this level,
time-point, with B<set_lats> I<write> and B<lats_data>.

=item 7. 

Close the file, with B<set_lats> I<close>.

=back

By default, all errors are reported.

=head2 QUALITY CONTROL 

LATS performs some basic quality control on the data written. The
intention is to provide a quick check of data validity. For each level
written the following statistics are calculated: range
(maximum-minimum) and area-weighted average. If a missing data flag is
defined for the variable, any missing data are ignored in calculating
the statistics.  Quality control is not performed on integer-valued
variables.

A QC exception is generated if abs(average - observed_average) >
(tolerance * observed_standard_deviation).  Similarly, an exception is
generated if range > (range_tolerance * observed_range). In either
case, a warning is issued, and an entry is written to the QC log
file. The values of observed_average, tolerance,
observed_standard_deviation, range_tolerance, and observed_range are
tabled in the QC section of the I<Parameter Table> file. If no entry in this
section is found for the given (variable,level_type,level), then no
quality control is performed for that level.

Data are always written, regardless of whether a QC exception is
generated.

The default name of the log file is I<lats.log>. This name is
superseded by the value of the environment variable LATS_LOG, if
defined.

=head1 COMMANDS PROVIDED

=head2 B<set_lats> I<basetime YEAR MONTH DAY HOUR>

Set the basetime for file with ID fileid.  The basetime is the initial
time in which the file can be referenced.  The function
returns 1 if successful, 0 if an error oc- curs.

=head2 B<set_lats> I<close>

Close the file. The function returns 1 if successful, 0 if an error
occurs.

=over 8

=item B<NOTE:> 
It is important to call B<set_lats close>, to ensure that any
buffered output is written to the file.

=back

=head2 B<set_lats> I<create FILENAME>

Create a LATS file with the given I<FILENAME>, a string of length <=
256. If I<FILENME> does not end in the proper extension ('.nc' for
netCDF, '.grb' for GRIB), the extension will be appended to the path.

=head2 B<set_lats> I<convention CONVENTION>

Deprecated. Same as B<set_lats> I<format>.

=head2 B<set_lats> I<format FORMAT>

The parameter I<FORMAT> defines the data format to be written, and the
metadata convention to be followed when writing the format. The
options are:

=over 8

=item I<grads_grib>

WMO GRIB format, plus a GrADS control file and the ancillary GRIB map
file.  If this format is used the time step (see B<set_lats deltat>)
must be non-zero, implying that timepoints are evenly-spaced.  All
variables in a file must share the same horizontal grid.  This
convention is readable with GrADS.  The GRIB data may be processed by
the utility B<wgrib> shipped with most GrADS distributions.

=item I<grib>

Similar to I<grads_grib> but without a GrADS control file and the
ancillary GRIB map file.

=item I<netcdf>

NetCDF-3 format, observing the COARDS metadata standard. When this
format is specified, the calendar must be I<STANDARD>.  Climatologies
are indicated by specifying I<year = 2> in B<set_lats write>.  For the
I<clim> and I<climleap> calendars, the year is automatically set to
2. Files wiritten with this format are readable with GrADS.

=item I<netcdf4>

Like the I<netcdf> format, except that the files are in the new
NetCDF-4 (actually HDF-5) format. 

=item I<hdf4>

Like the I<netcdf> format, except that the files are in the new HDF-4
format. Notice that in GrADS v2, NetCDF and HDF-4 files are produced
with the same GrADS executables.

=back


=head2 B<set_lats> I<calendar CALENDAR>

I<CALENDAR> is the calendar type, one of the following values:

=over 4

=item I<STANDARD>	    

Standard Gregorian calendar. This is the default.

=item I<NOLEAP>	    

365days/year, no leap years

=item I<CLIM>

Climatological time (no associated year), 365 days

=item I<CLIMLEAP>

Climatological time, 366 days

=back


=head2 B<set_lats> I<center CENTER>

I<CENTER> is the name of the modeling center or group creating the
file, a string of <= 128 characters. For GRIB output, center must
match one of the centers listed in the parameter file.

=head2 B<set_lats> I<deltat DeltaT>

I<DeltaT> is the number of time units in the time increment, where the
units are specified by frequency.  For example, data which are defined
at multiples of 6 hours would be specified with 

   ga-> set_lats frequency hourly
   ga-> set_lats deltat 6 

Similarly, monthly average data would be indicated by 

   ga-> set_lats frequency monthly
   ga-> set_lats deltat 1

Note that times may be skipped for formats other than I<GRADS_GRIB>;
the only requirement imposed by the LATS interface is that timepoints,
as specified via B<set_lats write>, be at a multiple of the time
increment, relative to the base time (the first time written).

=head2 B<set_lats> I<frequency FREQUENCY>

I<FREQUENCY> is the time frequency of variables to be written to the
file, and has one of the values:

=over 4

=item I<YEARLY>

Only the year component of time is significant. I<DeltaT> (see
B<set_lats>) is expressed in years.

=item I<MONTHLY>

The year and month components of time are significant. I<DeltaT> (see
B<set_lats>) is expressed in months. Floating-point data in the
I<GRADS_GRIB> format are compressed to 16-bits.

=item I<MONTHLY_TABLE_COMP>

The year and month components of time are significant.  I<DeltaT> (see
B<set_lats>) is expressed in months. Floating-point data in the
I<GRADS_GRIB> format are compressed according to the specification in
the parameter table.

=item I<WEEKLY>

The year, month, and day component of time are significant. I<DeltaT> is
expressed in weeks.

=item I<DAILY>

The year, month, and day component of time are significant. delta is
expressed in days.

=item I<HOURLY>

The year, month, day, and hour component of time are significant.
I<DeltaT> is expressed in hours.

=item I<MINUTES>

The year, month, day, hour, and minute component of time are significant.
I<DeltaT> is expressed in minutes.

=item I<FORECAST_HOURLY>

The year, month, day, and hour component of time are significant.
I<DeltaT> is expressed in hours.

=item I<FORECAST_MINUTES>

The year, month, day, hour, and minute component of time are significant.
I<DeltaT> is expressed in minutes.

=item I<FIXED>

Data are not time-dependent, e.g., for surface type, orography, etc.
I<DeltaT> is ignored and year is set to 1 by convention.

=back

=head2 B<set_lats> I<gridtype GRIDTYPE>

Define the type of horizontal grid. I<GRIDTYPE> is GAUSSIAN for
Gaussian grids, LINEAR for evenly spaced grids, or GENERIC
otherwise. The actual horizontal grid definition is performed with
command B<lats_grid>.

=head2 B<set_lats> I<gzip COMPRESSION_LEVEL>

When writing NetCDF-4 files, specifes the level of GZIP compression to
be employed. The higher the level, the harder the library works doing
compression, usually (but not always) producing smaller files. The
default is -1, meaning no compression. Notice that only NetCDF-4 files
can be compressed. When I<COMPRESION_LEVEL> > 0 is specified with
NetCDF-3 files, the format is automatically changed to NetCDF-4. For
improving the compression effectiveness see B<set_lats> I<shave>.


=head2 B<set_lats> I<model MODEL>

I<MODEL> is the name of the model version which created this data.
comments is a string of length 256 or less , including any null-
terminator.  The command returns an integer file ID, or 0 if the file
cannot be created.

=head2 B<set_lats> I<parmtab TABLE_FILENAME> 

Specify an external parameter table file.  I<TABLE_FILENAME> is the
pathname of a file containing a list of variable descriptions, and is
a string of length <= 256 characters. Use of an external parameter
table is optional. The location of the parameter table is determined
as follows: (1) if the command B<set_lats> I<parmtab> is issued, the
value of I<TABLE_FILENAME> is used, otherwise (2) if the environment
variable LATS_PARMS is defined, its value is used, else (3) an
internally-defined table of AMIP parameters is used. The command
returns 1 on success, 0 on failure.

=head2 B<set_lats> I<shave NBITS> 

Shave I<NBITS> off the mantissa of float-point numbers. By definition,
IEEE float numbers have 24 bits dedicated for the mantissa. This
command set to zero the last I<NBITS> of the mantissa, this way
reducing entropy and improving the effectiveness of GZIP compression
(see B<set_lats> I<gzip>). I<NBITS> must be in the range [1,23]. When
I<NBITS> > 0 is specified, it automatically sets GZIP compression on
at level 2, unless the compression level has already been
set. (Currently compression is implemented only for NetCDF04 output.)

NOTE: The actual shaving algorithm, first scales the variable being written
for each horizontal layer, and then shaves bits off the mantissa. See
      http://en.wikipedia.org/wiki/Ieee_float
for additional information on IEEE float-point numbers.

=head2 B<set_lats> I<var VARNAME TIMESTAT LEVEL_ID>

Declare a variable to be written to a LATS file. fileid is the integer
file ID returned from lats_create.  The variable name I<VARNAME> must
match a name in the parameter table, and is a string of length <= 128
characters.


I<TIMESTAT> is a time statistic identifier, one of:

=over 4

=item I<AVERAGE> 

An average over the delta time interval frequency
defined by lats_create.

=item I<INSTANT>

The field is valid at the instan- taneous time set by the year, month,
day, hour.

=item I<ACCUM>

Accumulation during delta time interval I<DeltatT> (see B<set_lats deltat>).

=back

I<LEVID> is the ID of the vertical dimension of the variable, as
returned from B<set_lats> I<vertdim>. If the variable has a default surface
defined in the parameter table, or has no associated
vertical dimension, I<LEVID> should be 0. (Note: if levid is 0 and the
variable has a default surface defined, the netCDF representation
of the variable will not have an explicit vertical dimension, but
will have a I<level_description> attribute).

B<set_lats var> should be called exactly once for each variable to be
written to a LATS file and must be called BEFORE B<set_lats> I<write>.
The function returns the integer variable ID on success, or 0 on
failure.

=head2 B<set_lats> I<vertdim DIM_NAME LEV_TYPE LEV_1 ... LEV_N>

I<DIM_NAME> is the name of the vertical dimension (e.g., "height",
"depth", "level"), and it should not contain any whitespace
characters.

I<LEV_TYPE> is the vertical dimension type. It must match one of the level
types defined in the vertical dimension types section of the
parameterfile, e.g., I<plev, hybrid>, etc.

I<LEV_1>, ..., I<LEV_N> is a strictly monotonic list of level
values. 

Multi-level variables must have a vertical dimension defined.  If a
single-level (e.g., surface) variable has a default level type
specified in the parameter table, it is not necessary to call
B<set_lats> I<vertdim>, since the level type will be associated with
the variable by default. Note that the level units are determined from
the vertical dimension type table entry for name.

This command returns an integer level ID on success, or 0 on
failure.

=head2 B<set_lats> I<write VAR_ID [LEVEL]>

Specifies which variable in the file will be written next.

I<VAR_ID> is the integer variable ID returned from B<set_lats>
I<var>. 

I<LEVEL> is the level value, and must match one of the levels
declared via lats_vert_dim. (Exception: if the variable was declared
with levid=0, the value of I<LEVEL> is ignored.)  year is the four-digit
integer year, month is the month number (1-12), day is the day in
month (1-31), and hour is the integer hour (0-23). 

=head2 B<lats_grid> I<EXPR>

Define a horizontal, longitude-latitude grid, based on the dimension
environment associated with the GrADS expression I<EXPR>. The grid
type is specified with command B<set_lats> I<gridtype>.

=head2 B<lats_data> I<EXPR>

Write a horizontal longitude-latitude cross-section of a variable
contained in the GrADS expression I<EXPR>. The actual variable on file
being written to is specified with he B<set_lats> I<write> command.

For monthly data, as specified by lats_create, day and hour are
ignored. For daily and weekly data, the hour is ignored.  For fixed
data, the year, month, day, and hour are ignored. For surface
variables, lev is ignored.

This command returns 1 if successful, 0 on failure.

=head2 B<query_lats> 

Prints out the value of all internal parameters.

=head1 ENVIRONMENT VARIABLES

=over 4

=item LATS_PARMS 

Parameter table file (supersedes internal table if
defined)

=item LATS_LOG	      

Log file (default: lats.log)

=back

=head1 HISTORICAL NOTES

LATS was first introduced in GrADS v1.7 in the late 1990s by Mike
Fiorino as means of producing GRIB-1/NetCDF output from GrADS. With
the introduction of GrADS v2.0 in 2008, COLA removed LATS from the
GrADS code base. In 2009, the OpenGrADS Project reintroduced LATS in
GrADS v2.0 as a User Defiend Extension, adding support for
NetCDF-3, NetCDF-4/HDF-5 and HDF-4 all from the same executable.

=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://cookbooks.opengrads.org/index.php?title=Main_Page> -
OpenGrADS Cookbooks

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=item *

L<http://www-pcmdi.llnl.gov>  - PCMDI		      


=item *

L<http://www-pcmdi.llnl.gov/software/lats> - LATS		      

=item *

L<http://www.unidata.ucar.edu/packages/netcdf> - NetCDF	      

=item *

L<ftp://ncardata.ucar.edu/docs/grib/guide.txt> - GRIB		      

=item *

L<http://ferret.wrc.noaa.gov/noaa_coop/coop_cdf_profile.html> - COARDS	      

=item *

L<http://wesley.wwb.noaa.gov/wgrib.html> - wgrib		      

=item *

L<http://opengrads.org/doc/scripts/lats4d/> - LATS4D 

=item *

L<http://en.wikipedia.org/wiki/Ieee_float> - IEEE Float numbers

=back

=head1 AUTHORS 

Arlindo da Silva (dasilva@opengrads.org) wrote the GrADS UDXT
interface and created this dcumentation from the LATS manual page.
Mike Fiorino (mfiorino@gmail) wrote the actual LATS interface to
GrADS. Robert Drach, Mike Fiorino and Peter Gleckler (PCMDI/LLNL)
wrote the original LATS library.

=head1 COPYRIGHT

The code implementing the GrADS extension has been placed in the public domain.
Portions (C) 1996, Regents of the University of California.

See the source for copying conditions.  There is NO warranty; not even
for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

#endif
