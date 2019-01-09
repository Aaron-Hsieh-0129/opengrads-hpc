
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS time functions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latstime.c,v 1.3 2009/10/29 19:37:44 dasilva Exp $
 * Version:     $Id: latstime.c,v 1.3 2009/10/29 19:37:44 dasilva Exp $
 *
 * Revision History:
 *
 * $Log: latstime.c,v $
 * Revision 1.3  2009/10/29 19:37:44  dasilva
 * ams: fixed min bug
 *
 * Revision 1.2  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
 * Revision 1.7  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid
 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.6  2008/01/19 19:24:47  pertusus
 * Use the pkgconfig result unless dap root was set.
 * change <config.h> to "config.h".
 *
 * Revision 1.5  2007/08/26 23:32:03  pertusus
 * Add standard headers includes.
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.2  2002/10/28 19:08:33  joew
 * Preliminary change for 'autonconfiscation' of GrADS: added a conditional
 * #include "config.h" to each C file. The GNU configure script generates a unique config.h for each platform in place of -D arguments to the compiler.
 * The include is only done when GNU configure is used.
 *
 * Revision 1.1.1.1  2002/06/27 19:44:22  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:58  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.6  1997/10/15 17:53:21  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.5  1996/10/22  19:05:13  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.4  1996/08/27 19:44:26  drach
 * - Fixed up minor compiler warnings
 *
 * Revision 1.3  1996/06/27 01:11:12  drach
 * - Check for POSIX compliance
 *
 * Revision 1.2  1996/05/03 18:48:44  drach
 * - Added CDMS time routines
 *
 * Revision 1.1  1996/04/25  23:35:07  drach
 * - Initial repository version
 *
 *
 */

#define _POSIX_SOURCE 1
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "latsint.h"
#include "latstime.h"

#define CD_DEFAULT_BASEYEAR "1979"	     /* Default base year for relative time (no 'since' clause) */
#define ISLEAP(year,timeType)	(((timeType) & CdHasLeap) && (!((year) % 4) && (((timeType) & CdJulianType) || (((year) % 100) || !((year) % 400)))))

