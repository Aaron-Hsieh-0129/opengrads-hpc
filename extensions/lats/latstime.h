/* -*-Mode: C;-*-
 * Module:      LATS time definitions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latstime.h,v 1.2 2009/10/10 06:34:15 mike_fiorino Exp $
 * Version:     $Id: latstime.h,v 1.2 2009/10/10 06:34:15 mike_fiorino Exp $
 *
 * Revision History:
 *
 * $Log: latstime.h,v $
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
 * Revision 1.1.1.1  2002/06/27 19:44:22  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:58  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.5  1997/10/15 17:53:22  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.4  1996/10/22  19:05:14  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.3  1996/08/20 18:34:12  drach
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
 * Revision 1.2  1996/07/12 00:36:28  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.1  1996/05/03 18:48:07  drach
 * - Initial repository version
 *
 *
 */
/*
 * =================================================================
 *			Macros and Enums
 * =================================================================
 */
#ifndef _LATSTIME_H
#define _LATSTIME_H

#include "lats.h"

#define CD_LAST_DAY -1			     /* Last day of month */
#define CD_MAX_CHARTIME 48		     /* Max characters in character time */
#define CD_MAX_RELUNITS 64		     /* Max characters in relative units */
#define CD_MAX_TIME_DELTA 64		     /* Max characters in time delta */
#define CD_NULL_DAY 1			     /* Null day value */
#define CD_NULL_HOUR 0.0		     /* Null hour value */
#define CD_NULL_MIN 0.0		     /* Null hour value */
#define CD_NULL_MONTH 1			     /* Null month value */
#define CD_NULL_YEAR 0			     /* Null year value, component time */

typedef enum cdMonths {cdJan = 1, cdFeb, cdMar, cdApr, cdMay, cdJun, cdJul, cdAug,
			       cdSep, cdOct, cdNov, cdDec } cdMonths;
typedef enum cdSeasons {cdDJF = cdDec,	/* DJF */
				cdMAM = cdMar,		/* MAM */
				cdJJA = cdJun,		/* JJA */
				cdSON = cdSep} cdSeasons;		/* SON */

typedef enum cdCalenType {cdStandard = LATS_STANDARD, cdJulian = LATS_JULIAN, cdNoLeap = LATS_NOLEAP, cd360 = LATS_360, cdClim = LATS_CLIM,  cdClimLeap = LATS_CLIMLEAP, cdClim360 = LATS_CLIM360 } cdCalenType;

/*
 * =================================================================
 *			Structures
 * =================================================================
 */

/* Component time */
typedef struct {
	long 		year;		     /* Year */
	short 		month;		     /* Numerical month (1..12) */
	short 		day;		     /* Day of month (1..31) */
	double 		hour;		     /* Hour and fractional hours */
	double 		min;		     /* min and fractional min */
} cdCompTime;

/*
 * =================================================================
 *	This stuff is used in the original time stuff,
 * =================================================================
 */

typedef enum CdMonths {CdJan = 1, CdFeb, CdMar, CdApr, CdMay, CdJun, CdJul, CdAug,
			       CdSep, CdOct, CdNov, CdDec } CdMonths;

typedef enum CdSeasons {CdWinter = CdDec,	/* DJF */
				CdSpring = CdMar,		/* MAM */
				CdSummer = CdJun,		/* JJA */
				CdFall = CdSep} CdSeasons;		/* SON */

typedef enum CdTimeUnit {
	CdMinute = 1,
	CdHour = 2,
	CdDay = 3,
	CdWeek = 4,			     /* Always = 7 days */
	CdMonth = 5,
	CdSeason = 6,			     /* Always = 3 months */
	CdYear = 7,
	CdSecond = 8
} CdTimeUnit;

typedef enum cdUnit {
	cdMinute = CdMinute,
	cdHour = CdHour,
	cdDay = CdDay,
	cdWeek = CdWeek,			     /* Always = 7 days */
	cdMonth = CdMonth,
	cdSeason = CdSeason,			     /* Always = 3 months */
	cdYear = CdYear,
	cdSecond = CdSecond
} cdUnitTime;

#define CdChronCal    0x1
#define CdClimCal     0x0
#define CdBaseRel    0x00
#define CdBase1970   0x10
#define CdHasLeap   0x100
#define CdNoLeap    0x000
#define Cd365      0x1000
#define Cd360      0x0000
#define CdJulianType 0x10000

