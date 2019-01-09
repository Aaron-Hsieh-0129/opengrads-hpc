/* -*-Mode: C;-*-
 * Module:      Generic LATS HDF/netCDF functions. This file not to be compiled directly but
 *              rather included from latsnc.c or latssd.c.
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latssdf.h,v 1.4 2010/02/16 23:01:31 mike_fiorino Exp $
 *
 * Revision History:
 *
 * $Log: latssdf.h,v $
 * Revision 1.4  2010/02/16 23:01:31  mike_fiorino
 * mods for supporting yflip and better handling of forecast_hourly forecast_minutes
 *
 * Revision 1.3  2009/11/12 22:38:32  dasilva
 * ams: minor typo
 *
 * Revision 1.2  2009/10/29 19:37:44  dasilva
 * ams: fixed min bug
 *
 * Revision 1.1  2009/10/15 03:56:41  dasilva
 * ams: generic netcd/hdf interface code
 *
 *
 */

#include "galats.h"         
extern struct galats glats ; 

/* netCDF write mode */
#define LATS_NC_DEFINE 1
#define LATS_NC_DATA 2

typedef struct latsncfile {
	int timeid;				     /* netCDF time dimension ID */
	int ncid;				     /* netCDF file ID */
	int ncmode;				     /* netCDF mode (LATS_NC_DEFINE or LATS_NC_DATA) */
	int ndims;				     /* number of dimensions defined */
	int vtimeid;				     /* netCDF time variable ID */
} latsNcFile;

/* Close a netCDF file. Returns 1 on success, 0 on failure.
 */
int lats_close_nc(latsFile *file){
#ifdef GOT_NETCDF
	latsNcFile *ncfile;
	
	ncfile = (latsNcFile *)file->depend;
	if(ncclose(ncfile->ncid) != 0){
		latsError("netCDF (latsnc.c) --> Closing netCDF file %s", file->path);
		return 0;
	}
	free(ncfile);
	return 1;
#else
	return lats_stub_nc();
#endif
}

/* Create a netCDF file. Returns the file ID, or 0 on error
 */
int lats_create_nc(latsFile *file){
#ifdef GOT_NETCDF
	latsNcFile *ncfile;
	int ncid, rc;
	char *calendar_name;
	
	ncopts = NC_VERBOSE;		     /* Return on fatal errors */
	if((ncfile = (latsNcFile *)malloc(sizeof(latsNcFile)))==0){
		latsError("netCDF (latsnc.c) --> Allocating memory for netCDF file %s", file->path);
		return 0;
	}
	file->depend = (void *)ncfile;
	ncfile->ndims = 0;

#ifdef NC_NETCDF4
        if ( glats.convention == LATS_NC4 ) {
          rc = nc_create(file->path,NC_CLOBBER|NC_NETCDF4,&ncid);
          ncfile->ncid = ncid;
          if(rc) {
		latsError("netCDF (latsnc.c) --> Creating netCDF-4/HDF-5 file %s",file->path);
                ncfile->ncid = -1;
		return 0;
          }
	} else {
          rc = nc_create(file->path,NC_CLOBBER,&ncid);
          ncfile->ncid = ncid;
          if(rc) {
		latsError("netCDF (latsnc.c) --> Creating classic netCDF file %s",file->path);
                ncfile->ncid = -1;
		return 0;
          }
	}
#else	
	if((ncid = ncfile->ncid = nccreate(file->path,NC_CLOBBER)) == -1) {
		latsError("netCDF (latsnc.c) --> Creating netCDF file %s",file->path);
		return 0;
	}
#endif

	/*mf--------	if(file->frequency != LATS_FIXED) { ------*/
	if((ncfile->timeid = ncdimdef(ncid, "time", NC_UNLIMITED)) == -1) {
		latsError("netCDF (latsnc.c) --> Creating netCDF time dimension, file %s",file->path);
		return 0;
	}
	ncfile->ndims++;
	if((ncfile->vtimeid = ncvardef(ncid, "time", NC_DOUBLE, 1, &(ncfile->timeid))) == -1) {
		latsError("netCDF (latsnc.c) --> Creating netCDF time variable, file %s",file->path);
		return 0;
	}
	/*mf-------- } -----*/
	
	switch(file->calendar){
	case LATS_STANDARD:
		calendar_name = "standard";
		break;
	case LATS_JULIAN:
		calendar_name = "julian";
		break;
	case LATS_NOLEAP:
		calendar_name = "noleap";
		break;
	case LATS_360:
		calendar_name = "360";
		break;
	case LATS_CLIM:
		calendar_name = "climatology";
		break;
	case LATS_CLIMLEAP:
		calendar_name = "climatology_366";
		break;
	case LATS_CLIM360:
		calendar_name = "climatology_360";
		break;
	}
	
	if((file->convention == LATS_PCMDI ?
	    (ncattput(ncid, NC_GLOBAL, "Conventions", NC_CHAR, 5, "LATS") == -1)  :
	    (ncattput(ncid, NC_GLOBAL, "Conventions", NC_CHAR, 7, "COARDS") == -1)) ||
	   (ncattput(ncid, NC_GLOBAL, "calendar", NC_CHAR, strlen(calendar_name)+1, calendar_name) == -1) ||
	   (ncattput(ncid, NC_GLOBAL, "comments", NC_CHAR, strlen(file->comments)+1, file->comments) == -1) ||
	   (ncattput(ncid, NC_GLOBAL, "model", NC_CHAR, strlen(file->model)+1, file->model) == -1) ||
	   (ncattput(ncid, NC_GLOBAL, "center", NC_CHAR, strlen(file->center)+1, file->center) == -1)) {
		latsError("netCDF (latsnc.c) --> Writing netCDF file attribute, file %s",file->path);
		return 0;
	}
	ncfile->ncmode = LATS_NC_DEFINE;
	return file->id;
#else
	return lats_stub_nc();
#endif
}

