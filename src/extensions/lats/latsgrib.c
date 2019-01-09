
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS GRIB functions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Authors:     Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 *              Mike Fiorino, Lawrence Livermore National Laboratory
 *              fiorino@pcmdi.llnl.gov
 *
 * Revision 1.7  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid
 * Version:     $Id: latsgrib.c,v 1.6 2011/10/09 20:20:49 dasilva Exp $
 *
 * Revision History:

 * Revision 1.7  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid
 *
 * $Log: latsgrib.c,v $
 * Revision 1.6  2011/10/09 20:20:49  dasilva
 * ams: minor mods for mac os x snow leopard
 *
 * Revision 1.5  2010/02/16 23:01:31  mike_fiorino
 * mods for supporting yflip and better handling of forecast_hourly forecast_minutes
 *
 * Revision 1.4  2009/10/15 01:17:53  dasilva
 * ams: work in progress
 *
 * Revision 1.3  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
 * Revision 1.2  2009/10/09 21:01:58  dasilva
 * ams: provisional gribmap fix
 *
 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.6  2008/01/19 19:24:47  pertusus
 * Use the pkgconfig result unless dap root was set.
 * change <config.h> to "config.h".
 *
 * Revision 1.5  2007/11/16 04:49:31  dasilva
 * ams: declared mons[] as static to cope what appears to be a bug in gcc 4 on darwin/intel
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.3  2004/10/12 18:04:38  administrator
 * fiddled #include statements in LATS to work with LFS on Red Hat 9 (#define _POSIX_SOURCE caused off_t to be not defined)
 *
 * Revision 1.2  2002/10/28 19:08:33  joew
 * Preliminary change for 'autonconfiscation' of GrADS: added a conditional
 * #include "config.h" to each C file. The GNU configure script generates a unique config.h for each platform in place of -D arguments to the compiler.
 * The include is only done when GNU configure is used.
 *
 * Revision 1.1.1.1  2002/06/27 19:44:14  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:56  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.22  1997/10/15 17:53:16  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.21  1996/12/18 22:06:22  fiorino
 * initialize fltpnt in gagmap.c to cover c90 compiler problem
 * added support for climatological and 365-day calendars in GrADS
 *
 * Revision 1.20  1996/12/12 18:39:45  fiorino
 * 961212
 *
 * Mike's changes to parm table
 * GraDS updated source
 * and improvements in GrADS output to handle yearly data and 365-day calendars
 * added gaprnt routine in latsfort.c to aid link with straight GraDS source
 *
 * Revision 1.19  1996/10/22  19:05:07  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.18  1996/10/16 22:10:00  drach
 * - Added automatic gribmap generation
 * - Restricted LATS_GRADS_GRIB convention to one grid per file
 *
 * Revision 1.17  1996/10/10 23:15:44  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.16  1996/09/30 18:57:26  drach
 * - Relax test for GrADS/GRIB equal-spacing
 *
 * Revision 1.15  1996/09/25 23:16:04  fiorino
 * added grib map code
 * corrected convertions of lat/lon dx/dy to ints in the GDS
 *
 * Revision 1.14  1996/08/20  18:34:08  drach
 * - lats_create has a new argument: calendar
 * - lats_grid: longitude, latitude dimension vectors are now double
 *   precision (double, C).
 * - lats_vert_dim: vector of levels is now double precision (double,
 *   C). lats_vert_dim need not be called for single-value/surface
 *   dimensions, if defined in the parameter table. Multi-valued vertical
 *   dimensions, such as pressure levels, must be defined with
 *   lats_vert_dim.
 * - lats_var: set level ID to 0 for implicitly defined surface
 *   dimension.
 * - lats_write: level value is double precision (double, C).
 * - lats_parmtab: replaces routine lats_vartab.
 * - FORTRAN latserropt added: allows program to set error handling
 *   options.
 * - The parameter file format changed.
 *
 * Revision 1.13  1996/08/12 19:19:47  drach
 * - various bug fixes (fiorino)
 *
 * Revision 1.12  1996/07/20  01:23:26  drach
 * - Restored compatible amip2.parms
 * - Provide default file comments for GrADS control file title
 *
 * Revision 1.11  1996/07/17 18:17:26  fiorino
 * outout long_name and units to var description in .ctl file
 *
 * Revision 1.10  1996/07/17  01:04:33  fiorino
 * bug in .ctl file for ydef
 *
 * Revision 1.9  1996/07/12 00:36:25  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.8  1996/06/27 01:12:27  drach
 * - Added setting timestat entry (removed from latsint.c)
 *
 * Revision 1.7  1996/06/11 21:43:33  fiorino
 * version 0.1 --  first version for Bob D to test
 *
 * Revision 1.6  1996/05/31  18:58:06  fiorino
 * v0.0
 * before the new stuff from Bob
 *
 * Revision 1.5  1996/05/10  23:06:22  fiorino
 * - test of commit
 *
 * Revision 1.4  1996/05/10  22:44:40  drach
 * - Initial version before GRIB driver added:
 * - Made grids, vertical dimensions file-independent
 *
 * Revision 1.3  1996/05/03 18:59:24  drach
 * - Moved vertical dimension definition from lats_var to lats_vert_dim
 * - Changed lats_miss_double to lats_miss_float
 * - Made time dimension file-dependent, revised lats_write accordingly
 * - Added lats_var_nc, lats_vert_dim_nc
 * - Allow GRIB-only compilation
 * - Added FORTRAN interface
 *
 * Revision 1.2  1996/04/25  23:32:05  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:53:00  drach
 * Initial repository version
 *
 *
 */

#include <stdio.h>

#define _POSIX_SOURCE 1
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "fgrib.h"
#include "fgrib_init.h"
#include "latsint.h"

#include "grads.h"
#define GRADS_CLIM_YEAR 2


/*mf 961205 --- expose Mike Fiorino's global struct to these routines for 365 day calandars mf*/
extern struct gamfcmn mfcmn;
/*mf 961205 --- expose Mike Fiorino's global struct to these routines for 365 day calandars mf*/

#define LATS_VERT_UNDEF 99

typedef struct latsgribfile {
  FILE *gfi;
  FILE *cfi;
  latsCompTime time;  /* beginning time */
  int gbmode;
  char ctlname[256];
} latsGribFile;


