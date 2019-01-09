
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS internal functions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latsint.c,v 1.3 2009/10/15 01:17:53 dasilva Exp $
 * Version:     $Id: latsint.c,v 1.3 2009/10/15 01:17:53 dasilva Exp $
 *
 * Revision History:
 *
 * $Log: latsint.c,v $
 * Revision 1.3  2009/10/15 01:17:53  dasilva
 * ams: work in progress
 *
 * Revision 1.2  2009/10/10 06:34:15  mike_fiorino
 * mf 20091010 -- incorporate all my mods 1.10 lats into 2.0 lats extension
 *
 * Revision 1.6  2009/03/18 15:52:39  mike_fiorino
 * mf:lats bugs fix + minutes support; set z 1 last ; control of line properties of gxout grid
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
 * Revision 1.1.1.1  2002/06/27 19:44:16  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:57  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 * Revision 1.14  1997/10/15 17:53:17  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.1  1997/02/14 20:13:11  fiorino
 * Initial revision
 *
 * Revision 1.13  1996/11/11 22:39:20  drach
 * - Added function to set the basetime (lats_basetime)
 *
 * Revision 1.12  1996/10/22 19:05:08  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.11  1996/10/10 23:15:45  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.10  1996/08/27 19:44:25  drach
 * - Fixed up minor compiler warnings
 *
 * Revision 1.9  1996/08/20 18:34:09  drach
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
 * Revision 1.8  1996/07/12 00:36:26  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.7  1996/06/27 01:13:01  drach
 * - Remove timestat table
 *
 * Revision 1.6  1996/06/12 00:42:39  drach
 * - Create default table from latsparm.h
 *
 * Revision 1.5  1996/05/25 00:27:49  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.4  1996/05/10 22:44:41  drach
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
 * Revision 1.2  1996/04/25  23:32:06  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:53:01  drach
 * Initial repository version
 *
 *
 */

#define _POSIX_SOURCE 1
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "latsint.h"
#include "latsparm.h"

/* Globals */
int lats_fatal = 1;			     /* If set to 1, exit on fatal error */
int lats_verbose = 1;			     /* If set to 1, errors are reported */
int lats_qc = 1;			     /* If set to 1, call quality control routines */

static int latsNextFileID = 1;		     /* Next File ID */
static int latsNgrids = 0;		     /* Number of entries in latsGridList */
static int latsNextVarID = 1;		     /* Next Variable ID */
static int latsNextVertID = 1;		     /* Next vertical dimension ID */
static latsFile *latsFileHead = (latsFile *)0; /* Head of file list */
static latsGrid latsGridList[LATS_MAX_GRIDS];	     /* Global grid array */
static latsVertDim *latsVertHead = (latsVertDim *)0; /* Head of vertical dimension list */
static int latsParmTableSize;		     /* Length of parameter table */
static latsParm latsParmTable[LATS_MAX_PARMS];	     /* Parameter table */
static int latsVertTableSize;		     /* Length of vertical types table */
static latsVertType latsVertTypeTable[LATS_MAX_VERT_TYPES]; /* Vertical types table */
static int latsCenterTableSize;		     /* Length of center table */
static latsCenter latsCenterTable[LATS_MAX_CENTERS];	     /* Center table */
static int latsQCTableAllocSize = 0;	     /* Allocated ength of QC table in number of entries*/
static int latsQCTableSize;		     /* Number of entries in QC table */
static latsParmQC *latsQCTable; 	     /* QC table */

FILE* parmFile=NULL;                         /*mf 970818 - moved here to initialize for multiply opens */

/* Copy up to n characters from 'src' to 'sink', starting
 * with the first non-space character of 'src'. Trim trailing
 * whitespace characters. 'sink' will be null-terminated,
 * regardless of whether 'src' is.
 */

void latsCpyTrim(char* sink, char* src, int n){
	char *s, *t;
	/* Find the first non-space character */
	for(s=src; *s && s<src+n && isspace(*s); s++);
	if(s==(src+n)){
		*sink = '\0';
		return;
	}
	/* Copy up to n characters */
	for(t=sink; t<sink+n && (*t++ = *s++););
	
	/* Find the last non-space character */
	for(t-=2; t>=sink && isspace(*t); t--);
	
	/* Null-terminate src */
	if(t<sink)
		*sink = '\0';
	else if(t>=sink+n-1)
		*(sink+n-1)='\0';
	else
		*++t = '\0';
	return;
}

/* Copy src to sink, up to n characters, trim leading, trailing whitespace,
 * change to lower case.
 */
void latsCpyLower(char* sink, char* src, int n){
	char *cp;
	
	latsCpyTrim(sink, src, n);
	for(cp=sink; *cp; cp++)
		*cp = tolower(*cp);
	return;
}

/* Translate item to double value d. If item is blank, set d to dfault.
 * Return 1 on success, 0 on error.
 */
int latsStrtod(char *item, double *d, double dfault){
	char tempname[LATS_MAX_NAME];
	char *final;
	
	latsCpyTrim(tempname, item, LATS_MAX_NAME);
	if(tempname[0] == '\0'){
		*item = dfault;
		return 1;
	}
	*d = strtod(tempname, &final);
	if(*final){
		*d = dfault;
		return 0;
	}
	else
		return 1;
}