typedef enum CdTimeType {
	CdChron       = ( CdChronCal | CdBase1970 | CdHasLeap | Cd365),	/* 4369 */
	CdJulianCal   = ( CdChronCal | CdBase1970 | CdHasLeap | Cd365 | CdJulianType),
	CdChronNoLeap = ( CdChronCal | CdBase1970 | CdNoLeap  | Cd365),	/* 4113 */
	CdChron360    = ( CdChronCal | CdBase1970 | CdNoLeap  | Cd360),	/*   17 */
	CdRel         = ( CdChronCal | CdBaseRel  | CdHasLeap | Cd365),	/* 4353 */
	CdRelNoLeap   = ( CdChronCal | CdBaseRel  | CdNoLeap  | Cd365),	/* 4097 */
	CdClim        = ( CdClimCal  | CdBaseRel  | CdNoLeap  | Cd365), /* 4096 */
	CdClimLeap    = ( CdClimCal  | CdBaseRel  | CdHasLeap | Cd365),
	CdClim360     = ( CdClimCal  | CdBaseRel  | CdNoLeap  | Cd365)
}  CdTimeType;

#define CdNullYear 0
#define CdNullMonth CdJan
#define CdNullDay 1
#define CdLastDay -1
#define CdNullHour 0.0
#define CdNullMin 0.0

typedef struct {
	long    		year;	     /* e.g., 1979 */
	short			month;	     /* e.g., CdDec */
	short			day;	     /* e.g., 30 */
	double			hour;	     /* hour and fractional hour */
	double			min;	     /* min and fractional min */
	long			baseYear;    /* base year for relative, 1970 for CdChron */
	CdTimeType		timeType;    /* e.g., CdChron */
} CdTime;

typedef struct {
	long   			count;	     /* units count  */
	CdTimeUnit		units;	     /* time interval units */
} CdDeltaTime;

/*
 * =================================================================
 *			Function Prototypes
 * =================================================================
 */

extern void latsChar2Comp(cdCalenType timetype, char* chartime, cdCompTime* comptime);
extern void latsChar2Rel(cdCalenType timetype, char* chartime, char* relunits, double* reltime);
extern void latsComp2Char(cdCalenType timetype, cdCompTime comptime, char* time);
extern void latsComp2Rel(cdCalenType timetype, cdCompTime comptime, char* relunits, double* reltime);
extern void latsRel2Char(cdCalenType timetype, char* relunits, double reltime, char* chartime);
extern void latsRel2Comp(cdCalenType timetype, char* relunits, double reltime, cdCompTime* comptime);
extern void latsRel2Rel(cdCalenType timetype, char* relunits, double reltime, char* outunits, double* outtime);
int latsParseRelunits(cdCalenType timetype, char* relunits, cdUnitTime* unit, cdCompTime* base_comptime);
int latsParseDeltaTime(cdCalenType timetype, char* deltaTime, double* value, cdUnitTime* unit);

extern void latsDayOfYear(CdTime *date, int *doy);
extern void latsDaysInMonth(CdTime *date, int *days);
extern void latsMonthDay(int *doy, CdTime *date);
extern void latsc2e(char *ctime, CdTimeType timeType, double *etime, long *baseYear);
extern void latsc2h(char *ctime, CdTimeType timeType, CdTime *htime);
extern void latse2c(double etime, CdTimeType timeType, long baseYear, char *ctime);
extern void latse2h(double etime, CdTimeType timeType, long baseYear, CdTime *htime);
extern void latsh2c(CdTime *htime, char *ctime);
extern void latsh2e(CdTime *htime, double *etime);
extern void latsAddDelTime(double begEtm, long nDel, CdDeltaTime delTime, CdTimeType timeType, long baseYear, double *endEtm);
extern void latsDivDelTime(double begEtm, double endEtm, CdDeltaTime delTime, CdTimeType timeType, long baseYear, long *nDel);
extern int latsToOldTimetype(cdCalenType newtype, CdTimeType* oldtype);
extern void latsTrim(char* s, int n);
extern int latsValidateTime(cdCalenType timetype, cdCompTime comptime);
#endif