/*---
  prototypes
---*/

int lats_pds_set(latsFile *file, latsVar *var, 
		 int levindex, int timeindex, latsCompTime time, 
		 int fhour, int fmin,
		 grib_pds *);

int lats_gds_set(latsFile *file, latsVar *var,
		 int levindex, int timeindex, latsCompTime time, grib_gds_ll *);

void lats_flt2int(float *, int *, int);

/* Generate a time statistic for a GRIB file. Return a pointer to the entry on success,
 * 0 on failure.
 */
latsTimeStatEntry* latsTimeStatLookup(latsTimeFreq frequency, int delta, latsTimeStat statistic){
  latsTimeStatEntry *stat;

  if((stat = (latsTimeStatEntry *)malloc(sizeof(latsTimeStatEntry)))==(latsTimeStatEntry *)0)
    return 0;
  stat->frequency = frequency;
  stat->delta = delta;
  stat->stat = statistic;

  switch(frequency){
  case LATS_YEARLY:
    stat->grib_unit = 4;
    break;
  case LATS_MONTHLY:
    stat->grib_unit = 3;
    break;
  case LATS_WEEKLY:
    stat->grib_unit = 2;
    break;
  case LATS_DAILY:
    stat->grib_unit = 2;
    break;
  case LATS_HOURLY:
    stat->grib_unit = 1;
    break;
  case LATS_MINUTES:
    stat->grib_unit = 0;
    break;
  case LATS_FORECAST_HOURLY:
    stat->grib_unit = 1;
    break;
  case LATS_FORECAST_MINUTES:
    stat->grib_unit = 0;
    break;
  case LATS_FIXED:
    stat->grib_unit = 1;
    break;
  default:
    latsError("GRIB (latsgrib.c) --> Invalid frequency: %d",frequency);
    return 0;
  }
  
  stat->grib_p3 = stat->grib_p2 = stat->grib_p1 = 0;
  switch(statistic){
  case LATS_AVERAGE:
    stat->grib_p2 = delta;
    stat->grib_timerange = 3;
    break;
  case LATS_INSTANT:
    stat->grib_timerange = 0;
    break;
  case LATS_ACCUM:
    stat->grib_p2 = delta;
    stat->grib_timerange = 4;
    break;
  case LATS_OTHER_TIME_STAT:
  default:
    latsError("GRIB (latsgrib.c) --> Invalid time statistic: %d", statistic);
    return 0;
  }

  if(frequency == LATS_WEEKLY) stat->grib_p2 *= 7;
  if(frequency == LATS_FORECAST_HOURLY) stat->grib_timerange=10;
  if(frequency == LATS_FORECAST_MINUTES) stat->grib_timerange=10;

  return stat;
}

/*-------------
  Close a GRIB file. Returns 1 on success, 0 on failure.
 --------------*/

static  char *mons[] = {"jan","feb","mar","apr","may","jun","jul","aug",
	  	        "sep","oct","nov","dec"};

