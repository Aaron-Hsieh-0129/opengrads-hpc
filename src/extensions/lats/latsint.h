/* -*-Mode: C;-*-
 * Module:      LATS internal include file
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latsint.h,v 1.5 2010/02/16 23:01:31 mike_fiorino Exp $
 * Version:     $Id: latsint.h,v 1.5 2010/02/16 23:01:31 mike_fiorino Exp $
 *
 * Revision History:
 *
 * $Log: latsint.h,v $
 * Revision 1.5  2010/02/16 23:01:31  mike_fiorino
 * mods for supporting yflip and better handling of forecast_hourly forecast_minutes
 *
 * Revision 1.4  2009/10/15 01:17:53  dasilva
 * ams: work in progress
 *
 * Revision 1.3  2009/10/15 00:25:37  dasilva
 * ams: work in progress
 *
 * Revision 1.2  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.1.1.1  2002/06/27 19:44:17  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:57  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.15  1997/10/15 17:53:18  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.1  1997/02/14 20:10:10  fiorino
 * Initial revision
 *
 * Revision 1.14  1996/11/11 22:39:21  drach
 * - Added function to set the basetime (lats_basetime)
 *
 * Revision 1.13  1996/10/22 19:05:09  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.12  1996/10/10 23:15:46  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.11  1996/09/30 18:57:27  drach
 * - Relax test for GrADS/GRIB equal-spacing
 *
 * Revision 1.10  1996/08/20 18:34:10  drach
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
 * Revision 1.9  1996/07/12 00:36:27  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.8  1996/06/27 01:14:45  drach
 * - Changed QC min/max to range
 * - Added grid weights for QC
 *
 * Revision 1.7  1996/06/12 00:42:40  drach
 * - Create default table from latsparm.h
 *
 * Revision 1.6  1996/05/25 00:27:50  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.5  1996/05/10 22:44:41  drach
 * - Initial version before GRIB driver added:
 * - Made grids, vertical dimensions file-independent
 *
 * Revision 1.4  1996/05/04 01:11:11  drach
 * - Added name, units to lats_vert_dim
 * - Added missing data attribute (latsnc.c)
 *
 * Revision 1.3  1996/05/03 18:59:25  drach
 * - Moved vertical dimension definition from lats_var to lats_vert_dim
 * - Changed lats_miss_double to lats_miss_float
 * - Made time dimension file-dependent, revised lats_write accordingly
 * - Added lats_var_nc, lats_vert_dim_nc
 * - Allow GRIB-only compilation
 * - Added FORTRAN interface
 *
 * Revision 1.2  1996/04/25  23:32:07  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:53:02  drach
 * Initial repository version
 *
 *
 */

#ifndef _LATSINT_H
#define _LATSINT_H

#include "lats.h"

/*
 * =================================================================
 *			Macros and Enums
 * =================================================================
 */

#define LATS_DELTA_FACTOR 1.0e-1	     /* Dimension values are equal if within */
					     /* (smallest delta) * LATS_DELTA_FACTOR*/
#define LATS_QC_TABLE_INCR 512		     /* Number of entries to increase QC table by if overflow */
#define LATS_DEFAULT_QC_VALUE 0.0	     /* Default level value in quality control table, if not set */
#define LATS_LINEAR_GRID_DELTA 1.0e-3	     /* Linear grid fudge factor */

#define LATS_MODE_DEFINE 1                   /* define and data mode for LATS for consistency with netCDF */
#define LATS_MODE_DATA 2

					     /* LATS_NO_DATA:   No valid data on current grid
					        LATS_SOME_DATA: There is some valid data, some missing
						LATS_ALL_DATA:  All data is valid, none missing
					      */
enum data_amt {LATS_NO_DATA=1, LATS_SOME_DATA, LATS_ALL_DATA};

					     /* LATS_TABLE_COMP: Use parameter table compression parameters */
					     /* LATS_FIXED_COMP: Use value of LATS_FIXED_COMPRESSION_NBITS */
typedef enum latsCompressionType {LATS_TABLE_COMP=1, LATS_FIXED_COMP} latsCompressionType;

/*
 * =================================================================
 *			Structures
 * =================================================================
 */

/* Component time (same as CDMS cdCompTime */
typedef struct {
	long 		year;		     /* Year */
	short 		month;		     /* Numerical month (1..12) */
	short 		day;		     /* Day of month (1..31) */
	double 		hour;		     /* Hour and fractional hours */
	double 		min;		     /* Min and fractional mins */
} latsCompTime;