/* Define a grid for a netCDF 'file'.
 * Return 1 on success, 0 on failure;
 * 
 * Note: this routine is called by lats_var for the
 *   first variable which is defined on this grid.
 */
int lats_grid_nc(latsFile *file, latsGrid *grid){
#ifdef GOT_NETCDF
	latsNcFile *ncfile;
	char name[LATS_MAX_NAME];
	int ncid;
	
	ncfile = (latsNcFile *)file->depend;
	ncid = ncfile->ncid;
	if(ncfile->ncmode != LATS_NC_DEFINE){
		latsError("netCDF (latsnc.c) --> lats_grid calls must precede any lats_write call");
		return 0;
	}
	/* Create longitude_grid */
	if(file->ngrid == 1)
		strcpy(name,"longitude");
	else
		sprintf(name, "longitude_%s", grid->name);
	if((grid->lonid = ncdimdef(ncid, name, grid->nlon)) == -1){
		latsError("netCDF (latsnc.c) --> Creating netCDF longitude dimension, file %s",file->path);
		return 0;
	}
	if((grid->vlonid = ncvardef(ncid, name, NC_DOUBLE, 1, &(grid->lonid))) == -1){
		latsError("netCDF (latsnc.c) --> Creating netCDF longitude variable, file %s",file->path);
		return 0;
	}
	
	/* Create latitude_grid */
	if(file->ngrid == 1)
		strcpy(name,"latitude");
	else
		sprintf(name, "latitude_%s", grid->name);
	if((grid->latid = ncdimdef(ncid, name, grid->nlat)) == -1){
		latsError("netCDF (latsnc.c) --> Creating netCDF latitude dimension, file %s",file->path);
		return 0;
	}
	if((grid->vlatid = ncvardef(ncid, name, NC_DOUBLE, 1, &(grid->latid))) == -1){
		latsError("netCDF (latsnc.c) --> Creating netCDF latitude variable, file %s",file->path);
		return 0;
	}
	ncfile->ndims += 2;
	
	if((ncattput(ncid, grid->vlonid, "units", NC_CHAR, 13, "degrees_east") == -1) ||
	   (ncattput(ncid, grid->vlatid, "units", NC_CHAR, 14, "degrees_north") == -1) ||
	   (ncattput(ncid, grid->vlonid, "long_name", NC_CHAR, 10, "Longitude") == -1) ||
	   (ncattput(ncid, grid->vlatid, "long_name", NC_CHAR, 9, "Latitude") == -1)) {
		latsError("netCDF (latsnc.c) --> Writing netCDF grid attribute, file %s",file->path);
		return 0;
	}
	
	return 1;
#else
	return lats_stub_nc();
#endif
}

/* Define a variable to be written to a netCDF file.
 * 'vertdim' is the vertical dimension structure, or 0 if no level.
 * Return the variable ID on success, 0 on failure.
 */
