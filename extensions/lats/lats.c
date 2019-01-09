
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

/* -*-Mode: C;-*-
 * Module:      LATS user API
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: lats.c,v 1.16 2011/10/09 20:20:49 dasilva Exp $
 * Version:     $Id: lats.c,v 1.16 2011/10/09 20:20:49 dasilva Exp $
 *
 * Revision History:
 *
 * $Log: lats.c,v $
 * Revision 1.16  2011/10/09 20:20:49  dasilva
 * ams: minor mods for mac os x snow leopard
 *
 * Revision 1.15  2010/10/03 14:28:31  dasilva
 * ams: removed stray ;
 *
 * Revision 1.14  2010/02/16 23:01:31  mike_fiorino
 * mods for supporting yflip and better handling of forecast_hourly forecast_minutes
 *
 * Revision 1.13  2009/10/15 03:42:42  dasilva
 * ams: working in progress, still cannot run twice in a row
 *
 * Revision 1.12  2009/10/15 02:21:00  dasilva
 * ams: added missing break;
 *
 * Revision 1.11  2009/10/15 01:44:35  dasilva
 * ams: work in progress
 *
 * Revision 1.10  2009/10/15 01:17:52  dasilva
 * ams: work in progress
 *
 * Revision 1.9  2009/10/15 00:25:37  dasilva
 * ams: work in progress
 *
 * Revision 1.8  2009/10/14 22:19:05  dasilva
 * ams: work in progress
 *
 * Revision 1.7  2009/10/14 22:16:58  dasilva
 * ams: work in progress
 *
 * Revision 1.6  2009/10/14 04:00:32  dasilva
 * ams: work in progress
 *
 * Revision 1.5  2009/10/13 04:22:39  dasilva
 * ams: nc-4 compression seems to work
 *
 * Revision 1.4  2009/10/13 04:07:58  dasilva
 * ams: nc-4 compression seems to work
 *
 * Revision 1.3  2009/10/13 03:34:20  dasilva
 * ams: work in progress
 *
 * Revision 1.2  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
* Revision 1.10  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid
  * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.9  2008/01/19 19:24:47  pertusus
 * Use the pkgconfig result unless dap root was set.
 * change <config.h> to "config.h".
 *
 * Revision 1.8  2007/11/12 14:44:46  dasilva
 * ams: fixed bug reported by Mike Fiorino
 *
 * Revision 1.7  2007/10/19 13:07:31  pertusus
 * Compilation fixes from Graziano Giuliani.
 *
 * Revision 1.6  2007/08/26 23:32:03  pertusus
 * Add standard headers includes.
 *
 * Revision 1.5  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.2  2002/10/28 19:08:33  joew
 * Preliminary change for 'autonconfiscation' of GrADS: added a conditional
 * #include "config.h" to each C file. The GNU configure script generates a unique config.h for each platform in place of -D arguments to the compiler.
 * The include is only done when GNU configure is used.
 *
 * Revision 1.1.1.1  2002/06/27 19:44:12  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:56  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.17  1997/10/15 17:53:13  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.3  1997/02/14 20:11:52  fiorino
 * before latsmode change
 *
 * Revision 1.2  1997/01/17  00:38:03  fiorino
 * with latsTimeCmp bug diagnostics
 *
 * Revision 1.1  1997/01/03 00:30:15  fiorino
 * Initial revision
 *
 * Revision 1.16  1996/12/18 22:06:20  fiorino
 * initialize fltpnt in gagmap.c to cover c90 compiler problem
 * added support for climatological and 365-day calendars in GrADS
 *
 * Revision 1.15  1996/11/11 22:39:18  drach
 * - Added function to set the basetime (lats_basetime)
 *
 * Revision 1.14  1996/10/22 19:05:02  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.13  1996/10/21 17:20:15  drach
 * - Fixed LATS_MONTHLY_TABLE_COMP option
 *
 * Revision 1.12  1996/10/10 23:15:43  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.11  1996/08/27 19:40:21  drach
 * - Corrected generation of basetime string for climatologies
 *
 * Revision 1.10  1996/08/20 18:34:06  drach
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
 * Revision 1.9  1996/07/12 00:36:25  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.8  1996/06/27 01:07:02  drach
 * - Added QC weights calculation
 *
 * Revision 1.7  1996/05/25 00:27:47  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.6  1996/05/11 00:08:01  fiorino
 * - Added COARDS compliance
 *
 * Revision 1.5  1996/05/10  22:44:38  drach
 * - Initial version before GRIB driver added:
 * - Made grids, vertical dimensions file-independent
 *
 * Revision 1.4  1996/05/04 01:11:09  drach
 * - Added name, units to lats_vert_dim
 * - Added missing data attribute (latsnc.c)
 *
 * Revision 1.3  1996/05/03 18:59:23  drach
 * - Moved vertical dimension definition from lats_var to lats_vert_dim
 * - Changed lats_miss_double to lats_miss_float
 * - Made time dimension file-dependent, revised lats_write accordingly
 * - Added lats_var_nc, lats_vert_dim_nc
 * - Allow GRIB-only compilation
 * - Added FORTRAN interface
 *
 * Revision 1.2  1996/04/25  23:32:03  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:52:59  drach
 * Initial repository version
 *
 *
 */

#define _POSIX_SOURCE 1
#define MAX(a,b) ((a)>(b)?(a):(b))
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "latsint.h"
#include "latstime.h"

#ifdef GOT_NETCDF
# include "netcdf.h"
#endif

char *_lats_routine_name_ = (char *)0;

static int latsParmTableCreated = 0;	     /* 1 iff parameter table has been created */

#include "galats.h"               /*ams not very kosher, but simplifies implementation ams*/
extern struct galats glats ;      /* LATS option struct */