/* Vertical dimension type
 * NB! Keep in sync with genlatsparm.pl !!!
 */

typedef struct {
  char name[LATS_MAX_NAME];	             /* Vertical dimension type name */
  char descrip[LATS_MAX_NAME];	             /* Description */
  char units[LATS_MAX_NAME];		     /* Units */
  latsPositive positive;		     /* LATS_UP or LATS_DOWN (cf. COARDS conventions) */
  latsVerticality verticality;		     /* single/multi-level */
  int gribid;				     /* GRIB ID */
  int grib_p1;				     /* GRIB value 1 */
  int grib_p2;				     /* GRIB value 2 */
  int grib_p3;				     /* GRIB combined p1,p2 */
} latsVertType;

/* A parameter is a file-independent variable specification. */
/* NB! Keep in sync with genlatsparm.pl !!! */

typedef struct {
  char name[LATS_MAX_NAME];		     /* Parameter name */
  char title[LATS_MAX_NAME];		     /* Long name */
  char units[LATS_MAX_NAME];		     /* Parameter units */
  int id;				     /* Parameter number (GRIB) */
  int scalefac;				     /* Decimal scale factor (GRIB) */
  int precision;			     /* Number of bits for packed representation (GRIB) */
  char levelname[LATS_MAX_NAME];	     /* Name of associated (implicit) surface, or blank if multi-level */
  int levelset;				     /* 1 iff there is an associated (implicit) surface, 0 else */
  latsVertType *verttype;		     /* Associated vertical type (implicit surface), if levelset is 1 */
  latsType datatype;			     /* Datatype (float or int) */
} latsParm;

/* Time statistic
 * NB! Keep in sync with genlatsparm.pl !!!
 */
typedef struct {
  latsTimeFreq frequency;		     /* Statistic time frequency (monthly, daily, etc.) */
  int delta;				     /* Number of time increment units (e.g., 6 for 6-hour incr) */
  latsTimeStat stat;			     /* average, accum, etc. */
  int grib_unit;			     /* GRIB forecast time units (GRIB PDS octet 18) */
  int grib_p1;				     /* GRIB period of time (GRIB PDS octet 19) */
  int grib_p2;				     /* GRIB period of time (GRIB PDS octet 20) */
  int grib_p3;				     /* GRIB combined GRIB_P1, GRIB_P2; */
					     /*   if specified, overrides GRIB_P1 and GRIB_P2 */
  int grib_timerange;			     /* GRIB time range indicator, PDS octet 21 */
} latsTimeStatEntry;

/* Originating center
 * NB! Keep in sync with genlatsparm.pl !!!
 */
typedef struct {
  char center[LATS_MAX_NAME];		     /* Center name */
  int gribid;				     /* GRIB generating process (PDS octet 6) */
  int grib_center;			     /* GRIB data center (PDS octet 5) */
  int grib_subcenter;			     /* GRIB data subcenter (PDS octet 26) */
} latsCenter;

/* Quality control record */
/* NB! Keep in sync with genlatsparm.pl !!! */

typedef struct {
  char name[LATS_MAX_NAME];		     /* Parameter name */
  char levtype[LATS_MAX_NAME];		     /* Level type (plev, sfc, ...) */
  double value;				     /* Level value */
  float obsmean;			     /* Observed mean value  */
  float obsstd;				     /* Observed standard deviation (of the mean) */
  float nstd;				     /* Calculated mean is valid if within obsmean +/- (nstd*obsstd) */
  float obsrange;			     /* Observed (maximum - minimum) */
  float obsrangetol;			     /* Range is valid if < (obsrangetol * obsrange) */
  float mean;				     /* Calculated mean in level */
  float max;				     /* Maximum value in level */
  float min;				     /* Minimum value in level */
  float std;				     /* Calculated standard deviation in level */
} latsParmQC;

/* Horizontal grid */

typedef struct {
  char name[LATS_MAX_NAME];		     /* Grid name */
  char comments[LATS_MAX_COMMENTS];	     /* Grid comments */
  double* lats;				     /* Grid latitude vector */
  double* lons;				     /* Grid longitude vector */
  double* wlats; 			     /* Grid latitude weights */
  double* wlons;			     /* Grid longitude weights */
  int id;				     /* LATS identifier */
  int lonid;				     /* netCDF longitude dimension ID */
  int latid;				     /* netCDF latitude dimension ID */
  int nlat;				     /* Number of grid latitudes */
  int nlon;				     /* Number of grid longitudes */
  int vlonid;				     /* netCDF longitude variable ID */
  int vlatid;				     /* netCDF latitude variable ID */
  latsGridType type;    		     /* Grid classification */
} latsGrid;

