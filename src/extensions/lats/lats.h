/* -*-Mode: C;-*-
 * Module:      LATS include file
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version: $Id: lats.h,v 1.7 2010/02/16 23:01:31 mike_fiorino Exp $
 * Version:     $Id: lats.h,v 1.7 2010/02/16 23:01:31 mike_fiorino Exp $
 *
 * Revision History:
 *
 * $Log: lats.h,v $
 * Revision 1.7  2010/02/16 23:01:31  mike_fiorino
 * mods for supporting yflip and better handling of forecast_hourly forecast_minutes
 *
 * Revision 1.6  2009/10/15 01:17:53  dasilva
 * ams: work in progress
 *
 * Revision 1.5  2009/10/14 22:19:05  dasilva
 * ams: work in progress
 *
 * Revision 1.4  2009/10/14 22:16:58  dasilva
 * ams: work in progress
 *
 * Revision 1.2  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
 * Revision 1.5  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid

 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.1.1.1  2002/06/27 19:44:13  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:57  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.12  1997/10/15 17:53:14  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.11  1996/11/11 22:39:19  drach
 * - Added function to set the basetime (lats_basetime)
 *
 * Revision 1.10  1996/10/22 19:05:03  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.9  1996/10/10 23:15:43  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.8  1996/08/20 18:34:07  drach
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
 * Revision 1.7  1996/06/27 01:11:36  drach
 * - Remove timestats table
 *
 * Revision 1.6  1996/05/25 00:27:48  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.5  1996/05/10 22:44:38  drach
 * - Initial version before GRIB driver added:
 * - Made grids, vertical dimensions file-independent
 *
 * Revision 1.4  1996/05/04 01:11:10  drach
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
 * Revision 1.2  1996/04/25  23:32:04  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:53:00  drach
 * Initial repository version
 *
 *
 */

#ifndef _LATS_H
#define _LATS_H

/*
 * =================================================================
 *			Macros and Enums
 * 
 *   Note: modifications should be mirrored in lats.inc
 * =================================================================
 */

#define LATS_MAX_CENTERS 256		     /* Max number of originating centers */
#define LATS_MAX_COMMENTS 256		     /* Max length of comment arguments */
#define LATS_MAX_GRIDS 32		     /* Max number of grids */
#define LATS_MAX_NAME 128		     /* Max name/units length */
#define LATS_MAX_PARMS 512		     /* Max number of parameters */
#define LATS_MAX_PARM_LINE 512		     /* Max characters in parameter file entry (one line) */
#define LATS_MAX_PATH 256		     /* Max file pathname length  */
#define LATS_MAX_RELUNITS 64		     /* Max relative time units length */
#define LATS_MAX_VAR_DIMS 4		     /* Max number of dimensions in a variable */
#define LATS_MAX_VERT_DIMS 32		     /* Max number of level dimensions */
#define LATS_MAX_VERT_TYPES 64		     /* Max number of vertical dimension types */

#define LATS_EXIT_ON_ERROR 0x1		     /* Error flag: exit on error (lats.inc: LATS_FATAL) */
#define LATS_REPORT_ERRORS 0x2		     /* Error flag: report errors (verbose) (lats.inc: LATS_VERBOSE) */
#define LATS_QC_ON 0x1			     /* Quality control flag */
#define LATS_FIXED_COMPRESSION_NBITS 16	     /* Default bit length for compression of monthly mean data */

typedef enum latsCloudLevels {LATS_LOW_LEVEL = 1, LATS_MEDIUM_LEVEL, LATS_HIGH_LEVEL} latsCloudLevels;
typedef enum latsConvention {LATS_PCMDI = 1, LATS_GRIB_ONLY, LATS_GRADS_GRIB, LATS_NC3, LATS_NC4, LATS_HDF4} latsConvention;
typedef enum latsFileType {LATS_NETCDF = 1, LATS_GRIB, LATS_HDF} latsFileType;
typedef enum latsGridType {LATS_GAUSSIAN = 1, LATS_LINEAR, LATS_GENERIC} latsGridType;
typedef enum latsMonotonicity {LATS_SINGLE = 1, LATS_INCREASING, LATS_DECREASING} latsMonotonicity;
typedef enum latsPositive {LATS_UP = 1, LATS_DOWN} latsPositive;
typedef enum latsTimeFreq {LATS_HOURLY = 1, LATS_DAILY, LATS_WEEKLY, LATS_MONTHLY, LATS_YEARLY, LATS_FIXED, LATS_MONTHLY_TABLE_COMP, LATS_FORECAST_HOURLY, LATS_MINUTES, LATS_FORECAST_MINUTES} latsTimeFreq;
typedef enum latsTimeStat {LATS_AVERAGE = 1, LATS_INSTANT, LATS_ACCUM, LATS_OTHER_TIME_STAT} latsTimeStat;
typedef enum latsType {LATS_FLOAT = 1, LATS_INT} latsType;
typedef enum latsVerticality {LATS_SINGLE_LEVEL = 1, LATS_MULTI_LEVEL} latsVerticality;

#define cdStandardCal   0x11
#define cdClimCal        0x0
#define cdHasLeap      0x100
#define cdHasNoLeap    0x000
#define cd365Days     0x1000
#define cd360Days     0x0000
#define cdJulianCal  0x10000

typedef enum latsCalenType {
	LATS_STANDARD    = ( cdStandardCal | cdHasLeap   | cd365Days),
	LATS_JULIAN      = ( cdStandardCal | cdHasLeap   | cd365Days | cdJulianCal),
	LATS_NOLEAP      = ( cdStandardCal | cdHasNoLeap | cd365Days),
	LATS_360         = ( cdStandardCal | cdHasNoLeap | cd360Days),
	LATS_CLIM        = ( cdClimCal     | cdHasNoLeap | cd365Days),
	LATS_CLIMLEAP    = ( cdClimCal     | cdHasLeap   | cd365Days),
	LATS_CLIM360     = ( cdClimCal     | cdHasNoLeap | cd360Days)
}  latsCalenType;

/*
 * =================================================================
 *			Function Prototypes
 * =================================================================
 */

extern int lats_basetime(int fileid, int year, int month, int day, int hour, int min);
extern int lats_close(int fileid);
extern int lats_create(char* path, int filetype, int calendar, int frequency, int delta, char* center, char* model, char* comment);
extern int lats_grid(char *name, int gridtype, int nlon, double lons[], int nlat, double lats[]);
extern int lats_miss_float(int fileid, int varid, float missing, float delta);
extern int lats_miss_int(int fileid, int varid, int missing);
extern int lats_var(int fileid, char* varname, int datatype, int timestat, int gridid, int levid, char* comments);
extern int lats_parmtab(char* table_path);
extern int lats_vert_dim(char* name, char* type, int nlev, double levs[]);
extern int lats_write(int fileid, int varid, double lev, int year, int month, int day, int hour, int min, int fhour, int fmin, void* data);

/*
 * =================================================================
 *			Globals
 * =================================================================
 */

extern int lats_fatal;			     /* If set to 1, exit on error (default 0) */
extern int lats_verbose;		     /* If set to 1, errors are reported (default 1) */
extern int lats_qc;			     /* If set to 1, execute quality control (default 1) */

#endif
