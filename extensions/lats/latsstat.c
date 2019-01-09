
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS statistics functions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latsstat.c,v 1.2 2011/10/09 20:20:49 dasilva Exp $
 *
 * Revision History:
 *
 * $Log: latsstat.c,v $
 * Revision 1.2  2011/10/09 20:20:49  dasilva
 * ams: minor mods for mac os x snow leopard
 *
 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.5  2008/01/19 19:24:47  pertusus
 * Use the pkgconfig result unless dap root was set.
 * change <config.h> to "config.h".
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.2  2002/10/28 19:08:33  joew
 * Preliminary change for 'autonconfiscation' of GrADS: added a conditional
 * #include "config.h" to each C file. The GNU configure script generates a unique config.h for each platform in place of -D arguments to the compiler.
 * The include is only done when GNU configure is used.
 *
 * Revision 1.1.1.1  2002/06/27 19:44:18  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:57  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.8  1997/10/15 17:53:21  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.7  1996/10/22  19:05:12  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.6  1996/08/20 18:34:12  drach
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
 * Revision 1.5  1996/07/12 00:36:28  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.4  1996/06/27 01:15:33  drach
 * - Added QC calculations
 *
 * Revision 1.3  1996/05/25 00:27:51  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.2  1996/05/03 18:59:26  drach
 * - Moved vertical dimension definition from lats_var to lats_vert_dim
 * - Changed lats_miss_double to lats_miss_float
 * - Made time dimension file-dependent, revised lats_write accordingly
 * - Added lats_var_nc, lats_vert_dim_nc
 * - Allow GRIB-only compilation
 * - Added FORTRAN interface
 *
 * Revision 1.1  1996/04/25  23:35:06  drach
 * - Initial repository version
 *
 *
 */

#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include "latsint.h"

#define LATS_SIMTIME_CHARS 10
#define LATS_TIMESTAMP_CHARS 14
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static FILE *latsLogFile = (FILE *)0;	     /* QC log file */