/* Vertical dimension */

typedef struct latsvertdim{
  char name[LATS_MAX_NAME];		     /* Vertical dimension name */
  double *levs;				     /* Vertical dimension values */
  double delta;				     /* Vertical dimension lookup delta */
  int id;				     /* LATS dimension ID */
  int ncid;				     /* netCDF dimension ID */
  int nlev;				     /* Number of levels */
  int vncid;				     /* netCDF variable ID */
  latsVertType *type;			     /* Vertical dimension type */
  struct latsvertdim *next;		     /* Next dimension in list */
} latsVertDim;

/* Variable */

typedef struct latsvar {
  char comments[LATS_MAX_COMMENTS];	     /* Variable comments */
  char name[LATS_MAX_NAME];		     /* Variable name */
  double levdelta;			     /* Level lookup delta */
  double* levs;				     /* Level vector (null if implicit surface) */
  int hasmissing;			     /* 1 iff variable has missing data value */
  int id;				     /* Variable identifier */
  int ncid;				     /* netCDF variable ID  */
  int nlev;				     /* Number of levels (0 for implicit surface) */
  int nlevwritten;			     /* Number of levels written for the current time */
  int ntimewritten;			     /* Number of times written */
  int precision;			     /* Compression precision (number of bits, fixed bitsize compression) */
  int scalefac;				     /* Compression scale factor */
  int timeerror;			     /* 1 iff error message about decreasing times has been issued */
  int timemissing;			     /* 1 iff a missing time value has been found for this var */
  latsTimeStat tstat;			     /* Time statistic (LATS_AVERAGE, etc.)*/
  struct latsfile* file;		     /* File associated with this var */
  latsGrid *grid;			     /* Grid for this variable */
  latsParm *parm;			     /* Parameter associated with this variable */
  latsParmQC **qctable;  		     /* QC table, nlev pointers, or 1 pointer if no vertical dimension */
  latsTimeStatEntry *timestat;		     /* Time statistics (NULL for fixed time or netCDF!) */
  latsVertDim *vertdim;			     /* Explicit vertical dimension, cf parm->verttype for implicit surface */
  struct latsvar *next;			     /* Next variable in file variable list */
  void* depend;				     /* GRIB/netCDF-specific struct */
  union {			             /* Missing data value (cf. hasmissing */
	  int i;			     /* Missing data value (integer variable) */
	  float f;			     /* Missing data value (float variable) */
  } missing;
  float missingdelta;			     /* Delta for missing value comparison (floats only) */
} latsVar;

/* File */

typedef struct latsfile{
  char comments[LATS_MAX_COMMENTS];	     /* File comments */
  char center[LATS_MAX_COMMENTS];	     /* Organization which produced this data */
  char model[LATS_MAX_COMMENTS];	     /* Model which produced this data */
  char path[LATS_MAX_PATH];		     /* File pathname */
  char timeunits[LATS_MAX_RELUNITS];	     /* Relative time units as written to file (e.g., "hours since 1979-01-01") */
  char latstimeunits[LATS_MAX_RELUNITS];     /* 'Actual' relative time units as known to LATS (e.g., "months since ..." */
					     /*   for COARDS monthly data, etc. */
  int basetimeset;                           /* 0 if not set, 1 if set */
  int id;				     /* File identifier */
  int centerid;				     /* GRIB process ID (for PCMDI, this is the center which wrote the data) */
  int grib_center;			     /* GRIB center (PDS octet 5) */
  int grib_subcenter;			     /* GRIB subcenter (PDS octet 26) */
  int delta;				     /* Number of units in time increment (cf. frequency) */
  int ndelta;				     /* Number of time increments (may be > ntimewritten if times skipped) */
  int ngrid;				     /* Number of grids in gridlist */
  int ntimewritten;			     /* Number of times written */
  int nvar;				     /* Number of variables in variable list */
  int nvertdim;				     /* Number of vertical dimensions defined*/
  int latsmode;				     /*mf --- 970214  Define or data mode for consistency with netcdf*/
  int fhour;                                /*mf --- 970517  Current forecast hour for LATS_FORECAST_HOURLY */
  int fmin;                                /*mf --- 970517  Current forecast hour for LATS_FORECAST_HOURLY */
  latsCompTime btime;                        /*mf --- 970517  Store base time from lats_basetime for use in GRIB and incrementing for LATS_FORECAST_HOURLY */
  latsCalenType calendar;		     /* Calendar type (LATS_STANDARD, LATS_JULIAN, etc.) */
  latsCompressionType compressiontype;	     /* Tabled compression (LATS_TABLE_COMP) or fixed number of bits (LATS_FIXED_COMP) */
  latsConvention convention;		     /* Convention for writing data/metadata (LATS_PCMDI, LATS_GRADS_GRIB, etc.) */
  latsCompTime time;			     /* Last (greatest) time written */
  latsFileType type;			     /* Type of file (GRIB or netCDF) */
  latsGrid gridlist[LATS_MAX_GRIDS];	     /* Grids written to this file */
  latsTimeFreq frequency;		     /* File time frequency (time increment units) */
  latsVar* varlist;			     /* Pointer to list of file variables */
  latsVertDim vertlist[LATS_MAX_VERT_DIMS]; /* Vertical dimension list */
  struct latsfile* next;		     /* Next file in list */
  void* depend;				     /* GRIB/netCDF-specific struct */
} latsFile;

