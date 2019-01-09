
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "grads.h"
#include "galats.h"
#include "lats.h"
#include "latsint.h"
#include "ShaveMantissa.h"

extern struct galats glats ;      /* LATS option struct */

static int gridnumber = 0;

/* GrADS-LATS interface  LATS  */

int galats (struct gacmn *pcm, int opt1, int opt2) {

  struct gagrid *pgr;
  struct galats *plats; 
  struct dt ctime;
  
  latsFile *lfile;
  latsVar *lvar;

  int i,j,ii,jj,size,rc;
  double (*conv) (double *, double);
  double *lat,*lon;
  double *lat0,*lon0;
  
  float32 *fgrid;

  static char pout[256];   /* Build error msgs here */

  int id_parmtab,id_undef,id_write,id_close,id_reset,id_basetime;
  int varlev;
  int verb=0;

/* set the global LATS variable lats_fatal to 0 to NOT kill grads if a fatal LATS error occurs */

  lats_fatal=0;

  plats=&(glats); 


/*---
  000000000000000000000000000 - lats_parmtab -- set the parameter table
---*/

  if(opt1 == 0) {

    id_parmtab = lats_parmtab(plats->ptname) ;
    return(id_parmtab);

  }

/*---
  111111111111111111111111111 -  lats_create -- create the output file
---*/

  if(opt1 == 1) {


/*--- 

  970317 - eased restriction on multiply files open
  create the output file

 ---*/

    plats->id_file = lats_create(plats->oname,
				 plats->convention,
				 plats->calendar,   /*--- may support other calendars if 365 day works ---*/
				 plats->frequency, 
				 plats->deltat,
				 plats->center,
				 plats->model,
				 plats->comment);

    return(plats->id_file);

  }

/*---
  222222222222222222222222222 -  lats_vert_dim -- set the vertical dimension
---*/


  if(opt1 == 2) {

    if(plats->nlev > 0) {

/*--- only define once ---*/

      // if(!plats->id_lev) {

	plats->id_lev=lats_vert_dim("levels",
				    plats->vertdimname,
				    plats->nlev,
				    plats->levels);

	return(plats->id_lev);

        //} else {

	//return(-999);

        // } 

    }

    return(0);

  }


/*---
  333333333333333333333333333 -  lats_grid -- set up the LATS horizontal grid 
---*/


  if(opt1 == 3) {

    pgr = pcm->result[0].pgr;

/*--- 
   GrADS grid info
---*/

    if(verb) {
      printf("ddd idim = %d jdim = %d\n",pgr->idim,pgr->jdim);
      printf("ddd ilinr = %d jlinr = %d\n",pgr->ilinr,pgr->jlinr);
      for(i=0;i<4;i++) {
	printf("dddd %d %d\n",pgr->dimmin[i],pgr->dimmax[i]);
      }
      for(i=0;i<3;i++) {
	printf("dddd dmin = %f dmax = %f\n",pcm->dmin[i],pcm->dmax[i]);
      }
    }

/*--- 
  abort if not a lon,lat grid
---*/

    if( !(pgr->idim == 0 && pgr->jdim == 1) ) {
      sprintf (pout,"GrADS-LATS Error:  the grid must be lon/lat\n");
      gaprnt (0,pout);
      return(0);
    } 

    lon=(double *)malloc(sizeof(double)*pgr->isiz);
    lat=(double *)malloc(sizeof(double)*pgr->jsiz);

    lon0=lon;
    lat0=lat;

    if(lon == NULL && lat == NULL) {
      sprintf (pout,"Memory Allocation Error in galats.c \n");
      gaprnt (0,pout);
      exit(0);
    }

/* LATS grid type */

    if(pgr->ilinr && pgr->jlinr) {
      plats->gridtype=LATS_LINEAR;
    } else if ( pgr->ilinr==1 && pgr->jlinr==0 && plats->gridtype == LATS_GAUSSIAN ) {
      plats->gridtype=LATS_GAUSSIAN;
    } else {
      plats->gridtype=LATS_GENERIC;
    }

/* get the lon and lats from the GrADS dimension environment */

    if (pgr->idim<3) {
      conv = pgr->igrab;
      for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
	*lon =(double)conv(pgr->ivals,(double)i);
	if(verb) printf("iiii %d %g\n",i,*lon); 
	lon++;
      }
    }

    if (pgr->jdim<3) {
      conv = pgr->jgrab;

      if(pcm->yflip == 1) {
	  for (i=pgr->dimmax[pgr->jdim];i>=pgr->dimmin[pgr->jdim];i--) {
	      *lat =(double)conv(pgr->jvals,(double)i);
	      if(verb)  printf("jjjj REVERSEE %d %f\n",i,*lat); 
	      lat++;
	  }
      } else {
	  for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
	      *lat =(double)conv(pgr->jvals,(double)i);
	      if(verb)  printf("jjjj %d %f\n",i,*lat); 
	      lat++;
	  }
      }

    }
    
    plats->ilinear=pgr->ilinr;
    plats->jlinear=pgr->jlinr;
    plats->nlon=pgr->isiz;
    plats->nlat=pgr->jsiz;
    plats->lon_1=(float)*lon0;
    plats->lat_1=(float)*lat0;
    plats->lon_nlon=(float)*(lon-1);
    plats->lat_nlat=(float)*(lat-1);

    gridnumber++;
    //ams sprintf(plats->gridname,"grid%02d",plats->id_file);
    sprintf(plats->gridname,"grid%02d",gridnumber); /* will be unique */
	
    plats->id_grid=lats_grid(plats->gridname,
			     plats->gridtype,
			     pgr->isiz,lon0,
			     pgr->jsiz,lat0);

    sprintf (pout,"LATS GRID ID = %d\n",plats->id_grid);
    gaprnt (2,pout);

    return(0);


  }