/* Translate item to float value f. If item is blank, set f to dfault.
 * Return 1 on success, 0 on error.
 */
int latsStrtof(char *item, float *f, float dfault){
	char tempname[LATS_MAX_NAME];
	char *final;
	
	latsCpyTrim(tempname, item, LATS_MAX_NAME);
	if(tempname[0] == '\0'){
		*item = dfault;
		return 1;
	}
	*f = (float)strtod(tempname, &final);
	if(*final){
		*f = dfault;
		return 0;
	}
	else
		return 1;
}
/* Translate item to int value i, base 10. If item is blank, set i to dfault.
 * Return 1 on success, 0 on failure.
 */
int latsStrtoi(char *item, int *i, int dfault){
	char tempname[LATS_MAX_NAME];
	char *final;
	
	latsCpyTrim(tempname, item, LATS_MAX_NAME);
	if(tempname[0] == '\0'){
		*i = dfault;
		return 1;
	}
	*i = (int)strtol(tempname, &final, 10);
	if(*final){
		*i = dfault;
		return 0;
	}
	else
		return 1;
	
}

/* Lookup a file, given its 'id'. Return its struct, or 0
 * if not found.
 */

latsFile* latsFileLookup(int id) {
	latsFile *file;
	
	for(file = latsFileHead; file; file = file->next){
		if(file->id == id)
			return file;
	}
	latsError("LATS (latsint.c) --> File ID: %d, not found", id);
	return 0;
}

/* Delete a file entry from the file list, given its 'id'.
 * Return 1 if successful, 0 if an error occurred.
 */

int latsFileDeleteEntry(int id) {
	latsFile *file, *p;
	latsVar *var, *varnext;
	latsGrid *grid;
	int found, i;
	
	found = 0;
	if(latsFileHead && latsFileHead->id == id){ /* First file */
		file = latsFileHead;
		latsFileHead = file->next;
		found = 1;
	}
	else if(latsFileHead){	     /* >1 file open */
		for(p=latsFileHead; p->next; p = p->next){
			if(p->next->id == id){
				file = p->next;
				p->next = file->next;
				found = 1;
				break;
			}
		}
	}
	/* Found == 1 iff lookup succeeded */
	if(found==0){
		latsError("LATS (latsint.c) --> File ID: %d, not found", id);
		return 0;
	}
	
	/* Delete variables */
	for(var=file->varlist; var; var=varnext) {
		varnext = var->next;
		if(var->qctable) free(var->qctable);
		free(var);
	}
	
	/* Free memory */
	free(file);
	return 1;
	
}

/* Add a file to the file list, given its 'path'.
 * Return its struct, or 0 if an error occurred.
 */

latsFile* latsFileAddEntry(char* path) {
	latsFile *file;
	
	if((file = (latsFile *)malloc(sizeof(latsFile)))==0){
		latsError("LATS (latsint.c) --> Adding entry for file %s: no more memory",path);
		return 0;
	}
	
	file->next = latsFileHead;
	latsFileHead = file;
	
	strncpy(file->path, path, LATS_MAX_PATH);
	file->basetimeset = 0;
	file->id = latsNextFileID++;
	file->ndelta = 0;
	file->ngrid = 0;
	file->ntimewritten = 0;
	file->nvertdim = 0;
	
	file->latsmode = LATS_MODE_DEFINE;    /*mf 970214 intialize to define mode for consistency with netcdf */
	
	file->fhour = 0        ;              /*mf 970517 intialize forecast hour */
	file->fmin = 0        ;              /*mf 970517 intialize forecast hour */
	
	file->btime.year = 0;                 /*mf 970517 intialize base year */
	file->btime.month = 0;                /*mf 970517 intialize base month */
	file->btime.day = 0;                  /*mf 970517 intialize base day */
	file->btime.hour = 0.0;               /*mf 970517 intialize base hour */
	file->btime.min = 0.0;               /*mf 970517 intialize base min */
	
	file->nvar = 0;
	file->varlist = 0;
	file->depend = 0;
	
	return file;
}

/* Lookup vertical dimension with ID 'vertid' in 'file' vertlist. */
/* If not found in file list, look in global list, and, if found, */
/* add to file list. Return file dimension pointer if found, or 0 if */
/* error or not found. */
latsVertDim *latsFileVertLookup(latsFile *file, int vertid){
	latsVertDim *vert, *rvert;
	
	/* Look in file list first */
	for(vert = file->vertlist; vert < file->vertlist + file->nvertdim; vert++){
		if(vert->id == vertid)
			return vert;
	}
	
	/* Not in file list, search global list */
	if(vert = latsVertLookup(vertid)){
		if(file->nvertdim < LATS_MAX_VERT_DIMS){
			/* NOTE! copy to file list, so that netCDF IDs are localized */
			rvert = file->vertlist + file->nvertdim++;
			*rvert = *vert;
		}
		else {
			latsError("LATS (latsint.c) --> Max vertical dimensions: %d, exceeded for file: %s",
				  LATS_MAX_VERT_DIMS, file->path);
			return 0;
		}
	}
	else {
		/* Dimension not found in file or global list */
		return 0;
	}
	return rvert;
}

/* Lookup a grid with ID 'gridid', in 'file' gridlist.
 * If not found in file gridlist, copy to file gridlist from
 * global grid array, if present, and return pointer to
 * file gridlist entry. Return 0 if error or not found.
 */