int lats_close_grib(latsFile *file){

  FILE *cfi;
  FILE *gfi;
  latsVar *var;
  latsVar *vara[LATS_MAX_PARMS];
  latsGribFile *gbfile;
  char gmppath[LATS_MAX_PATH];
  char gmpfile[LATS_MAX_PATH];
  char grbfile[LATS_MAX_PATH];

  char grads_options [3][50];

  int i,j,k,j1,j2,ik;
  int ni,nj,nk,nt;
  int nvara;


  float dx,dy,dum1;
  float lonb,latb,lone,late;
  int tv;

  float *glon,*glat;
  int nlev,ntype,l1,l2;
  float undef;
  int xdir=0,ydir=0,zdir=0;

  int options_set=0;
  int notaucheck; 

  gbfile = (latsGribFile *)file->depend;
  gfi=gbfile->gfi;


  if(file->convention != LATS_GRIB_ONLY) {


    /*-----------------!!!!!!!!!!!!!!!!

      Force gribmap to match by the base DTG ALWAYS
      appropriate for AMIP II, but will have to be changed for NWP

      !!!!!!!!!!!!!!!!!!-----------*/

    if(file->frequency == LATS_FORECAST_HOURLY) {
      notaucheck=0; 
    } else {
      notaucheck=1; 
    }

    /*---
      set the calendar to support 365 day calendars in LATS_GRADS_GRIB
      ---*/

    mfcmn.cal365=1;
    if( !(file->calendar & cdHasLeap) == 0) mfcmn.cal365=0;

    cfi=stdout;
    cfi=gbfile->cfi;
  
    if(VERB) {
      printf("cccccccc nvertdim    %d\n",file->nvertdim);
      printf("cccccccc nvar        %d\n",file->nvar);
      printf("cccccccc ngrid       %d\n",file->ngrid);
      printf("cccccccc ntimewriten %d\n",file->ntimewritten);
    }

    /*--- calc the grid from the first variable ----*/

    var = file->varlist;

    ni=var->grid->nlon;
    nj=var->grid->nlat;
    latb=var->grid->lats[0];
    late=var->grid->lats[nj-1];
    lonb=var->grid->lons[0];
    lone=var->grid->lons[ni-1];
  
    /*--- check if uniform --- */

/* --- single value check, set dx to 0 ---- */

    if(ni == 1) {

      dx=0.0;

    } else {
/*
 * 970517 - add fuzz to check of constant grid increment ; for cray
 */

      dx=fabs(var->grid->lons[1]-var->grid->lons[0]);
      for(i=2;i<ni;i++) {
	if( fabs((fabs(var->grid->lons[i]-var->grid->lons[i-1]) - (double)dx )) > LATS_LINEAR_GRID_DELTA*dx) {
	  dx=-1.0;
	  break;
	}
      }

    }


    if(nj == 1) {

      dy=0.0;

    } else {

      dy=fabs(var->grid->lats[1]-var->grid->lats[0]);
      for(j=2;j<nj;j++) {
	if( fabs((fabs(var->grid->lats[j]-var->grid->lats[j-1]) - (double)dy )) > LATS_LINEAR_GRID_DELTA*dy) {
	  dy=-1.0;
	  break;
	}
      }

    }

    /*----- undef is anything you like in GRIB ----*/

    undef=1e20;

    /*---
      pull out the GRIB file name
      ---*/

    j=strlen(file->path)-5;
    while( (size_t)j > 0 ) {
      if( ( *(file->path+j) == '/' ) ||  ( *(file->path+j) == '\\' ) ) {
	break;
      }
      j--;
    }

    if(j==0) {
      j1=0;
    } else {
      j1=j+1;
    }

    for(j=j1;j<strlen(file->path);j++) {
      j2=j-j1;
      *(grbfile+j2)=*(file->path+j);
    }
    *(grbfile+j2+1)='\0';


    /*---
      pull out the gribmap file name
      ---*/

    strcpy(gmppath, file->path);
    strcpy(gmppath+strlen(gmppath)-4, ".gmp");

    j=strlen(file->path)-5;

    while( (size_t)j > 0 ) {
      if( ( *(gmppath+j) == '/' ) ||  ( *(gmppath+j) == '\\' ) ) {
	break;
      }
      j--;
    }


    if(j==0) {
      j1=0;
    } else {
      j1=j+1;
    }

    for(j=j1;j<strlen(gmppath);j++) {
      j2=j-j1;
      *(gmpfile+j2)=*(gmppath+j);
    }
    *(gmpfile+j2+1)='\0';

    /*---
      now set up the data stuff in the .ctl file
      ---*/

    fprintf(cfi,"dset ^%s\n",grbfile);
    fprintf(cfi,"title %s\n",file->comments);
    fprintf(cfi,"undef %g\n",undef);
    fprintf(cfi,"dtype grib\n");
    fprintf(cfi,"index ^%s\n",gmpfile);

    /*---
      options
      ---*/

    for (i=0; i<3; i++ ) {
      grads_options[i][0] = ' ';
      grads_options[i][1] = 0;
    }

    if(latb>late) {
      ydir=1;
      latb=late;
      options_set=1;
      strcpy(grads_options[0],"yrev");
    }

    if(mfcmn.cal365) {
      options_set=1;
      strcpy(grads_options[1],"365_day_calendar");
    }
    
    if(options_set) {
      fprintf(cfi,"options %s %s %s %s\n",grads_options[0],
	      grads_options[1],grads_options[2],grads_options[3]);
    }

    /*---------- xdef ------------ */

    if(dx > 0.0) {
      fprintf(cfi,"xdef %d linear %f %f\n",ni,lonb,dx);
    } else {

      if(ni == 1) {
	fprintf(cfi,"xdef %d levels % 7.3f\n",ni,var->grid->lons[0]);
      } else {
	fprintf(cfi,"xdef %d levels\n",ni);
	if(xdir) {
	  for (i=ni-1;i>=0;i--) {
	    fprintf(cfi,"% 7.3f ",var->grid->lons[i]);
	    if((i-ni)%10 == 0) fprintf(cfi,"\n");
	  }
	} else {
	  for (i=0;i<ni;i++) {
	    fprintf(cfi,"% 7.3f ",var->grid->lons[i]);
	    if((i-1)%10 == 0) fprintf(cfi,"\n");
	  }
	}
	if(ni%10 != 0) fprintf(cfi,"\n");
	
      }
    }

    /*---------- ydef ------------ */

    if(nj == 1) {
      fprintf(cfi,"ydef %d levels % 7.3f\n",nj,var->grid->lats[0]);
    } else {

      if(dy > 0.0) {
	fprintf(cfi,"ydef %d linear %f %f\n",nj,latb,dy);
      } else {
	fprintf(cfi,"ydef %d levels\n",nj);
	if(ydir) {
	  for (j=nj-1;j>=0;j--) {
	    fprintf(cfi,"% 7.3f ",var->grid->lats[j]);
	    if((j-nj)%10 == 0) fprintf(cfi,"\n");
	  }
	} else {
	  for (j=0;j<nj;j++) {
	    fprintf(cfi,"% 7.3f ",var->grid->lats[j]);
	    if((j-1)%10 == 0) fprintf(cfi,"\n");
	  }
	}
	if(nj%10 != 0) fprintf(cfi,"\n");
      }

    }

    /*---------- zdef ---------- */

    nk=0;
    k=0;
    ik=0;
    for(i=0;i<file->nvertdim;i++) {
      if(file->vertlist[i].nlev > k) {
	k=file->vertlist[i].nlev;
	ik=i;
      }
    }

    if(k == 0) {

      fprintf(cfi,"zdef 1 levels 1013\n");

    } else {

      nk=file->vertlist[ik].nlev;
/*
 *
 *  round the level vealues to ints because GRIB 1 does not 
 *  support storing of floats in the PDS for levels
 *
 */
      if(nk == 1) {

	dum1=(float)file->vertlist[ik].levs[0]+0.5;
	fprintf(cfi,"zdef %d levels %d\n",nk,(int)dum1);

      } else {

	fprintf(cfi,"zdef %d levels\n",nk);
	if(file->vertlist[ik].levs[0] <= file->vertlist[ik].levs[nk-1]) zdir=1;
	if(zdir) {
	  for (k=nk-1;k>=0;k--) {
	    dum1=(float)file->vertlist[ik].levs[k]+0.5;
	    fprintf(cfi,"%d ",(int)dum1);
	    if((k-nk)%10 == 0) fprintf(cfi,"\n");
	  }
	} else {
	  for (k=0;k<nk;k++) {
	    dum1=(float)file->vertlist[ik].levs[k]+0.5;
	    fprintf(cfi,"%d ",(int)dum1);
	    if((k+1)%10 == 0) fprintf(cfi,"\n");
	  }
	}
	if(nk%10 != 0) fprintf(cfi,"\n");
      }

    }
    /*---------- tdef ------------ */


    if(file->frequency == LATS_FIXED) {

      fprintf(cfi,"tdef 1 linear 00Z1jan0001 1dy\n");

    } else {

      /*---
	COARDS convention for climo -> my convention for climo year
	---*/

      if( !(file->calendar & cdStandardCal) ) {
	gbfile->time.year = GRADS_CLIM_YEAR;
      }

      fprintf(cfi,"tdef %d linear %dZ%d%s%04d ",file->ndelta,
	      (int)gbfile->time.hour,
	      gbfile->time.day,
	      mons[gbfile->time.month-1],
	      (int) gbfile->time.year );

      if(VERB) {
	printf(">>>>> t=%d, month is <%s>\n", gbfile->time.month-1,
	       mons[gbfile->time.month-1]);
      }

      if(file->frequency == LATS_HOURLY)  fprintf(cfi," %dhr\n",file->delta);
      if(file->frequency == LATS_MINUTES)  fprintf(cfi," %dmn\n",file->delta);
      if(file->frequency == LATS_FORECAST_MINUTES)  fprintf(cfi," %dmn\n",file->delta);
      if(file->frequency == LATS_FORECAST_HOURLY)  fprintf(cfi," %dhr\n",file->delta);
      if(file->frequency == LATS_DAILY)   fprintf(cfi," %ddy\n",file->delta);
      if(file->frequency == LATS_WEEKLY)  fprintf(cfi," %ddy\n",file->delta*7);
      if(file->frequency == LATS_MONTHLY) fprintf(cfi," %dmo\n",file->delta);
      if(file->frequency == LATS_YEARLY) fprintf(cfi," %dyr\n",file->delta);

    }

    /*----------- vars ------------ */

    fprintf(cfi,"vars %d\n",file->nvar);
/*
 *
 * save var struct so we can reverse order (the way we defined the vars)
 * to the ctl file
 *
 */
    nvara=file->nvar-1;
    for(var = file->varlist; var; var = var->next){
      vara[nvara]=var;
      nvara--;
    }

/*
 * original version
 *
 *   for(var = file->varlist; var; var = var->next){
 *
 *  write out the vars the way we defined them
 */

    for( nvara=0 ; nvara < file->nvar ; nvara++ ){
      var=vara[nvara];

      /* If the vertical dimension was declared explicitly
	 via lats_vert_dim ...*/
      if(var->vertdim != NULL){
	ntype=var->vertdim->type->gribid;
	l1=var->vertdim->type->grib_p1;
	l2=var->vertdim->type->grib_p2;
      }
      /* Else if the vertical dimension is implicit, via the
	 leveltype of the variable in the parameter table ...*/
      else if(var->parm->levelset == 1){
	ntype=var->parm->verttype->gribid;
	if(var->parm->verttype->grib_p3 != 0) {
	  l1=var->parm->verttype->grib_p3;
	  l2=0;
	} else {
	  l1=var->parm->verttype->grib_p1;
	  l2=var->parm->verttype->grib_p2;
	}
      }
      /* Else no vertical dimension */
      else{
	ntype=LATS_VERT_UNDEF;
	l1=0;
	l2=0;
      }

      if(var->parm->levelset == 1 ) {
	nlev=0;

	if(var->parm->verttype->grib_p3 != 0) {
	  fprintf(cfi,"%-8s %2d  %3d,%3d,%3d %s [%s]\n",
		  var->name,nlev,var->parm->id,
		  ntype,l1,  /*----- set above -----*/
		  var->parm->title,var->parm->units);
	} else {
	  fprintf(cfi,"%-8s %2d  %3d,%3d,%3d,%3d %s [%s]\n",
		  var->name,nlev,var->parm->id,
		  ntype,l1,l2,
		  var->parm->title,var->parm->units);
	}

      } else {

	fprintf(cfi,"%-8s %2d  %3d,%3d %s [%s]\n",
		var->name,var->nlev,var->parm->id,ntype,
		var->parm->title,var->parm->units);

      } 
    
    }

    fprintf(cfi,"endvars\n");

    /*------ close ------*/

    fclose(gfi);
    fclose(cfi);

    /*----- run gribmap ----*/

    latsgribmap(gbfile->ctlname,notaucheck);

    return 1;


    /*--- only create GRIB data ---- */

  } else {

    fclose(gfi);
    return 1;
    
  }

}