/*---
  444444444444444444444444444 -  lats_var -- set up a variable
---*/

  if(opt1 == 4) {


    if(plats->varlev > -1e19) {
      varlev=plats->id_lev;
    } else {
      varlev=0;
    }

    if(plats->id_user_grid <= 0) {
      sprintf (pout,"GrADS-LATS Error:  (lats_var) the LATS GRID ID is out of bounds\n");
      gaprnt (0,pout);
      return(4);
    }

    if(plats->id_user_file <= 0) {
      sprintf (pout,"GrADS-LATS Error:  (lats_var) the LATS FILE ID is not set\n");
      gaprnt (0,pout);
      sprintf (pout,"GrADS-LATS Error:  (lats_var) be sure to (set lats create ...)\n");
      gaprnt (0,pout);
      return(4);
    }


    plats->id_var=lats_var(plats->id_user_file,
			   plats->var,
			   LATS_FLOAT,
			   plats->timestat,
			   plats->id_user_grid,
			   plats->id_user_lev,
			   plats->var_comment);

    return(plats->id_var);

  }

/*---
  555555555555555555555555555 -  lats_write -- write out a grid
---*/

  if(opt1 == 5) {

/*--- 
  check the dimension environment to make sure we have a valid grid, i.e.,
  z and t must be fixed
---*/

    if( (pcm->dmin[2] !=pcm->dmax[2]) && (pcm->dmin[3]!=pcm->dmax[3]) ) {

      sprintf (pout,"GrADS-LATS Error:  (lats_write) the dimension environment must be fixed in z and t\n");
      gaprnt (0,pout);
      return(1);

    } else {

      pgr = pcm->result[0].pgr;

/*--- 

  Set the undefined value at the first call to lats_write as in LATS
  use internal functions because it's just too hard to maintain in GrADS

---*/

      if( (lfile = latsFileLookup(plats->id_user_file)) == 0 ) {
	sprintf (pout,"GrADS-LATS Error: (lats_write) the LATS FILE lookup failed\n");
	gaprnt (0,pout);
	return 0;
      }

      if(lfile->latsmode == LATS_MODE_DEFINE ) { 
	for(lvar = lfile->varlist; lvar; lvar = lvar->next){
	  id_undef=lats_miss_float(plats->id_user_file,lvar->id,
				   (float) pgr->undef,0.01*fabs(pgr->undef));
	}
      }


/*---
  calculate the time
---*/
    
/*--- grid relative ---*/

      if(plats->time_opt == 0) {
	
	gr2t(pcm->pfid->grvals[3],pgr->dimmax[3],&ctime);

/*--- relative to the dimension environment ---*/

      } else if(plats->time_opt >= 1 ) {

	ctime.yr=pcm->tmin.yr;
	ctime.mo=pcm->tmin.mo;
	ctime.dy=pcm->tmin.dy;
	ctime.hr=pcm->tmin.hr;
	ctime.mn=pcm->tmin.mn;

/*--- forecast hours use fhour setting ---*/

      } else if(plats->time_opt == 3 ) {

	ctime.yr=0;
	ctime.mo=0;
	ctime.dy=0;
	ctime.hr=glats.fhour;
	ctime.mn=glats.fmin;

      }

/*--- write it out ---*/

      if(!plats->id_user_var<0) {
	sprintf (pout,"GrADS-LATS Error:  (lats_write) the LATS VAR ID is out of bounds\n");
	gaprnt (0,pout);
	sprintf (pout,"GrADS-LATS Error:  (lats_write) the 'set lats write id_user_var did not work\n");
	gaprnt (0,pout);
	return(1);
      }

      if(plats->id_file<1) {
	sprintf (pout,"GrADS-LATS Error: (lats_write) the LATS FILE ID is 0\n");
	gaprnt (0,pout);
	sprintf (pout,"GrADS-LATS Error: (lats_write) the set lats create did not  work\n");
	gaprnt (0,pout);
	return(1);
      }
      if(plats->id_user_write) {

        /* First convert grid to float */
        size = pgr->isiz*pgr->jsiz;
        fgrid = (float32 *) malloc (sizeof(float) * pgr->isiz * pgr->jsiz);

        if ( fgrid ) {

	    if(pcm->yflip == 1) {
		jj=0;
		for (j=pgr->jsiz-1; j >= 0; j--) {
		    for ( i=0; i<pgr->isiz; i++ )  {
			ii=j*pgr->isiz + i;
			fgrid[jj] = (float) pgr->grid[ii];
			jj++;
		    }
		}
	    } else {
		for ( i=0; i<size; i++ ) fgrid[i] = (float) pgr->grid[i];
	    }

        } else {
          gaprnt (0,"GrADS-LATS Error: cannot allocate memory...\n");
          return(1);
        }

        /* Shave it if necessary */
        if ( glats.shave > 0 ) {
          rc=ShaveMantissa32 (fgrid,fgrid,(int32) size,glats.shave,
                              1,(float32) pgr->undef,(int32) size);
          if (rc) return(rc);
        }

	id_write=lats_write(plats->id_user_file,
			    plats->id_user_var,
			    plats->varlev,
			    ctime.yr,
			    ctime.mo,
			    ctime.dy,
			    ctime.hr,
			    ctime.mn,
			    glats.fhour,
			    glats.fmin,
			    fgrid);
        free(fgrid);

      } else {
	sprintf (pout,"GrADS-LATS Error: (lats_write) the set lats write did not work\n");
	gaprnt (0,pout);
      }

      if(id_write) {
	sprintf (pout,"LATS DATA_WRITE ID = %d\n",id_write);
	gaprnt (2,pout);
      } else {
	sprintf (pout,"LATS DATA_WRITE ID = 0\n");
	gaprnt (2,pout);
	sprintf (pout,"GrADS-LATS Error: (lats_write) failed...\n");
	gaprnt (0,pout);
      } 

      return(0);

    }

  }