int lats_var_nc(latsFile *file, latsVar *var, latsGrid *grid, latsVertDim *vertdim){
	
#ifdef GOT_NETCDF
	latsNcFile *ncfile;
	nc_type datatype;
	int dimids[LATS_MAX_VAR_DIMS];
	int idim;
	char *timestat, *gridtype;
        size_t chunksize[4];	

	ncfile = (latsNcFile *)file->depend;
	if(ncfile->ncmode != LATS_NC_DEFINE){
		latsError("netCDF (latsnc.c) --> lats_var calls must precede any lats_write call");
		return 0;
	}
	datatype = (var->parm->datatype == LATS_FLOAT ? NC_FLOAT : NC_LONG);
	
	/* Set the dimension IDs */
	idim = 0;
	/*mf ---------	if(file->frequency != LATS_FIXED) ------*/ 
	dimids[idim++] = ncfile->timeid;
	if(vertdim)
		dimids[idim++] = vertdim->ncid;
	dimids[idim++] = grid->latid;
	dimids[idim++] = grid->lonid;
	
	if((var->ncid = ncvardef(ncfile->ncid, var->name, datatype, idim, dimids)) == -1){
		latsError("netCDF (latsnc.c) --> Defining variable %s, file %s",
			  var->name, file->path);
		return 0;
	}
	gridtype = (var->grid->type == LATS_GAUSSIAN ? "gaussian" :
		    var->grid->type == LATS_LINEAR ? "linear" : "generic");
	if((ncattput(ncfile->ncid, var->ncid, "comments", NC_CHAR, strlen(var->comments)+1, var->comments) == -1) ||
	   (ncattput(ncfile->ncid, var->ncid, "long_name", NC_CHAR, strlen(var->parm->title)+1, var->parm->title) == -1) ||
	   (ncattput(ncfile->ncid, var->ncid, "units", NC_CHAR, strlen(var->parm->units)+1, var->parm->units) == -1) ||
	   (ncattput(ncfile->ncid, var->ncid, "grid_name", NC_CHAR, strlen(var->grid->name)+1, var->grid->name) == -1) ||
	   (ncattput(ncfile->ncid, var->ncid, "grid_type", NC_CHAR, strlen(gridtype)+1, gridtype) == -1)
	   ) {
		latsError("netCDF (latsnc.c) --> Writing netCDF file attribute, file %s, var %s",
			  file->path, var->name);
		return 0;
	}
	
	/* If the vertical dimension is implicit, save its description */
	if(!vertdim && var->parm->levelset == 1){
		if(ncattput(ncfile->ncid, var->ncid, "level_description", NC_CHAR, strlen(var->parm->verttype->descrip)+1, var->parm->verttype->descrip)==-1){
			latsError("netCDF (latsnc.c) --> Writing netCDF file level description, file %s, var %s",
				  file->path, var->name);
			return 0;
		}
	}
	
	/*mf--------	if(file->frequency != LATS_FIXED){ --------*/
	timestat = (var->tstat == LATS_AVERAGE ? "average" :
		    var->tstat == LATS_INSTANT ? "instantaneous" :
		    var->tstat == LATS_ACCUM ? "accumulation" :
		    "other");
	if(ncattput(ncfile->ncid, var->ncid, "time_statistic", NC_CHAR, strlen(timestat)+1, timestat) == -1){
		latsError("netCDF (latsnc.c) --> Writing netCDF file attribute, file %s, var %s",
			  file->path, var->name);
		return 0;
	}
	/*mf-------- } --------*/
	

#ifdef NC_NETCDF4
        /* Chucksizes */
        if (vertdim) {
          chunksize[0] = chunksize[1] = 1; /* time and level */
          chunksize[2] = grid->nlat;
          chunksize[3] = grid->nlon;
        } else {
          chunksize[0] = 1;            /* time */
          chunksize[1] = grid->nlat;
          chunksize[2] = grid->nlon;
        }
        if ( glats.gzip >= 0 ) {
          if(nc_def_var_chunking(ncfile->ncid,var->ncid,NC_CHUNKED, chunksize)) {
            latsError("netCDF (latsnc.c) --> Defining chunking size, file %s, var %s",file->path, var->name);
            return 0;
          }
          if(nc_def_var_deflate(ncfile->ncid,var->ncid,0,1,glats.gzip)){
            latsError("netCDF (latsnc.c) --> Defining variable deflation, file %s, var %s",file->path, var->name);
            return 0;
          }
        }

#endif

	return var->id;
#else
	return lats_stub_nc();
#endif
}