/*----------------------------------------
 *
 * Create a GRIB file. Returns the file ID, or 0 on error
 *
 ---------------------------------------*/

int lats_create_grib(latsFile *file){
  FILE *gfi;
  FILE *cfi;
  latsGribFile *gbfile;
  char *gname;
  int j;

  gname = (char *)malloc(strlen(file->path)+5);
  if(gname == NULL) {
    latsError("GRIB (latsgrib.c) --> Allocating memory for GRIB file name %s", file->path);
    return 0;
  }
  
  j=0;
  while( (size_t)j < strlen(file->path) ) {
    *(gname+j) = *(file->path+j);
    j++;
  }
  *(gname+j) = '\0';

  if((gbfile = (latsGribFile *)malloc(sizeof(latsGribFile)))==0){
    latsError("GRIB (latsgrib.c) --> Allocating memory for GRIB file name %s", file->path);
    return 0;
  }

  gfi=fopen(gname,"wb") ;
  if(gfi == NULL) {
    latsError("GRIB (latsgrib.c) --> lats_create create .grb failed for %s\n",gname);
    return 0;
  }

  if(file->convention != LATS_GRIB_ONLY) {
					     /* Make sure that GrADS control file has a title */
    if(strlen(file->comments)==0)
      strcpy(file->comments, file->center);

    j=0;
    while( (size_t)j < strlen(file->path) ) {
      *(gname+j) = *(file->path+j);
      j++;
    }
    *(gname+j) = '\0';
      strcpy(gname+strlen(gname)-4,".ctl");
      
      cfi=fopen(gname,"w") ;
      if(cfi == NULL) {
	latsError("GRIB (latsgrib.c) --> lats_create create .ctl failed for %s\n",gname);
	return 0;
      }

  } else {

    cfi=NULL;

  }
  
/*------ initialize the first time flag in gbfile struct */

  gbfile->gbmode=1;
  gbfile->gfi=gfi;
  gbfile->cfi=cfi;
  strcpy(gbfile->ctlname,gname);
  free(gname);


  file->depend = (void *)gbfile;


  return file->id;

}


/*----------------------------------------
 * Define a grid for a GRIB 'file'.
 * Return 1 on success, 0 on failure;
 * 
 * Note: this routine is called by lats_var for the
 *   first variable which is defined on this grid.
 *
 * Note: This routine is a no-op 
 -----------------------------------------*/
int lats_grid_grib(latsFile *file, latsGrid *grid){
	if(file->ngrid > 1 && file->convention==LATS_GRADS_GRIB){
		latsError("GRIB (latsgrib.c) --> Only one grid per file supported in the GrADS/GRIB convention; file: %s, grid: %s",
			  file->path, grid->name);
		return 0;
	}
	return 1;
}