latsGrid *latsFileGridLookup(latsFile *file, int gridid){
	latsGrid *grid;
	
	if(gridid <= 0 || gridid > latsNgrids){
		latsError("LATS (latsint.c) --> Invalid grid ID: %d", gridid);
		return 0;
	}
	/* First check the file gridlist */
	for(grid = file->gridlist; grid < file->gridlist + file->ngrid; grid++){
		if(grid->id == gridid)
			return grid;
	}
	/* Not in the file gridlist ... */
	if(file->ngrid == LATS_MAX_GRIDS){
		latsError("LATS (latsint.c) --> Max number of grids allowed for file %s is %d, gridid: %d",
			  file->path, LATS_MAX_GRIDS, gridid);
		return 0;
	}
	/* Copy global entry to file gridlist */
	grid = file->gridlist + file->ngrid++;
	*grid = latsGridList[gridid-1];
	return grid;
}

/* Lookup a variable, given its file 'fileid', and 
 * variable 'varid'. Return the variable struct if
 * found, or 0 id not found or an error occurred.
 */

latsVar* latsVarLookup(int fileid, int varid) {
	latsFile *file;
	latsVar *var;
	
	/* Find the file */
	if((file = latsFileLookup(fileid))==0)
		return 0;
	
	/* Search for the variable */
	for(var = file->varlist; var; var = var->next){
		if(var->id == varid)
			return var;
	}
	latsError("LATS (latsint.c) --> Variable ID: %d, not found, file %s", varid, file->path);
	return 0;
}

/* Add a variable with name 'varname' to the variable list
 * of file with ID 'fileid'. Return the initialized struct, 
 * or 0 if an error occurred. It is considered an error if
 * the variable already exists in the file.
 */

latsVar* latsVarAddEntry(int fileid, char* varname) {
	latsFile *file;
	latsVar *var, *v;
	
	/* Find the file */
	if((file = latsFileLookup(fileid))==0)
		return 0;
	
	/* Look for a variable of the same name */
	for(v = file->varlist; v; v = v->next){
		if(!strcmp(varname, v->name)){
			latsError("LATS (latsint.c) --> Variable already defined: %s, file %s", varname, file->path);
			return 0;
		}
	}
	
	if((var = (latsVar *)malloc(sizeof(latsVar)))==0){
		latsError("LATS (latsint.c) --> Adding entry for variable %s: no more memory", varname);
		return 0;
	}
	
	/* Add to the file's varlist */
	var->next = file->varlist;
	file->varlist = var;
	file->nvar++;
	
	/* Initialize */
	strncpy(var->name, varname, LATS_MAX_NAME);
	var->id = latsNextVarID++;
	var->levs = 0;
	var->ncid = -1;
	var->nlev = var->nlevwritten = 0;
	var->qctable = 0;
	var->ntimewritten = 0;
	var->timeerror = 0;
	var->timemissing = 0;
	var->file = file;
	var->depend = 0;
	var->hasmissing = 0;
	
	return var;
}

/* Lookup 'lev' in strictly monotonic array 'levs', of length 'nlev'.
 * lev1 and lev2 are equal iff abs(lev1-lev2) < delta.
 * It is assumed that:
 *   'levs' is strictly monotonic
 *   'delta' is strictly positive
 *   'delta' is small enough to resolve any element of 'levs'
 * Return 0-origin index if found, -1 if not found.
 */
int latsLevLookup(double lev, int nlev, double levs[], double delta){
	int klo, khi, kmid;
	int rel;
	
	klo = 0;
	khi = nlev - 1;
	if(nlev==1 || levs[0]<levs[1]) {
		/* levs increasing */
		while(klo <= khi){
			kmid = (khi + klo) >> 1;
			if(lev <= (levs[kmid]-delta))
				khi = kmid-1;
			else if(lev >= (levs[kmid]+delta))
				klo = kmid+1;
			else {
				return kmid;
			}
		}
	}
	else {
		/* levs decreasing */
		while(klo <= khi){
			kmid = (khi + klo) >> 1;
			if(lev >= (levs[kmid]+delta))
				khi = kmid-1;
			else if(lev <= (levs[kmid]-delta))
				klo = kmid+1;
			else {
				return kmid;
			}
		}
	}
	return -1;
}

/* Lookup a parameter in the parameter table.
 * Return the parm struct if found, or 0 if not found
 * or an error occurred.
 */

latsParm* latsParmLookup(char* name) {
	int klo, khi, kmid;
	int rel;
	
	klo = 0;
	khi = latsParmTableSize - 1;
	while(klo <= khi){
		kmid = (khi + klo) >> 1;
		if((rel = strcmp(name,latsParmTable[kmid].name))<0)
			khi = kmid-1;
		else if(rel>0)
			klo = kmid+1;
		else {
			return latsParmTable+kmid;
		}
	}
	return 0;
}

/* Comparison function for sorting parameter table.
 * Returns -1, 0, or 1, depending on the relative order
 * of parm1 and parm2.
 */

int latsSortParms(latsParm* parm1, latsParm* parm2) {
	return strcmp(parm1->name, parm2->name);
}

/* Comparison function for sorting QC table.
 * Sort by name, level type, and increasing value, in that order.
 * Returns -1, 0, or 1, depending on the relative order
 * of qc1 and qc2.
 */