/* Set the basetime for file 'fileid' Return 1 on success, 0 on failure */
int lats_basetime(int fileid, int year, int month, int day, int hour, int min) {

	latsFile *file;
	latsCompTime time;
	
	_lats_routine_name_ = "lats_basetime";
	
	if((file = latsFileLookup(fileid))==0) {
		return 0;
	}
	
	if(file->latsmode != LATS_MODE_DEFINE && file->ntimewritten > 0){
		latsError("LATS (lats.c) --> lats_basetime calls must precede any lats_write call");
		return 0;
	}
	
	/*mf---
	  check if climo field for GRIB options
	  set year to 2 it will be overwritten in latsgrib.c by GRADS_CLIM_YEAR
	  ---mf*/
	
	if(!(file->calendar & cdStandardCal) && 
	   file->type == LATS_GRIB && 
	   file->frequency != LATS_FIXED) {
		year=2;
	}
	
	/* Check that basetime not set yet */
	if(file->basetimeset==1) {
		latsError("LATS (lats.c) -->  Basetime must be set at most once before any data is written, file %s", file->path);
		return 0;
	}
	
	/* Check valid time */
	if(month<1 || month>12){
		latsError("LATS (lats.c) -->  Invalid month: %d, file %s", month, file->path);
		return 0;
	}
	
	/*mf 970121 --- year 0 not supported ---mf*/
	
	if(file->frequency != LATS_FIXED) {
		if(year <= 0) {
			latsError("LATS (lats.c) -->  Invalid year: %d, file %s year <0 NOT Supported", year, file->path);
			return 0;
		}
	} else {
		
		/*mf --- set year to 1 for fixed data  --- mf*/
		year=1;
	}
	
	if(day<1 || day>31){
		latsError("LATS (lats.c) -->  Invalid day: %d, file %s",day, file->path);
		return 0;
	}
	if(hour<0 || hour>23){
		latsError("LATS (lats.c) -->  Invalid hour: %d, file %s",hour, file->path);
		return 0;
	}
	if(min<0 || min>59){
		latsError("LATS (lats.c) -->  Invalid min: %d, file %s",min, file->path);
		return 0;
	}
	
	
	time.year = year;
	time.month = month;
	time.day = day;
	time.hour = (double)hour;
	time.min = (double)min;
	
	file->btime.year = year;
	file->btime.month = month;
	file->btime.day = day;
	file->btime.hour = (double)hour;
	file->btime.min = (double)min;
	
	/* Generate base time */
	/* Non-climatology base time */
	if(file->calendar & cdStandardCal){
		sprintf(file->latstimeunits,"%s since %d-%d-%d %d",
			(file->frequency == LATS_YEARLY ? "years" :
			 file->frequency == LATS_MONTHLY ? "months" :
			 file->frequency == LATS_WEEKLY ? "days" :
			 file->frequency == LATS_DAILY ? "days" : "hours"),
			(int) time.year, time.month, time.day, hour);
	}
	/* Climatology base time */
	else {
		sprintf(file->latstimeunits,"%s since %d-%d %d",
			(file->frequency == LATS_MONTHLY ? "months" :
			 file->frequency == LATS_WEEKLY ? "days" :
			 file->frequency == LATS_DAILY ? "days" : "hours"),
			time.month, time.day, hour);
	}
	strncpy(file->timeunits, file->latstimeunits, LATS_MAX_RELUNITS); file->timeunits[LATS_MAX_RELUNITS-1]='\0';
	if(file->convention == LATS_NC3 && file->frequency == LATS_MONTHLY){
		sprintf(file->timeunits, "days since %d-%d-%d %d",
			(int) time.year, time.month, time.day, hour);
	}
	
	
	file->basetimeset = 1;
	return 1;                                  /* Success */
}

/* Close file with ID 'fileid'. Return 1 on success, 0 on failure. */
int lats_close(int fileid) {
	latsFile *file;
	int ierr;
	double timevalue;
	cdCompTime lasttime;
	
	_lats_routine_name_ = "lats_close";
	if((file = latsFileLookup(fileid))==0)
		return 0;
	
	/* Number of time increments for GrADS control file */
	file->ndelta = file->ntimewritten;
	
	/* Calculate global statistics */
	latsStatFile(file);
	
	// ierr = (file->type == LATS_GRIB ? lats_close_grib(file) : lats_close_nc(file));
	
        switch(file->type) {
           case LATS_GRIB:   ierr=lats_close_grib(file);  break;
           case LATS_NETCDF: ierr=lats_close_nc(file);    break;
           case LATS_HDF:    ierr=lats_close_sd(file);    break;
           default: ierr=-2;
        }

	if(latsFileDeleteEntry(fileid)==0)
		return 0;
	
	return ierr;
}