/* Define a variable to be written to a GRIB file.
 * 'grid' is the grid structure.
 * 'vertdim' is the vertical dimension structure, or 0 no level.
 * Return the variable ID on success, 0 on failure.
 */

int lats_var_grib(latsFile *file, latsVar *var, latsGrid *grid, latsVertDim *vertdim){

  return var->id;
}

/*----------------------------------------
 *
 * Write a vertical dimension 'vertdim' to GRIB 'file'.
 * Return dimension ID on success, 0 on failure.
 *
 -----------------------------------------*/
int lats_vert_dim_grib(latsFile *file, latsVertDim *vertdim) {

  if(file->latsmode != LATS_MODE_DEFINE){
    latsError("GRIB (latsgrib.c) --> lats_vert_dim calls must precede any lats_write call");
    return 0;
  }

  return file->nvertdim;
}

/*----------------------------------------
 *
 * Write a horizontal lon-lat section 'data' for variable 'var' to GRIB 'file'.
 * 'levindex' is the 0-origin index, into var->levs, of the level value, or -1 if there are no levels.
 * 'timeindex' is the 0-origin index of the time value, or -1 if there are no times.
 * Return 1 on success, 0 on failure.
 *
 -----------------------------------------*/

int lats_write_grib(latsFile *file, latsVar *var, 
		    int levindex, int timeindex, latsCompTime time, int fhour, int fmin, void *data) {

  FILE *gfi;
  int i,j,k;
  float undef, undef_delta;
  int rc,rcp,rcg,rcb;
  int glen;
  float *dataf;
  latsGribFile *gbfile;
  
  
/*---
  GRIB section pointers
---*/

  grib_is *is;
  grib_pds *pds;
  grib_gds_ll *gds;  
  grib_bms *bms;
  grib_bds *bds;
  grib_es *es;

/*---
  point to  the default static GRIB section structs
---*/

  is=&FGRIBAPI_is;
  pds=&FGRIBAPI_pds;
  gds=&FGRIBAPI_gds;
  bms=&FGRIBAPI_bms;
  bds=&FGRIBAPI_bds;
  es=&FGRIBAPI_es;

  pds->pds=(unsigned char *) malloc(pds->len);

/*---
  initial checks
---*/

  if(var->parm->datatype == LATS_INT) {

    lats_flt2int(data,data,(var->grid->nlon*var->grid->nlat));

/*    return 100;  */
  }

/*---
  point to the GRIB file struct
---*/

  gbfile = (latsGribFile *)file->depend;

  gfi=gbfile->gfi;
    
  if(VERB) {
    printf("vvv %s\n",var->name);
    if(var->parm->datatype == LATS_FLOAT) printf("vvv DATA TYPE is float\n");
    if(var->parm->datatype == LATS_INT) printf("vvv DATA TYPE is int\n");
    printf("vvv GRIB id = %d\n",var->parm->id);
    printf("vvv GRIB dsf = %d\n",var->scalefac);
    printf("vvv GRIB bits per grid point = %d\n",var->precision);
  }

/*---
 * undef values
 ---*/

  if(var->hasmissing) {

    if(var->parm->datatype == LATS_FLOAT) {
      if(VERB) {
	printf("vvv %g missing float\n",var->missing.f);
	printf("vvv %g missing delta float\n",var->missingdelta);
      }
      undef=var->missing.f;
      undef_delta = var->missingdelta;
    }

    if(var->parm->datatype == LATS_INT) {
      if(VERB) {
	printf("vvv %d missing integer\n",var->missing.i);
	printf("vvv %g missing delta integer\n",var->missingdelta);
      }
      undef=(float)var->missing.i;
      undef_delta = 0.0;
    }

  } else {
    if(VERB) printf("vvv NO MISSING DATA\n");
  }


/*---
 * create the PDS, GDS and BDS (BMS)
 ---*/

  rcp=lats_pds_set(file,var,levindex,timeindex,time,fhour,fmin,pds) ;
  rcg=lats_gds_set(file,var,levindex,timeindex,time,gds) ;
  rcb=bds_set(data,pds,bds,bms,undef,gds->ni*gds->nj,pds->nbits,var->hasmissing,undef_delta);

  if(rcp) {
    latsError("GRIB (latsgrib.c) --> bds_set failed rc = %d\n",rcp);
    return 0;
  } else if(rcg) {
    latsError("GRIB (latsgrib.c) --> gds_set failed rc = %d\n",rcg);
    return 0;
  } else if(rcb) {
    latsError("GRIB (latsgrib.c) --> bds_set failed rc = %d\n",rcb);
    return 0;
  } 

  glen=is->len + pds->len + gds->len + bms->len + bds->len + es->len ;

  if(VERB) printf("vvv glen %d %d %d %d %d %d\n",
		  glen,is->len,pds->len,gds->len,bms->len,bds->len,es->len);

  rc=is_set(is,glen) ;
  if(rc) {
    latsError("GRIB (latsgrib.c) --> is_set failed rc = %d\n",rc);
    return 0;
  }

/*---
 * write out the sections
 ---*/

  rc=fwrite(is->is,sizeof(char),is->len,gfi) ;
  rc=fwrite(pds->pds,sizeof(char),pds->len,gfi) ;
  rc=fwrite(gds->gds,sizeof(char),gds->len,gfi) ;
  if(bms->len) rc=fwrite(bms->bms,sizeof(char),bms->len,gfi) ;
  rc=fwrite(bds->bds,sizeof(char),bds->len,gfi) ;
  rc=fwrite(es->es,sizeof(char),es->len,gfi) ;

  if(gbfile->gbmode) {
    gbfile->time=time;
    gbfile->gbmode=0;
  }

  free(pds->pds);
  free(gds->gds);

  return 1;

}

int is_set(grib_is *is, unsigned int glen) {
  set_int3(&is->is[4],glen);
  is->is[7]=1;
  return 0;
}

/*-----------------------------------
 *
 *  LATS GRIB PDS routine
 *
 -------------------------------------*/