int latsSortQC(latsParmQC *qc1, latsParmQC *qc2){
	int sort1;
	
	if((sort1 = strcmp(qc1->name, qc2->name)) !=0)
		return sort1;
	else if((sort1 = strcmp(qc1->levtype, qc2->levtype)) != 0)
		return sort1;
	else if(qc1->value < qc2->value)
		return -1;
	else if(qc1->value > qc2->value)
		return 1;
	else
		return 0;
}

/* Comparison function for sorting QC table with delta fudge factor.
 * Sort by name, level type, and increasing value, in that order.
 * Returns -1, 0, or 1, depending on the relative order
 * of qc1 and qc2.
 * Equality of value is defined +/- delta.
 */
int latsSortQCDelta(latsParmQC *qc1, latsParmQC *qc2, double delta){
	int sort1;
	
	if((sort1 = strcmp(qc1->name, qc2->name)) !=0)
		return sort1;
	else if((sort1 = strcmp(qc1->levtype, qc2->levtype)) != 0)
		return sort1;
	else if(qc1->value < qc2->value - delta)
		return -1;
	else if(qc1->value > qc2->value + delta)
		return 1;
	else
		return 0;
}

/* Lookup a QC entry. Return the qc struct if found, or 0 if not.
 * Equality of value is defined +/- delta
 */
latsParmQC* latsQCLookup(char* name, char* levtype, double value, double delta){
	latsParmQC qc;
	int klo, khi, kmid;
	int rel;
	
	latsCpyTrim(qc.name, name, LATS_MAX_NAME);
	latsCpyTrim(qc.levtype, levtype, LATS_MAX_NAME);
	qc.value = value;
	
	klo = 0;
	khi = latsQCTableSize - 1;
	while(klo <= khi){
		kmid = (khi + klo) >> 1;
		if((rel = latsSortQCDelta(&qc,latsQCTable+kmid,delta))<0)
			khi = kmid-1;
		else if(rel>0)
			klo = kmid+1;
		else {
			return latsQCTable+kmid;
		}
	}
	return 0;
}

/* Check all entries in the parm table, ensure
   that the surface level, if defined, appears in
   the vert type table, and link the parm struct
   to the vert type entry.
   
   This functions is called after the parm table
   latsParmTable and vertical type table latsVertTypeTable
   have been created.
   
   Return 1 on success, 0 on failure.
   */
int latsParmJoinVert(char *path){
	latsParm *parm;
	latsVertType *vert;
	
	for(parm = latsParmTable; parm<(latsParmTable+latsParmTableSize); parm++){
		if(parm->levelset){
			if((vert = latsVertTypeLookup(parm->levelname))==(latsVertType *)0){
				latsWarning("Level type %s: not found for parameter %s, file %s",
					    parm->levelname, parm->name, path);
				parm->levelset=0;
				parm->verttype=(latsVertType *)0;
			}
			else if(vert->verticality == LATS_MULTI_LEVEL){
				latsWarning("Parameter %s: if specified, level type must be a surface (currently %s), file %s",
					    parm->name, parm->levelname, path);
				parm->levelset=0;
				parm->verttype=(latsVertType *)0;
			}
			else {
				parm->verttype = vert;
			}
		}
	}
	return 1;
}

/* Create the parm table from the file with given 'path'.
 * Return 1 if successful, 0 if an error occurred.
 * 
 * Note: this function sets the parm table latsParmTable
 * with the entries in 'path'; any previous
 * entries are deleted. If an error occurs, it should be 
 * assumed that the parameter table information may have
 * been destroyed.
 */