/* Create file with pathname 'path'. Return file ID on success, 0 on failure. */
int lats_create(char* path, int convention, int calendar, int frequency, int delta, char* center, char* model, char* comments) {
	latsFile *file;
	char *defaultParmFile;
	latsCenter *cent;
	char tempname[LATS_MAX_COMMENTS];
	char tmppath[LATS_MAX_PATH];
	char *extension;
	latsCompressionType compressiontype;
	latsFileType filetype;
	
	_lats_routine_name_ = "lats_create";
	/* Validity checks */
	if(convention != LATS_PCMDI &&
	   convention != LATS_GRADS_GRIB &&
	   convention != LATS_GRIB_ONLY &&
	   convention != LATS_NC3 &&
	   convention != LATS_NC4 &&
	   convention != LATS_HDF4){
		latsError("LATS (lats.c) -->  Invalid file convention: %d", convention);
		return 0;
	}
	if(frequency != LATS_YEARLY &&
	   frequency != LATS_MONTHLY &&
	   frequency != LATS_WEEKLY &&
	   frequency != LATS_DAILY &&
	   frequency != LATS_HOURLY &&
	   frequency != LATS_MINUTES &&
	   frequency != LATS_FORECAST_HOURLY &&
	   frequency != LATS_FORECAST_MINUTES &&
	   frequency != LATS_FIXED &&
	   frequency != LATS_MONTHLY_TABLE_COMP){
		latsError("LATS (lats.c) -->  Invalid frequency: %d", frequency);
		return 0;
	}
	
	if(calendar != LATS_STANDARD &&
	   calendar != LATS_JULIAN &&
	   calendar != LATS_NOLEAP &&
	   calendar != LATS_360 &&
	   calendar != LATS_CLIM &&
	   calendar != LATS_CLIMLEAP &&
	   calendar != LATS_CLIM360){
		latsError("LATS (lats.c) -->  Invalid calendar: %d", calendar);
		return 0;
	}
	
	if(convention == LATS_NC3 && calendar != LATS_STANDARD){
		latsError("LATS (lats.c) -->  COARDS data must be written with the standard Gregorian calendar, file %s", path);
		return 0;
	}
	
	if( (convention == LATS_GRADS_GRIB || convention == LATS_GRIB_ONLY)
	    && !(calendar == LATS_STANDARD
		 || calendar == LATS_NOLEAP
		 || calendar == LATS_CLIM 
		 || calendar == LATS_CLIMLEAP ) )
		{
			latsError("LATS (lats.c) -->  GRADS/GRIB data must be written with: 1) the standard Gregorian; 2) 365 day ; or 3) climatology calendar , file %s", path);
			return 0;
		}
	
	if( (frequency == LATS_FORECAST_HOURLY || frequency == LATS_MINUTES || frequency == LATS_FORECAST_MINUTES)  && !(calendar == LATS_STANDARD) ) {
		latsError("LATS (lats.c) -->  Forecast hourly data must written with the standard Gregorian calendar (LATS_STANDARD), file %s", path);
		return 0;
	}
	
	if(convention == LATS_GRADS_GRIB && delta <= 0){
		latsError("LATS (lats.c) -->  GRADS_GRIB data must be written with nonzero delta, file %s", path);
		return 0;
	}
	
	/* Map weekly data to daily */
	if(frequency == LATS_WEEKLY){
		frequency = LATS_DAILY;
		delta *= 7;
	}
	/* Set the file type */
	switch(convention){
	case LATS_PCMDI:
	case LATS_NC3:
		filetype = LATS_NETCDF;
		break;
	case LATS_NC4:
		filetype = LATS_NETCDF;
		break;
	case LATS_HDF4:
		filetype = LATS_HDF;
		break;
	case LATS_GRADS_GRIB:
		filetype = LATS_GRIB;
		break;
	case LATS_GRIB_ONLY:
		filetype = LATS_GRIB;
		break;
	}
	
	/* Override parameter table compression values for monthly data */
	compressiontype = (frequency == LATS_MONTHLY ? LATS_FIXED_COMP : LATS_TABLE_COMP);
	if (frequency == LATS_MONTHLY_TABLE_COMP) frequency = LATS_MONTHLY;
       
	/* Set the correct file extension */
	latsCpyTrim(tmppath, path, LATS_MAX_PATH-4);
        if ( filetype==LATS_NETCDF ) {
#ifdef NC_NETCDF4
          if ( glats.convention==LATS_NC4) extension = ".nc4";
          else                             extension = ".nc";
#else
          extension = ".nc";
#endif
        } else if ( filetype==LATS_HDF ) {
          extension = ".hdf";
        } else {
          extension = ".grb";
        }
	if(strcmp(extension, tmppath + MAX(0,strlen(tmppath)-strlen(extension))) != 0)
		strcat(tmppath,extension);
	
	
	/* Create file entry, add to file list */
	if((file = latsFileAddEntry(tmppath)) == 0)
		return 0;
	
	/* Create the parameter table, if necessary */
	if(latsParmTableCreated == 0){
		if((defaultParmFile = getenv("LATS_PARMS")) == NULL) {
			if(latsParmCreateDefaultTable()==0)
				return 0;
		}
		else {
			if(latsParmCreateTable(defaultParmFile)==0)
				return 0;
		}
		latsParmTableCreated = 1;
	}
	
	/* Lookup center for GRIB */
	if(filetype == LATS_GRIB){
		latsCpyLower(tempname, center, LATS_MAX_COMMENTS);
		if((cent = latsCenterLookup(tempname))==0){
			latsError("LATS (lats.c) -->  Center not found in center table: %s, file %s", tempname, tmppath);
			return 0;
		}
		file->centerid = cent->gribid;
		file->grib_center = cent->grib_center;
		file->grib_subcenter = cent->grib_subcenter;
	}
	else
		file->centerid = file->grib_center = file->grib_subcenter = -1;
	
	file->type = filetype;
	file->calendar = (latsCalenType) calendar;
	file->frequency = (latsTimeFreq) frequency;
	file->delta = delta;
	file->convention = (latsConvention) convention;
	file->compressiontype = compressiontype;
	strncpy(file->center, center, LATS_MAX_COMMENTS); file->center[LATS_MAX_COMMENTS-1]='\0';
	strncpy(file->model, model, LATS_MAX_COMMENTS); file->model[LATS_MAX_COMMENTS-1]='\0';
	strncpy(file->comments, comments, LATS_MAX_COMMENTS); file->comments[LATS_MAX_COMMENTS-1]='\0';
	
	/*mf  set latsmode to DEFINE mf*/
	file->latsmode = LATS_MODE_DEFINE;
	
	// return (filetype == LATS_GRIB ? lats_create_grib(file) : lats_create_nc(file));

        switch(file->type) {
          case LATS_GRIB:    return (lats_create_grib(file)); 
          case LATS_NETCDF:  return (lats_create_nc(file));   
          case LATS_HDF:     return (lats_create_sd(file));   
          default: return (-2);
        }

}

/* Set the error options. `erropt' is a logical
   combination of flags LATS_EXIT_ON_ERROR and
   LATS_REPORT_ERROR. */
void lats_erropt(int erropt){
	lats_fatal = (erropt & LATS_EXIT_ON_ERROR);
	lats_verbose = (erropt & LATS_REPORT_ERRORS);
}

#define LATS_TORAD (3.14159265359/180.0)     /* Degrees to radians */