int lats_pds_set(latsFile *file, latsVar *var, 
		 int levindex, int timeindex, latsCompTime time, 
		 int fhour, int fmin,
		 grib_pds *pds) {

  unsigned char cent,yr,mo,da,hr,mn;
  float dum1;

/*---
  time processing
---*/

  if(file->frequency == LATS_FORECAST_HOURLY || file->frequency == LATS_FORECAST_MINUTES) {

    cent=(unsigned char)(((int)((float)(file->btime.year*0.01)+0.001))+1);
    yr=(unsigned char)(((float)file->btime.year-((float)cent-1.0)*100.0)+0.1);
    mo=(unsigned char)(file->btime.month);
    da=(unsigned char)(file->btime.day);
    hr=(unsigned char)((int)file->btime.hour+0.1);
    mn=(unsigned char)((int)file->btime.min+0.1);


  } else {

    cent=(unsigned char)(((int)((float)(time.year*0.01)+0.001))+1);
    yr=(unsigned char)(((float)time.year-((float)cent-1.0)*100.0)+0.1);
    mo=(unsigned char)(time.month);
    da=(unsigned char)(time.day);
    hr=(unsigned char)((int)time.hour+0.1);
    mn=(unsigned char)((int)time.min+0.1);

  }

  /*mf 970820
    --- correct coding for last year of the century is: 

    year=100
    century=century-1

    mf*/

  if(yr == 0) {
    yr=100;
    cent=cent-1;
  }


/*---
  case with 0 start year -- set to 1776
---*/
/*DDDD 970122 disable because lats.c should prevent this */
/*
  if(yr == 0 && cent == 0) {
    cent=( GRADS_CLIM_YEAR / 100 ) + 1;
    yr=GRADS_CLIM_YEAR - ( ( GRADS_CLIM_YEAR / 100 ) * 100 ) ;
  }
*/

/*---
  set time to the default for fixed data irregardless of the input time
---*/
/*DDDD 970122 disable because lats.c should prevent this */
/*
  if(file->frequency == LATS_FIXED ) {

    cent=1;
    yr=1;
    mo=1;
    da=1;
    hr=0;
mn=0;
  }
*/

/*---
  set time to the default for climo data irregardless of the input time
---*/

  if( !(file->calendar & cdStandardCal) && !(file->frequency == LATS_FIXED) ) {
    cent=( GRADS_CLIM_YEAR / 100 ) + 1;
    yr=GRADS_CLIM_YEAR - ( ( GRADS_CLIM_YEAR / 100 ) * 100 ) ;
  }

  if(VERB) {
    printf("------- %d %d %d %d %d %d\n",cent,yr,mo,da,hr,mn)  ;
    printf("ttt %d %d %d %f %f\n",(int)time.year,time.month,time.day,time.hour,time.min);
  }

/*
   ------------ set the pds
*/

  pds->ver=128;                    /* our own table version */
  pds->ctr=file->grib_center;      /* from table */
  pds->sctr=file->grib_subcenter;  /* subcenter is AMIP II */
  pds->proc=file->centerid;        /* dummy model */

  pds->parm=var->parm->id;
					     /* Note: Get scalefac, precision from var->..., */
					     /* since these have correct values if parameter */
					     /* table has been overridden (e.g., for monthly data) */

  pds->dsf=var->scalefac;
  pds->nbits=var->precision;
  if(pds->dsf == -999 && pds->nbits == -999)
	  pds->nbits = DEFAULT_NBITS;	     /* If no precision or decimal scale factor set,
					      use default number of bits. */
/*
   ----------- vertical defintion
*/

					     /* If the vertical dimension was declared explicitly
					      via lats_vert_dim ...*/
  if(var->vertdim != NULL){

      pds->ltyp=var->vertdim->type->gribid;
      dum1=(float)var->vertdim->levs[levindex]+0.5;
      pds->l12=(int)dum1;

/*      
      if(pds->ltyp == 100) {
	pds->l12=var->vertdim->levs[levindex];
      } else {
	pds->l1=var->vertdim->type->grib_p1;
	pds->l2=var->vertdim->type->grib_p2;
	pds->l12=var->vertdim->type->grib_p3;
      }
*/
      
  }
					     /* Else if the vertical dimension is implicit, via the
					      leveltype of the variable in the parameter table ...*/
  else if(var->parm->levelset == 1){
      pds->ltyp=var->parm->verttype->gribid;
      pds->l1=var->parm->verttype->grib_p1;
      pds->l2=var->parm->verttype->grib_p2;
      pds->l12=var->parm->verttype->grib_p3;
  }
					     /* Else no vertical dimension */
  else {
      pds->ltyp=LATS_VERT_UNDEF;
      pds->l1=0;
      pds->l2=0;
  }

  pds->cent=cent;
  pds->yr=yr;
  pds->mo=mo;
  pds->da=da;
  pds->hr=hr;
  pds->mn=mn;

if(var->timestat == NULL ) {
  printf("Sayoonara, where out of here, timestat not properly set\n");
  printf("contact fiorino@llnl.gov\n");
  exit (6969);
}

/*---
   -------- the time is defined in latsTimeStatLookup
---*/

  pds->ftu=var->timestat->grib_unit;  
  pds->p1=var->timestat->grib_p1;
  pds->p2=var->timestat->grib_p2;
  pds->p12=var->timestat->grib_p3;
  pds->tri=var->timestat->grib_timerange;

/*
 *    forecast hour uses time range of of 10 from latsTimeStatLookup
 *    set the forecast hour in p12
 */

  if(file->frequency == LATS_FORECAST_HOURLY) {
    pds->p12=fhour;
  } else if( file->frequency == LATS_FORECAST_MINUTES) {
    pds->p12=fmin;
  }


/*----
  if climo set time range indicator to 51
---*/

  if( (file->calendar & cdClimCal) ) pds->tri=51;

/*--- 
  CREATE THE PDS
----*/

  set_int3(&pds->pds[0],pds->len);

  pds->pds[3]=pds->ver;
  pds->pds[4]=pds->ctr;
  pds->pds[5]=pds->proc;
  pds->pds[6]=pds->grid;

  pds->pds[7]=0;
  if(pds->gflg) SETBIT(pds->pds[7],7);
					     /* By default, no bit map section
					        pds->bflg is set in bds_set.
					      */
/*   if(pds->bflg) SETBIT(pds->pds[7],6);
 */

  pds->pds[8]=pds->parm;
  pds->pds[9]=pds->ltyp;

/*
 *
 * strict GRIB standard  - Gospel according to John
 *
 */

/*
  if( (pds->ltyp == 100) || 
     (pds->ltyp == 103) ||
     (pds->ltyp == 107) ||
     (pds->ltyp == 105) ||
     (pds->ltyp == 109) ||
     (pds->ltyp == 111) ||
     (pds->ltyp == 113) ||
     (pds->ltyp == 115) ||
     (pds->ltyp == 125) ||
     (pds->ltyp == 160) )   {

 */

/*
 *
 *  looser LATS standard
 *
 */

  if(var->vertdim != NULL ||
     (var->parm->levelset==1 && var->parm->verttype->grib_p3 != 0)){
     set_int2(&pds->pds[10],((int)pds->l12+0.5));
  } else {
    pds->pds[10]=pds->l1;
    pds->pds[11]=pds->l2;
  }

  pds->pds[12]=pds->yr;
  pds->pds[13]=pds->mo;
  pds->pds[14]=pds->da;
  pds->pds[15]=pds->hr;
  pds->pds[16]=pds->mn;
  pds->pds[17]=pds->ftu;

  if( (pds->tri == 10) ) { 
    set_int2(&pds->pds[18],pds->p12);
  } else {
    pds->pds[18]=pds->p1;
    pds->pds[19]=pds->p2;
  }
  pds->pds[20]=pds->tri;
  set_int2(&pds->pds[21],pds->nave);
  pds->pds[23]=pds->nmis;
  pds->pds[24]=pds->cent;
  pds->pds[25]=pds->sctr;
  set_int2(&pds->pds[26],(unsigned int)abs(pds->dsf));
  if(pds->dsf<0) SETBIT(pds->pds[26],7);

  if(VERB) {

    printf("PPP (1-3)      %d\n",pds->len);
    printf("PPP 4          %d %d\n",pds->pds[3],pds->ver);
    printf("PPP 5          %d %d\n",pds->pds[4],pds->ctr);
    printf("PPP 6          %d %d\n",pds->pds[5],pds->proc);
    printf("PPP 7          %d %d\n",pds->pds[6],pds->grid);
    printf("PPP 8 B1       %d %d\n",pds->pds[7],pds->gflg);
    printf("PPP 8 B2       %d %d\n",pds->pds[7],pds->bflg);
    printf("PPP 9          %d %d\n",pds->pds[8],pds->parm);
    printf("PPP 10         %d %d\n",pds->pds[9],pds->ltyp);
    printf("PPP (11-12)    %d %d %d %d %d\n",
	   pds->pds[10],pds->pds[11],pds->l1,pds->l2,pds->l12);
    printf("PPP 13         %d %d\n",pds->pds[12],pds->yr);
    printf("PPP 14         %d %d\n",pds->pds[13],pds->mo);
    printf("PPP 15         %d %d\n",pds->pds[14],pds->da);
    printf("PPP 16         %d %d\n",pds->pds[15],pds->hr);
    printf("PPP 17         %d %d\n",pds->pds[16],pds->mn);
    printf("PPP 18         %d %d\n",pds->pds[17],pds->ftu);
    printf("PPP (19-20)    %d %d %d %d %d\n",
	   pds->pds[18],pds->pds[19],pds->p1,pds->p2,pds->p12);
    printf("PPP 21         %d %d\n",pds->pds[20],pds->tri);
    printf("PPP (22-23)    %d\n",pds->nave);
    printf("PPP 24         %d %d\n",pds->pds[23],pds->nmis);
    printf("PPP 25         %d %d\n",pds->pds[24],pds->cent);
    printf("PPP 26         %d %d\n",pds->pds[25],pds->sctr);
    printf("PPP (27-28)    %d\n",INT2(pds->pds[26],pds->pds[27]));

  }

  return 0;
}


