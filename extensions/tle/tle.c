/*

  This GrADS extension has been placed in the public domain.

                               ---
                        About POD Documentation 

The following documentation uses the "Perl On-line Documentation"
(POD) mark up syntax. It is a simple and yet adequate mark up language
for creating basic man pages, and there are converters to html,
MediaWiki, etc. In adittion, the perldoc utility can be used to
display this documentation on the screen by entering:

% perldoc orb

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < orb.c > orb.pod

and place orb.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < orb.pod > orb.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < orb.pod > orb.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make orb.html
% make orb.wiki

*/

#ifndef POD

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "grads.h"
#include "gx.h"
#include "tle.h"

/* Date/time structure with secs */
struct dt_ {
  gaint yr;
  gaint mo;
  gaint dy;
  gaint hr;
  gaint mn;
  gaint sc;
};

static int Init = 0; /* whether initialized */

static char pout[256];   /* Build error msgs here */

static char Sat_name[512]; /* to pass sat name to fortran */

static void timadd_ (struct dt *dtim, struct dt *dto);
static void timsub_ (struct dt *dtim, struct dt *dto);

static void dtcopy ( struct dt_ *t_, struct dt *t);
static void timadd__ (struct dt_ *dtim, struct dt_ *dto);

int gex_setUndef (struct gastat *pst);
static int get_latlons ( double **lons, double **lats, 
                         struct gagrid *pgr, char *name );

/* Fortran prototypes */

void ftntrack2_ (double lons[], double lats[], int *nobs, int *nmax,
		char *sat, int nymd[], int nhms[], int *dt, int *rc );

void ftnmasking2_ (double field[], int *nlon, int *nlat, 
                  double lons[], double lats[], double *undef, 
                  char *sat, int nymd[], int nhms[], int *dt, 
                  double *swath, int ihalo[], int jhalo[], int *rc );


/* Internal parameters, see c_set() */
static int dt_in_secs = 10;
static int ihalo[2] = {0, 0};
static int jhalo[2] = {0, 0};
static int mtype = -1;

/* ........................................................... */

static void marks(double lons[], double lats[], int nobs, 
                  struct gacmn *pcm, float off)
{

  int i;
  double x, y;

  /* do the plotting */
  gxcolr (pcm->lincol);
  gxstyl (pcm->linstl);
  gxwide (pcm->linthk);
  for (i=0; i<nobs; i++) {
      gxconv(lons[i]+off,lats[i],&x,&y,2);
      gxmark(6,x,y,0.1);
  }

  return;
}

/* ........................................................... */

static void lines(double lons[], double lats[], int nobs, 
                  struct gacmn *pcm, float off)
{

  int i, pendown;
  double xmin, xmax, ymin, ymax, x, y;

  /* Bounding box */
  gxconv(pcm->dmin[0],pcm->dmin[1],&xmin,&ymin,2);
  gxconv(pcm->dmax[0],pcm->dmax[1],&xmax,&ymax,2);

  /* do the plotting */
  gxcolr (pcm->lincol);
  gxstyl (pcm->linstl);
  gxwide (pcm->linthk);
  gxconv(lons[0]+off,lats[0],&x,&y,2);
  gxplot(x,y,0);
  gxplot(x,y,1);
  if ( x < xmin || x > xmax ) pendown = 0;
  else                        pendown = 1;
  for (i=1; i<nobs; i++) {
    if ( fabs(lons[i]-lons[i-1])>90. ) pendown = 0;
      gxconv(lons[i]+off,lats[i],&x,&y,2);
      if ( x < xmin || x > xmax ) {
        pendown = 0;
      } else {
        if ( pendown ) gxplot(x,y,2);
        else           gxplot(x,y,1);
        pendown = 1;
      }
  }

  return;

}

/* ---------------------------------------------------------------------- */