/*---
  666666666666666666666666666 -  lats_close -- close the down the file
---*/

  if(opt1 == 6) {

    if(plats->id_file != 0) {
      id_close=lats_close(opt2);
    }
    

/*---  free up the space ---*/
    /*
    if(*lon0 != 1e20) free(lon);
    if(*lat0 != 1e20) free(lat);
    */

    return(0);

  }

/*---
  7777777777777777777777777 - reset the parameters????
---*/

  if(opt1 == 7) {


    /* --- reinitialize the id's --- */

    plats->frequency=LATS_HOURLY;
    plats->gridtype=LATS_LINEAR;
    plats->timestat=LATS_INSTANT;
    plats->deltat=0;
    plats->varlev=-1e20;
    strcpy(plats->oname,"grads.lats");
    strcpy(plats->var,"misc1");
    strcpy(plats->center,"PCMDI");
    strcpy(plats->model,"Unknown LATS model");
    strcpy(plats->comment,"Written using the GrADS-LATS interface");

    plats->id_file=-1;
    plats->id_lev=0;
    plats->id_grid=0;

    plats->nlev=-1;

/*---  free up the space ---*/
    /*
    if(*lon0 != 1e20) free(lon);
    if(*lat0 != 1e20)free(lat);
    */

    return(0);

  }


/*---
  10 10 10 10 10 10 10 10 10 10 10 -  lats_basetime -- close the down the file
---*/

  if(opt1 == 10) {

    if(plats->id_user_file != 0) {
      id_basetime=lats_basetime(plats->id_user_file,plats->lyr,plats->lmo,plats->lda,plats->lhr,plats->lmn);
    }
    if(!id_basetime) {
      sprintf (pout,"LATS BASETIME ID = 0\n");
      gaprnt (2,pout);
      sprintf (pout,"GrADS-LATS Error: (lats_basetime) failed...\n");
      gaprnt (0,pout);
    } 
    return(id_basetime);
  }



}

/* #endif */