int latsParmCreateTable(char* path) {
	char parmEntry[LATS_MAX_PARM_LINE];
	char tempEntry[LATS_MAX_PARM_LINE];
	char tempname[LATS_MAX_NAME];
	int iline, nvar, nvert, ncenter, nqc;
	int currentTable;
	char *cp;
	int lats_save_fatal;
	
	enum table_type {LATS_VAR_TABLE, LATS_VERT_TABLE,
			 LATS_CENTER_TABLE, LATS_QC_TABLE};
	
	lats_save_fatal = lats_fatal;
	nvar = nvert = ncenter = nqc = 0;
	currentTable = LATS_VAR_TABLE;
	
	/*mf 970818 - close if already open and issue warning mf*/
	
	if(parmFile != NULL) {
		fclose(parmFile);
		latsWarning("Closing the previously opened Parameter Table: %s",path);
	}
	
	/* Open the file */
	if((parmFile = fopen(path, "r"))==NULL){
		latsError("LATS (latsint.c) --> Cannot open parameter file %s",path);
		perror("");
		return 0;
	}
	/* Scan the file */
	for(iline=1; fgets(parmEntry, LATS_MAX_PARM_LINE-1, parmFile); iline++){
		/* Check for comments and section delimiters */
		if(parmEntry[0]=='#'){
			if(parmEntry[1]=='!'){	     /* Section delimiter? */
				latsCpyTrim(tempname, parmEntry+2, LATS_MAX_NAME);
				for(cp = tempname; *cp; cp++)
					*cp = tolower(*cp);
				if(!strcmp(tempname,"variable"))
					currentTable = LATS_VAR_TABLE;
				else if(!strcmp(tempname,"vert"))
					currentTable = LATS_VERT_TABLE;
				else if(!strcmp(tempname,"center"))
					currentTable = LATS_CENTER_TABLE;
				else if(!strcmp(tempname,"qc"))
					currentTable = LATS_QC_TABLE;
				else {
					latsError("LATS (latsint.c) --> Warning: section delimiter: %s, ignored in parameter file %s, line %d",
						  tempname, path, iline);
				}
			}
			continue;			     /* Skip comments, section delimiters */
		}
		/* Check for blank lines */
		else {
			latsCpyTrim(tempEntry, parmEntry, LATS_MAX_PARM_LINE);
			if(tempEntry[0] == '\0') continue;
		}
		
		/* Call the appropriate table routine */
		
		lats_fatal = 0;			     /* Turn off error flag temporarily so that line number can be reported */
		switch(currentTable){
		case LATS_VAR_TABLE:
			if(latsParmFillVarTable(parmEntry, nvar) == 0) goto error;
			nvar++;
			break;
		case LATS_VERT_TABLE:
			if(latsParmFillVertTable(parmEntry, nvert) == 0) goto error;
			nvert++;
			break;
		case LATS_CENTER_TABLE:
			if(latsParmFillCenterTable(parmEntry, ncenter) == 0) goto error;
			ncenter++;
			break;
		case LATS_QC_TABLE:
			if(latsParmFillQCTable(parmEntry, nqc) == 0) goto error;
			nqc++;
			break;
		}
		lats_fatal = lats_save_fatal;
	}
	/* Error if too many variables */
	if(nvar >= LATS_MAX_PARMS) {
		latsError("LATS (latsint.c) --> Maximum number of parameters = %d, remainder ignored, file %s", LATS_MAX_PARMS, path);
		nvar--;
	}
	latsParmTableSize = nvar;
	/* Sort the parameter table */
	qsort(latsParmTable, latsParmTableSize, sizeof(latsParm), (int (*)(const void *, const void *))latsSortParms);
	
	if(nvert >= LATS_MAX_VERT_TYPES){
		latsError("LATS (latsint.c) --> Maximum number of vertical dimension types = %d, remainder ignored, file %s", LATS_MAX_VERT_TYPES, path);
		nvert--;
	}
	latsVertTableSize = nvert;
	
	/* Join parameter table to vertical types table
	   (Note: depends on latsVertTable being set) */
	if(latsParmJoinVert(path)==0)
		return 0;
	
	if(ncenter >= LATS_MAX_CENTERS){
		latsError("LATS (latsint.c) --> Maximum number of centers = %d, remainder ignored, file %s", LATS_MAX_CENTERS, path);
		ncenter--;
	}
	latsCenterTableSize = ncenter;
	
	latsQCTableSize = nqc;
	/* Sort the QC table */
	qsort(latsQCTable, latsQCTableSize, sizeof(latsParmQC), (int (*)(const void *, const void *))latsSortQC);
	
	fclose(parmFile);
	return 1;
	
error:
	lats_fatal = lats_save_fatal;
	latsError("LATS (latsint.c) --> line %d, parameter file %s", iline, path);
	return 0;
}

/* Fill the parameter table. 'nvar' is the 0-origin index of the parameter, 
 * Return 1 on success, 0 on failure.
 */
int latsParmFillVarTable(char *parmEntry, int nvar){
	char tempname[LATS_MAX_NAME];
	char *name, *id, *title, *units, *scalefac, *precision, *levelname, *datatype, *comment_1, *comment_2;
	char *final;
	latsParm* parm;
	char* cp;
	
	/* Ignore item if table is full */
	if(nvar >= LATS_MAX_PARMS)
		return 0;
	
	parm = latsParmTable + nvar;
	if(((name = strtok(parmEntry,"|"))==NULL) ||
	   ((id = strtok(NULL,"|"))==NULL) ||
	   ((title = strtok(NULL,"|"))==NULL) ||
	   ((units = strtok(NULL,"|"))==NULL) ||
	   ((datatype = strtok(NULL,"|"))==NULL) ||
	   ((levelname = strtok(NULL,"|"))==NULL) ||
	   ((scalefac = strtok(NULL,"|"))==NULL) ||
	   ((precision = strtok(NULL,"|"))==NULL) ||
	   ((comment_1 = strtok(NULL,"|"))==NULL) ||
	   ((comment_2 = strtok(NULL,"|"))==NULL)
	   ) {
		latsError("LATS (latsint.c) --> Invalid parameter file entry: %s", parmEntry);
		return 0;
	}
	
	latsCpyTrim(parm->name, name, LATS_MAX_NAME);
	latsCpyTrim(parm->title, title, LATS_MAX_NAME);
	latsCpyTrim(parm->units, units, LATS_MAX_NAME);
	
	latsCpyTrim(tempname, datatype, LATS_MAX_NAME);
	for(cp=tempname; *cp; cp++)
		*cp = tolower(*cp);
	if(!strcmp(tempname,"float"))
		parm->datatype = LATS_FLOAT;
	else if(!strcmp(tempname,"int"))
		parm->datatype = LATS_INT;
	else {
		latsError("LATS (latsint.c) --> Datatype: %s, must be 'float' or 'int'", datatype);
		return 0;
	}
	
	latsCpyTrim(parm->levelname, levelname, LATS_MAX_NAME);
	parm->levelset = (strcmp(parm->levelname,"")!=0);
	parm->verttype = (latsVertType *)0;
	
	latsCpyTrim(tempname, precision, LATS_MAX_NAME);
	parm->precision = strtol(tempname, &final, 10);
	if(*final) {
		latsError("LATS (latsint.c) --> Invalid precision: %s", tempname);
		return 0;
	}
	
	latsCpyTrim(tempname, id, LATS_MAX_NAME);
	parm->id = strtol(tempname, &final, 10);
	if(*final) {
		latsError("LATS (latsint.c) --> Invalid GRIB parameter #: %s", tempname);
		return 0;
	}
	
	latsCpyTrim(tempname, scalefac, LATS_MAX_NAME);
	parm->scalefac = strtol(tempname, &final, 10);
	if(*final) {
		latsError("LATS (latsint.c) --> Invalid GRIB scale factor: %s", tempname);
		return 0;
	}
	
	return 1;
}