/* Plot satellite ground track */
int c_set ( int argc, char **argv, struct gacmn *pcm) {
  
  char *name = argv[0];
  int rc;

   rc = 0; 

  if (argc<2) {
    sprintf(pout,"\nError from %s: Too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s dt    [dt_in_secs]\n",name); gaprnt(0,pout);
    sprintf(pout,"                  %s mark  mtype\n",name); gaprnt(0,pout);
    sprintf(pout,"                  %s  halo [left[,right]]\n",name); gaprnt(0,pout);
    sprintf(pout,"                  %s ihalo [left[,right]]\n",name); gaprnt(0,pout);
    sprintf(pout,"                  %s jhalo [left[,right]]\n",name); gaprnt(0,pout);
    sprintf(pout,"       Examples:  %s dt 30\n",name);         gaprnt(0,pout);
    sprintf(pout,"                  %s halo 1\n\n",name);         gaprnt(0,pout);
    goto done;
  }

  if ( strcmp(argv[1],"dt") == 0 )  {
    dt_in_secs = atoi(argv[2]);
  } else if ( strcmp(argv[1],"mark") == 0 )  {
    mtype = atoi(argv[2]);

  } else if ( strcmp(argv[1],"ihalo") == 0 )  {
    ihalo[0] = atoi(argv[2]);
    if (argc==4) {
      ihalo[1] = atoi(argv[3]);
    } else {
      ihalo[1] = jhalo[0];
    }
  } else if ( strcmp(argv[1],"jhalo") == 0 )  {
    jhalo[0] = atoi(argv[2]);
    if (argc==4) {
      jhalo[1] = atoi(argv[3]);
    } else {
      jhalo[1] = jhalo[0];
    }
  } else if ( strcmp(argv[1],"halo") == 0 )  {
    ihalo[0] = atoi(argv[2]);
    if (argc==4) {
      ihalo[1] = atoi(argv[3]);
    } else {
      ihalo[1] = ihalo[0];
    }
    jhalo[0] = ihalo[0];
    jhalo[1] = ihalo[1];
 } else if ( strcmp(argv[1],"?") == 0 )  {
    goto done;
  } else {
    sprintf(pout,"ERROR: Invalid option %s\n",argv[1]);
    gaprnt(0,pout);
    dt_in_secs = 60;
    return (1);
  }

  if ( dt_in_secs < 0 ) { 
    sprintf(pout,"ERROR: Invalid dt = %d, using default dt = 30 instead.\n", dt_in_secs);
    gaprnt(0,pout);
    dt_in_secs = 60;
    return (1);
  }

  if ( ihalo[0] < 0 || ihalo[1] < 0 ) { 
    sprintf(pout,"ERROR: Invalid ihalo=(%d,%d), using default ihalo=(0,0) instead.\n", ihalo[0], ihalo[1]);
    gaprnt(0,pout);
    ihalo[0] = 0;
    ihalo[1] = 0;
    return (1);
  }

  if ( jhalo[0] < 0 || jhalo[1] < 0 ) { 
    sprintf(pout,"ERROR: Invalid jhalo=(%d,%d), using default jhalo=(0,0) instead.\n", ihalo[0], ihalo[1]);
    gaprnt(0,pout);
    jhalo[0] = 0;
    jhalo[1] = 0;
    return (1);
  }

 done:
   sprintf(pout,"%s: DT = %d secs\n",name,dt_in_secs); 
   gaprnt(1,pout);
   sprintf(pout,"%s: MTYPE = %d\n",name,mtype); 
   gaprnt(1,pout);
   sprintf(pout,"%s: iHALO = %d %d  jHALO =  %d %d \n",name,
           ihalo[0],ihalo[1],jhalo[0],jhalo[1]);
   gaprnt(1,pout);

  return(rc);

}

/* ---------------------------------------------------------------------- */