static char monthnames[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

/* Calculate statistics on a horizontal cross-section of a variable.
 * 'levindex' is the 0-origin index (into var->levs) of the level, or -1 if there are no levels.
 * 'timeindex' is the 0-origin time index, or -1 if there are no times.
 */
int latsStatVar(latsFile *file, latsVar *var, latsParmQC *qc, int levindex, int timeindex, latsCompTime ctime, void *data) {
        float *x, xmin, xmax, xmean;
	int data_amt;
	float xMissing;
	double sumw, sumx, sumx2, sumwinv;
	int i, j;
	double w, wi, wx, variance;
	char simtime[LATS_SIMTIME_CHARS+1];
	char timestamp[LATS_TIMESTAMP_CHARS+1];
	time_t now;
	struct tm *t;
	int qcerror;
	char *stat;
	float qcvalue, qcmark1, qcmark2;
	char *defaultLogFile;
	int ihour;
	
	/* If quality control is turned off, or no QC table information */
	/* for this level, just return */
	if(!lats_qc || !qc)
		return 1;
	
	/* No QC at present for integer data */
	if(var->parm->datatype == LATS_INT)
		return 1;
	
	/* Open the QC log file, if necessary */
	if(latsLogFile == (FILE *)0){
		if((defaultLogFile = getenv("LATS_LOG")) == NULL){
			defaultLogFile = "lats.log";
		}
		if((latsLogFile = fopen(defaultLogFile,"ab")) == NULL){
			latsError("Cannot open log file: %s for append", defaultLogFile);
			return 0;
		}
	}
	
	xmin = FLT_MAX;
	xmax = -FLT_MAX;
	sumw = sumx = sumx2 = 0.0;
	
	x = (float *)data;
	data_amt = LATS_ALL_DATA;
	
	if(var->hasmissing){
		xMissing = var->missing.f;
		for(i=0; i<var->grid->nlat; i++){
			wi = var->grid->wlats[i];
			for(j=0; j<var->grid->nlon; j++){
				if(fabs(*x - xMissing) > var->missingdelta){
					xmin = MIN(*x,xmin);
					xmax = MAX(*x,xmax);
					w = wi * var->grid->wlons[j];
					wx = w * (*x);
					sumw += w;
					sumx += wx;
					sumx2 += (wx * (*x));
				}
				else{
					data_amt = LATS_SOME_DATA;
				}
				x++;
			}
		}
	}
	else{				     /* No missing data */
		for(i=0; i<var->grid->nlat; i++){
			wi = var->grid->wlats[i];
			for(j=0; j<var->grid->nlon; j++){
				xmin = MIN(*x,xmin);
				xmax = MAX(*x,xmax);
				w = wi * var->grid->wlons[j];
				wx = w * (*x);
				sumw += w;
				sumx += wx;
				sumx2 += (wx * (*x));
				x++;
			}
		}
	}
	/* Area-weighted average and variance */
	if(sumw != 0.0){
		sumwinv = 1.0/sumw;
		xmean = sumwinv * sumx;
		variance = sumwinv * (sumx2 - sumwinv*sumx*sumx);
		qc->std = (variance < 0.0 ? 0.0 : sqrt(variance));
	}
	else
		data_amt = LATS_NO_DATA;
	
	qc->mean = xmean;
	qc->max = xmax;
	qc->min = xmin;
	
	/* Issue error if:
	 * (1) abs(mean-obsmean) > nstd*obsstd, or
	 * (2) xmax-xmin > obsrange*obsrangetol
	 */
	qcerror = 0;
	if(fabs(xmean - qc->obsmean) > (qc->nstd * qc->obsstd)){
		/* Mean is out of range */
		qcerror = 1;
		stat = "mean";
		qcvalue = xmean;
		qcmark1 = qc->obsmean;
		qcmark2 = qc->nstd * qc->obsstd;
	}
	else if((xmax-xmin) > (qc->obsrangetol * qc->obsrange)){
		/* Range is too large */
		qcerror = 1;
		stat = "range";
		qcvalue = xmax-xmin;
		qcmark1 = qc->obsrange;
		qcmark2 = qc->obsrangetol * qc->obsrange;
	}
	
	if(qcerror){
		now = time((time_t *)NULL);
		t = localtime(&now);
		sprintf(timestamp, "%4d%2d%2d%2d%2d%2d", t->tm_year+1900, t->tm_mon+1,
			t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		for(i=0; i<LATS_TIMESTAMP_CHARS; i++)
			if(timestamp[i]==' ') timestamp[i] = '0';
		sprintf(simtime, "%4d%2d%2d%2d", (int)ctime.year, ctime.month, ctime.day, (ihour = ctime.hour));
		for(i=0; i<LATS_SIMTIME_CHARS; i++)
			if(simtime[i]==' ') simtime[i] = '0';
		/* name | time | level | stat | value | QCmark1 | QCmark2 | timestamp */
		fprintf(latsLogFile, "E|%s|%s|%f|%s|%f|%f|%f|%s\n",
			var->name, simtime, (levindex<0 ? 0.0 : var->levs[levindex]), stat, qcvalue, qcmark1, qcmark2, timestamp);
		return 0;
	}
	
	return 1;
}

/* Calculate global statistics for all variables in a file.
 */
int latsStatFile(latsFile *file) {
	latsVar *var;
	int ihour;
	
	for(var=file->varlist; var; var=var->next){
		
		/* Check that all times were written */
		if(var->nlevwritten == 0){
			latsWarning("Variable declared but never written, file %s, variable %s",
				    file->path, var->name);
		}
		else if(var->nlev>0 && var->nlevwritten<var->nlev){
			latsWarning("Fewer levels written (%d) then declared (%d), file %s, variable %s, time = %dZ %d %s %d: The output file may be unnecessarily large",
				    var->nlevwritten, var->nlev, file->path, var->name,
				    (ihour = file->time.hour), file->time.day, monthnames[file->time.month-1],
				    file->time.year);
		}
	}
	
	
	/* Calculate variable global statistics */
	if(!lats_qc)
		return 1;
	
	/* Close the QC log file */
	if( (latsLogFile != (FILE *)0) ) {
		fclose(latsLogFile);
	}
	latsLogFile = (FILE *)0;
	
	return 1;
}