/* Fill the vertical type table. 'nitem' is the 0-origin index of the item, 
 * Return 1 on success, 0 on failure.
 */
int latsParmFillVertTable(char *parmEntry, int nitem){
	char tempname[LATS_MAX_NAME];
	latsVertType *vert;
	char *name, *descrip, *units, *positive, *verticality, *gribid, *grib_p1, *grib_p2, *grib_p3;
	
	if(nitem >= LATS_MAX_VERT_TYPES)
		return 0;
	
	vert = latsVertTypeTable + nitem;
	if(((name = strtok(parmEntry,"|"))==NULL) ||
	   ((descrip = strtok(NULL,"|"))==NULL) ||
	   ((units = strtok(NULL,"|"))==NULL) ||
	   ((verticality = strtok(NULL,"|"))==NULL) ||
	   ((positive = strtok(NULL,"|"))==NULL) ||
	   ((gribid = strtok(NULL,"|"))==NULL) ||
	   ((grib_p1 = strtok(NULL,"|"))==NULL) ||
	   ((grib_p2 = strtok(NULL,"|"))==NULL) ||
	   ((grib_p3 = strtok(NULL,"|"))==NULL)
	   ) {
		latsError("LATS (latsint.c) --> Invalid parameter file entry: %s", parmEntry);
		return 0;
	}
	
	latsCpyTrim(vert->name, name, LATS_MAX_NAME);
	latsCpyTrim(vert->descrip, descrip, LATS_MAX_NAME);
	latsCpyTrim(vert->units, units, LATS_MAX_NAME);
	
	latsCpyLower(tempname, verticality, LATS_MAX_NAME);
	if(!strcmp(tempname,"single"))
		vert->verticality = LATS_SINGLE_LEVEL;
	else if(!strcmp(tempname,"multi"))
		vert->verticality = LATS_MULTI_LEVEL;
	else {
		latsError("LATS (latsint.c) --> Level type: %s, must be 'single' or 'multi'", verticality);
		return 0;
	}
	
	latsCpyLower(tempname, positive, LATS_MAX_NAME);
	if(!strcmp(tempname, "up"))
		vert->positive = LATS_UP;
	else if(!strcmp(tempname, "down"))
		vert->positive = LATS_DOWN;
	else {
		latsError("LATS (latsint.c) --> Positive: %s, must be 'up' or 'down'", positive);
		return 0;
	}
	
	if(latsStrtoi(gribid, &vert->gribid, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib id: %s", gribid);
		return 0;
	}
	if(latsStrtoi(grib_p1, &vert->grib_p1, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib P1: %s", grib_p1);
		return 0;
	}
	if(latsStrtoi(grib_p2, &vert->grib_p2, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib P2: %s", grib_p2);
		return 0;
	}
	if(latsStrtoi(grib_p3, &vert->grib_p3, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib P3: %s", grib_p3);
		return 0;
	}
	return 1;
}

/* Fill the originating center table. 'nitem' is the 0-origin index of the item, 
 * Return 1 on success, 0 on failure.
 */
int latsParmFillCenterTable(char *parmEntry, int nitem){
	char tempname[LATS_MAX_NAME];
	latsCenter *center;
	char *name, *gribid, *grib_center, *grib_subcenter;
	
	if(nitem >= LATS_MAX_CENTERS)
		return 0;
	
	center = latsCenterTable + nitem;
	if(((name = strtok(parmEntry,"|"))==NULL) ||
	   ((gribid = strtok(NULL,"|"))==NULL) ||
	   ((grib_center = strtok(NULL,"|"))==NULL) ||
	   ((grib_subcenter = strtok(NULL,"|"))==NULL)
	   ) {
		latsError("LATS (latsint.c) --> Invalid parameter file entry: %s", parmEntry);
		return 0;
	}
	
	latsCpyTrim(center->center, name, LATS_MAX_NAME);
	
	if(latsStrtoi(gribid, &center->gribid, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib process id: %s", gribid);
		return 0;
	}
	if(latsStrtoi(grib_center, &center->grib_center, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib center: %s", grib_center);
		return 0;
	}
	if(latsStrtoi(grib_subcenter, &center->grib_subcenter, -1)==0){
		latsError("LATS (latsint.c) --> Invalid grib subcenter: %s", grib_subcenter);
		return 0;
	}
	return 1;
}

/* Fill the quality control table. 'nitem' is the 0-origin index of the item, 
 * Return 1 on success, 0 on failure.
 */
int latsParmFillQCTable(char *parmEntry, int nitem){
	char tempname[LATS_MAX_NAME];
	latsParmQC *qc, *newTable;
	char *name, *levtype, *value, *obsmean, *obsstd, *nstd, *obsrangetol, *obsrange;
	size_t newSize;
	long newBytes;
	
	/* Increase the table size if necessary */
	if(nitem >= latsQCTableAllocSize){
		newSize = latsQCTableAllocSize+LATS_QC_TABLE_INCR;
		newBytes = newSize*sizeof(latsParmQC);
		if((newTable = (latsParmQC *)calloc(newSize, sizeof(latsParmQC)))==0){
			latsError("LATS (latsint.c) --> Allocating new QC table, size %d", newBytes);
			return 0;
		}
		if(latsQCTableAllocSize > 0) {
			memcpy((void *)newTable, (void *)latsQCTable, (latsQCTableAllocSize*sizeof(latsParmQC)));
			free(latsQCTable);
		}
		latsQCTable = newTable;
		latsQCTableAllocSize = newSize;
	}
	
	qc = latsQCTable + nitem;
	if(((name = strtok(parmEntry,"|"))==NULL) ||
	   ((levtype = strtok(NULL,"|"))==NULL) ||
	   ((value = strtok(NULL,"|"))==NULL) ||
	   ((obsmean = strtok(NULL,"|"))==NULL) ||
	   ((obsstd = strtok(NULL,"|"))==NULL) ||
	   ((nstd = strtok(NULL,"|"))==NULL) ||
	   ((obsrange = strtok(NULL,"|"))==NULL) ||
	   ((obsrangetol = strtok(NULL,"|"))==NULL)
	   ) {
		latsError("LATS (latsint.c) --> Invalid parameter file entry: %s", parmEntry);
		return 0;
	}
	
	latsCpyTrim(qc->name, name, LATS_MAX_NAME);
	latsCpyTrim(qc->levtype, levtype, LATS_MAX_NAME);
	if(latsStrtod(value, &qc->value, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid QC value: %s", value);
		return 0;
	}
	if(latsStrtof(obsmean,&qc->obsmean, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid mean value: %s", obsmean);
		return 0;
	}
	if(latsStrtof(obsstd,&qc->obsstd, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid standard deviation: %s", obsstd);
		return 0;
	}
	if(latsStrtof(nstd,&qc->nstd, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid tolerance: %s", nstd);
		return 0;
	}
	if(latsStrtof(obsrangetol,&qc->obsrangetol, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid range tolerance value: %s", obsrangetol);
		return 0;
	}
	if(latsStrtof(obsrange,&qc->obsrange, LATS_DEFAULT_QC_VALUE)==0){
		latsError("LATS (latsint.c) --> Invalid range value: %s", obsrange);
		return 0;
	}
	
	return 1;
}

/* Lookup a vertical dimension type. Return a pointer to the entry on success,
 * 0 on failure. Lookup is case-insensitive.
 */
latsVertType* latsVertTypeLookup(char *name){
	char tempname[LATS_MAX_NAME];
	latsVertType *vert;
	
	latsCpyLower(tempname, name, LATS_MAX_NAME);
	for(vert = latsVertTypeTable; vert < latsVertTypeTable+latsVertTableSize; vert++){
		if(!strcmp(tempname, vert->name))
			return vert;
	}
	return 0;
}

/* List available vertical dimension types
 */
void latsVertTypeList(void){
	static int reported = 0;
	latsVertType *vert;
	
	if(!reported){
		fprintf(stderr, "The following vertical dimension types are defined:\n");
		for(vert = latsVertTypeTable; vert < latsVertTypeTable+latsVertTableSize; vert++){
			fprintf(stderr,"%s\t%s\n",vert->name, vert->descrip);
		}
		reported = 1;
	}
}


/* Lookup an originating center. Return a pointer to the entry on success,
 * 0 on failure. Lookup is case-insensitive.
 */
latsCenter* latsCenterLookup(char *name){
	latsCenter *center;
	char tempname[LATS_MAX_NAME];
	
	latsCpyLower(tempname, name, LATS_MAX_NAME);
	for(center = latsCenterTable; center < latsCenterTable+latsCenterTableSize; center++){
		if(!strcmp(tempname, center->center))
			return center;
	}
	return 0;
}

/* Create the default parameter table. */
/* Return 1 on success, 0 on failure. */
int latsParmCreateDefaultTable(void){
	int i;
	
	if(LATS_DEFAULT_NPARMS > LATS_MAX_PARMS){
		latsError("LATS (latsint.c) --> Too many parameters in internal variable table (latsparm.h): %d, maximum of %d",
			  LATS_DEFAULT_NPARMS, LATS_MAX_PARMS);
		return 0;
	}
	/* Copy from default table */
	for(i=0; i<LATS_DEFAULT_NPARMS; i++){
		latsParmTable[i] = latsDefaultParms[i];
	}
	latsParmTableSize = LATS_DEFAULT_NPARMS;
	/* Sort the parameter table */
	qsort(latsParmTable, latsParmTableSize, sizeof(latsParm),
	      (int (*)(const void *, const void *))latsSortParms);
	
	/*-------------------------------------------------------------------------------*/
	/* Copy vertical dimension types */
	if(LATS_DEFAULT_NVERTS > LATS_MAX_VERT_TYPES){
		latsError("LATS (latsint.c) --> Too many parameters in internal vertical dimension table (latsparm.h): %d, maximum of %d",
			  LATS_DEFAULT_NVERTS, LATS_MAX_VERT_TYPES);
		return 0;
	}
	for(i=0; i<LATS_DEFAULT_NVERTS; i++){
		latsVertTypeTable[i] = latsDefaultVerts[i];
	}
	latsVertTableSize = LATS_DEFAULT_NVERTS;
	
	/* Join parameter, verttype table for surfaces
	   (Note: depends on latsVertTypeTable being set) */
	if(latsParmJoinVert("<internal table>")==0)
		return 0;
	
	/*-------------------------------------------------------------------------------*/
	/* Copy centers */
	if(LATS_DEFAULT_NCENTERS > LATS_MAX_CENTERS){
		latsError("LATS (latsint.c) --> Too many parameters in internal centers table (latsparm.h): %d, maximum of %d",
			  LATS_DEFAULT_NCENTERS, LATS_MAX_CENTERS);
		return 0;
	}
	for(i=0; i<LATS_DEFAULT_NCENTERS; i++){
		latsCenterTable[i] = latsDefaultCenters[i];
	}
	latsCenterTableSize = LATS_DEFAULT_NCENTERS;
	/*-------------------------------------------------------------------------------*/
	/* Copy QC marks */
	latsQCTable = latsDefaultQCs;
	latsQCTableSize = LATS_DEFAULT_NQCS;
	/* Sort the QC table */
	qsort(latsQCTable, latsQCTableSize, sizeof(latsParmQC), (int (*)(const void *, const void *))latsSortQC);
	return 1;
}

/* Lookup vertical dimension in global list. Return pointer to dimension, */
/* or 0 if error or not found. */
latsVertDim *latsVertLookup(int id){
	latsVertDim *vert;
	
	for(vert = latsVertHead; vert; vert = vert->next){
		if(id == vert->id)
			return vert;
	}
	return 0;
}

/* Create and add a vertical dimension to the global list. */
/* 'name' does not have to be unique. Returns pointer to dimension, */
/* or 0 on error. */
latsVertDim *latsVertAddEntry(char *name){
	latsVertDim *vert;
	
	if((vert = (latsVertDim *)malloc(sizeof(latsVertDim)))==0){
		latsError("LATS (latsint.c) --> Adding entry for level %s", name);
		return 0;
	}
	
	vert->next = latsVertHead;
	latsVertHead = vert;
	
	vert->id = latsNextVertID++;
	vert->delta = 0.0;
	vert->levs = 0;
	vert->ncid = -1;
	vert->nlev = 0;
	vert->type = 0;
	vert->vncid = -1;
	
	return vert;
}

/* Create and add a grid entry to the global grid list.
 * 'name' must be unique. Return the entry on success,
 * 0 on failure.
 * 
 * Note: Grid IDs start at 1
 */
latsGrid *latsGridAddEntry(char *name){
	int i;
	latsGrid *grid;
	
	for(i=0; i<latsNgrids; i++){
		if(!strcmp(name, latsGridList[i].name)){
			latsError("LATS (latsint.c) --> Duplicate grid name: %s", name);
			return 0;
		}
	}
	grid = latsGridList + latsNgrids;
	grid->id = ++latsNgrids;
	grid->lonid = grid->latid = grid->vlonid = grid->vlatid = -1;
	grid->nlon = grid->nlat = 0;
	grid->lons = grid->lats = 0;
	grid->type = (latsGridType)0;
	return grid;
}

/* Check the monotonicity of array 'f', of length 'n'. */
/* Return the monotonicity on success, 0 on failure */
latsMonotonicity latsCheckMono(int n, double f[]){
	latsMonotonicity mono;
	int i;
	
	if(n == 1)
		mono = LATS_SINGLE;
	else if(n>1) {
		mono = (latsMonotonicity)((f[0]<f[1]) ? LATS_INCREASING :
					  (f[0]>f[1]) ? LATS_DECREASING : 0);
	}
	else {
		return (latsMonotonicity)0;
	}
	
	for(i=1; i<n-1; i++){
		if(((f[i]<f[i+1]) ? LATS_INCREASING :
		    (f[i]>f[i+1]) ? LATS_DECREASING : 0) != mono)
			return (latsMonotonicity)0;
	}
	return mono;
}

void latsError(char *fmt, ...){
	va_list args;
	
	va_start(args,fmt);
	if(lats_verbose){
		fprintf(stderr, "LATS error (%s): ",(_lats_routine_name_ ? _lats_routine_name_ : "unknown routine"));
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
	if(lats_fatal)
		exit(1);
	
	return;
}

void latsWarning(char *fmt, ...){
	va_list args;
	
	va_start(args,fmt);
	if(lats_verbose){
		fprintf(stderr, "LATS warning (%s): ",(_lats_routine_name_ ? _lats_routine_name_ : "unknown routine"));
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
	}
	va_end(args);
	
	return;
}