/* Define a grid. The longitude vector 'lons' */
/* has length 'nlon', latitude vector 'lats' has length 'nlat'. Return 1 on success, */
/* 0 on failure. */
int lats_grid(char *name, int gridtype, int nlon, double lons[], int nlat, double lats[]) {
	latsFile *file;
	latsGrid *grid;
	latsMonotonicity monolon, monolat;
	int i, j;
	double *tlon, *plat;
	
	_lats_routine_name_ = "lats_grid";
	
	/* Validity checks */
	if(gridtype != LATS_GAUSSIAN &&
	   gridtype != LATS_LINEAR &&
	   gridtype != LATS_GENERIC){
		latsError("LATS (lats.c) -->  Invalid grid type: %d, grid %s", gridtype, name);
		return 0;
	}
	if(nlon <= 0 || nlat <= 0){
		latsError("LATS (lats.c) -->  Number of longitudes: %d, and number of latitudes: %d must be positive, grid %s",
			  nlon, nlat, name);
		return 0;
	}
	if((monolon=latsCheckMono(nlon, lons))==0 || (monolat=latsCheckMono(nlat, lats))==0){
		latsError("LATS (lats.c) -->  Longitude and latitude vectors must be monotonic, grid %s",
			  name);
		return 0;
	}
	if(fabs((double)(lons[nlon-1]-lons[0])) >= 360.0){
		latsError("LATS (lats.c) -->  Longitude vector must not wrap around, grid %s", name);
		return 0;
	}
	if(fabs((double)(lats[nlat-1]))>90.0 || fabs((double)lats[0])>90.0){
		latsError("LATS (lats.c) -->  Latitude values must be in the range -90 to 90");
		return 0;
	}
	
	if((grid = latsGridAddEntry(name)) == 0)
		return 0;
	
	/* Create the grid */
	latsCpyTrim(grid->name, name, LATS_MAX_NAME);
	grid->type = (latsGridType) gridtype;
	if((grid->lats = (double *)malloc(nlat*sizeof(double)))==NULL){
		latsError("LATS (lats.c) -->  Allocating latitude vector memory, nlat=%d",nlat);
		return 0;
	}
	grid->nlat = nlat;
	memcpy(grid->lats, lats, nlat*sizeof(double));
	
	if((grid->lons = (double *)malloc(nlon*sizeof(double)))==NULL){
		latsError("LATS (lats.c) -->  Allocating longitude vector memory, nlon=%d",nlon);
		return 0;
	}
	grid->nlon = nlon;
	memcpy(grid->lons, lons, nlon*sizeof(double));
	
	/* Calculate longitude weights  */
	if(((grid->wlons = (double *)malloc(nlon*sizeof(double)))==NULL) ||
	   ((tlon = (double *)malloc(nlon*sizeof(double)))==NULL)){
		latsError("LATS (lats.c) -->  Allocating longitude weight vector, nlon=%d",nlon);
		return 0;
	}
	
	tlon[0] = (lons[0] + lons[nlon-1] + 360.0)/2.0;
	for(i=1; i<nlon; i++){
		tlon[i] = (lons[i-1] + lons[i])/2.0;
	}
	
	if(monolon == LATS_INCREASING){
		grid->wlons[0] = tlon[1]-tlon[0]+360.0;
		for(i=1; i<nlon-1; i++){
			grid->wlons[i] = tlon[i+1]-tlon[i];
		}
		grid->wlons[nlon-1] = tlon[0] - tlon[nlon-1];
	}
	else {
		grid->wlons[0] = tlon[0]-tlon[1];
		for(i=1; i<nlon-1; i++){
			grid->wlons[i] = tlon[i]-tlon[i+1];
		}
		grid->wlons[nlon-1] = tlon[nlon-1]-tlon[0]+360.0;
	}
	
	/* Calculate latitude weights */
	if(((grid->wlats = (double *)malloc(nlat*sizeof(double)))==NULL) ||
	   ((plat = (double *)malloc((nlat+1)*sizeof(double)))==NULL)){
		latsError("LATS (lats.c) -->  Allocating latitude weight vector, nlat=%d",nlat);
		return 0;
	}
	
	plat[0] = (monolat==LATS_INCREASING ? -90.0 : 90.0);
	for(j=1; j<nlat; j++){
		plat[j] = (lats[j-1] + lats[j])/2.0;
	}
	plat[nlat] = (monolat==LATS_INCREASING ? 90.0 : -90.0);
	
	for(j=0; j<nlat; j++){
		grid->wlats[j] = fabs(sin(LATS_TORAD*(double)plat[j+1]) - sin(LATS_TORAD*(double)plat[j]));
	}
	
	free(tlon);
	free(plat);
	return grid->id;
}

/* Set the quality control:
   qcopt = LATS_QC_ON iff turn on quality control calculations
   qcopt = 0 iff turn off QC calcs */
void lats_qcopt(qcopt){
	lats_qc = (qcopt & LATS_QC_ON);
}