/* Write a vertical dimension 'vertdim' to netCDF 'file'.
 * Return dimension ID on success, 0 on failure.
 */
int lats_vert_dim_nc(latsFile *file, latsVertDim *vertdim) {
#ifdef GOT_NETCDF
	latsNcFile *ncfile;
	char *type, *units, *descrip, *positive;
	
	ncfile = (latsNcFile *)file->depend;
	if(ncfile->ncmode != LATS_NC_DEFINE){
		latsError("netCDF (latsnc.c) --> lats_vert_dim calls must precede any lats_write call");
		return 0;
	}
	
	if(ncfile->ndims == LATS_MAX_VERT_DIMS){
		latsError("netCDF (latsnc.c) --> Too many vertical dimensions defined, maximum of %d, file %s",
			  LATS_MAX_VERT_DIMS, file->path);
		return 0;
	}
	if((vertdim->ncid = ncdimdef(ncfile->ncid, vertdim->name, vertdim->nlev)) == -1){
		latsError("netCDF (latsnc.c) --> Creating vertical dimension, file %s", file->path);
		return 0;
	}
	ncfile->ndims++;
	if((vertdim->vncid = ncvardef(ncfile->ncid, vertdim->name, NC_DOUBLE, 1, &(vertdim->ncid))) == -1){
		latsError("netCDF (latsnc.c) --> Creating vertical dimension variable, file %s", file->path);
		return 0;
	}
	units = vertdim->type->units;
	if(ncattput(ncfile->ncid, vertdim->vncid, "units", NC_CHAR, strlen(units)+1, units) == -1){
		latsError("netCDF (latsnc.c) --> Writing vertical dimension attribute, file %s, dimension %s",
			  file->path, vertdim->name);
		return 0;
	}
	descrip = vertdim->type->descrip;
	if(ncattput(ncfile->ncid, vertdim->vncid, "description", NC_CHAR, strlen(descrip)+1, descrip) == -1){
		latsError("netCDF (latsnc.c) --> Writing vertical dimension description, file %s, dimension %s",
			  file->path, vertdim->name);
		return 0;
	}
	
	type = vertdim->type->name;
	positive = (vertdim->type->positive==LATS_UP ? "up" : "down");
	if((ncattput(ncfile->ncid, vertdim->vncid, "type", NC_CHAR, strlen(type)+1, type) == -1) ||
	   (ncattput(ncfile->ncid, vertdim->vncid, "long_name", NC_CHAR, 6, "Level") == -1) ||
	   (ncattput(ncfile->ncid, vertdim->vncid, "positive", NC_CHAR, strlen(positive)+1, positive) == -1)
	   ) {
		latsError("netCDF (latsnc.c) --> Writing vertical dimension type attribute, file %s, dimension %s",
			  file->path, vertdim->name);
		return 0;
	}
	
	return file->nvertdim;
#else
	return lats_stub_nc();
#endif    
}

/* Write a horizontal lon-lat section 'data' for variable 'var' to netCDF 'file'.
 * 'levindex' is the 0-origin index, into var->levs, of the level value, or -1 if there are no levels.
 * 'timeindex' is the 0-origin index of the time value, or -1 if there are no times.
 * Return 1 on success, 0 on failure.
 */