/*
 * =================================================================
 *			Function Prototypes
 * =================================================================
 */

extern int lats_close_grib(latsFile *file);
extern int lats_create_grib(latsFile *file);
extern int lats_grid_grib(latsFile *file, latsGrid *grid);
extern int lats_var_grib(latsFile *file, latsVar *var, latsGrid *grid, latsVertDim *vertdim);
extern int lats_vert_dim_grib(latsFile *file, latsVertDim *vertdim);
extern int lats_write_grib(latsFile *file, latsVar *var, int levindex, int timeindex, latsCompTime time, int fhour, int fmin, void *data);

extern int lats_close_nc(latsFile *file);
extern int lats_create_nc(latsFile *file);
extern int lats_grid_nc(latsFile *file, latsGrid *grid);
extern int lats_var_nc(latsFile *file, latsVar *var, latsGrid *grid, latsVertDim *vertdim);
extern int lats_vert_dim_nc(latsFile *file, latsVertDim *vertdim);
extern int lats_write_nc(latsFile *file, latsVar *var, int levindex, int timeindex, latsCompTime time, int fhour, int fmin, void *data);
extern int lats_stub_nc(void);

extern int lats_close_sd(latsFile *file);
extern int lats_create_sd(latsFile *file);
extern int lats_grid_sd(latsFile *file, latsGrid *grid);
extern int lats_var_sd(latsFile *file, latsVar *var, latsGrid *grid, latsVertDim *vertdim);
extern int lats_vert_dim_sd(latsFile *file, latsVertDim *vertdim);
extern int lats_write_sd(latsFile *file, latsVar *var, int levindex, int timeindex, latsCompTime time, int fhour, int fmin, void *data);
extern int lats_stub_sd(void);

extern latsFile* latsFileLookup(int id);
extern int latsFileDeleteEntry(int id);
extern latsFile* latsFileAddEntry(char* path);
extern latsVertDim *latsFileVertLookup(latsFile *file, int vertid);
extern latsGrid *latsFileGridLookup(latsFile *file, int gridid);
extern latsVar* latsVarLookup(int fileid, int varid);
extern latsVar* latsVarAddEntry(int fileid, char* varname);
extern int latsLevLookup(double lev, int nlev, double levs[], double delta);
extern latsParm* latsParmLookup(char* name);
extern latsParmQC* latsQCLookup(char* name, char* levtype, double value, double delta);
extern latsVertType* latsVertTypeLookup(char *name);
extern void latsVertTypeList(void);
extern latsTimeStatEntry* latsTimeStatLookup(latsTimeFreq frequency, int delta, latsTimeStat statistic);
extern latsCenter* latsCenterLookup(char *name);
extern int latsParmCreateDefaultTable(void);
extern int latsParmCreateTable(char* path);
extern latsVertDim *latsVertLookup(int id);
extern latsVertDim *latsVertAddEntry(char *name);
extern latsGrid *latsGridAddEntry(char *name);
extern int latsStatVar(latsFile *file, latsVar *var, latsParmQC *qc, int levindex, int timeindex, latsCompTime time, void *data);
extern int latsStatFile(latsFile *file);
extern latsMonotonicity latsCheckMono(int n, double f[]);
extern void latsError(char* fmt, ...);
extern void latsWarning(char *fmt, ...);

extern int latsTimeCmp(latsCompTime t1, latsCompTime t2);

/*
 * =================================================================
 *			Globals
 * =================================================================
 */

extern char *_lats_routine_name_;	     /* Name of user-level routine last called */

#endif