/* Define a variable to be written to file with ID 'fileid'. */
/* 'gridid' is the grid identifier, 'levid' is the vertical dimension identifier. */
/* Return the variable ID on success, 0 on failure. */
int lats_var(int fileid, char* varname, int datatype, int timestat, int gridid, int levid, char* comments) {
	latsFile *file;
	latsVar *var;
	latsVertDim *vertdim;
	latsParm *parm;
	latsParmQC *qc;
	latsGrid *grid;
	latsTimeStatEntry *stat;
	double levdelta, delta, *levvalue;
	int i, nlev, nprev, ierr;
	size_t qcalloc;
	
	_lats_routine_name_ = "lats_var";
	
	/* Lookup the parameter */
	if((parm = latsParmLookup(varname))==0){
		latsError("LATS (lats.c) -->  Variable %s: not in the variable table, file ID %d", varname, fileid);
		return 0;
	}
	
	/* Validity checks */
	if(datatype != LATS_FLOAT &&
	   datatype != LATS_INT){
		latsError("LATS (lats.c) -->  Datatype: %d, must be LATS_FLOAT or LATS_INT, file %d, var %s",
			  datatype, fileid, varname);
		return 0;
	}
	
	if(datatype != parm->datatype){
		latsError("LATS (lats.c) -->  Variable %s, file ID %d: datatype %s does not match predefined datatype",
			  varname, fileid, (datatype == LATS_FLOAT ? "LATS_FLOAT" : "LATS_INT"));
		return 0;
	}
	
	if(parm->id>255 | parm->id<1) {
		latsError("LATS (lats.c) -->  Variable %s, file ID %d: GRIB parameter id %d is invalid, must be >1 and < 255",
			  fileid, parm->id);
		return 0;
	}
	
	/* Create the variable */
	if((var = latsVarAddEntry(fileid, varname))==0)
		return 0;
	
	file = var->file;
	
	/* mode check */
	if(file->latsmode != LATS_MODE_DEFINE){
		latsError("LATS (lats.c) -->  lats_var calls MUST PRECEDE ANY lats_write call");
		return 0;
	}
	
	if(file->frequency != LATS_FIXED &&
	   timestat != LATS_AVERAGE &&
	   timestat != LATS_INSTANT &&
	   timestat != LATS_ACCUM &&
	   timestat != LATS_OTHER_TIME_STAT){
		latsError("LATS (lats.c) -->  Time statistic: %d must be LATS_AVERAGE, LATS_INSTANT, or LATS_ACCUM, file %d, var %s",
			  timestat, fileid, varname);
		return 0;
	}
	
	/* Lookup the time statistic for non-fixed variables */
	/*mf ----
	  961212 
	  bug fix -- LATS_FIXED now handled properly
	  if(file->type == LATS_GRIB && file->frequency != LATS_FIXED &&
	  ---mf*/
	

	if(file->type == LATS_GRIB && 
	   (stat = latsTimeStatLookup(file->frequency, file->delta, (latsTimeStat) timestat))==0){
		latsError("LATS (lats.c) -->  Time statistic not defined: frequency = %s, delta = %d, timestat = %s, for variable %s",
			  (file->frequency == LATS_YEARLY ? "year" :
			   file->frequency == LATS_MONTHLY ? "month" :
			   file->frequency == LATS_WEEKLY ? "week" :
			   file->frequency == LATS_DAILY ? "day" : "hour"),
			  file->delta,
			  (timestat == LATS_AVERAGE ? "average" :
			   timestat == LATS_INSTANT ? "instant" :
			   timestat == LATS_ACCUM ? "accum" : "other"),
			  varname);
		return 0;
	}
	
	/* Check level ID */
	if(levid == 0) {
		vertdim = (latsVertDim *)0;
		nlev = 0;
		levdelta = 1.0e20;
	}
	else if(levid>0) {
		nprev = file->nvertdim;
		if((vertdim = latsFileVertLookup(file, levid))==0){
			latsError("LATS (lats.c) -->  Vertical dimension not found, ID: %d, file %s, variable %s",
				  levid, file->path, var->name);
			return 0;
		}
		/* If the lookup added the vertical dimension to */
		/* the file vertlist, create it in the file. */
		
		/*mf 970822 
		  
		  abort if trying to use more than one vertical dimension for GrADS_GRIB
		  
		  mf*/
		
		if(file->convention == LATS_GRADS_GRIB && file->nvertdim > 1) {
			latsError("LATS (lats.c) -->  For LATS_GRADS_GRIB convention,\nvertdim can only be called once; define a vertical dimension that includes all levels\nVertical Dim ID: %d, file %s, variable %s",levid, file->path, var->name);
			return 0;
		}
		
		if(nprev<file->nvertdim) {

                  // if ( (file->type == LATS_GRIB ? lats_vert_dim_grib(file, vertdim) :
		  //  lats_vert_dim_nc(file, vertdim))==0)
                  //	return 0;
		
                  switch(file->type) {
                  case LATS_GRIB:   ierr = (lats_vert_dim_grib(file,vertdim)); break;
                  case LATS_NETCDF: ierr = (lats_vert_dim_nc(file,vertdim));   break;
                  case LATS_HDF:    ierr = (lats_vert_dim_sd(file,vertdim));   break;
                  default: return (-2);
                  }
                  if ( ierr==0 ) return 0;
                }

		nlev = vertdim->nlev;
		levdelta = vertdim->delta;
		/* Warning if changing the default surface type*/
		if(parm->levelset==1 && strcmp(vertdim->type->name,parm->verttype->name)){
			latsWarning("Overriding default level type %s, for variable %s",
				    parm->verttype->name, varname);
		}
	}
	else {
		latsError("LATS (lats.c) -->  Level dimension: %d, invalid, variable %s, file %s",
			  levid, varname, file->path);
		return 0;
	}
	

	/* levs must be monotonic */
	if(parm->levelset == 1 && nlev > 1){
		latsError("LATS (lats.c) -->  Variable %s, file ID %d: number of levels: %d, must be 0 or 1 for single-level (surface) variable",varname, fileid, nlev);
		return 0;
	}
	
	/*
	  if(vertdim && strcmp(parm->levunits,vertdim->units)!=0){
	  latsError("Defined level units (%s), differs from prescribed units (%s), variable %s, file %s",
	  parm->levunits, vertdim->units, var->name, file->path);
	  }
	  */
	/* Override table compression values for 'fixed compression' file */
	/* Note: Use var->scalefac instead of parm->scalefac, since other */
	/* non-fixed-compression variables may point to parm !!! */
	if(file->compressiontype == LATS_FIXED_COMP){
		var->scalefac = -999;
		var->precision = LATS_FIXED_COMPRESSION_NBITS;
	}
	else {
		var->scalefac = parm->scalefac;
		var->precision = parm->precision;
	}
	
	var->parm = parm;
	strncpy(var->comments, comments, LATS_MAX_COMMENTS); var->comments[LATS_MAX_COMMENTS-1]='\0';
	var->levdelta = levdelta;
	var->nlev = nlev;
	var->levs = (vertdim ? vertdim->levs : (double *)0);
	
	/*mf--- 
	  961212
	  bug fix -- LATS_FIXED now properly processed ...
	  var->timestat = (file->frequency == LATS_FIXED ? (latsTimeStatEntry *)0 : stat);
	  ---mf*/
	
	var->timestat = stat;
	var->tstat = (latsTimeStat) timestat;
	var->vertdim = vertdim;
	/* Declare the grid for this file, if necessary */
	nprev = file->ngrid;

	if((grid = latsFileGridLookup(file, gridid))==0)
		return 0;
	if(nprev < file->ngrid){

        // ierr = (file->type == LATS_GRIB ? lats_grid_grib(file, grid) : lats_grid_nc(file, grid));
           switch(file->type) {
           case LATS_GRIB:    ierr = (lats_grid_grib(file,grid)); break;
           case LATS_NETCDF:  ierr = (lats_grid_nc(file,grid));   break;
           case LATS_HDF:     ierr = (lats_grid_sd(file,grid));   break;
           default: return (-2);
           }
           if(ierr == 0)
             return 0;
	}
	var->grid = grid;
	
	/* Fill QC table: at most one entry for every level */
	if(lats_qc){
		qcalloc = (var->levs ? var->nlev : 1);
		if((var->qctable = (latsParmQC **)calloc(qcalloc,sizeof(latsParmQC *)))==0){
			latsError("LATS (lats.c) -->  Creating quality control table, variable %s", var->name);
			return 0;
		}
		/* Explicit vertical dimension? */
		if(var->levs){
			for(i=0; i<var->nlev; i++){
				if((qc = latsQCLookup(var->name, var->vertdim->type->name, var->levs[i], levdelta)))
					var->qctable[i] = qc;
				else
					var->qctable[i] = (latsParmQC *)0;
			}
		}
		/* Implicit vertical dimension? */
		else if(parm->levelset == 1){
			if((qc = latsQCLookup(var->name, parm->verttype->name, LATS_DEFAULT_QC_VALUE, 0.0)))
				var->qctable[0] = qc;
			else
				var->qctable[0] = (latsParmQC *)0;
		}
		else {			     /* No associated level, look for empty string*/
			if((qc = latsQCLookup(var->name, "", LATS_DEFAULT_QC_VALUE, 0.0)))
				var->qctable[0] = qc;
			else
				var->qctable[0] = (latsParmQC *)0;
		}
	}
	
	
	// return (file->type == LATS_GRIB ? lats_var_grib(file, var, grid, vertdim) : lats_var_nc(file, var, grid, vertdim));

        switch(file->type) {
          case LATS_GRIB:   return (lats_var_grib(file,var,grid,vertdim)); 
          case LATS_NETCDF: return (lats_var_nc(file,var,grid,vertdim));   
          case LATS_HDF:    return (lats_var_sd(file,var,grid,vertdim));   
          default: return (-2);
        }

}