int lats_write_nc(latsFile *file, latsVar *var, int levindex, int timeindex, latsCompTime time, int fhour, int fmin, void *data) {
#ifdef GOT_NETCDF
  latsVar *v;
  latsGrid *grid;
  latsNcFile *ncfile;
  int fileid, i, ihour, idim;
  long start[LATS_MAX_VAR_DIMS], count[LATS_MAX_VAR_DIMS];
  double timevalue;
  long itime;
  cdCompTime cdtime;
	
  static int monthdays[12]={31,29,31,30,31,30,31,31,30,31,30,31};
  static char monthnames[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	
  ncfile = (latsNcFile *)file->depend;
  fileid = ncfile->ncid;
	
  /* If this is the first write, first output */
  /* all the fixed-size dimensions */
  if(ncfile->ncmode == LATS_NC_DEFINE){
    /* Create the time units */
    /*mf----------		if(file->frequency != LATS_FIXED){ ---------*/
    if(ncattput(fileid, ncfile->vtimeid, "units", NC_CHAR, strlen(file->timeunits)+1, file->timeunits) == -1){
      latsError("netCDF (latsnc.c) --> Writing netCDF time units attribute, file %s", file->path);
      return 0;
    }
    /*mf----------	} ---------*/
    /* Write the missing data values, where applicable*/
    for(v = file->varlist; v; v = v->next){
      if(v->hasmissing){
	if(v->parm->datatype == LATS_FLOAT){
	  if(ncattput(fileid, v->ncid, "missing_value", NC_FLOAT, 1, &(v->missing.f)) == -1){
	    latsError("netCDF (latsnc.c) --> Writing missing data value, file %s, variable %s",
		      file->path, var->name);
	    return 0;
	  }
	}
	else {	     /* datatype = LATS_INT */
	  if(ncattput(fileid, v->ncid, "missing_value", NC_LONG, 1, &(v->missing.i)) == -1){
	    latsError("netCDF (latsnc.c) --> Writing missing data value, file %s, variable %s",
		      file->path, var->name);
	    return 0;
	  }
	}
      }
    }
    /* End define mode after all attribute defs */
    if(ncendef(fileid) == -1) {
      latsError("netCDF (latsnc.c) --> Ending define mode for file %s",
		file->path);
      return 0;
    }
    ncfile->ncmode = LATS_NC_DATA;
		
    /* Write the grids */
    for(grid = file->gridlist; grid < file->gridlist+file->ngrid; grid++){
      start[0] = 0;
      count[0] = grid->nlon;
      if(ncvarput(fileid, grid->vlonid, start, count, grid->lons) == -1){
	latsError("netCDF (latsnc.c) --> Writing longitude values, file %s",
		  file->path);
	return 0;
      }
      count[0] = grid->nlat;
      if(ncvarput(fileid, grid->vlatid, start, count, grid->lats) == -1){
	latsError("netCDF (latsnc.c) --> Writing latitude values, file %s",
		  file->path);
	return 0;
      }
    }
		
    /* Write all fixed-size levels */
    for(i=0; i<file->nvertdim; i++){
      count[0] = file->vertlist[i].nlev;
      if(ncvarput(fileid, file->vertlist[i].vncid, start, count, file->vertlist[i].levs) == -1){
	latsError("netCDF (latsnc.c) --> Writing vertical level %d, file %s",
		  i+1, file->path);
	return 0;
      }
    }
  }
	
  /* Translate, write time if first time for this time point  */
  if(timeindex == file->ntimewritten){
    cdtime.year = time.year;
    cdtime.month = time.month;
    cdtime.day = time.day;
    cdtime.hour = time.hour;
    cdtime.min = time.min;		
    latsComp2Rel((cdCalenType)file->calendar, cdtime, file->timeunits, &timevalue);
		
    start[0] = timeindex;
    count[0] = 1;
    if(ncvarput(fileid, ncfile->vtimeid, start, count, &timevalue) == -1){
      latsError("netCDF (latsnc.c) --> Writing time value %ld (index %d)", timevalue, timeindex);
      return 0;
    }
  }
	
  /* Write the data */
  idim = 0;
  /*mf----------	if(file->frequency != LATS_FIXED){ ------*/
  start[idim] = timeindex;
  count[idim++] = 1;
  /*mf--------	}        --------*/
  if(var->nlev > 0){
    start[idim] = levindex;
    count[idim++] = 1;
  }
  grid = var->grid;
  start[idim] = 0;
  count[idim++] = grid->nlat;
  start[idim] = 0;
  count[idim++] = grid->nlon;
	
  if(ncvarput(fileid, var->ncid, start, count, data) == -1){
    latsError("netCDF (latsnc.c) --> Writing data for file %s, var %s", file->path, var->name);
    if(var->levs && levindex>=0){
      latsError("netCDF (latsnc.c) -->    Level = %f (index %d)", var->levs[levindex], levindex);
    }
    /*mf----------		if(file->frequency != LATS_FIXED){ ------*/
    latsError("netCDF (latsnc.c) -->    Time = %dZ %d %s %d",
	      (ihour = time.hour), time.day,
	      monthnames[time.month-1], time.year);
    /*mf----------	} ------*/
    return 0;
  }
  return 1;
#else
  return lats_stub_nc();
#endif
}

int lats_stub_nc(void) {
	latsError("netCDF (latsnc.c) --> netCDF output not supported by this version of LATS");
	return 0;
}