/* Plot satellite ground track */
int c_track ( int argc, char **argv, struct gacmn *pcm) {
  
  struct gagrid *pgr;
  int dmin, dt, rc, sat;
  int nymd[2], nhms[2];
  char *name = argv[0];
  // char *sat_name;

  struct dt ta, tb;
  struct dt delt;

  struct dt_ ta_, delt_; /* has secs */

  int i, nmax, nobs;
  double *lons, *lats;

   rc = 0; 

  if (argc<2) {
    sprintf(pout,"\nError from %s: Too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s file.tle\n",name);
    gaprnt(0,pout);
    sprintf(pout,"        Example:  %s aqua.tle \n\n",name);
    gaprnt(0,pout);
    return(1);
  }

  if (*(pcm->pfid->grvals[3]+5)!=0) { /* monthly data not supported for now */
    sprintf(pout,"ERROR: monthly data not supported\n");
    gaprnt(0,pout);
    rc = 2;
    return(rc);
  } else {
    dmin = (int) (*(pcm->pfid->grvals[3]+6)); /* file time step in minutes */
  }

   delt.yr = 0; 
   delt.mo = 0; 
   delt.dy = 0; 
   delt.hr = 0;
   delt.mn = dmin/2; 

   ta = pcm->tmin;
   tb = pcm->tmax;
   timsub_(&delt,&ta);
   timadd_(&delt,&tb);

   nymd[0] = ta.yr * 10000 + ta.mo * 100 + ta.dy;
   nymd[1] = tb.yr * 10000 + tb.mo * 100 + tb.dy;
   nhms[0] = ta.hr * 10000 + ta.mn * 100;
   nhms[1] = tb.hr * 10000 + tb.mn * 100;

  /* satellite name */
  strncpy(Sat_name,argv[1],512);

#if 0
  sat_name = strdup(argv[1]);
  lowcas(sat_name);
       if ( strcmp(sat_name,"aqua")     == 0 )  sat = AQUA;
  else if ( strcmp(sat_name,"calipso")  == 0 )  sat = CALIPSO;
  else if ( strcmp(sat_name,"cloudsat") == 0 )  sat = CLOUDSAT;
  else if ( strcmp(sat_name,"aura")     == 0 )  sat = AURA;
  else if ( strcmp(sat_name,"terra")    == 0 )  sat = TERRA;
  else {
    sprintf(pout,"ERROR: unknown satellite name <%s>\n",argv[1]);
    gaprnt(0,pout);
    rc = 2;
    return(rc);
  }
#endif

   /* Timestep */
   dt = dt_in_secs; /* user specified with set_orb UDC */

   /* Size of output arrays */
   nmax = 1 + dmin * 60 / dt;
   if ( nmax <= 0 ) {
      sprintf(pout,"Error from %s: invalid nmax = %d\n", name, nmax );
      gaprnt (0,pout);
      return(1);
   }   
   lons = (double *) malloc ( nmax * sizeof(double) );
   if ( ! lons ) goto mem;
   lats = (double *) malloc ( nmax * sizeof(double) );
   if ( ! lats ) goto mem;


   sprintf(pout,"%s ground tracks for Tmin = %d %d, Tmax = %d %d\n", 
           Sat_name, nymd[0], nhms[0], nymd[1], nhms[1]);
   gaprnt (2,pout);


    /* call fortran rotuine */
    ftntrack2_ (lons,lats, &nobs, &nmax, Sat_name, nymd, nhms, &dt, &rc );
    if ( rc ) {
      sprintf(pout,"Error from %s: ftntrack returned rc = %d\n", name, rc );
      gaprnt (0,pout);
      goto done;
    }

    /* Print ground track coordinates */
    dtcopy(&ta_,&ta);
    dtcopy(&delt_,&delt);
    delt_.mn = 0;  
    delt_.sc = dt; 
    for ( i=0; i<nobs; i++ ) {
      sprintf(pout,"%4d-%02d-%02d %02d:%02d:%02d %9.3f %8.3f\n", 
              ta_.yr, ta_.mo, ta_.dy, ta_.hr, ta_.mn, ta_.sc, lons[i],lats[i]);
      timadd__(&delt_,&ta_);
      gaprnt (0,pout);
    }

    /* Plot tracks */
    gamscl(pcm);
    gafram(pcm);
    gxclip (pcm->xsiz1, pcm->xsiz2, pcm->ysiz1, pcm->ysiz2);
    if ( mtype > 0 ) {
      marks(lons,lats,nobs,pcm,0.0);
      if ( pcm->dmax[0] > 180.0 ) 
        marks(lons,lats,nobs,pcm,360.0);
    } else {
      lines(lons,lats,nobs,pcm,0.0);
      if ( pcm->dmax[0] > 180.0 ) 
        lines(lons,lats,nobs,pcm,360.0);
    }
      gxfrme(9);

    /* all done */
 done:
    free(lons);
    free(lats);
    return(rc);

mem:
      sprintf(pout,"Error from %s: out of memory\n", name);
      gaprnt (0,pout);
      return(1);

}