/* Declare the missing data value for a floating-point variable. */
/* Return 1 on success, 0 on failure*/
int lats_miss_float(int fileid, int varid, float missing, float delta){
	latsVar *var;
	latsFile *file;
	
	_lats_routine_name_ = "lats_miss_float";
	
	if((file = latsFileLookup(fileid))==0) {
		return 0;
	}
	if(file->latsmode != LATS_MODE_DEFINE){
		latsError("LATS (lats.c) -->  lats_miss_float calls must precede any lats_write call");
		return 0;
	}
	
	if((var = latsVarLookup(fileid, varid))==0)
		return 0;
	
	if(var->parm->datatype != LATS_FLOAT){
		latsError("LATS (lats.c) -->  Missing data value for variable %s declared as float, but variable is not floating-point",
			  var->name);
		return 0;
	}
	if(delta < 0.0){
		latsError("LATS (lats.c) -->  Missing delta value = %f, must be nonnegative",delta);
		return 0;
	}
	var->missing.f = missing;
	var->missingdelta = delta;
	var->hasmissing = 1;
	return 1;
}

/* Declare the missing data value for an integer variable. */
/* Return 1 on success, 0 on failure*/
int lats_miss_int(int fileid, int varid, int missing){
	latsVar *var;
	latsFile *file;
	
	_lats_routine_name_ = "lats_miss_int";
	
	if((file = latsFileLookup(fileid))==0) {
		return 0;
	}
	if(file->latsmode != LATS_MODE_DEFINE){
		latsError("LATS (lats.c) -->  lats_miss_int calls must precede any lats_write call");
		return 0;
	}
	
	if((var = latsVarLookup(fileid, varid))==0)
		return 0;
	
	if(var->parm->datatype != LATS_INT){
		latsError("LATS (lats.c) -->  Missing data value for variable %s declared as integer, but variable is not an integer",
			  var->name);
		return 0;
	}
	var->missing.i = missing;
	var->hasmissing = 1;
	return 1;
}

/* Specify an alternate parameter table. */
int lats_parmtab(char* table_path) {
	
	_lats_routine_name_ = "lats_parmtab";
	if(latsParmCreateTable(table_path)==0)
		return 0;
	else{
		latsParmTableCreated = 1;
		return 1;
	}
}

/* Define a vertical dimension */
int lats_vert_dim(char* name, char* levtype, int nlev, double levs[]) {
	latsFile *file;
	latsVertDim *vertdim,*vertdim_check;
	latsVertType *verttype;
	latsMonotonicity mono;
	double levdelta, delta;
	int i;
	char *units;
	char *defaultParmFile;
	
	_lats_routine_name_ = "lats_vert_dim";
	
	/*mf mode check
	  if(file->latsmode != LATS_MODE_DEFINE){
	  latsError("LATS (lats.c) -->  lats_vert_dim calls must precede any lats_write call");
	  return 0;
	  }
	  */
	
	/* Create the parameter table, if necessary */
	if(latsParmTableCreated == 0){
		if((defaultParmFile = getenv("LATS_PARMS")) == NULL) {
			if(latsParmCreateDefaultTable()==0)
				return 0;
		}
		else {
			if(latsParmCreateTable(defaultParmFile)==0)
				return 0;
		}
		latsParmTableCreated = 1;
	}
	
	if((verttype = latsVertTypeLookup(levtype))==0){
		latsVertTypeList();
		latsError("LATS (lats.c) -->  Vertical dimension type not found: %s, vertical dimension %s", levtype, name);
		return 0;
	}
	
	if(nlev <= 0){
		latsError("LATS (lats.c) -->  Vertical dimension: %s, length: %d, must be positive",
			  name, nlev);
		return 0;
	}
	
	if(nlev>0 && (mono = latsCheckMono(nlev, levs))==0){
		latsError("LATS (lats.c) -->  Vertical dimension: %s: must be strictly monotonic, increasing or decreasing",
			  name);
		return 0;
	}
	
	/* Determine lookup fudge factor for level values */
	if(mono == LATS_INCREASING){
		levdelta = 1.0e20;
		for(i=0; i<nlev-1; i++){
			delta = levs[i+1] - levs[i];
			levdelta = (levdelta < delta) ? levdelta : delta;
		}
	}
	else if(mono == LATS_DECREASING){
		levdelta = 1.0e20;
		for(i=0; i<nlev-1; i++){
			delta = levs[i] - levs[i+1];
			levdelta = (levdelta < delta) ? levdelta : delta;
		}
	}
	else
		/* Singleton level, make sure delta is positive, */
		/* otherwise lookup will fail. */
		levdelta = ((delta=fabs((double)levs[0])) > 0.0 ? delta : 1.0);
	
	levdelta *= LATS_DELTA_FACTOR;
	
	if((vertdim = latsVertAddEntry(name))==0)
		return 0;
	
	vertdim->delta = levdelta;
	vertdim->nlev = nlev;
	vertdim->vncid = vertdim->ncid = -1;
	vertdim->type = verttype;
	latsCpyTrim(vertdim->name, name, LATS_MAX_NAME);
	
	if((vertdim->levs = (double *)malloc(nlev*sizeof(double)))==0){
		latsError("LATS (lats.c) -->  Allocating memory for vertical dimension: %s, length %d",
			  name, nlev);
		return 0;
	}
	memcpy(vertdim->levs, levs, nlev*sizeof(double));
	
	return vertdim->id;
}