/*-----------------------------------
 *
 *  LATS GRIB GDS routine
 *
 -------------------------------------*/

int lats_gds_set(latsFile *file, latsVar *var, 
		    int levindex, int timeindex, latsCompTime time, grib_gds_ll *gds) {


  int i,j,k;
  int ni,nj;
  float dx,dy, dtmp;
  float lonb,latb,lone,late;
  int tv;

  double *glon,*glat;
  int ndx;

  float undef;
  /*---
    grid type
    ---*/

  ni=var->grid->nlon;
  nj=var->grid->nlat;
  latb=var->grid->lats[0];
  late=var->grid->lats[nj-1];
  lonb=var->grid->lons[0];
  lone=var->grid->lons[ni-1];
  
  /*--- check if uniform --- */
					     /* Note: lats[], lons[] were checked for
						strict monotonicity in lats_grid/latsCheckMono
					      */

  dx=fabs(var->grid->lons[1]-var->grid->lons[0]);
  dy=fabs(var->grid->lats[1]-var->grid->lats[0]);
  for(i=2;i<ni;i++) {
	  dtmp = fabs(var->grid->lons[i]-var->grid->lons[i-1]);
	  if(dtmp<=(1.0-LATS_LINEAR_GRID_DELTA)*dx || dtmp>=(1.0+LATS_LINEAR_GRID_DELTA)*dx){
		  dx=-1.0;
		  break;
	  }
  }

  for(j=2;j<nj;j++) {
	  dtmp = fabs(var->grid->lats[j]-var->grid->lats[j-1]);
	  if(dtmp<=(1.0-LATS_LINEAR_GRID_DELTA)*dy || dtmp>=(1.0+LATS_LINEAR_GRID_DELTA)*dy){
		  dy=-1.0;
		  break;
	  }
  }

/*---
 * check if grid OK
 *--*/

  if(lonb>lone) {
    latsError("GRIB (latsgrib.c) --> has longitude DECREASING with increasing index");
    return 999;
  }


  if( (var->grid->type == LATS_LINEAR) 
     && ( (dx < 0.0) || (dy < 0.0) ) ) {
    latsError("GRIB (latsgrib.c) --> grid classified as LATS_LINEAR but dx and dy are NOT constant");
    return 999;
  }

  if(VERB) {

    if(var->grid->type == LATS_GAUSSIAN) {
      printf("ggg TYPE a gauss grid\n");
      printf("ggg dx = %g\n",dx);
    }

    if(var->grid->type == LATS_LINEAR) {
      printf("ggg TYPE uniform lat/lon grid\n");
      printf("ggg dx, dy = %g %g\n",dx,dy);
    }
  
    if(var->grid->type == LATS_GENERIC) printf("ggg TYPE generic grid\n");

  }


/*---
  drt=0 ---- data representation type for equidistant lon/lat grid
---*/

  if(var->grid->type == LATS_LINEAR) {

    gds->nv=0;
    gds->pv=255;
    gds->drt=0;
    gds->ni=ni;
    gds->nj=nj;
    gds->len=32;

/*---
  drt=4 ---- Gaussian Grids
---*/

  } else if(var->grid->type == LATS_GAUSSIAN) {
    gds->nv=0;
    gds->pv=255;
    gds->drt=4;
    gds->len=32;
    gds->ni=ni;
    gds->nj=nj;
    dy=(int)(((gds->nj)/2));

/*---
  drt=220 ----  PCMDI CONVENTION FOR generic lon/lat grids
---*/
  } else if(var->grid->type == LATS_GENERIC) {
    gds->nv=0;
    gds->pv=255;
    gds->drt=220;
    gds->ni=ni;
    gds->nj=nj;
    gds->len=24 + ((ni)*3) + ((nj)*3);
    /*    printf("generic lat/lon GRID gds len = %d\n",gds->len); */
  }

/*----
 * CREATE THE GDS
 ----*/

  gds->gds=(unsigned char *) malloc(gds->len);
  set_int3(&gds->gds[0],gds->len);

  gds->gds[3]=gds->nv;
  gds->gds[4]=gds->pv;
  gds->gds[5]=gds->drt;
  set_int2(&gds->gds[6],gds->ni);
  set_int2(&gds->gds[8],gds->nj);
  gds->lat1=(int)(fabs(latb*1000.0)+0.5);
  gds->lon1=(int)(fabs(lonb*1000.0)+0.5);
  if(latb<0) gds->lat1=-gds->lat1;
  if(lonb<0) gds->lon1=-gds->lon1;
  set_int3(&gds->gds[10],(unsigned int)abs(gds->lat1));
  if(latb<0) SETBIT(gds->gds[10],7);
  set_int3(&gds->gds[13],(unsigned int)abs(gds->lon1));
  if(lonb<0) SETBIT(gds->gds[13],7);
  
  gds->gds[16]=0;
  gds->rcdi=1; /* dx and dy will be specfied for this grid */
  if(gds->rcdi) SETBIT(gds->gds[16],7);
  if(gds->rcre) SETBIT(gds->gds[16],6);
  if(gds->rcuv) SETBIT(gds->gds[16],3);

  gds->lat2=(int)(late*1000+0.5);
  gds->lon2=(int)(lone*1000+0.5);
  set_int3(&gds->gds[17],(unsigned int)abs(gds->lat2));
  if(late<0) SETBIT(gds->gds[17],7);
  set_int3(&gds->gds[20],(unsigned int)abs(gds->lon2));
  if(lone<0) SETBIT(gds->gds[20],7);

  if(gds->drt != 220) {
    gds->dx=(int)(dx*1000+0.5);
    gds->dy=(int)(dy*1000+0.5);
    set_int2(&gds->gds[23],(unsigned int)abs(gds->dx));
    if(dx<0) SETBIT(gds->gds[23],7);
    set_int2(&gds->gds[25],(unsigned int)abs(gds->dy));
    if(dy<0) SETBIT(gds->gds[25],7);

/*---  scan mode flags  
    (lon,lat) grids only
    (lone > lonb ) (xrev not supported)
----*/

    gds->gds[27]=0;

/*------------------ bug in setting scan mode -- wrong bit
mf 20080619
--- */
    if(latb < late) gds->smj=1 ;

    if(gds->smi) SETBIT(gds->gds[27],7);
    if(gds->smj) SETBIT(gds->gds[27],6);
    if(gds->smdir) SETBIT(gds->gds[27],2);


  } else {

/*---
 * code the lon and lats
 ---*/

    glon=var->grid->lons;
    glat=var->grid->lats;
    ndx=23;

    for(i=0;i<ni;i++) {
      
      set_int3(&gds->gds[ndx],(unsigned int)abs((int)(*(glon+i)*1000+0.5)));
        if(*(glon+i)<0) SETBIT(gds->gds[ndx],7);
      if(VERB) printf("lon....... gggg %d %d %f\n",i,ndx,*(glon+i));
      ndx+=3;

    }

    for(j=0;j<nj;j++) {
      
      set_int3(&gds->gds[ndx],(unsigned int)abs((int)(*(glat+j)*1000+0.5)));
      if(*(glat+j)<0) SETBIT(gds->gds[ndx],7);
      if(VERB) printf("lat....... gggg %d %d %f\n",j,ndx,*(glat+j));
      ndx+=3;

    }

  }

  if(VERB) {
    printf("GGG (1-3)      %d\n",gds->len);
    printf("GGG 4          %d\n",gds->gds[3]);
    printf("GGG 5          %d\n",gds->gds[4]);
    printf("GGG 6          %d\n",gds->gds[5]);
    printf("GGG (7-8)      %d\n",gds->ni);
    printf("GGG (9-10)     %d\n",gds->nj);
    printf("GGG (11-13)    %d\n",INT3(gds->gds[10],gds->gds[11],gds->gds[12]));
    printf("GGG (14-16)    %d\n",INT3(gds->gds[13],gds->gds[14],gds->gds[15]));
    printf("GGG 17         %d\n",gds->gds[16]);
    printf("GGG (18-20)    %d\n",INT3(gds->gds[17],gds->gds[18],gds->gds[19]));
    printf("GGG (21-23)    %d\n",INT3(gds->gds[20],gds->gds[21],gds->gds[22]));
    if(gds->drt != 220) {
      printf("GGG (24-25)    %d\n",INT2(gds->gds[23],gds->gds[24]));
      printf("GGG (26-27)    %d\n",INT2(gds->gds[25],gds->gds[26]));
      printf("GGG 28         %d\n",gds->gds[27]);
    } else {

    }
  }

  return 0;

}

void lats_flt2int(float *dataf, int *datai, int npts) {

  float f1;
  int i1;
  int i;

  for(i=0;i<npts;i++) {
    i1=*(datai+i);
    f1=(float)i1;
    *(dataf+i)=f1;
  }   

}