/* ---------------------------------------------------------------------- */

int f_mask (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  double *val, undef, *lat, *lon, swath[3];      /* GrADS v2 uses doubles */
  int rc, i, j, im, jm, sat, idim, jdim;
  int dt, dmin, nymd[2], nhms[2];
  struct dt ta, tb;
  struct dt delt;
  char *name = pfc->argpnt[pfc->argnum];
  // char *sat_name;

   rc = 0; 

  if (pfc->argnum<2) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s(expr,file.tle[,swath_left[,swath_right[,ds]])\n",name);
    gaprnt(0,pout);
    sprintf(pout,"        Examples:  %s(ts,aqua.tle)\n",name);
    gaprnt(0,pout);
    sprintf(pout,"                   %s(ts,aqua.tle,400)\n",name);
    gaprnt(0,pout);
    sprintf(pout,"                   %s(ts,aqua.tle,300,500,10)\n",name);
    gaprnt(0,pout);
    return(1);
  }

 if (*(pst->pfid->grvals[3]+5)!=0) { /* monthly data not supported for now */
    sprintf(pout,"ERROR: monthly data not supported\n");
    gaprnt(0,pout);
    rc = 2;
    return(rc);
  } else {
    dmin = (int) (*(pst->pfid->grvals[3]+6)); /* file time step in minutes */
  }

   delt.yr = 0; 
   delt.mo = 0; 
   delt.dy = 0; 
   delt.hr = 0;
   delt.mn = dmin/2; 

   ta = pst->tmin;
   tb = pst->tmax;
   timsub_(&delt,&ta);
   timadd_(&delt,&tb);

   nymd[0] = ta.yr * 10000 + ta.mo * 100 + ta.dy;
   nymd[1] = tb.yr * 10000 + tb.mo * 100 + tb.dy;
   nhms[0] = ta.hr * 10000 + ta.mn * 100;
   nhms[1] = tb.hr * 10000 + tb.mn * 100;

  /* evaluate expression */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  /* satellite name */
  strncpy(Sat_name,pfc->argpnt[1],512);
#if 0
  sat_name = strdup(pfc->argpnt[1]);
  lowcas(sat_name);
       if ( strcmp(sat_name,"aqua")     == 0 )  sat = AQUA;
  else if ( strcmp(sat_name,"calipso")  == 0 )  sat = CALIPSO;
  else if ( strcmp(sat_name,"cloudsat") == 0 )  sat = CLOUDSAT;
  else if ( strcmp(sat_name,"aura")     == 0 )  sat = AURA;
  else if ( strcmp(sat_name,"terra")    == 0 )  sat = TERRA;
  else {
    sprintf(pout,"ERROR from %s: unknown satellite name <%s>\n",name,pfc->argpnt[1]);
    gaprnt(0,pout);
    rc = 2;
    return(rc);
  }
#endif

  /* Swath */
  if ( pfc->argnum == 3 ) {
      swath[0] = atof(pfc->argpnt[2])/2.0;
      swath[1] = swath[0];
      swath[2] = 10.0; /* cross-track resolution */ 
  } else if ( pfc->argnum == 4 ) {
      swath[0] = atof(pfc->argpnt[2]);
      swath[1] = atof(pfc->argpnt[3]);
      swath[2] = 10.;  /* cross-track resolution */ 
  } else if ( pfc->argnum == 5 ) {
      swath[0] = atof(pfc->argpnt[2]);
      swath[1] = atof(pfc->argpnt[3]);
      swath[2] = atof(pfc->argpnt[4]);  /* cross-track resolution */ 
  } else {
      swath[0] =  0.0;
      swath[1] =  0.0;
      swath[2] = 10.0;
  }
      
  if ( swath[0] < 0 || swath[1] < 0 ) {
      sprintf(pout,"\nERROR from %s: invalid swath =(%f,%f)\n", name,swath[0],swath[1]);
      gaprnt(0,pout);
      return(1);
  }

  /* Time step */
  dt = dt_in_secs;

   sprintf(pout,"%s satellite masking for Tmin = %d %d, Tmax = %d %d with dt = %d secs\n", 
           Sat_name, nymd[0], nhms[0], nymd[1], nhms[1], dt);
   gaprnt (2,pout);


  if (pst->type==1) {  /* gridded data */
 
    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;
    idim   = pgr->idim;
    jdim   = pgr->jdim;
    undef = pgr->undef;

    /* generate coordinate variables */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* Run the masking code in Fortran */
    ftnmasking2_ ( val, &im, &jm, lon, lat, &undef, 
                  Sat_name, nymd, nhms, &dt, swath, 
                  ihalo, jhalo, &rc );
    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    }

  } else {  /* station data */

    sprintf(pout,"Error from %s: station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

  free(lat);
  free(lon);

  /* Make sure GrADS undef mak is properly set */
  rc = gex_setUndef(pst);  
  return(rc);

 memerr:
    sprintf(pout,"Error from %s: not enough memory\n", name);
    gaprnt (0,pout);
    return (1);

}

/* .................................................................. */

 static
 int get_latlons ( double **lons, double **lats, 
                   struct gagrid *pgr, char *name  ) {

  int i, j, im, jm;
  double dlon, dlat, *lon, *lat;
  double (*conv) (double *, double);

  /* varying dimensions must be lon and lat for spherepak */
  if ( pgr->idim != 0 || pgr->jdim != 1 ) {
    sprintf(pout,"Error from %s: input must be lat/lon grid \n", name);
    gaprnt (0,pout);
    return (1);
  }

  /* Longitudes */
  im   = pgr->isiz;
  j = 0;
  lon = (double *) malloc ( sizeof(double)*im );
  if ( lon ) {
    conv = pgr->igrab;
    for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) 
      lon[j++] = conv(pgr->ivals,(double) i );
  } 
  else {
    sprintf(pout,"Error from %s: out of memory (lon)\n", name);
    gaprnt (0,pout);
    return (1);
    }
  
  /* latitudes */
  j = 0;
  jm   = pgr->jsiz;
  lat = (double *) malloc ( sizeof(double)*jm );
  if ( lat ) {
    conv = pgr->jgrab;
    for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) 
      lat[j++] = conv(pgr->jvals,(double) i );
  }
  else {
    sprintf(pout,"Error from %s: out of memory (lat)\n", name);
    gaprnt (0,pout);
    free (lon);
    return (1);
  }

  *lons = lon;
  *lats = lat;

  return (0);

}