/* Write a horizontal lon-lat section 'data' to file with ID 'fileid', */
/* variable with ID 'varid'. Return 1 on success, 0 on failure. */
int lats_write(int fileid, int varid, double lev, int year, int month, int day, int hour, int min, 
	       int fhour, int fmin,
	       void* data) {
	latsFile *file;
	latsVar *var;
	int ierr, levindex, timeindex, ihour, imin;
	latsCompTime time,curtime;
	cdCompTime cdtime;
	int timecmp;
	double old_time, new_time;
	int idel, ndels;
	
	static int monthdays[12]={31,29,31,30,31,30,31,31,30,31,30,31};
	static char monthnames[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	
	_lats_routine_name_ = "lats_write";
	
	/* Lookup the file and variable */
	
	if((var = latsVarLookup(fileid, varid))==0)
		return 0;
	file = var->file;
	
	/*mf - put lats in DATA mode mf*/
	
	file->latsmode=LATS_MODE_DATA;
	
	/* Validity checks */
	
	switch(file->frequency){
	case LATS_HOURLY:
		break;
	case LATS_MINUTES:
		break;
	case LATS_FORECAST_HOURLY:
		break;
	case LATS_FORECAST_MINUTES:
		break;
	case LATS_DAILY:
	case LATS_WEEKLY:
		hour = 0;
		min = 0;
		break;
	case LATS_MONTHLY:
		day = 1;
		hour = 0;
		min = 0;
		break;
	case LATS_YEARLY:
		month = 1;
		day = 1;
		hour = 0;
		min = 0;
		break;
	case LATS_FIXED:
		year = 1;
		month = 1;
		day = 1;
		hour = 0;
		min = 0;
		break;
	}
	
	/*mf---
	  check if climo field for GRIB options
	  set year to 2, but it will be overwritten in latsgrib.c by GRADS_CLIM_YEAR
	  ---mf*/
	
	if(!(file->calendar & cdStandardCal) && 
	   file->type == LATS_GRIB && 
	   file->frequency != LATS_FIXED) {
		year=2;
	}
	
	/*
	 *   time QC
	 */
	
	
	if(file->frequency != LATS_FORECAST_HOURLY && file->frequency != LATS_FORECAST_MINUTES) {
		
		if(year <= 0){
			latsError("LATS (lats.c) -->  \n  Invalid year: %d, file %s year <= 0 NOT Supported\n  if climatology use year 2 in lats_write", year, file->path);
		}
		
		if(month<1 || month>12){
			latsError("LATS (lats.c) -->  Invalid month: %d, file %s, variable %s",
				  month, file->path, var->name);
			return 0;
		}
		if(day<1 || day>31){
			latsError("LATS (lats.c) -->  Invalid day: %d, file %s, variable %s",
				  day, file->path, var->name);
			return 0;
		}
		if(hour<0 || hour>23){
			latsError("LATS (lats.c) -->  Invalid hour: %d, file %s, variable %s",
				  hour, file->path, var->name);
			return 0;
		}
		if(min<0 || min>59){
			latsError("LATS (lats.c) -->  Invalid min: %d, file %s, variable %s",
				  min, file->path, var->name);
			return 0;
		}
		
	} else {
		
		if(hour<0){
			latsError("LATS (lats.c) -->  Invalid hour for LATS_FORECAST_HOURLY: %d, file %s, variable %s",
				  hour, file->path, var->name);
			return 0;
		}
		
		if(min<0){
			latsError("LATS (lats.c) -->  Invalid hour for LATS_FORECAST_MINUTES: %d, file %s, variable %s",
				  min, file->path, var->name);
			return 0;
		}
		
	}
	
	

	if(file->frequency == LATS_FORECAST_HOURLY || file->frequency == LATS_FORECAST_HOURLY) {

	    /* set the time to the base time */

		time.year = file->btime.year;
		time.month = file->btime.month;
		time.day = file->btime.day;
		time.hour = (double)file->btime.hour;
		time.min = (double)file->btime.min;

		time.year = year;
		time.month = month;
		time.day = day;
		time.hour = (double)hour;
		time.min = (double)min;
		
	} else {
		
		time.year = year;
		time.month = month;
		time.day = day;
		time.hour = (double)hour;
		time.min = (double)min;
	}

	
	timecmp=latsTimeCmp(time,file->time);
	       
	/* if no times have been written; require that basetime be called for forecasts or call with current time */
	if(file->ntimewritten == 0) {
	  timeindex = 0;
	  if(file->basetimeset == 0) {
	      if(file->frequency == LATS_FORECAST_HOURLY || file->frequency == LATS_FORECAST_MINUTES) {
		  latsError("LATS (lats.c) --> lats_basetime must be called before writing data for LATS_FORECAST_HOURLY|MINUTES");
		  return 0;
	      } else {
		  if(lats_basetime(fileid, year, month, day, hour, min) != 1)
		      return 0;
	      }
	  }


	/* Time == current file time */
	} else if(timecmp == 0) {
	  timeindex = file->ntimewritten-1;
	  
	/* Time > current file time */
	} else if(timecmp > 0) {
	  timeindex = file->ntimewritten;
	  
	  
	  /* Check that (new_time - old_time)/delta is integral */
	  if(file->delta > 0) {
	    cdtime.year = file->time.year;
	    cdtime.month = file->time.month;
	    cdtime.day = file->time.day;
	    cdtime.hour = file->time.hour;
	    cdtime.min = file->time.min;
	    latsComp2Rel((cdCalenType)file->calendar, cdtime, file->latstimeunits, &old_time);
	    cdtime.year = time.year;
	    cdtime.month = time.month;
	    cdtime.day = time.day;
	    cdtime.hour = time.hour;
	    cdtime.min = time.min;
	    latsComp2Rel((cdCalenType)file->calendar, cdtime, file->latstimeunits, &new_time);

	    /* --- old_time,new_time in hours....*/
	    if(file->frequency == LATS_MINUTES || file->frequency == LATS_FORECAST_MINUTES) {
	      idel = (int)((new_time - old_time)*60.0 + 0.5);
	    } else {
	      idel = (int)(new_time - old_time + 0.5);
	    }


	    ndels = idel % file->delta;
	    
	    if(file->convention==LATS_GRADS_GRIB && idel != file->delta){
	      latsError("LATS (lats.c) -->  For GrADS convention, cannot skip timepoints; current time = %f %s, previous time = %f %s, delta = %d, file = %s, variable = %s",
			new_time,file->latstimeunits,old_time,file->latstimeunits,file->delta, file->path, var->name);
	      return 0;
	    }
	    if(ndels != 0){
	      latsError("LATS (lats.c) -->  Current time (%f %s) minus previous time (%f %s) not a multiple of delta (%d), file = %s, variable = %s",
			new_time,file->latstimeunits,old_time,file->latstimeunits,file->delta, file->path, var->name);
	      return 0;
	    }
	    if(idel != file->delta) var->timemissing = 1;
	  } else if(file->delta == 0) {
	    timeindex = file->ntimewritten;
	    var->timeerror=1;
	  }
	  
	  /* Time < current file time (only report error once) */

	} else if(var->timeerror == 0){

	  latsError("LATS (lats.c) -->  Variable(s) must be written in non-decreasing time order:\n   Last time written = %d:%02dZ %d %s %d\n   Current time = %dZ %d %s %d\n   File = %s, variable = %s",
		    (ihour = file->time.hour),(imin = file->time.min), file->time.day, monthnames[file->time.month-1], file->time.year,
		    hour, day, monthnames[month-1], year,
		    file->path, var->name);
	  var->timeerror = 1;
	  return 0;
	}
	
	/* Lookup the level (set levindex to index of level being written */
	if(var->nlev > 0){
	  if((levindex = latsLevLookup(lev, var->nlev, var->levs, var->levdelta))==-1){
	    latsError("LATS (lats.c) -->  Level not found: %f, file %s, variable %s",
		      lev, file->path, var->name);
	    return 0;
	  }
	  
	} else {
	  
	  levindex = -1;
		
	}   /* ---------------------------------------- END OF ntimewritten check*/
	
	
	/* Check that number of times, levels written makes sense */
	/* New time for this variable */
	if(timeindex > var->ntimewritten-1 && var->ntimewritten>0){
		if(var->nlev>0 && var->nlevwritten<var->nlev){
			latsError("LATS (lats.c) -->  Warning, fewer levels written (%d) then declared, file %s, variable %s, time = %dZ %d %s %d: The output file may be unnecessarily large",
				  var->nlevwritten, file->path, var->name,
				  hour, day, monthnames[month-1], year);
		}
		var->nlevwritten = 0;
		
	} else {
		/* Current time for this variable, or fixed variable */
		
		if((var->nlev>0 && var->nlevwritten==var->nlev) ||
		   (var->nlev==0 && var->nlevwritten==1)) {
			latsError("LATS (lats.c) -->  Warning, too many levels written (%d) than declared, file %s, variable %s, time = %dZ %d %s %d: Data may be overwritten or duplicated",
				  var->nlevwritten+1, file->path, var->name,
				  hour, day, monthnames[month-1], year);
		}
	}
	
	
	
	
	
	/* Calculate statistics */
	if(lats_qc && latsStatVar(file, var, (var->levs ? var->qctable[levindex] : var->qctable[0]), levindex, timeindex, time, data)==0){
		latsWarning("Quality control exception, variable %s, level %f, time %2dZ%d%s%d; see log file",
			    var->name, (var->levs ? var->levs[levindex] : 0.0), (ihour = time.hour),
			    time.day, monthnames[time.month-1], time.year);
	}
	
	// if((file->type == LATS_GRIB ? lats_write_grib(file, var, levindex, timeindex, time, data)
        // : lats_write_nc(file, var, levindex, timeindex, time, data)) == 0)
	//	return 0;
	
	/* Write the data */
        switch(file->type) {
          case LATS_GRIB:   ierr = (lats_write_grib(file,var,levindex,timeindex,time,fhour,fmin,data)); break;
          case LATS_NETCDF: ierr = (lats_write_nc(file,var,levindex,timeindex,time,fhour,fmin,data));   break;
          case LATS_HDF:    ierr = (lats_write_sd(file,var,levindex,timeindex,time,fhour,fmin,data));   break;
          default: return (-2);
        }
        if ( ierr==0 ) return 0;

	/* Update file, variable time and level counters */
	var->nlevwritten++;
	/* If a new time ... */
	if(timeindex == file->ntimewritten){
		file->time = time;
		file->ntimewritten++;
	}
	
	if(timeindex > var->ntimewritten-1)
		var->ntimewritten++;
	if(var->ntimewritten != file->ntimewritten){
		latsError("LATS (lats.c) -->  Warning: more times (%d) written to file %s than for variable %s (%d), time = %dZ %d %s %d",
			  file->ntimewritten, file->path, var->name, var->ntimewritten,
			  (ihour = file->time.hour), file->time.day, monthnames[file->time.month-1],
			  file->time.year);
	}
	return 1;
}