static int mon_day_cnt[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
static int days_sum[12] = {0,31,59,90,120,151,181,212,243,273,304,334};

/* Return -1, 0, or 1 iff t1 is less than, equal to, */
/* or greater than t2. */
int latsTimeCmp(latsCompTime t1, latsCompTime t2){


  if(t1.year < t2.year) {
    return -1;
    
  } else if( t1.year > t2.year) {
    return 1;
  } else if (t1.year == t2.year) {
    
    if(t1.month < t2.month) {
      return -1;
    } else if(t1.month > t2.month) {
      return 1;
    } else if (t1.month == t2.month) {
      
      if(t1.day < t2.day) {
	return -1;
      } else if(t1.day > t2.day) {
	return 1;
      } else if (t1.day == t2.day) {
	
	if(t1.hour < t2.hour) {
	  return -1;
	} else if(t1.hour > t2.hour) {
	  return 1;
	} else if (t1.hour == t2.hour) {

	  if(t1.min < t2.min) {
	    return -1;
	  } else if(t1.min > t2.min) {
	    return 1;
	  } else {
	    return 0;
	  }
	  
	}
	
      }
    }
    
  } else if(t1.year == t2.year &&
	    t1.month == t2.month &&
	    t1.day == t2.day &&
	    t1.hour == t2.hour &&
	    t1.min == t2.min) {
    return 0;
  } else {
    return 1;
  }
  
  return -2;			     /* Can't be reached, but stupid Cray compiler complains otherwise */
}

/* Compute month and day from year and day-of-year.
 *
 *	Input:
 *		doy	     (int)  (day-of-year)
 *		date->year   (long)  (year since 0 BC)
 *              date->timeType (CdTimetype) (time type)
 *              date->baseYear   base year for relative times
 *	Output: 
 *		date->month  (short)  (month in year) 
 *		date->day    (short)  (day in month)
 *
 * 
 * Derived from NRL NEONS V3.6.
 */

void
latsMonthDay(int *doy, CdTime *date)
{
	int i;				/* month counter */
	int idoy;			/* day of year counter */
	long year;
	
	if ((idoy = *doy) < 1) {
		date->month = 0;
		date->day   = 0;
		return;
	}
	
	if(!(date->timeType & CdChronCal))   /* Ignore year for Clim calendar */
		year = 0;
	else if(!(date->timeType & CdBase1970))	/* year is offset from base for relative time */
		year = date->baseYear + date->year;
	else
		year = date->year;
	
	if (ISLEAP(year,date->timeType)) {
		mon_day_cnt[1] = 29;
	} else {
		mon_day_cnt[1] = 28;
	}
	date->month	= 0;
	for (i = 0; i < 12; i++) {
		(date->month)++;
		date->day	= idoy;
		if ((idoy -= ((date->timeType & Cd365) ? (mon_day_cnt[date->month-1]) : 30)) <= 0) {
			return;
		}
	}
	return;
}


/* Compute number of days in a month
 *
 *	Input:
 *		date->year       (long)  (year since 0 BC)
 *		date->month      (short)  (month in year) 
 *              date->timeType   (CdTimetype) (time type)
 *              date->baseYear   base year for relative times
 * 
 *	Output: 
 *		days    (short)  (number of days in month)
 *
 */

void
latsDaysInMonth(CdTime *date, int *days)
{
	long year;
	
	if(!(date->timeType & CdChronCal))   /* Ignore year for Clim calendar */
		year = 0;
	else if(!(date->timeType & CdBase1970))	/* year is offset from base for relative time */
		year = date->baseYear + date->year;
	else
		year = date->year;
	
	if (ISLEAP(year,date->timeType)) {
		mon_day_cnt[1] = 29;
	} else {
		mon_day_cnt[1] = 28;
	}
	
	*days = (date->timeType & Cd365) ? (mon_day_cnt[date->month-1]) : 30;
	
	return;
}

/* Compute day-of-year from year, month and day
 * 
 *	Input:
 *		date->year  (long)  (year since 0 BC)
 *		date->month (short)  (month in year)
 *		date->day   (short)  (day in month)
 *              date->baseYear   base year for relative times
 *	Output: doy         (int)  (day-of-year)
 * 
 * Derived from NRL NEONS V3.6
 */

void
latsDayOfYear(CdTime *date, int *doy)
{
	int leap_add = 0;		/* add 1 day if leap year */
	int month;			/* month */
	long year;
	
   	month	= date->month;
	if (month < 1 || month > 12) {
		latsError( "Day-of-year error; month: %d\n", month);
		month = 1;	
	}
	
	if(!(date->timeType & CdChronCal))   /* Ignore year for Clim calendar */
		year = 0;
	else if(!(date->timeType & CdBase1970))	/* year is offset from base for relative time */
		year = date->baseYear + date->year;
	else
		year = date->year;
	
	if (ISLEAP(year,date->timeType) && month > 2) leap_add = 1;
	*doy 	 = ((date->timeType & Cd365) ? (days_sum[month-1]) : 30*(month-1)) + date->day + leap_add;
	return;
}
/* Convert human time to epochal time (hours since 00 jan 1, 1970)
 * 
 * Input: htime = human time representation
 * 
 * Output: etime = epochal time representation
 * 
 * Derived from NRL Neons V3.6
 */
void
latsh2e(CdTime *htime, double *etime)
{
	long 	ytemp, year;			/* temporary year holder */
	int	day_cnt;		/* count of days */
	int 	doy;			/* day of year */
	long    baseYear;		     /* base year for epochal time */
	int     daysInLeapYear;		     /* number of days in a leap year */
	int     daysInYear;		     /* days in non-leap year */
	extern void latsDayOfYear(CdTime *date, int *doy);
	
	latsDayOfYear(htime,&doy);
	
	day_cnt	= 0;
	
	baseYear = ((htime->timeType) & CdBase1970) ? 1970 : htime->baseYear;
	year = ((htime->timeType) & CdBase1970) ? htime->year : (htime->year + htime->baseYear);
	if(!((htime->timeType) & CdChronCal)) baseYear = year = 0;	/* set year and baseYear to 0 for Clim */
	daysInLeapYear = ((htime->timeType) & Cd365) ? 366 : 360;
	daysInYear = ((htime->timeType) & Cd365) ? 365 : 360;
	
	if (year > baseYear) {
		for (ytemp = year - 1; ytemp >= baseYear; ytemp--) {
			day_cnt += ISLEAP(ytemp,htime->timeType) ? daysInLeapYear : daysInYear;
		}
	} else if (year < baseYear) {
		for (ytemp = year; ytemp < baseYear; ytemp++) {
			day_cnt -= ISLEAP(ytemp,htime->timeType) ? daysInLeapYear : daysInYear;
		}
	}	
	*etime	= (double) (day_cnt + doy - 1) * 24. + htime->hour +  + htime->min/60.0;
        return;
}
/* Convert epochal time (hours since 00 jan 1, 1970)
 *   to human time (structured)
 * 
 * Input: 
 *   etime = epochal time representation
 *   timeType = time type (e.g., CdChron, CdClim, etc.) as defined in cdms.h
 *   baseYear = base real, used for relative time types only
 * 
 * Output: htime = human (structured) time representation
 * 
 * Derived from NRL Neons V3.6
 */
void
latse2h(double etime, CdTimeType timeType, long baseYear, CdTime *htime)
{
	long 	ytemp;			/* temporary year holder */
	int 	yr_day_cnt;		/* count of days in year */
	int 	doy;			/* day of year */
	int     daysInLeapYear;		     /* number of days in a leap year */
	int     daysInYear;		     /* days in non-leap year */
	extern void latsMonthDay(int *doy, CdTime *date);
	
	doy	= (long) floor(etime / 24.) + 1;
	htime->hour	= etime - (double) (doy - 1) * 24.;
	
	htime->baseYear = (timeType & CdBase1970) ? 1970 : baseYear;
	if(!(timeType & CdChronCal)) htime->baseYear = 0; /* Set base year to 0 for Clim */
	daysInLeapYear = (timeType & Cd365) ? 366 : 360;
	daysInYear = (timeType & Cd365) ? 365 : 360;
	
	if (doy > 0) {
		for (ytemp = htime->baseYear; ; ytemp++) {
			yr_day_cnt = ISLEAP(ytemp,timeType) ? daysInLeapYear : daysInYear;
			if (doy <= yr_day_cnt) break;
			doy -= yr_day_cnt;
		}
	} else {
		for (ytemp = htime->baseYear-1; ; ytemp--) {
			yr_day_cnt = ISLEAP(ytemp,timeType) ? daysInLeapYear : daysInYear;
			doy += yr_day_cnt;
			if (doy > 0) break;
		}
	}
        htime->year = (timeType & CdBase1970) ? ytemp : (ytemp - htime->baseYear);
	if(!(timeType & CdChronCal)) htime->year = 0; /* Set year to 0 for Clim */
	htime->timeType = timeType;
	latsMonthDay(&doy,htime);
	
        return;
}
/* Convert character time to human time
 * 
 * Input:
 *   ctime    = character time
 *   timeType = time type (e.g. CdChron) as defined in cdms.h
 *   
 * Output:
 *   htime    = human (structured) time
 */
void
latsc2h(char *ctime, CdTimeType timeType, CdTime *htime)
{
	int iyear, imon, iday, ihour, imin;
	double dsec;
	long baseYear;
	
	switch(timeType){
	case CdChron: case CdChronNoLeap: case CdChron360:
		sscanf(ctime,"%ld/%hd/%hd %d:%d:%lf",&htime->year,&htime->month,
		       &htime->day,&ihour,&imin,&dsec);
		htime->hour = (double)ihour + (double)imin/60. + dsec/3600;
		htime->baseYear = 1970;
		htime->timeType = timeType;
		break;
	case CdRel: case CdRelNoLeap:
		sscanf(ctime,"%ld+%ld/%hd/%hd %d:%d:%lf",&htime->baseYear,
		       &htime->year,&htime->month,&htime->day,&ihour,&imin,&dsec);
		htime->hour = (double)ihour + (double)imin/60. + dsec/3600;
		htime->timeType = timeType;
		break;
	case CdClim:
		sscanf(ctime,"%hd/%hd %d:%d:%lf",&htime->month,&htime->day,
		       &ihour,&imin,&dsec);
		htime->hour = (double)ihour + (double)imin/60. + dsec/3600;
		htime->year = 0;
		htime->baseYear = 0;
		htime->timeType = timeType;
		break;
	default:
		latsError("Invalid time type: %d\n",timeType);
	}
	return;
}
/* Convert human (structured) time to character time.
 * 
 * Input:
 *   htime = human time
 * 
 * Output:
 *   ctime = character time
 */
void
latsh2c(CdTime *htime, char *ctime)
{
	
	int ihour, imin;
	double dmin, dsec;
	
	ihour = (int) htime->hour;
	dmin = (htime->hour - (double)ihour) * 60.0;
	imin = (int) dmin;
	dsec = (dmin - (double)imin) * 60.0;
	
	switch(htime->timeType){
	case CdChron: case CdChronNoLeap: case CdChron360:
		sprintf(ctime,"%ld/%hd/%hd %d:%d:%.1f",htime->year,htime->month,
			htime->day,ihour,imin,dsec);
		break;
	case CdRel: case CdRelNoLeap:
		sprintf(ctime,"%ld+%ld/%hd/%hd %d:%d:%.1f",htime->baseYear,
			htime->year,htime->month,htime->day,ihour,imin,dsec);
		break;
	case CdClim:
		sprintf(ctime,"%hd/%hd %d:%d:%.1f",htime->month,htime->day,
			ihour,imin,dsec);
		break;
	default:
		latsError("Invalid time type: %d\n",htime->timeType);
		
	}
	return;
}
/* Convert character time to epochal time (hours since 00 jan 1, 1970)
 * 
 * Input:
 *   ctime    = character time
 *   timeType = time type (e.g. CdChron) as defined in cdms.h
 * 
 * Output:
 *   etime    = epochal time 
 */
void
latsc2e(char *ctime, CdTimeType timeType, double *etime, long *baseYear)
{
	CdTime htime;
	extern void latsc2h(char *ctime, CdTimeType timeType, CdTime *htime);
	extern void latsh2e(CdTime *htime, double *etime);
	
	latsc2h(ctime,timeType,&htime);
	latsh2e(&htime,etime);
	*baseYear = htime.baseYear;
	return;
}
/* Convert epochal time (hours since 00 jan 1, 1970) to character time
 * 
 * Input:
 *   etime    = epochal time
 *   timeType = time type, (e.g., CdChron) as defined in cdms.h
 *   baseYear = base year, used for relative time only
 * 
 * Output:
 *   ctime    = character time
 */
void
latse2c(double etime, CdTimeType timeType, long baseYear, char *ctime)
{
	CdTime htime;
	extern void latse2h(double etime, CdTimeType timeType, long baseYear, CdTime *htime);
	extern void latsh2c(CdTime *htime, char *ctime);
	
	latse2h(etime,timeType,baseYear,&htime);
	latsh2c(&htime,ctime);
	return;
}

/* Add 'nDel' times 'delTime' to epochal time 'begEtm',
 * return the result in epochal time 'endEtm'.
 */
void
latsAddDelTime(double begEtm, long nDel, CdDeltaTime delTime, CdTimeType timeType,
	       long baseYear, double *endEtm)
{
	double delHours;
	long delMonths, delYears;
	CdTime bhtime, ehtime;
	
	extern void latse2h(double etime, CdTimeType timeType, long baseYear, CdTime *htime);
	extern void latsh2e(CdTime *htime, double *etime);
	
	switch(delTime.units){
	case CdYear:
		delMonths = 12;
		break;
	case CdSeason:
		delMonths = 3;
		break;
	case CdMonth:
		delMonths = 1;
		break;
	case CdWeek:
		delHours = 168.0;
		break;
	case CdDay:
		delHours = 24.0;
		break;
	case CdHour:
		delHours = 1.0;
		break;
	case CdMinute:
		delHours = 1./60.;
		break;
	case CdSecond:
		delHours = 1./3600.;
		break;
	default:
		latsError("Invalid delta time units: %d\n",delTime.units);
		return;
	}
	
	switch(delTime.units){
	case CdYear: case CdSeason: case CdMonth:
		latse2h(begEtm,timeType,baseYear,&bhtime);
		delMonths = delMonths * nDel * delTime.count + bhtime.month - 1;
		delYears = (delMonths >= 0 ? (delMonths/12) : (delMonths+1)/12 - 1);
		ehtime.year = bhtime.year + delYears;
		ehtime.month = delMonths - (12 * delYears) + 1;
		ehtime.day = 1;
		ehtime.hour = 0.0;
		ehtime.timeType = timeType;
		ehtime.baseYear = !(timeType & CdChronCal) ? 0 :
			(timeType & CdBase1970) ? 1970 : baseYear; /* base year is 0 for Clim, */
		/* 1970 for Chron, */
		/* or input base year for Rel */
		latsh2e(&ehtime,endEtm);
		break;
	case CdWeek: case CdDay: case CdHour: case CdMinute: case CdSecond:
		delHours *= (nDel * delTime.count);
		*endEtm = begEtm + delHours;
		break;
	}
	return;
}

/* Divide ('endEtm' - 'begEtm') by 'delTime',
 * return the integer portion of the result in 'nDel'.
 */
void
latsDivDelTime(double begEtm, double endEtm, CdDeltaTime delTime, CdTimeType timeType,
	       long baseYear, long *nDel)
{
	double delHours, frange;
	long delMonths, range;
	CdTime bhtime, ehtime;
	int hoursInYear;
	
	extern void latse2h(double etime, CdTimeType timeType, long baseYear, CdTime *htime);
	
	switch(delTime.units){
	case CdYear:
		delMonths = 12;
		break;
	case CdSeason:
		delMonths = 3;
		break;
	case CdMonth:
		delMonths = 1;
		break;
	case CdWeek:
		delHours = 168.0;
		break;
	case CdDay:
		delHours = 24.0;
		break;
	case CdHour:
		delHours = 1.0;
		break;
	case CdMinute:
		delHours = 1./60.;
		break;
	case CdSecond:
		delHours = 1./3600.;
		break;
	default:
		latsError("Invalid delta time units: %d\n",delTime.units);
		return;
	}
	
	switch(delTime.units){
	case CdYear: case CdSeason: case CdMonth:
		delMonths *= delTime.count;
		latse2h(begEtm,timeType,baseYear,&bhtime);
		latse2h(endEtm,timeType,baseYear,&ehtime);
		if(timeType & CdChronCal){   /* Chron and Rel time */
			range = 12*(ehtime.year - bhtime.year)
				+ (ehtime.month - bhtime.month);
		}
		else{			     /* Clim time, ignore year */
			range = (ehtime.month - bhtime.month);
			if(range < 0) range += 12;
		}
		*nDel = abs(range)/delMonths;
		break;
	case CdWeek: case CdDay: case CdHour: case CdMinute: case CdSecond:
		delHours *= (double)delTime.count;
		if(timeType & CdChronCal){   /* Chron and Rel time */
			frange = fabs(endEtm - begEtm);
		}
		else{			     /* Clim time, ignore year, but */
			/* wraparound relative to hours-in-year*/
			frange = endEtm - begEtm;
			hoursInYear = (timeType & Cd365) ? 8760. : 8640.;
			/* Normalize frange to interval [0,hoursInYear) */
			if(frange < 0.0 || frange >= hoursInYear)
				frange -= hoursInYear * floor(frange/hoursInYear);
		}
		*nDel = (frange + 1.e-10*delHours)/delHours;
		break;
	}
	return;
}

/* Validate the component time, return 0 if valid, 1 if not */
int
latsValidateTime(cdCalenType timetype, cdCompTime comptime)
{
	if(comptime.month<1 || comptime.month>12){
		latsError("Error on time conversion: invalid month = %hd\n",comptime.month);
		return 1;
	}
	if(comptime.day<1 || comptime.day>31){
		latsError("Error on time conversion: invalid day = %hd\n",comptime.day);
		return 1;
	}
	if(comptime.hour<0.0 || comptime.hour>24.0){
		latsError("Error on time conversion: invalid hour = %lf\n",comptime.hour);
		return 1;
	}
	return 0;
}

/* Trim trailing whitespace, up to n characters. */
/* If no whitespace up to the last character, set */
/* the last character to null, else set the first */
/* whitespace character to null. */
void
latsTrim(char* s, int n)
{
	char* c;
	
	if(s==NULL)
		return;
	for(c=s; *c && c<s+n-1 && !isspace(*c); c++);
	*c='\0';
	return;
}
/* Map to old timetypes */
int
latsToOldTimetype(cdCalenType newtype, CdTimeType* oldtype)
{
	switch(newtype){
	case cdStandard:
		*oldtype = CdChron;
		break;
	case cdJulian:
		*oldtype = CdJulianCal;
		break;
	case cdNoLeap:
		*oldtype = CdChronNoLeap;
		break;
	case cd360:
		*oldtype = CdChron360;
		break;
	case cdClim:
		*oldtype = CdClim;
		break;
	case cdClimLeap:
		*oldtype = CdClimLeap;
		break;
	case cdClim360:
		*oldtype = CdClim360;
		break;
	default:
		latsError("Error on relative units conversion, invalid timetype = %d",newtype);
		return 1;
	}
	return 0;
}

/* Parse relative units, returning the unit and base component time. */
/* Function returns 1 if error, 0 on success */
int
latsParseRelunits(cdCalenType timetype, char* relunits, cdUnitTime* unit, cdCompTime* base_comptime)
{
	char charunits[CD_MAX_RELUNITS];
	char basetime_1[CD_MAX_CHARTIME];
	char basetime_2[CD_MAX_CHARTIME];
	char basetime[CD_MAX_CHARTIME];
	double factor;
	CdTime humantime;
	int nconv;
	CdTimeType old_timetype;
	/* Parse the relunits */
	nconv = sscanf(relunits,"%s since %s %s",charunits,basetime_1,basetime_2);
	if(nconv==EOF || nconv==0){
		latsError("Error on relative units conversion, string = %s\n",relunits);
		return 1;
	}
	
	/* Get the units */
	latsTrim(charunits,CD_MAX_RELUNITS);
	if(!strncmp(charunits,"sec",3) || !strcmp(charunits,"s")){
		*unit = cdSecond;
	}
	else if(!strncmp(charunits,"min",3) || !strcmp(charunits,"mn")){
		*unit = cdMinute;
	}
	else if(!strncmp(charunits,"hour",4) || !strcmp(charunits,"hr")){
		*unit = cdHour;
	}
	else if(!strncmp(charunits,"day",3) || !strcmp(charunits,"dy")){
		*unit = cdDay;
	}
	else if(!strncmp(charunits,"week",4) || !strcmp(charunits,"wk")){
		*unit = cdWeek;
	}
	else if(!strncmp(charunits,"month",5) || !strcmp(charunits,"mo")){
		*unit = cdMonth;
	}
	else if(!strncmp(charunits,"season",6)){
		*unit = cdSeason;
	}
	else if(!strncmp(charunits,"year",4) || !strcmp(charunits,"yr")){
		if(!(timetype & cdStandardCal)){
			latsError("Error on relative units conversion: climatological units cannot be 'years'.\n");
			return 1;
		}
		*unit = cdYear;
	}
	else {
		latsError("Error on relative units conversion: invalid units = %s\n",charunits);
		return 1;
	}
	
	/* Build the basetime, if any (default is 1979), */
	/* or month 1 for climatological time. */
	if(nconv == 1){
		if(timetype & cdStandardCal)
			strcpy(basetime,CD_DEFAULT_BASEYEAR);
		else
			strcpy(basetime,"1");
	}
	/* Convert the basetime to component, then epochal (hours since 1970) */
	else{
		if(nconv == 2){
			latsTrim(basetime_1,CD_MAX_CHARTIME);
			strcpy(basetime,basetime_1);
		}
		else{
			latsTrim(basetime_1,CD_MAX_CHARTIME);
			latsTrim(basetime_2,CD_MAX_CHARTIME);
			sprintf(basetime,"%s %s",basetime_1,basetime_2);
		}
	}
	
	latsChar2Comp(timetype, basetime, base_comptime);
	
	return 0;
}

/* Parse delta time. Return 0 if success, 1 on error. */
int
latsParseDeltaTime(cdCalenType timetype, char* deltaTime, double* value, cdUnitTime* unit){
	char charunits[CD_MAX_TIME_DELTA];
	int nconv;
	
	nconv = sscanf(deltaTime,"%lf %s",value,charunits);
	if(nconv==EOF || nconv==0){
		latsError("Error on delta time conversion, string = %s",deltaTime);
		return 1;
	}
	latsTrim(charunits,CD_MAX_TIME_DELTA);
	if(!strncmp(charunits,"sec",3) || !strcmp(charunits,"s")){
		*unit = cdSecond;
	}
	else if(!strncmp(charunits,"min",3) || !strcmp(charunits,"mn")){
		*unit = cdMinute;
	}
	else if(!strncmp(charunits,"hour",4) || !strcmp(charunits,"hr")){
		*unit = cdHour;
	}
	else if(!strncmp(charunits,"day",3) || !strcmp(charunits,"dy")){
		*unit = cdDay;
	}
	else if(!strncmp(charunits,"week",4) || !strcmp(charunits,"wk")){
		*unit = cdWeek;
	}
	else if(!strncmp(charunits,"month",5) || !strcmp(charunits,"mo")){
		*unit = cdMonth;
	}
	else if(!strncmp(charunits,"season",6)){
		*unit = cdSeason;
	}
	else if(!strncmp(charunits,"year",4) || !strcmp(charunits,"yr")){
		if(!(timetype & cdStandardCal)){
			latsError("Error on delta time conversion: climatological units cannot be 'years'.");
			return 1;
		}
		*unit = cdYear;
	}
	else {
		latsError("Error on delta time conversion: invalid units = %s",charunits);
		return 1;
	}
	return 0;
}

void
latsChar2Comp(cdCalenType timetype, char* chartime, cdCompTime* comptime)
{
	double hour, sec;
	int ihr, imin, nconv;
	long year;
	short day;
	short month;
	
	comptime->year = CD_NULL_YEAR;
	comptime->month = CD_NULL_MONTH;
	comptime->day = CD_NULL_DAY;
	comptime->hour = CD_NULL_HOUR;
	comptime->min = CD_NULL_MIN;
	
	if(timetype & cdStandardCal){
		nconv = sscanf(chartime,"%ld-%hd-%hd %d:%d:%lf",&year,&month,&day,&ihr,&imin,&sec);
		if(nconv==EOF || nconv==0){
			latsError("Error on character time conversion, string = %s\n",chartime);
			return;
		}
		if(nconv >= 1){
			comptime->year = year;
		}
		if(nconv >= 2){
			comptime->month = month;
		}
		if(nconv >= 3){
			comptime->day = day;
		}
		if(nconv >= 4){
			if(ihr<0 || ihr>23){
				latsError("Error on character time conversion: invalid hour = %d\n",ihr);
				return;
			}
			comptime->hour = (double)ihr;
		}
		if(nconv >= 5){
			if(imin<0 || imin>59){
				latsError("Error on character time conversion: invalid minute = %d\n",imin);
				return;
			}
			comptime->hour += (double)imin/60.;
		}
		if(nconv >= 6){
			if(sec<0.0 || sec>60.0){
				latsError("Error on character time conversion: invalid second = %lf\n",sec);
				return;
			}
			comptime->hour += sec/3600.;
		}
	}
	else{				     /* Climatological */
		nconv = sscanf(chartime,"%hd-%hd %d:%d:%lf",&month,&day,&ihr,&imin,&sec);
		if(nconv==EOF || nconv==0){
			latsError("Error on character time conversion, string = %s",chartime);
			return;
		}
		if(nconv >= 1){
			comptime->month = month;
		}
		if(nconv >= 2){
			comptime->day = day;
		}
		if(nconv >= 3){
			if(ihr<0 || ihr>23){
				latsError("Error on character time conversion: invalid hour = %d\n",ihr);
				return;
			}
			comptime->hour = (double)ihr;
		}
		if(nconv >= 4){
			if(imin<0 || imin>59){
				latsError("Error on character time conversion: invalid minute = %d\n",imin);
				return;
			}
			comptime->hour += (double)imin/60.;
		}
		if(nconv >= 5){
			if(sec<0.0 || sec>60.0){
				latsError("Error on character time conversion: invalid second = %lf\n",sec);
				return;
			}
			comptime->hour += sec/3600.;
		}
	}
	(void)latsValidateTime(timetype,*comptime);
			return;
}
void
latsChar2Rel(cdCalenType timetype, char* chartime, char* relunits, double* reltime)
{
	cdCompTime comptime;
	
	latsChar2Comp(timetype, chartime, &comptime);
	latsComp2Rel(timetype, comptime, relunits, reltime);
	return;
}
void
latsComp2Char(cdCalenType timetype, cdCompTime comptime, char* time)
{
	double dtmp, sec;
	int ihr, imin;
	int nskip;
	
	if(latsValidateTime(timetype,comptime))
		return;
	
	ihr = (int)comptime.hour;
	dtmp = 60.0 * (comptime.hour - (double)ihr);
	imin = (int)dtmp;
	sec = 60.0 * (dtmp - (double)imin);
	
	nskip = 0;
	if(sec == 0.0){
		if(imin == 0)
			nskip = 2;
		else
			nskip = 1;
	}
	
	if(timetype & cdStandardCal){
		if(nskip == 0)
			sprintf(time,"%ld-%hd-%hd %d:%d:%lf",comptime.year,comptime.month,comptime.day,ihr,imin,sec);
		else if(nskip == 1)
			sprintf(time,"%ld-%hd-%hd %d:%d",comptime.year,comptime.month,comptime.day,ihr,imin);
		else
			sprintf(time,"%ld-%hd-%hd %d:0",comptime.year,comptime.month,comptime.day,ihr);
	}
	else {				     /* Climatological */
		if(nskip == 0)
			sprintf(time,"%hd-%hd %d:%d:%lf",comptime.month,comptime.day,ihr,imin,sec);
		else if(nskip == 1)
			sprintf(time,"%hd-%hd %d:%d",comptime.month,comptime.day,ihr,imin);
		else
			sprintf(time,"%hd-%hd %d:0",comptime.month,comptime.day,ihr);
	}
	return;
}
void
latsComp2Rel(cdCalenType timetype, cdCompTime comptime, char* relunits, double* reltime)
{
	cdCompTime base_comptime;
	CdDeltaTime deltime;
	CdTime humantime;
	CdTimeType old_timetype;
	cdUnitTime unit;
	double base_etm, etm, delta;
	long ndel, hoursInYear;
	
	/* Parse the relunits */
	if(latsParseRelunits(timetype, relunits, &unit, &base_comptime))
		return;
	
	/* Convert basetime to epochal */
	humantime.year = base_comptime.year;
	humantime.month = base_comptime.month;
	humantime.day = base_comptime.day;
	humantime.hour = base_comptime.hour;
	humantime.min = base_comptime.min;
	humantime.baseYear = 1970;
	/* Map to old-style timetype */
	if(latsToOldTimetype(timetype,&old_timetype))
		return;
	humantime.timeType = old_timetype;
	latsh2e(&humantime,&base_etm);
	
	/* Map end time to epochal */
	humantime.year = comptime.year;
	humantime.month = comptime.month;
	humantime.day = comptime.day;
	humantime.hour = comptime.hour;
	humantime.min = comptime.min;
	latsh2e(&humantime,&etm);
	/* Calculate relative time value for months or hours */
	deltime.count = 1;
	deltime.units = (CdTimeUnit)unit;
	switch(unit){
	case cdWeek: case cdDay: case cdHour: case cdMinute: case cdSecond:
		delta = etm - base_etm;
		if(!(timetype & cdStandardCal)){	/* Climatological time */
			hoursInYear = (timetype & cd365Days) ? 8760. : (timetype & cdHasLeap) ? 8784. : 8640.;
			/* Normalize delta to interval [0,hoursInYear) */
			if(delta < 0.0 || delta >= hoursInYear)
				delta -= hoursInYear * floor(delta/hoursInYear);
		}
		break;
	case cdYear: case cdSeason: case cdMonth:
		latsDivDelTime(base_etm, etm, deltime, old_timetype, 1970, &ndel);
		break;
	}
	
	/* Convert to output units */
	switch(unit){
	case cdSecond:
		*reltime = 3600.0 * delta;
		break;
	case cdMinute:
		*reltime = 60.0 * delta;
		break;
	case cdHour:
		*reltime = delta;
		break;
	case cdDay:
		*reltime = delta/24.0;
		break;
	case cdWeek:
		*reltime = delta/168.0;
		break;
	case cdMonth: case cdSeason: case cdYear: /* Already in correct units */
		if(timetype & cdStandardCal)
			*reltime = (base_etm <= etm) ? (double)ndel : (double)(-ndel);
		else			     /* Climatological time is already normalized*/
			*reltime = (double)ndel;
		break;
	}
	
	return;
}
void
latsRel2Char(cdCalenType timetype, char* relunits, double reltime, char* chartime)
{
	cdCompTime comptime;
	
	latsRel2Comp(timetype, relunits, reltime, &comptime);
	latsComp2Char(timetype, comptime, chartime);
	
	return;
}
void
latsRel2Comp(cdCalenType timetype, char* relunits, double reltime, cdCompTime* comptime)
{
	CdDeltaTime deltime;
	CdTime humantime;
	CdTimeType old_timetype;
	cdCompTime base_comptime;
	cdUnitTime unit, baseunits;
	double base_etm, result_etm;
	double delta, hour_fraction;
	long idelta;
	
	/* Parse the relunits */
	if(latsParseRelunits(timetype, relunits, &unit, &base_comptime))
		return;
	
	
	switch(unit){
	case cdSecond:
		delta = reltime/3600.0;
		baseunits = cdHour;
		break;
	case cdMinute:
		delta = reltime/60.0;
		baseunits = cdHour;
		break;
	case cdHour:
		delta = reltime;
		baseunits = cdHour;
		break;
	case cdDay:
		delta = 24.0 * reltime;
		baseunits = cdHour;
		break;
	case cdWeek:
		delta = 168.0 * reltime;
		baseunits = cdHour;
		break;
	case cdMonth:
		idelta = (long)(reltime + (reltime<0 ? -1.e-10 : 1.e-10));
		baseunits = cdMonth;
		break;
	case cdSeason:
		idelta = (long)(3.0 * reltime + (reltime<0 ? -1.e-10 : 1.e-10));
		baseunits = cdMonth;
		break;
	case cdYear:
		idelta = (long)(12 * reltime + (reltime<0 ? -1.e-10 : 1.e-10));
		baseunits = cdMonth;
		break;
	}
	
	deltime.count = 1;
	deltime.units = (CdTimeUnit)baseunits;
	
	humantime.year = base_comptime.year;
	humantime.month = base_comptime.month;
	humantime.day = base_comptime.day;
	humantime.hour = base_comptime.hour;
	humantime.baseYear = 1970;
	/* Map to old-style timetype */
	if(latsToOldTimetype(timetype,&old_timetype))
		return;
	humantime.timeType = old_timetype;
	
	latsh2e(&humantime,&base_etm);
	/* If months, seasons, or years, */
	if(baseunits == cdMonth){
		
		/* Calculate new epochal time from integer months. */
		/* Convert back to human, then comptime. */
		/* For zero reltime, just return the basetime*/
		if(reltime != 0.0){
			latsAddDelTime(base_etm,idelta,deltime,old_timetype,1970,&result_etm);
			latse2h(result_etm, old_timetype, 1970, &humantime);
		}
	}
	/* Calculate new epochal time. */
	/* Convert back to human, then comptime. */
	else{
		latse2h(base_etm+delta, old_timetype, 1970, &humantime);
		
	}
	comptime->year = humantime.year;
	comptime->month = humantime.month;
	comptime->day = humantime.day;
	comptime->hour = humantime.hour;
	
	return;
}
void
latsRel2Rel(cdCalenType timetype, char* relunits, double reltime, char* outunits, double* outtime)
{
	cdCompTime comptime;
	
	latsRel2Comp(timetype, relunits, reltime, &comptime);
	latsComp2Rel(timetype, comptime, outunits, outtime);
	return;
}