/* .................................................................... */

/* Subtract an offset from a time.  Subtract minutes/hours/days
   first so that we will exactly reverse the operation of timadd     */

/* Date/Time manipulation routines.  Note that these routines
   are not particularly efficient, thus Date/Time conversions
   should be kept to a minimum.                                      */

static gaint mosiz[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
static gaint momn[13] = {0,44640,40320,44640,43200,44640,43200,
                        44640,44640,43200,44640,43200,44640};
static gaint mnacum[13] = {0,0,44640,84960,129600,172800,217440,
                        260640,305280,349920,393120,437760,480960};
static gaint mnacul[13] = {0,0,44640,86400,131040,174240,218880,
                        262080,306720,351360,394560,439200,482400};


/* Subtract an offset from a time.  Subtract minutes/hours/days
   first so that we will exactly reverse the operation of timadd.
   This is modified version of the timsub() function in GrADS.     

*/

static void timsub_ (struct dt *dtim, struct dt *dto) {
gaint s1,s2;

  /* Subtract minutes, hour, and days directly.  Then normalize
     to days, then normalize deficient days from months/years.       */

  dto->mn -= dtim->mn;
  dto->hr -= dtim->hr;
  dto->dy -= dtim->dy;

  while (dto->mn < 0) {dto->mn+=60; dto->hr--;}
  while (dto->hr < 0) {dto->hr+=24; dto->dy--;}

  while (dto->dy < 1) {
    dto->mo--;
    if (dto->mo < 1) {dto->mo=12; dto->yr--;}
    if (dto->mo==2 && qleap(dto->yr)) dto->dy += 29;
    else dto->dy += mosiz[dto->mo];
  }

  /* Now subtract months and years.  Normalize as needed.            */

  dto->mo -= dtim->mo;
  dto->yr -= dtim->yr;

  while (dto->mo < 1) {dto->mo+=12; dto->yr--;}

  /* Adjust for leaps */

  if (dto->mo==2 && dto->dy==29 && !qleap(dto->yr)) {
    dto->mo=3; dto->dy=1;
  }
}

static void timadd_ (struct dt *dtim, struct dt *dto) {
gaint i;
gaint cont;

  /* First add months and years.  Normalize as needed.               */
  dto->mo += dtim->mo;
  dto->yr += dtim->yr;

  while (dto->mo>12) {
    dto->mo -= 12;
    dto->yr++;
  }

  /* Add minutes, hours, and days directly.  Then normalize
     to days, then normalize extra days to months/years.             */

  dto->mn += dtim->mn;
  dto->hr += dtim->hr;
  dto->dy += dtim->dy;

  if (dto->mn > 59) {
    i = dto->mn / 60;
    dto->hr += i;
    dto->mn = dto->mn - (i*60);
  }
  if (dto->hr > 23) {
    i = dto->hr / 24;
    dto->dy += i;
    dto->hr = dto->hr - (i*24);
  }

  cont = 1;
  while (dto->dy > mosiz[dto->mo] && cont) {
    if (dto->mo==2 && qleap(dto->yr)) {
      if (dto->dy == 29) cont=0;
      else {
        dto->dy -= 29;
        dto->mo++;
      }
    } else {
      dto->dy -= mosiz[dto->mo];
      dto->mo++;
    }
    while (dto->mo > 12) {dto->mo-=12; dto->yr++;}
  }
}

/* .................................................................... */

static void dtcopy ( struct dt_ *t_, struct dt *t) {
  t_->yr = t->yr; 
  t_->mo = t->mo; 
  t_->dy = t->dy; 
  t_->hr = t->hr; 
  t_->mn = t->mn;
  t_->sc = 0;
}

void timadd__ (struct dt_ *dtim, struct dt_ *dto) { /* version with secs */
gaint i;
gaint cont;

  /* First add months and years.  Normalize as needed.               */
  dto->mo += dtim->mo;
  dto->yr += dtim->yr;

  while (dto->mo>12) {
    dto->mo -= 12;
    dto->yr++;
  }

  /* Add minutes, hours, and days directly.  Then normalize
     to days, then normalize extra days to months/years.             */

  dto->sc += dtim->sc;
  dto->mn += dtim->mn;
  dto->hr += dtim->hr;
  dto->dy += dtim->dy;

  if (dto->sc > 59) {
    i = dto->sc / 60;
    dto->mn += i;
    dto->sc = dto->sc - (i*60);
  }
  if (dto->mn > 59) {
    i = dto->mn / 60;
    dto->hr += i;
    dto->mn = dto->mn - (i*60);
  }
  if (dto->hr > 23) {
    i = dto->hr / 24;
    dto->dy += i;
    dto->hr = dto->hr - (i*24);
  }

  cont = 1;
  while (dto->dy > mosiz[dto->mo] && cont) {
    if (dto->mo==2 && qleap(dto->yr)) {
      if (dto->dy == 29) cont=0;
      else {
        dto->dy -= 29;
        dto->mo++;
      }
    } else {
      dto->dy -= mosiz[dto->mo];
      dto->mo++;
    }
    while (dto->mo > 12) {dto->mo-=12; dto->yr++;}
  }
}


/* .................................................................... */


  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

tle.gex - GrADS Extension Library for TLE-based Satellite Orbit Masking

=head1 SYNOPSIS

=head3 GrADS Commands:

=over 4

=item 

run B<set_tle> I<PARAM> I<VALUE> - Set internal parameters

=item 

run B<tle_track> I<TLEFILE> - Plot satellite orbits

=back

=head3 GrADS Function:

=over 4

=item 

display B<tle_mask>(I<EXPR,TLEFILE[,SWATH_WIDTH|SWATH_LEFT,SWATH_RIGHT[,dS]]>)  - Mask I<EXPR> according to orbit

=back

=head1 DESCRIPTION 

This library provides GrADS extensions (I<gex>) for simulating
satellite orbits and masking gridded variables according to these
orbits. The calculation in this extension is meant to be a
simulation of the satellite orbit for research purposes and should 
not be used for forecasting real satellite overpasses. This calculation is based
on Two-line Element (TLE) data using the SGP4 algorithm.

This GrADS extension is based on SGP4 procedures for analytical propagation
of a satellite trajectory. The code was originally released in the 1980 and 1986
spacetrack papers. A detailed discussion of the theory and history
may be found in the 2006 AIAA paper by Vallado, Crawford, Hujsak,
and Kelso. Consult also the book "Fundamentals of Astrodynamics and Applications" (2007) by David Vallado.

GrADS datasets are given on a constant time interval. For a given
GrADS time index I<t>, satellite orbits can be simulated for the
interval from I<t-0.5> to I<t+0.5>. During this period the satellite
describe a ground track which can be plotted with the command
B<tle_track>. With function B<tle_mask()> one can mask gridded variables
by setting to undefined those gridpoints which were not visited by the
satellite, with the possibility of specifying the swath width and halo
gridpoints.

When combined with a compression algorithm (such as in GRIB),
satellite masked variables can use considerabily less storage space.

=head1 TLE FILES

A NORAD two-line element set consists of two 69-character lines of 
data which can be used together with NORAD SGP4/SDP4 orbital model 
to determine the position and velocity of
the associated satellite.  More detailed information about TLE files,
including description of the format, can be obtained from:

=over 4

=item

L<http://celestrak.com/columns/v04n03/>

=back

Sample TLE files are shipped with the OpenGrADS Bundle under the GrADS Data
directory (GADDIR).  Additional TLE files can be found at these sites:

=over 4

=item 

L<http://celestrak.com>

=item 

L<http://www.space-track.org>

=back


=head1 EXAMPLES

=head2 Plotting orbits

=over 4

One can plot lines depicting the satellite ground track with 
the B<tle_track> command:

 ga-> open model
 ga-> d ts
 ga-> tle_track aqua.tle

Instead of lines, one can also use marks to plot the ground track,

 ga-> set_tle mark 6
 ga-> tle_track aqua.tle

=back

=head2 Masking variables according to satelite orbits

=over 4

This example shows how to set to undefined all grid points which have not
been visited by the satellite. For a given GrADS time index I<t>, the
satellite ground track is computed from I<t-0.5> to I<t+0.5> and this 
simulated ground track is used to mask a variable:

 ga-> open model
 ga-> define xts = re(ts,0.5) # higher resolution version
 ga-> set gxout grfill

 ga-> d tle_mask(xts,aqua.tle)

By default, the variable is masked according to the satellite ground track. However, it is 
also possible to specify the swath width (in km),

 ga-> d tle_mask(xts,aqua.tle,300)

You can also specify a halo, that is, additional gridpoints around the mask: 

 ga-> set_tle halo 2
 ga-> d tle_mask(xts,aqua.tle)

Halos are useful for interpolating gridded values to observation locations.

=back

=head1 COMANDS PROVIDED

=head2  B<set_tle> I<PARAM> VALUE 

=over 4

This command can be use to set several internal paramers, namely

=over 8

=item B<set_tle> I<DT> TIMESTEP_IN_SECS 

Specify the timestep in seconds used to simulate the ground track. For
example, a timestep of 60 seconds will cause of the
(longitude,latitude) of the ground track to be computed every minute.

=item B<set_tle> I<MARK> MARK_TYPE

Specify the the type of mark used to plot the ground track. By
default, MARK_TYPE=-1, and straight lines are used to plot the ground
track. Other possibles values for I<MARK_TYPE> are:

  1 - plus sign
  2 - open circle (default)
  3 - closed circle 
  4 - open square 
  5 - closed square 
  6 - multiplication sign
  7 - open diamond 
  8 - open triangle 
  9 - closed triangle
 10 - open circle with vertical bar
 11 - closed circle with vertical bar

=item B<set_tle> I<IHALO> LEFT[,RIGHT]

Specify the number of I<zonal> gridpoints to extend an orbital mask
with. The parameter I<LEFT> is the number of gridpoints west of the
mask, while I<RIGHT> is the number of gridpoints east of the
mask. When I<RIGHT> is missing it is set the same as I<LEFT>.

=item B<set_tle> I<JHALO> BELOW[,ABOVE]

Specify the number of I<meridional> gridpoints to extend an orbital
mask with. The parameter I<BELOW> is the number of gridpoints south of
the mask, while I<ABOVE> is the number of gridpoints to the north of
the mask. When I<ABOVE> is missing it is set the same as I<BELOW>.

=item B<set_tle> I<HALO> LEFT[,RIGHT]

Specify the number of I<meridional> and I<zonal> gridpoints to extend
an orbital mask with. The parameter I<LEFT> is the number of
gridpoints to the west/south of the mask, while I<RIGHT> is the number of
gridpoints to the east/north of the mask. When I<RIGHT> is missing it is
set the same as I<LEFT>.

=back

=back


=head2  B<tle_track> I<TLEFILE> 

=over 4

This command plots the ground track for a satellite given a TLE file.  For a
given GrADS time index I<t>, the satellite ground track is computed
from I<t-0.5> to I<t+0.5>. This ground track is meant to be a
simulation of the satellite orbit and should not be used 
for forecasting real satellite overpasses. 

=over 8

=item I<TLEFILE> 

Two-line element file name.

=back

=back


=head1 FUNCTIONS PROVIDED

=head2  B<tle_mask>(I<EXPR,TLEFILE[,SWATH_WIDTH|SWATH_LEFT,SWATH_RIGHT[,dS]]>) 

=over 4

This function masks out those grind points which have not been visited
by the satellite from grads time index I<t-0.5> to time index
I<t+0.5>. On can optionally specify a swath width.

=over 8 

=item I<EXPR> 

GrADS expressions to be masked

=item I<TLEFILE> 

Two-line element (TLE) file name.

=item I<SWATH_WIDTH>

Width of the swath in kilometers, centered around the ground
track. This gives SWATH_WIDTH/2 kilometers to the left/right of the ground
track. Default is 0 (no swath).

=item I<SWATH_LEFT,SWATH_RIGHT>

Width of the swath in kilometers to the left and right of the satellite ground track.
Default is 0 (no swath).

=item I<dS>

Swath resolution, in kilometers. Default is 10 km.

=back

=back


=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://cookbooks.opengrads.org/index.php?title=Main_Page> -
OpenGrADS Cookbooks

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=item *

OpenGrADS extension I<ORB> which implements similar functionality for
select NASA EOS satellites.

=back

=head1 AUTHORS 

Arlindo da Silva (dasilva@opengrads.org) and Arif Albayrak.

=head1 COPYRIGHT

This extension has been placed in the public domain.

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif
