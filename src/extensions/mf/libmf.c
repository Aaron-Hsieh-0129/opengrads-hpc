/*

    Copyright (C) 2009,2010 by Michael Fiorino
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; using version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, please consult  
              
              http://www.gnu.org/licenses/licenses.html

    or write to the Free Software Foundation, Inc., 59 Temple Place,
    Suite 330, Boston, MA 02111-1307 USA


                        About POD Documentation 

The following documentation uses the "Perl On-line Documentation"
(POD) mark up syntax. It is a simple and yet adequate mark up language
for creating basic man pages, and there are converters to html,
MediaWiki, etc. In adittion, the perldoc utility can be used to
display this documentation on the screen by entering:

% perldoc re

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < libmf.c > libmf.pod

and place libmf.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < libmf.pod > libmf.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < libmf.pod > libmf.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make libmf.html
% make libmf.wiki

*/

#ifndef POD

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "grads.h"

#include "gex.h"

#if GEX_GAMAJOR == 2
int gex_setUndef (struct gastat *pst);
#endif

/* Enter here a prototype for your fortran subroutine; if you can,
   avoid using character arguments.

   IMPORTANT: *Do not* use underscores in the fortran routine name; keep the
              one at the end here; you fortran routine in this case should
              be called "ftnhilo", without the undercore at the end.

*/


void ftntcprop_ ( gaData_t *guesslon, gaData_t *geusslat, gaData_t *radinf,  gaData_t *bearing,
		   gaData_t *val, gaData_t *undef, gaData_t *lon, gaData_t *lat, 
                   int *im, int *jm, gaData_t *opoints, int *nr, int *nv);

static int get_latlons ( gaData_t **lons, gaData_t **lats, 
                         struct gagrid *pgr, char *name );

static char pout[256];   /* Build error msgs here */
static char chcard[120];
static char card[120];

static int counter=0;


/*-------------------------------- udc_tcprop -----------------------*/

int c_tcprop ( int argc, char **argv, struct gacmn *pcm ) {

  struct gastat *pst;
  struct gagrid *pgr;

  gaData_t *val, *lat, *lon;
  gaData_t undef, xbegi, dxi, ybegi, dyi;
  gaData_t radinf,cintinf,pcntile;
  gaData_t latc,lonc;

  int nr=100;
  int nv=20;

  gaData_t opoints[nv][nr];

  char *name = argv[0];

/*
    radinf is the distance (nm) from the storm center to calc the gradient
    fld(radinf) - fld(center) / radinf gradient
*/

  gaData_t bearing,bear1,bear2,bear3,b1ne,b2ne,b1se,b2se,b1sw,b2sw,b1nw,b2nw;

  gaData_t (*conv) (gaData_t *, gaData_t);

  int maxmin;
  int rc, rclatlon, i,j,n, im, jm;
  int verb=0;
  
  char *method;

  counter++;
  if(argc == 1) {
      printf("udc function '%s': field properties relative to a lon/lat grid point, typically a TC, and returns parameters to the script variable 'result'\n", argv[0]);
      printf("\n");
      printf("usage: %s expr lonc latc radinf\n", argv[0]);
      printf("\n");
      printf("    expr = grads 2-d lon/lat grid expression\n");
      printf("    lonc = longitude of center (must be consistent with grads dim env)\n");
      printf("    latc = latitude of center\n");
      printf("  radinf = distance in nm away from lonc/latc to calculate properties\n\n");
      gaprnt(0,"Examples:\n");
      gaprnt(0,"   ga->tcprop psl 144.5 13.2 300 000\n\n");

      return 1;
}

  /* defaults */

  rc=0;
  
  radinf=120.0;
  bearing=0.0;

  /* command line input */

  for (i=0; i<argc; i++) {

    if(verb) printf("    argv[%d] = <%s>\n", i, argv[i]);

    if(i == 2){
	latc=atof(argv[i]);
    } else if(i == 3) {
	lonc=atof(argv[i]);
    } else if(i == 4) {
	radinf=atof(argv[i]);
    } else if(i == 5) {
	bearing=atof(argv[i]);
    } else if(i == 6) {
	method=argv[i];
    }

}

  pst = getpst(pcm);
  if (pst==NULL) return(1);

  rc = gaexpr(argv[1],pst);
  if (rc)  return (rc);

  pgr  = pst->result.pgr;
  val  = pgr->grid;

  undef = pgr->undef;
  im    = pgr->isiz;
  jm    = pgr->jsiz;

  rclatlon = get_latlons ( &lon, &lat, pgr, argv[0] );


  if(argc <= 2) {
      lonc=0.0;
      for(i=0;i<im;i++) lonc=lonc+lon[i];
      if(im>0) lonc=lonc/im;

      latc=0.0;
      for(j=0;j<jm;j++)  latc=latc+lat[j];
      if(jm>0) latc=latc/jm;
  }

  bear1=bearing;
  bear2=bear1-180.0;
  if(bear2 < 0.0) bear2=360.0+bear2;

  bear3=bear1;
  if(bear2 > bear1) bear3=bear1+360.0;


  b1ne=bearing;
  b2ne=b1ne+90.0;
  b1se=b2ne;
  b2se=b1se+90.0;
  b1sw=b2se;
  b2sw=b1sw+90.0;
  b1nw=b2sw;
  b2nw=b1nw+90.0;

/* call the fortran */

  ftntcprop_ ( &latc, &lonc, &radinf, &bearing, val, &undef, lon, lat, &im, &jm,
	        (gaData_t *) opoints, &nr, &nv );

/* print the output to the result script var too */

  sprintf(pout,"%s for expr: %s Latc: %7.2f  Lonc: %7.2f  Radinf: %5.0f [nm] Bearing: %5.0f\n",
	  argv[0],argv[1],latc,lonc,radinf,bearing);
  gaprnt (0,pout);

  sprintf(pout,"MeanRadCenterGrad:  %12.6g [units: (rad-center)/nm]\n",opoints[0][2]);
  gaprnt (0,pout);

  sprintf(pout,"MAXRadCenterGrad:   %12.6g [units: (rad-center)/nm]\n",opoints[0][0]);
  gaprnt (0,pout);

  sprintf(pout,"minRadCenterGrad:   %12.6g [units: (rad-center)/nm]\n",opoints[0][1]);
  gaprnt (0,pout);

  sprintf(pout,"MeanRadinf: %12.6g \n",opoints[1][0]);
  gaprnt (0,pout);


  sprintf(pout,"RadMean:   %12.6g \n",opoints[2][0]);
  gaprnt (0,pout);

  sprintf(pout,"RadMAX:    %12.6g \n",opoints[3][0]);
  gaprnt (0,pout);

  sprintf(pout,"Radmin:    %12.6g \n",opoints[4][0]);
  gaprnt (0,pout);

  sprintf(pout,"Hemi1:           %3.0f-%3.0f  Hemi2:           %3.0f-%3.0f\n",
	  bear1,bear2,bear2,bear3);
  gaprnt (0,pout);

  sprintf(pout,"MeanHemi1:  %12.6g  MeanHemi2:  %12.6g \n",opoints[1][1],opoints[1][2]);
  gaprnt (0,pout);

  sprintf(pout,"QuadNE:          %3.0f-%3.0f  QuadSE:          %3.0f-%3.0f  QuadSW:          %3.0f-%3.0f  QuadNW:          %3.0f-%3.0f\n",
	  b1ne,b2ne,b1se,b2se,b1sw,b2sw,b1nw,b2nw);
  gaprnt (0,pout);

  sprintf(pout,"MeanNE:     %12.6g  MeanSE:     %12.6g  MeanSW:     %12.6g  MeanNW:     %12.6g \n",opoints[1][3],opoints[1][4],opoints[1][5],opoints[1][6]);
  gaprnt (0,pout);

  sprintf(pout,"RadMeanNE:  %12.6g  RadMeanSE:  %12.6g  RadMeanSW:  %12.6g  RadMeanNW:  %12.6g \n",opoints[2][3],opoints[2][4],opoints[2][5],opoints[2][6]);
  gaprnt (0,pout);

  sprintf(pout,"RadMaxNE:   %12.6g  RadMaxSE:   %12.6g  RadMaxSW:   %12.6g  RadMaxNW:   %12.6g \n",opoints[3][3],opoints[3][4],opoints[3][5],opoints[3][6]);
  gaprnt (0,pout);

  sprintf(pout,"RadMinNE:   %12.6g  RadMinSE:   %12.6g  RadMinSW:   %12.6g  RadMinNW:   %12.6g \n",opoints[4][3],opoints[4][4],opoints[4][5],opoints[4][6]);
  gaprnt (0,pout);

  sprintf(pout,"dRadMeanNE: %12.6g  dRadMeanSE: %12.6g  dRadMeanSW: %12.6g  dRadMeanNW: %12.6g \n",opoints[5][3],opoints[5][4],opoints[5][5],opoints[5][6]);
  gaprnt (0,pout);

  sprintf(pout,"dRadMaxNE:  %12.6g  dRadMaxSE:  %12.6g  dRadMaxSW:  %12.6g  dRadMaxNW:  %12.6g \n",opoints[6][3],opoints[6][4],opoints[6][5],opoints[6][6]);
  gaprnt (0,pout);

  sprintf(pout,"dRadMinNE:  %12.6g  dRadMinSE:  %12.6g  dRadMinSW:  %12.6g  dRadMinNW:  %12.6g \n",opoints[7][3],opoints[7][4],opoints[7][5],opoints[7][6]);
  gaprnt (0,pout);

  free(lon);
  free(lat);
  return(rc);

}



/*-------------------------------- udc_tcxy2rt -----------------------*/


void ftntcxy2rt_ ( gaData_t fld[], int *im, int *jm, gaData_t *undef,
	           gaData_t xylat[], gaData_t xylon[],
		   gaData_t *clat, gaData_t *clon,
		   gaData_t *drad, gaData_t *dtheta, gaData_t *radmax,
		   gaData_t *opoints, gaData_t *olats, gaData_t *olons, int *nr, int *nv,   
                   int *rc );


int c_tcxy2rt ( int argc, char **argv, struct gacmn *pcm ) {

  struct gastat *pst;
  struct gagrid *pgr;

  gaData_t *val, *xylat, *xylon;
  gaData_t undef;
  gaData_t clat,clon;

  gaData_t (*conv) (gaData_t *, gaData_t);

  int rc, rclatlon, i,j,n, im, jm;
  int verb=0;
  
  char *name = argv[0];

/* defaults
   units are nm and degrees */

  gaData_t rad,drad=15.0;
  gaData_t theta,dtheta=15.0;
  gaData_t radmax=300.0;

  rc=0;


/* command line input */

  counter++;
  if(argc == 1) {
      printf("udc function '%s': cartesian -> cylindrical coordinate xfrm; returns to script variable 'result'\n", argv[0]);
      printf("\n");
      printf("usage: %s expr clat clon [drad] [dtheta] [radmax]\n", argv[0]);
      printf("\n");
      printf("    expr = grads 2-d lon/lat grid expression\n");
      printf("    clat = latitude of center\n");
      printf("    clon = longitude of center (must be consistent with grads dim env)\n");
      printf("    drad = deltaR -- distance [nm] nm \n");
      printf("  dtheta = deltaTheta -- angle [deg] \n");
      printf("  radmax = max extent in R [nm] \n\n");
      gaprnt(0,"Examples:\n");
      gaprnt(0,"   ga->tcxy2rt psl 144.5 13.2 15 45\n\n");

      return 1;
  }

  for (i=0; i<argc; i++) {

    if(verb) printf("    argv[%d] = <%s>\n", i, argv[i]);

    if(i == 2){
	clat=atof(argv[i]);
    } else if(i == 3) {
	clon=atof(argv[i]);
    } else if(i == 4) {
        drad=atof(argv[i]);
    } else if(i == 5) {
        dtheta=atof(argv[i]);
    } else if(i == 6) {
        radmax=atof(argv[i]);
    }

  }


  /* calc # of data points in cylindrical coordinates */

  
  int nr=(radmax/drad) + 1;
  int nt=(360.0/dtheta);

  gaData_t opoints[nt][nr];
  gaData_t olats[nt][nr];
  gaData_t olons[nt][nr];

  pst = getpst(pcm);
  if (pst==NULL) return(1);

  rc = gaexpr(argv[1],pst);
  if (rc)  return (rc);

  pgr  = pst->result.pgr;
  val  = pgr->grid;

  undef = pgr->undef;
  im    = pgr->isiz;
  jm    = pgr->jsiz;

  rclatlon = get_latlons ( &xylon, &xylat, pgr, argv[0] );


  if(argc <= 2) {
      clon=0.0;
      for(i=0;i<im;i++) clon=clon+xylon[i];
      if(im>0) clon=clon/im;

      clat=0.0;
      for(j=0;j<jm;j++)  clat=clat+xylat[j];
      if(jm>0) clat=clat/jm;
  }


/* call the fortran */


  ftntcxy2rt_ ( val, &im, &jm, &undef,
       xylat, xylon,
       &clat, &clon,
       &drad, &dtheta, &radmax,
       (gaData_t *)opoints, (gaData_t *)olats, (gaData_t *)olons, &nr, &nt,
       &rc );

/* print the output to the result script var too */

  sprintf(pout,"%s for expr: %s  clat: %7.2f  clon: %7.2f  NB: OUTPUT ONLY to script var RESULT \n",
	  argv[0],argv[1],clat,clon);
  gaprnt (0,pout);

  sprintf(pout,"nr: %4d  nt: %4d  drad: %5.0f [nm] dtheta: %5.0f  radmax: %5.0f\n",
	  nr,nt,drad,dtheta,radmax);
  gaprnt (2,pout);

  sprintf(pout,"   i    j    r   theta       val\n");
  gaprnt (2,pout);


  for(i=0;i<nr;i++) {
      for(j=0;j<nt;j++) {
	  rad=i*drad;
	  theta=j*dtheta;
	  sprintf(pout,"%4d %4d  %4.0f %4.0f  %7.2f %7.2f   %13.8g\n",i,j,rad,theta,olats[j][i],olons[j][i],opoints[j][i]);
	  gaprnt (2,pout);
      }
  }

  free(xylon);
  free(xylat);
  return(rc);

}



void ftngrhist_ (  gaData_t *val,  int *np, gaData_t *undef,  gaData_t *opoints, int *nr, int *nv);


/*-------------------------------- udc_grhist -----------------------*/

int c_grhist ( int argc, char **argv, struct gacmn *pcm ) {

  struct gastat *pst;
  struct gagrid *pgr;

  gaData_t *val;
  gaData_t undef;

  int nr=100;
  int nv=2;

  int ncutpoints;
  int im,jm,np;
  int i,rc;
  int verb=0;

  gaData_t opoints[nr][nv];

  char *name = argv[0];

  if(argc == 1) {

      return 1;
}

  /* defaults */

  /* command line input */

  for (i=0; i<argc; i++) {

    if(verb) printf("    argv[%d] = <%s>\n", i, argv[i]);

    if(i == 2){
	ncutpoints=atoi(argv[i]);
    }	

  }

  pst = getpst(pcm);
  if (pst==NULL) return(1);

  rc = gaexpr(argv[1],pst);
  if (rc)  return (rc);

  pgr  = pst->result.pgr;
  val  = pgr->grid;

  undef = pgr->undef;
  im    = pgr->isiz;
  jm    = pgr->jsiz;

  np = im*jm;

  ftngrhist_ ( val, &np, &undef,
	          (gaData_t *) opoints, &nr, &nv );

  sprintf(pout,"%s for expr: %s\n",
	  argv[0],argv[1]);
  gaprnt (0,pout);

  return(rc);

}




/*----------------------- udc_mfhilo -----------------------*/


void ftnclhilo_ ( gaData_t *val, gaData_t *undef, gaData_t *lon, gaData_t *lat, int *im, int *jm,
		  int *maxmin, gaData_t *radinf, gaData_t *cintinf, 
		  gaData_t *lonc, gaData_t *latc,
		  gaData_t *opoints,
		  int *npoints, int *nv, int *nrr, int *rc);

void ftngrhilo_ ( gaData_t *val, gaData_t *undef, gaData_t *lon, gaData_t *lat, int *im, int *jm,
		  int *maxmin, gaData_t *pcntile, char *sortby, int *nsigdig,
		  gaData_t *lonc, gaData_t *latc, gaData_t *radinf,
		  gaData_t *opoints,
		  int *npoints, int *nv, int *nrr, int *rc);

void ftntmhilo_ ( gaData_t *guesslon, gaData_t *geusslat, int *ictype,
		  gaData_t *val, gaData_t *undef, gaData_t *lon, gaData_t *lat, int *im, int *jm,
		  int *itype, int *jtype, int *ilinr, int *jlinr,
		  gaData_t *xbegi, gaData_t *dxi, gaData_t *ybegi, gaData_t *dyi,
		  int *maxmin, int *resopt, gaData_t *radinf,
		  gaData_t *opoints,
		  int *npoints, int *nv, int *rc );

int c_mfhilo ( int argc, char **argv, struct gacmn *pcm ) {

  struct gastat *pst;
  struct gagrid *pgr;

  gaData_t *val, *lat, *lon;
  gaData_t *opointsftn;
  gaData_t undef, xbegi, dxi, ybegi, dyi;
  gaData_t radinf,cintinf,pcntile;
  gaData_t latc,lonc;


/*
    radinf is the distance (nm) from the storm center to calc the gradient
    fld(radinf) - fld(center) / radinf gradient
*/

  gaData_t (*conv) (gaData_t *, gaData_t);

  gaData_t guesslon,guesslat,olonc, olatc, ovalc;
  
  int maxmin;
  int rc, rclatlon, i,j,n, im, jm, itype, jtype, ilinr, jlinr, resopt;
  int verb=0;


  int ictype;
  int npoints=2000;
  int nv=12;

  int nrr=0;

  char method[2];

  int docl=1;
  int dogr=0;
  int dotm=0;

  char *name = argv[0];

/* output array */

  
  gaData_t opoints[npoints][nv];
  opointsftn=*opoints;
  
  counter++;

  if(argc == 1) {
      printf("udc function '%s': find max/min in a 2-D lat/lon field; return location and value between grid points to script variable 'result'\n", argv[0]);
      printf("\n");
      printf("usage: %s expr [method] [methodopts]\n", argv[0]);
      printf("\n");
      printf("    expr = grads expression\n");
      printf("    method = cl | gr | tm \n\n");


      printf(" for method=\'GR\' :: Mike Fiorino GRid routines ; methodopts = maxmin sortby pcntile latc lonc nsigdig\n\n");

      printf("   maxmin = l|L lows only; h|H highs only; a|A|b|B both\n");
      printf("   sortby = m order by mag/value at extrema; g by gradient; l by laplacian; t by latitude; n by longitude\n");
      printf("  pcntile = set percentile (0-100) of top N points sorted by gradient\n");
      printf("     latc = set center latitude for distance calculations (consistent with grads dim env); default [-999] is grid center\n");
      printf("     lonc = set center longitude for distance calculations (consistent with grads dim env); default [-999] is grid center\n");
      printf("   radinf = set radial distance [nm] from latc,lonc to search; default [-999] is to not check\n");
      printf("  nsigdig = controls gribbish noise when set between 2 and 7 defaults is -1 (no check)\n\n");


      printf(" for method=\'CL\' :: NCEP CLosed routines ; methodopts = maxmin rad cintinf latc lonc\n\n");

      printf("   maxmin = l|L lows only; h|H highs only; a|A|b|B both\n");
      printf("      radinf = size radius in nm, default is 120, bigger fewer\n");
      printf("     cintinf = contour interval between max and nearby point; smaller more extrema, < 0\n\n");
      printf("     latc = set center latitude for distance calculations (consistent with grads dim env)\n");
      printf("     lonc = set center longitude for distance calculations (consistent with grads dim env)\n\n");

      printf(" for method=\'TM\' :: Tim Marchok center locator using barnes analysis ; methodopts = maxmin guesslat guesslon\n\n");

      printf("   maxmin = l|L lows only; h|H highs only; a|A|b|B both\n");
      printf(" guesslat = set first guess center latitude (consistent with grads dim env)\n");
      printf(" guesslon = set first-guess center longitude (consistent with grads dim env)\n");
      //printf("   radinf = size radius in nm, default is 200, bigger fewer\n");

      return 1;
}

  rc=0;
/* defaults grhilo */

  pcntile=0;
  char sortby='m';
  char cmaxmin='a';

/*---
    nsigdig controls precision for an extrema to be considered 'significant'  default is 6 which
    knocks down gribish noise... can be set at end of options to mfhilo call, but not documented...
*/
  int nsigdig=-1;

/* command line input */

  latc=lonc=-999.0;
  dogr=1;
  docl=0;
  dotm=0;

  for (i=0; i<argc; i++) {

    if(verb) printf("    argv[%d] = <%s>\n", i, argv[i]);

    if(i == 2) {
	if(strcmp(argv[i],"gr") == 0 || strcmp(argv[i],"GR") == 0) {
	    docl=0; dogr=1; dotm=0;
	} else if(strcmp(argv[i],"cl") == 0 || strcmp(argv[i],"CL") == 0) {
	    docl=1; dogr=0; dotm=0;
	} else if(strcmp(argv[i],"tm") == 0 || strcmp(argv[i],"TM") == 0) {
	    dogr=0; docl=0; dotm=1; cmaxmin='X'; resopt=0;

	} 
    }

/* defaults clhilo */
    if(docl && i == 2) {
      radinf=240.0;
      cintinf=100.0;
    }

/* defaults grhilo */
    if(dogr && i == 2) {
      radinf=-999.0;
    }

/* defaults tmhilo */
    if(dotm && i == 2) {
      radinf=250.0;
      ictype=0;  /* 0 - 'pmsl - no special treatment' ; 1-'vmag'; 2='zeta' */

    }



/* clhilo opts */
    if((dogr || docl) && i == 3) {
	cmaxmin=argv[i][0];
    } else if(docl && i == 4) {
	radinf=atof(argv[i]);
    } else if(docl && i == 5) {
	cintinf=atof(argv[i]);
    } else if(docl && i == 6) {
	latc=atof(argv[i]);
    } else if(docl && i == 7) {
	lonc=atof(argv[i]);
    }

/* grhilo opts */
    if(dogr && i == 4){
	sortby=argv[i][0];
    } else if(dogr && i == 5) {
	pcntile=atof(argv[i]);
    } else if(dogr && i == 6) {
	latc=atof(argv[i]);
    } else if(dogr && i == 7) {
	lonc=atof(argv[i]);
    } else if(dogr && i == 8) {
	radinf=atof(argv[i]);
    } else if(dogr && i == 9) {
	nsigdig=atoi(argv[i]);
    }


/* tmhilo opts */
    if(dotm && i == 3) {
	latc=atof(argv[i]);
    } else if (dotm && i == 4) {
	lonc=atof(argv[i]);
    } else if (dotm && i == 5) {
	radinf=atof(argv[i]);
    } else if (dotm && i == 6) {
	ictype=atoi(argv[i]);
    }

}

  if( (sortby != 'm' && sortby != 'g' && sortby != 'l' && sortby != 'd' && sortby != 't' && sortby != 'n') && (dogr || docl) ) {
      printf("EEE invalid sortby: %c\n sortby= 'm' (mag or value of extrema) | 'g' (gradient) | 'l' (laplacian) | 'd' (distance [deg]) |'t' (LAtITUDE)  |'t' (LOnGITUDE)\n",sortby);
      return 1;
  }
  
  if(
     (	
      (cmaxmin != 'h') && (cmaxmin != 'l') && (cmaxmin != 'a') &&
      (cmaxmin != 'H') && (cmaxmin != 'L') && (cmaxmin != 'A') && 
      (cmaxmin != 'b') && (cmaxmin != 'B')
      ) && (dogr || docl)
     )
    {
      printf("EEE invalid cmaxmin: %c \n cmaxmin= 'l|L' (min/lows only) | 'h|H' (max/highs only) | 'a|A|b|B' (both/all max/min\n",cmaxmin);
      return 1;
    }
  
  maxmin=0;
  if(cmaxmin == 'h' || cmaxmin == 'H') maxmin=1;
  if(cmaxmin == 'l' || cmaxmin == 'L') maxmin=-1;



  pst = getpst(pcm);
  if (pst==NULL) return(1);

  rc = gaexpr(argv[1],pst);
  if (rc)  return (rc);

  pgr  = pst->result.pgr;
  val  = pgr->grid;

  undef = pgr->undef;
  itype = pgr->idim;
  jtype = pgr->jdim;
  im    = pgr->isiz;
  jm    = pgr->jsiz;
  ilinr = pgr->ilinr;
  jlinr = pgr->jlinr;

  conv  = pgr->igrab;
  xbegi = conv(pgr->ivals,pgr->dimmin[pgr->idim]);
  dxi   = *(pgr->ivals);

  conv  = pgr->jgrab;
  ybegi = conv(pgr->jvals,pgr->dimmin[pgr->jdim]);
  dyi   = *(pgr->jvals);

  rclatlon = get_latlons ( &lon, &lat, pgr, argv[0] );

/*-------------- get the grid center ---------- */

  if(latc == -999.0 && lonc == -999.0) {
    lonc=0.0;
    for(i=0;i<im;i++) lonc=lonc+lon[i];
    if(im>0) lonc=lonc/im;

    latc=0.0;
    for(j=0;j<jm;j++)  latc=latc+lat[j];
    if(jm>0) latc=latc/jm;
  }

/* -- run the fortran code -- */

  if(docl) 
    ftnclhilo_ ( val, &undef, lon, lat, &im, &jm, &maxmin, &radinf, &cintinf, 
                 &latc, &lonc, (gaData_t*)opoints, &npoints, &nv, &nrr, &rc);


  if(dogr) {
    ftngrhilo_ ( val, &undef, lon, lat, &im, &jm, &maxmin, &pcntile, &sortby, &nsigdig, 
//               &latc, &lonc, &radinf, (gaData_t*)opoints, &npoints, &nv, &nrr, &rc);
    &latc, &lonc, &radinf, opointsftn, &npoints, &nv, &nrr, &rc);
  
  }

  if(dotm) {

    guesslon=lonc;
    guesslat=latc;
   
    ftntmhilo_ ( &guesslon, &guesslat, &ictype,
		 val, &undef, lon, lat, &im, &jm,
		 &itype, &jtype, &ilinr, &jlinr,
		 &xbegi, &dxi, &ybegi, &dyi,
		 &maxmin, &resopt, &radinf,
		 (gaData_t*)opoints, &npoints, &nv,
		 &rc);
    
  }
  
  if ( rc >= 1) {
    sprintf(pout,"Error from %s: rc = %d\n", name, rc );
    gaprnt (0,pout);
    if(rc == 1) {
      sprintf(pout,"too many H/L points, max is: %d, try reducing lat/lon bounds or smooth the field\n", npoints);
      gaprnt (0,pout);
    }
    
    free(lon);
    free(lat);
    
    return(rc);

  } 


  if(docl || dogr) {

      if(docl)
	  sprintf(pout,"%s CL method N: %d  maxmin: %d   radinf: %7.1f [nm]   cintinf: %7.1f  latc: %6.2f lonc: %7.2f\n",argv[0],nrr,maxmin,radinf,cintinf,latc,lonc);
      if(dogr)
	  sprintf(pout,"%s GR method N: %d  maxmin: %d  pcntile: %6.0f  latc: %6.2f lonc: %7.2f radinfc: %7.0f [nm] nsigdig: %d\n",argv[0],nrr,maxmin,pcntile,latc,lonc,radinf,nsigdig);
      
      gaprnt (0,pout);


      for (n=0; n<nrr ; n++) {
	  
	  if(opoints[n][0] > 0.0) {
	      if(docl) {
		  sprintf(pout,"H  %7.2f %7.2f  M: %10.6g  D: % 7.2f\n",opoints[n][2],opoints[n][3],opoints[n][1],opoints[n][4]);
	      } else if(dogr) {
		  sprintf(pout,"H  %7.2f %7.2f  M: %10.6g  G: % 6.1f  L: % 6.1f  D: % 7.2f  GV: %10.6g  LV: %10.6g \n",
			  opoints[n][2],opoints[n][3],opoints[n][1],opoints[n][4],
			  opoints[n][5],opoints[n][6],
			  opoints[n][7],opoints[n][8]);
	      }
	      gaprnt (0,pout);
	  } else {
	      if(docl) {
		  sprintf(pout,"L  %7.2f %7.2f  M: %10.6g  D: % 7.2f\n",opoints[n][2],opoints[n][3],opoints[n][1],opoints[n][4]);
	      } else if(dogr) {
		  sprintf(pout,"L  %7.2f %7.2f  M: %10.6g  G: % 6.1f  L: % 6.1f  D: % 7.2f  GV: %10.6g  LV: %10.6g \n",
			  opoints[n][2],opoints[n][3],opoints[n][1],opoints[n][4],
			  opoints[n][5],opoints[n][6],
			  opoints[n][7],opoints[n][8]);
	      }
	      gaprnt (0,pout);
	  }
      }

  }


  if(dotm) {

      sprintf(pout,"%s TM method for expr: %s  Radinf: %5.0f [nm] ictype: %i (0=> cparm=''nothing special', 1=> cparm='vmag'; 2=> cparm='zeta'\n",argv[0],argv[1],radinf,ictype);
      gaprnt (0,pout);

      sprintf(pout,"GuessLon: %7.2f  GuessLat: %7.2f\n",guesslon,guesslat);
      gaprnt (0,pout);

      sprintf(pout,"FinalLon: %7.2f  FinalLat: %7.2f  FinalVal: %10.6g  FinalValBesselIntrp: %10.6g \n",opoints[0][0],opoints[0][1],opoints[0][2],opoints[0][3]);
      gaprnt (0,pout);

  }

  free(lon);
  free(lat);

  return(rc);

}

void ftnxy2rt_ ( gaData_t fld[], int *im, int *jm, gaData_t *undef,
	         gaData_t xylat[], gaData_t xylon[],
		 gaData_t *clat, gaData_t *clon,
		 gaData_t *drad, gaData_t *dphi, gaData_t *rmax,
		 gaData_t *opoints, int *nr, int *nv,   
                 char chrc[], int *rc );

/* ---------------------------------------------------------------------- */

int f_xy2rt (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  gaData_t *val, *dum, undef;
  gaData_t *xylon, *xylat;
  gaData_t clat=-999.0,clon=-999.0;

  char *chrc;
  int nich=120,njch=6;
  chrc = (char *) malloc(nich*njch);
  //if(chrc == NULL) goto merr;

  int nr=2;
  int nv=12;

  gaData_t opoints[nr][nv];


  int rc, i, j, n, im, jm;

  int verb=0;

  // defaults

  gaData_t drad=15.0, dphi=15.0, radmax=1000.0;

  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  xy2rt(expr,clat,clon,[drad(nm),dphi(deg),radmax(nm)])\n\n");
    gaprnt(0,"cartesian (xy) -> cylindical (radius,theta) \n");
    gaprnt(0,"Examples:\n");
    sprintf(pout,"   ga-> d xy2rt(psl,19.0,86.4)  (drad: %f  dphi: %f radmax: %f)\n\n",drad,dphi,radmax);
    gaprnt(0,pout);
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */

  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  for (i=0; i<pfc->argnum; i++) {

    if(i == 1) {
      clat=atof(pfc->argpnt[i]);

    } else if(i == 2) {
      clon=atof(pfc->argpnt[i]);

    } else if(i == 3) {
      drad=atof(pfc->argpnt[i]);

    } else if(i == 4) {
      dphi=atof(pfc->argpnt[i]);

    } else if(i == 5) {
      radmax=atof(pfc->argpnt[i]);

    } else if(i == 6) {

    }


  }


  /* gridded data */

  if (pst->type==1) { 

    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;
    undef = pgr->undef;
	
    if ( im == 1 || jm == 1) {
      sprintf(pout,"Error from %s: grid must be 2D im: %d jm %d\n", name,im,jm);
      gaprnt (0,pout);
      return(1);
    }

    if ( im <= 3 || jm <= 3) {
      sprintf(pout,"Error from %s: grid must be 2D  and at least 3x3 im: %d jm %d\n", name,im,jm);
      gaprnt (0,pout);
      return(1);
    }

    rc = get_latlons ( &xylon, &xylat, pgr,  name );
    
    if (clon == -999.0 && clat == -999.0 ) {
	clon=0.0;
	for(i=0;i<im;i++) clon=clon+xylon[i]; clon=clon/(gaData_t)im;
	
	clat=0.0;
	for(j=0;j<jm;j++) clat=clat+xylat[j]; clat=clat/(gaData_t)jm;
	
    }


    ftnxy2rt_ ( val, &im, &jm, &undef,
       xylat, xylon,
       &clat, &clon,
       &drad, &dphi, &radmax,
       (gaData_t *)opoints, &nr, &nv,
       chrc, &rc );

    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    } else  
#if GEX_GAMAJOR == 2
      rc = gex_setUndef(pst) ;
#endif
    return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

}




void ftnsmth2d_ ( gaData_t fld[], gaData_t *undef, int *im, int *jm, 
                  int *npass, int *nnu, gaData_t anu[], gaData_t dum[], 
                  int *rc );

/* ---------------------------------------------------------------------- */

int f_smth2d (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  gaData_t *val, *dum, undef;
  int rc, i, j, n, im, jm;

  int verb=0;
  int npass=1;     
  int numax=4;
  int nnu=2;
  gaData_t anu[nnu];
  char *name = pfc->argpnt[pfc->argnum];

  anu[0]=0.5;
  anu[1]=-0.5;

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  smth2d(expr,[npass,nu1,nu2,...,nuN])\n\n");
    gaprnt(0,"This function smooths a 2D field according to the Shuman (1957, MWR 357-361)\n");
    gaprnt(0,"smoother-desmoother; defaults npass=1 nu1=0.5 nu2=-0.5 (light) \n");
    gaprnt(0,"Examples:\n");
    gaprnt(0,"   ga-> d smth2d(psl,3,0.5)    (3 passes, heavy filter)\n\n");
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */

  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  for (i=0; i<pfc->argnum; i++) {

    if(i == 1) {

	npass = atoi(pfc->argpnt[i]); 

/* get the  nus */

    } else if(i > 1 && (i-1) <= numax ) {
	if(i == 2) nnu=0;
	anu[nnu]=atof(pfc->argpnt[i]);
	nnu++;
    } else if(i>numax) {
	sprintf(pout,"Error from %s: nnu > numax = %d\n",name,i);
	gaprnt (0,pout);
	return (1);

    }

  }

  if(verb) {
      printf("NNNNNNNNNNN npass: %d  nnu: %d\n",npass,nnu);
      for(n=0;n<nnu;n++) printf("NNN %d %5.2f\n",n,anu[n]);
  }


/* gridded data */
    if (pst->type==1) { 
			   
	pgr  = pst->result.pgr;
	val  = pgr->grid;
	im   = pgr->isiz;
	jm   = pgr->jsiz;
	undef = pgr->undef;
	
	dum = (gaData_t *) malloc ( sizeof(gaData_t)*(im*jm)) ;
	
	if ( im == 1 || jm == 1) {
	    sprintf(pout,"Error from %s: grid must be 2D im: %d jm %d\n", name,im,jm);
	    gaprnt (0,pout);
	    return(1);
	}

	if ( im <= 3 || jm <= 3) {
	    sprintf(pout,"Error from %s: grid must be 2D  and at least 3x3 im: %d jm %d\n", name,im,jm);
	    gaprnt (0,pout);
	    return(1);
	}


	// Run shuman smoother-desmoother 

	ftnsmth2d_ ( val,  &undef, &im, &jm, &npass, &nnu, anu, dum, &rc );

		       
	if ( rc ) {
	    sprintf(pout,"Error from %s: rc = %d\n", name, rc );
	    gaprnt (0,pout);
	} else  
#if GEX_GAMAJOR == 2
           rc = gex_setUndef(pst) ;
#endif
	return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }
  free(dum);

}


/* ---------------------------------------------------------------------- 

udf to convert u,v winds in cartesian coords to 
    tangential/radial (t,r) winds in cylindrical coordinates

    where the center is lonc, latc

example: 

    lonc=245.0
    latc=9.2

    vt=uv2trw(ua,va,1,lonc,latc)
    ur=uv2trw(ua,va,-1,lonc,latc)


-------------------------------------------------------------------------*/


void ftnuv2trw_ ( gaData_t ua[], gaData_t va[], gaData_t lon[], gaData_t lat[], gaData_t dum [], gaData_t *undef, 
		   int *im, int *jm, gaData_t *centlat, gaData_t *centlon, 
		   int *iout, int *rc );

int f_uv2trw (struct gafunc *pfc, struct gastat *pst) {

    struct gagrid *pgr;
    gaData_t *u, *v, *lon, *lat, *dum, undef;
    int iout;
    int rc, i, j, n, im, jm, imu, jmu, imv, jmv;
    char *name = pfc->argpnt[pfc->argnum];

    gaData_t centlon,centlat;

    if (pfc->argnum < 2) {
	sprintf(pout,"\nError from %s: Too few args \n\n", name);
	gaprnt(0,pout);
	gaprnt(0,"          Usage:  uv2trw(u,v,latc,lonc,[iout]))\n\n");
	gaprnt(0,"returns the tangential velocity (iout=1; the default) relative to a fixed lonc/latc\n");
	gaprnt(0,"if no lonc/latc is provided, then the center of the grid is assumed the fixed point\n\n");
	gaprnt(0,"Examples:\n\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,3)     (return the xy vcomp tangential wind\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,2)     (return the xy ucomp tangential wind\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,1)     (return the total tangential wind\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,-1)    (return the total radial wind\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,-2)    (return the xy ucomp radial wind\n");
	gaprnt(0,"   ga-> d uv2trw(ua,va,latc,lonc,-3)    (return the xy vcomp radial wind\n\n");
	return(1);
    }
    
  /* evaluate expression: should be vorticity or divergence */

    rc = gaexpr(pfc->argpnt[0],pst);
    if (rc) return (rc);

/* defaults */

    iout=1;

/* 1st grid */
    if (pst->type==1) { 
			   
	pgr   = pst->result.pgr;
	u     = pgr->grid;
	imu   = pgr->isiz;
	jmu   = pgr->jsiz;
	undef = pgr->undef;
	
	if ( imu == 1 || jmu == 1) {
	    sprintf(pout,"Error from %s: grid must be 2D imu: %d jmu: %d\n", name,imu,jmu);
	    gaprnt (0,pout);
	    return(1);
	}

	if ( imu <= 3 || jmu <= 3) {
	    sprintf(pout,"Error from %s: grid must be 2D  and at least 3x3 imu: %d jmu: %d\n", name,imu,jmu);
	    gaprnt (0,pout);
	    return(1);
	}


  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

/* 2nd grid */

    rc = gaexpr(pfc->argpnt[1],pst);
    if (rc) return (rc);


    if (pst->type==1) { 
			   
	pgr   = pst->result.pgr;
	v     = pgr->grid;
	imv   = pgr->isiz;
	jmv   = pgr->jsiz;

	undef = pgr->undef;

	if ( imv == 1 || jmv == 1) {
	    sprintf(pout,"Error from %s: grid must be 2D imv: %d jmv %d\n", name,imv,jmv);
	    gaprnt (0,pout);
	    return(1);
	}

	if ( imv <= 3 || jmv <= 3) {
	    sprintf(pout,"Error from %s: grid must be 2D  and at least 3x3 imv: %d jmv: %d\n", name,imv,jmv);
	    gaprnt (0,pout);
	    return(1);
	}


  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

  if(imu != imv || jmu != jmu) {
      sprintf(pout,"Error from %s: grid must be 2D the u and v lon/at dimension must be the same... %d\n", name);
      gaprnt (0,pout);
      return(1);
  } else {
      im=imu;
      jm=jmu;
  }

    if (pfc->argnum >= 2) {
	centlat=atof(pfc->argpnt[2]);
    } 

    if (pfc->argnum >= 3) {
	centlon=atof(pfc->argpnt[3]);
    } 

    if (pfc->argnum >= 4) {
	iout=atoi(pfc->argpnt[4]);
    } 


    dum = (gaData_t *) malloc ( sizeof(gaData_t)*(im*jm)) ;
	
    rc = get_latlons ( &lon, &lat, pgr,  name );

    if (pfc->argnum <= 3) {
	centlon=0.0;
	for(i=0;i<im;i++) centlon=centlon+lon[i]; centlon=centlon/(gaData_t)im;
	
	centlat=0.0;
	for(j=0;j<jm;j++) centlat=centlat+lat[j]; centlat=centlat/(gaData_t)jm;
	
    }


    ftnuv2trw_ ( u, v, lat, lon, dum, &undef,
		  &im, &jm, &centlat, &centlon,
		  &iout, &rc );
		       
    pgr->grid=u;

    if ( rc ) {
	sprintf(pout,"Error from %s: rc = %d\n", name, rc );
	gaprnt (0,pout);
	}
#if GEX_GAMAJOR == 2
    rc = gex_setUndef(pst) ;
#endif
    return(rc);

  free(dum);

}


/* .................................................................. */

int udf_get_ivals (char *name, struct gagrid  *pgr, gaData_t *ivals, gaData_t *xin, gaData_t *yin) {

  struct dt dtim;
  gaData_t (*conv) (gaData_t *, gaData_t);
  int rc,iarg,siz,i,ii,j;

  /* Fill in header */
  
  ivals[0] = pgr->undef;
  ivals[1] = pgr->idim;
  ivals[2] = pgr->jdim;
  ivals[3] = pgr->isiz;
  ivals[4] = pgr->jsiz;
  ivals[5] = pgr->ilinr;
  ivals[6] = pgr->jlinr;

  if (pgr->idim>-1 && pgr->ilinr==1) {     /* Linear scaling info */
      conv = pgr->igrab;
      ivals[7] = conv(pgr->ivals,pgr->dimmin[pgr->idim]);
      ivals[8] = *(pgr->ivals);
   }
  if (pgr->jdim>-1 && pgr->jlinr==1) {
      conv = pgr->jgrab;
      ivals[9] = conv(pgr->jvals,pgr->dimmin[pgr->jdim]);
      ivals[10] = *(pgr->jvals);
  }
  
  if (pgr->idim>-1) {                  /* write i dim scaling */
    
    if (pgr->idim<3) {
      conv = pgr->igrab;
      for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
	*xin = conv(pgr->ivals,(gaData_t)i);
	xin++;
      }
    } else {
      for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
	*xin = (gaData_t)i;
	xin++;
      }
    }
  }
  
  if (pgr->jdim>-1) {                  /* write j dim scaling */
    if (pgr->jdim<3) {
      conv = pgr->jgrab;
      for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
	*yin = conv(pgr->jvals,(gaData_t)i);
	yin++;
      }
    } else {
	for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
	*yin = (gaData_t)i;
	yin++;
      }
    }
  }

  return(0);

}


//------------------------------------------------------------------------------


int udf_put_ovals (char *name, struct gastat *pst, gaData_t fldout[], gaData_t *rvals, gaData_t xout[], gaData_t yout[]) {

  struct gagrid *pgr;
  int i, j;
  gaData_t *v;

  /* from gafunc.c */

  pgr = (struct gagrid *)malloc(sizeof(struct gagrid));
  if (pgr==NULL) goto merr;

  /* Fill in and check values */
  pgr->alocf = 0;
  pgr->undef = rvals[0];
  pgr->idim = (int)(floor(rvals[1]+0.1));
  pgr->jdim = (int)(floor(rvals[2]+0.1));
  pgr->iwrld = 0; pgr->jwrld = 0;
  pgr->isiz = (int)(rvals[3]+0.1);
  pgr->jsiz = (int)(rvals[4]+0.1);
  pgr->ilinr = (int)(rvals[5]+0.1);
  pgr->jlinr = (int)(rvals[6]+0.1);

  /* For now, only lat/lon grids */
  if ( pgr->idim!=0 || pgr->jdim!=1 ) {
    free(pgr);
    return (1);
  }

  for (i=0; i<5; i++) pgr->dimmin[i] = 1;
  for (i=0; i<5; i++) pgr->dimmax[i] = 1;
  pgr->dimmax[pgr->idim] = pgr->isiz;
  pgr->dimmax[pgr->jdim] = pgr->jsiz;

  /* Set up linear scaling info */

  if (pgr->idim>-1 && pgr->ilinr==1) {     /* Linear scaling info */
      v = (gaData_t *)malloc(sizeof(gaData_t)*6);
      if (v==NULL) goto merr;
      *v = rvals[8];
      *(v+1) = rvals[7]-rvals[8];
      *(v+2) = -999.9;
      pgr->ivals = v;
      *(v+3) = 1.0 / rvals[8];
      *(v+4) = -1.0 * (rvals[7]-rvals[8]) / rvals[8];
      *(v+5) = -999.9;
      // printf("iiii uniform:  %f %f \n",rvals[7],rvals[8]);
      pgr->iavals = v+3;
      pgr->iabgr = liconv;
      pgr->igrab = liconv;
  }
  if (pgr->jdim>-1 && pgr->jlinr==1) {     /* Linear scaling info */
      v = (gaData_t *)malloc(sizeof(gaData_t)*6);
      if (v==NULL) goto merr;
      *v = rvals[10];
      *(v+1) = rvals[9]-rvals[10];
      *(v+2) = -999.9;
      // printf("jjjj uniform:  %f %f \n",rvals[9],rvals[10]);
      pgr->jvals = v;
      *(v+3) = 1.0 / rvals[10];
      *(v+4) = -1.0 * (rvals[9]-rvals[10]) / rvals[10];
      *(v+5) = -999.9;
      pgr->javals = v+3;
      pgr->jabgr = liconv;
      pgr->jgrab = liconv;
  }

  /* load in the data */
  pgr->grid = fldout;

#if GEX_GAMAJOR == 2
  pgr->umask = (char *) malloc(pgr->isiz * pgr->jsiz);
#endif

  /* Read in non-linear scaling info, if any */
  if (pgr->idim>-1 && pgr->ilinr==0) {
    v = (gaData_t *)malloc(sizeof(gaData_t)*(pgr->isiz+2));
    if (v==NULL) {
      free(pgr->grid);
      free(pgr);
      goto merr;
    }

    *v = pgr->isiz;

    for(i=0; i<pgr->isiz; i++) {
      *(v+i+1)=xout[i];
    }

    *(v+pgr->isiz+1) = -999.9;
    pgr->ivals = v;
    pgr->iavals = v;
    pgr->iabgr = lev2gr;
    pgr->igrab = gr2lev;
  }

  if (pgr->jdim>-1 && pgr->jlinr==0) {
    v = (gaData_t *)malloc(sizeof(gaData_t)*(pgr->jsiz+2));
    if (v==NULL) {
      free(pgr->grid);
      free(pgr);
      goto merr;
    }

    *v = pgr->jsiz;

    for(j=0;j<pgr->jsiz;j++) {
      *(v+j+1)=yout[j];
     } 
    *(v+pgr->jsiz+1) = -999.9;

    pgr->jvals = v;
    pgr->javals = v;
    pgr->jabgr = lev2gr;
    pgr->jgrab = gr2lev;

  }

  /* We are done.  Return.  */

  pst->result.pgr = pgr;
  pst->type = 1;

  return (0);

merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n",name);
  gaprnt (0,pout);
  return (1);

ferr:
derr:
  sprintf (pout,"Error from %s: Invalid dimension environment ",name);
  gaprnt (0,pout);
  gaprnt (0,"in result grid\n");
  return (1);

}


//------------------------------------------------------------------------------

void ftnre2_ ( gaData_t fldin[],  gaData_t xin[], gaData_t yin[], int *nii, int *nji,
	       gaData_t ivals[], int *nargs, gaData_t args[], char card[],
	       gaData_t fldout[],  gaData_t xout[], gaData_t yout[],
	       int *nio, int *njo, int *njog,
	       gaData_t ovals[], int *iflag,
	       char chrc[], int *rc );

int f_re2 (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  gaData_t  *dum, undef;
  struct dt dtim;

  gaData_t *fldin,ivals[20],*xin,*yin;
  gaData_t fldoutdum[1],ovalsdum[20],xoutdum[2],youtdum[2];
  gaData_t *fldout,ovals[20],*xout,*yout;
  int nii,nji,nio,njo,njog,iflag;

  gaData_t args[10],*v;
  int rc,iarg,siz;
  char *ch,*copt;
  char incard[80];
  char *chrc;
  int rdw;
  int  i,ii,j, n, im, jm,nargs,nargsdum,rtype;



  chrc = (char *) malloc(720);
  if(chrc == NULL) goto merr;

  char *name = pfc->argpnt[pfc->argnum];
  
  int verb=0;

  nargs = pfc->argnum;
  nargsdum = nargs;

  ii=0;
  for (i=0; i<nargs; i++) {
    if(i != 0 && i != 3) {
      args[ii]=atof(pfc->argpnt[i]);
      ii++;
    }
    if(verb) {
	printf("arg: %d type: %s  func: %s\n",i,pfc->argpnt[i],name);
    }
  }

  if(nargs >= 4)  {
      for (i=0; i<80; i++) incard[i] = ' ';
      strcpy(incard,pfc->argpnt[3]);
  }


  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  re2(expr,[npass,nu1,nu2,...,nuN])\n\n");
    gaprnt(0,"Examples:\n");
    gaprnt(0,"   ga-> d re2(psl,1,1,bs) \n\n");
    return(1);
  }

  /* evaluate input expression */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  /* Can only handle gridded data */
  if (pst->type==1) { 

    /* Allocate memory for the input to the Fortran routine */
    pgr = pst->result.pgr;
    xin = (gaData_t *) malloc(sizeof(gaData_t)*pgr->isiz);
    if(xin == NULL) goto merr;
    yin = (gaData_t *) malloc(sizeof(gaData_t)*pgr->jsiz);
    if(yin == NULL) goto merr;

    /* Get dimension information on a flat 1D array just like
       the old, file-based, classic UDFs */

    rc = udf_get_ivals (name, pgr, ivals, xin, yin);

    /* get input grid dims and pass grid and dim ftnre2 */
    nii   = pgr->isiz;
    nji   = pgr->jsiz;
    fldin = pgr->grid;
    iflag = -999;
    nio = njo = njog= 1;

    /* Call the fortran to get dims of the output arrays */
    for (i=0; i<720; i++) chrc[i]=' ';

    ftnre2_ (fldin,  xin, yin, &nii, &nji, 
	     ivals, &nargs, args, incard,
	     fldoutdum, xoutdum, youtdum, &nio,&njo,&njog,
	     ovals,&iflag,
	     chrc, &rc );

    //printf("QQQQQ %d %d %d %d\n",nio,njo,njog,iflag);

    /* Allocate the required memory for output */

    xout = (gaData_t *) malloc(sizeof(gaData_t)*(nio+1));
    if(xout == NULL) goto merr;
      
    if(njog == 1) {
       yout = (gaData_t *) malloc(sizeof(gaData_t)*(njo+1));
       if(yout == NULL) goto merr;
       fldout= (gaData_t *) malloc(sizeof(gaData_t)*(nio*njo));
       if(fldout == NULL) goto merr;
    } else {
      yout = (gaData_t *) malloc(sizeof(gaData_t)*(njog+1));
      if(yout == NULL) goto merr;
      fldout= (gaData_t *) malloc(sizeof(gaData_t)*(nio*njog));
      if(fldout == NULL) goto merr;
    }

    if(yout == NULL) goto merr;
    if(fldout == NULL) goto merr;

    /* Now that all memory is allocated, call the workhorse
       Fortran routine */

    iflag=1;
    for (i=0; i<720; i++) chrc[i]=' ';
    ftnre2_ (fldin,  xin, yin, &nii, &nji,
	     ivals, &nargs, args, incard,
	     fldout, xout, yout, &nio,&njo,&njog,
	     ovals,&iflag,
	     chrc, &rc );

    if ( rc ) {
      sprintf(pout,"Error from %s calling the fortan: rc = %d\n", name, rc );
      gaprnt (0,pout);
return(1);
    }

    for (i=0; i<120; i++) chcard[i]=' '; 
    strncpy(chcard,chrc,110);  
    sprintf(pout,"%s\n",chcard);  
    gaprnt (0,pout); 
    chrc+=120 ;

    for (i=0; i<120; i++) chcard[i]=' '; strncpy(chcard,chrc,110);  sprintf(pout,"%s\n",chcard);  gaprnt (0,pout); chrc+=120 ;
    for (i=0; i<120; i++) chcard[i]=' '; strncpy(chcard,chrc,110);  sprintf(pout,"%s\n",chcard);  gaprnt (0,pout); chrc+=120 ;
    for (i=0; i<120; i++) chcard[i]=' '; strncpy(chcard,chrc,110);  sprintf(pout,"%s\n",chcard);  gaprnt (0,pout); chrc+=120 ;
    for (i=0; i<120; i++) chcard[i]=' '; strncpy(chcard,chrc,110);  sprintf(pout,"%s\n",chcard);  gaprnt (0,pout); chrc+=120 ;
    for (i=0; i<120; i++) chcard[i]=' '; strncpy(chcard,chrc,110);  sprintf(pout,"%s\n",chcard);  gaprnt (0,pout); 


    /* fill the output gagrid struct and put on the gastat pst struc */
    rc = udf_put_ovals (name, pst, fldout, ovals, xout, yout);

    if ( rc ) {
      sprintf(pout,"Error from %s loading the output grid: rc = %d\n", name, rc );
      gaprnt (0,pout);
    }

    /* Fill undef mask */

#if GEX_GAMAJOR == 2
    rc = gex_setUndef(pst) ;
#endif
    free(xin);
    free(yin);

    chrc=0;    
    free(chrc);
	    
    return(rc);

  /* Cannot handle station data */
  } else { 
    
    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);
    
  }

merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n",name);
  gaprnt (0,pout);
  return (1);

}

/* .................................................................. */

void ftnesmrf_ ( gaData_t fld[], gaData_t *undef, int *nij, 
                  int *counter, char *pout, int *rc );


int f_esmrf (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  gaData_t *val, *dum, undef;
  int rc, i, j, n, im, jm, nij;
  char *ddir;

  int verb=0;

  char *name = pfc->argpnt[pfc->argnum];

  counter++;

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  esmrf(expr)\n\n");
    gaprnt(0,"This function calculates saturation vapor pressure in Pa using\n");
    gaprnt(0,"a lookup table from the old NCEP MRF version of the GFS\n");
    gaprnt(0,"Examples:\n");
    gaprnt(0,"   ga-> d esmrf(ta)   (from temperature in the variable ta)\n\n");
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */

  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

/* gridded data */

    if (pst->type==1) { 
			   
	pgr  = pst->result.pgr;
	val  = pgr->grid;
	im   = pgr->isiz;
	jm   = pgr->jsiz;
	undef = pgr->undef;
	
	nij=im*jm;

	rc=0;

	ftnesmrf_ ( val, &undef, &nij, &counter, pout, &rc );
		       
	if ( rc ) {
	    sprintf(pout,"Error from %s: rc = %d\n", name, rc );
	    gaprnt (0,pout);
	}

	return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

}

/* .................................................................. */

void ftnlinreg_ ( gaData_t x[], gaData_t y[], gaData_t *undef, int *nij, char card[], int *rc );

int f_linreg (struct gafunc *pfc, struct gastat *pst ) {

  struct gagrid *pgr;
  gaData_t *x, *y, *lat, *lon, undef;
  int rc, i, j, n, isiz, jsiz, ilin, jlin, idim, jdim, nij;
  gaData_t (*conv) (gaData_t *, gaData_t);
   
  char *name = pfc->argpnt[pfc->argnum];

  int verb=0;

  counter++;

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  linreg(expr)\n\n");
    gaprnt(0,"This function calculates mx +b  linear fit\n");
    gaprnt(0,"Examples:\n");
    gaprnt(0,"   ga-> d linreg(ta)   (from temperature in the variable ta)\n\n");
    return(1);
  }

  /* evaluate expression */

  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

/* gridded data */
  
  if (pst->type==1) { 
			   
    pgr = pst->result.pgr;
    y     = pgr->grid;
    isiz  = pgr->isiz;
    jsiz  = pgr->jsiz;
    ilin  = pgr->ilinr;
    jlin  = pgr->jlinr;
    idim  = pgr->idim;
    jdim  = pgr->jdim;

    
    undef = pgr->undef;

    if(isiz == 1 && jsiz == 1 ) {
      sprintf(pout,"single value idim: %d  jdim: %d:  in: %s\n",idim,jdim,name) ;
      gaprnt (0,pout);
      return (1);

    } else if(isiz == 1 || jsiz ==1 ) {
      nij=isiz*jsiz;

    } else {
      sprintf(pout,"grid expression must be 1-D in: %s\n", 
	      name);
      gaprnt (0,pout);
      return (1);
    }

    if(isiz == 1) {

      j = 0;
      x = (gaData_t *) malloc ( sizeof(gaData_t)*jsiz );
      if ( x ) {
	conv = pgr->jgrab;
	for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
/* --- special treatment for time; use index */
	    if(idim == 3) {
		x[j++] = (gaData_t) j ;
	    } else {
		x[j++] = conv(pgr->jvals,(gaData_t) i );
	    }
	}
      }
      else {
	sprintf(pout,"Error from %s: out of memory (i direction)\n", name);
	gaprnt (0,pout);
	free (lon);
	return (1);
      }

    } else {

      j = 0;
      x = (gaData_t *) malloc ( sizeof(gaData_t)*isiz );
      if ( x ) {
	conv = pgr->igrab;
	for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
/* --- special treatment for time; use index */
	    if(idim == 3) {
		x[j++] = (gaData_t) j ;
	    } else {
		x[j++] = conv(pgr->ivals,(gaData_t) i );
	    }
	}
      } 
      else {
	sprintf(pout,"Error from %s: out of memory (j direction)\n", name);
	gaprnt (0,pout);
	return (1);
      }

    }


    rc=1;
 
    for(i=0;i<120;i++) card[i]=' ';
    ftnlinreg_ (x, y, &undef, &nij, card, &rc);

    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    } else {
	strncpy(chcard,card,119);
	sprintf(pout,"%s\n",chcard);
	gaprnt (0,pout);
    }
#if GEX_GAMAJOR == 2
    rc = gex_setUndef(pst) ;
#endif
    return(rc);
    
  } else {  /* station data */
    
    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

}

/* .................................................................. */

 static
 int get_latlons ( gaData_t **lons, gaData_t **lats, 
                         struct gagrid *pgr, char *name  ) {

   int i, j, im, jm;
   gaData_t dlon, dlat, *lon, *lat;
   gaData_t (*conv) (gaData_t *, gaData_t);

   /* varying dimensions must be lon and lat for spherepak */
   if ( pgr->idim != 0 || pgr->jdim != 1 ) {
     sprintf(pout,"Error from %s: input must be lat/lon grid \n", name);
     gaprnt (0,pout);
     return (1);
   }

   /* Longitudes */
   im   = pgr->isiz;
   j = 0;
   lon = (gaData_t *) malloc ( sizeof(gaData_t)*im );
   if ( lon ) {
     conv = pgr->igrab;
     for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) 
       lon[j++] = conv(pgr->ivals,(gaData_t) i );
   } 
   else {
     sprintf(pout,"Error from %s: out of memory (lon)\n", name);
     gaprnt (0,pout);
     return (1);
   }
  
   /* latitudes */
   j = 0;
   jm   = pgr->jsiz;
   lat = (gaData_t *) malloc ( sizeof(gaData_t)*jm );
   if ( lat ) {
     conv = pgr->jgrab;
     for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) 
       lat[j++] = conv(pgr->jvals,(gaData_t) i );
   }
   else {
     sprintf(pout,"Error from %s: out of memory (lat)\n", name);
     gaprnt (0,pout);
     free (lon);
     return (1);
   }

#if 0
   printf("\n%s:\n ",name);
   printf("%s: Longitudes = ",name);
   for ( i=0; i<im; i++ ) printf("%g ",lon[i]);
   printf("\n%s:\n ",name);
   printf("%s: Latitudes = ",name);
   for ( j=0; j<jm; j++ ) printf("%g ",lat[j]);
   printf("\n%s:\n ",name);
#endif

   *lons = lon;
   *lats = lat;

   return (0);

 }


/*

                         -----------------
                         POD Documentation
                         -----------------
*/


#else

=pod

=head1 NAME

libmf.gex - Mike Fiorino`s Collection of GrADS Extensions 

=head1 SYNOPSIS

=head3 GrADS Commands

=over 4

=item

B<tcprop> I<EXPR> I<LAT> I<LON> I<RADIUS[nm]> - Properties relative to lon/lat 

=item

B<tcxy2rt> I<EXPR> I<LAT> I<LON> I<DR [nm]> I<DTHETA [deg]> I<RADMAX [nm]> - convert lat/lon grid expression from cartesian [x,y] to cylindrical [r,theta] coordinates

=item 

B<mfhilo> I<EXPR> [I<METHOD>[,I<OPTIONS>] - Find max/min or H/L in 2D field

=item 

B<grhist> I<EXPR> [I<METHOD>[,I<OPTIONS>] - histogram of grid expression

=back

=head3 GrADS Functions:

=over 4

=item

B<smth2d>(I<EXPR>[,I<N>,I<nu1>,...,I<nuN>]) -  Shuman smoother/de-smoother

=item

B<uv2trw>(I<U,V>,[I<OPT>],[I<LON,LAT>]) -  Find radial/tangential velocity

=back


=head1 DESCRIPTION 

This library implements Mike Fiorino`s collection GrADS extensions,
including his classic I<regrid2> regridding function as an extension now
called I<re2>, I<smth2d> for applying the Shuman smoother-desmother and
computing radial/tangential velocities relative to a lat/lon point.
Commands are also provided to find H/L of a 2D field and to calculate
properties of a tropical cyclone (TC).

These functions were initially developed as I<classic> User Defined
Function (UDFs) in GrADS v1.x. These extensions were later implemented as
I<dynamic> linked extensions in GrADS v1.10.


=head1 COMMANDS PROVIDED

=head2 B<tcxy2rt> I<EXPR> I<LAT> I<LON> I<DR [nm]> I<DTHETA [deg]> I<RADMAX [nm]> - convert lat/lon grid expression from cartesian [x,y] to cylindrical [r,theta] coordinates

=over 4

This transforms the I<EXPR> lat/lon grid from cartesian to cylindrical
coordinates relative to the I<LON,LAT> point, typically a tropical
cyclone.  The parameters are returned to the calling GrADS script via
script variable I<result> ONLY.

Sample output:

tcxy2rt for expr: psl  clat:   28.90  clon:  287.80  NB: OUTPUT ONLY to script var RESULT 
nr:    9  nt:    8  drad:    30 [nm] dtheta:    45  radmax:   252
   i    j    r   theta       val
   0    0     0    0    28.90  287.80       100593.36
   0    1     0   45    28.90  287.80       100593.36
      .
      .
      .
   8    6   240  270    28.90  285.33       100967.55
   8    7   240  315    30.43  286.04       101041.14




=back


=over 4

I<EXPR> - a valid GrADS expression

I<LAT> - latitude of the center

I<LON> - longitude of the center

I<DR> - delta r [nm]

I<DTHETA> - delta theta [deg] 

I<RADMAX> - maximum distance/extent [nm] of the cicle

=back


=head2 B<tcprop> I<EXPR> I<LAT> I<LON> I<RADIUS [nm]> I<BEARING [deg]> 

=over 4

This command computes the I<EXPR> properties relative to a point located
at I<LON,LAT>, typically a tropical cyclone. The parameters are returned
to the calling GrADS script via script variable I<result>.

Sample output:

=back

 tcprop for expr: psl Latc:   16.80  Lonc:  129.70  Radinf:   200 [nm] Bearing:    60  Hemi1:  60-240 Hemi2: 240-420
 MAXCntRadGrad:  1.249e-02  minCntRadGrad:  8.694e-03  MeanCntRadGrad:  1.063e-02 [units: (out-ctr)/nm]
 MeanFieldInsideRadinf:     100573 
 Hemi1:         60-240  Hemi2:        240-420
 MeanHemi1:     100572  MeanHemi2:     100573 
 QuadNE:     60-150  QuadSE:    150-240  QuadSW:    240-330  QuadNW:    330-420
 MeanNE:     100598  MeanSE:     100545  MeanSW:     100548  MeanNW:     100597 

=over 4

I<EXPR> - a valid GrADS expression

I<LON,LAT> - longitude/latitude of the center

I<RADIUS> - distance in nm away from I<LON,LAT> to calculate properties

I<BEARING> - direction (0-360 deg) to calculate mean of the field within
I<RADIUS> in four quadrants and two hemispheres, e.g., of a TC is
heading towards the West (270 deg), then C<MeanHemi1> will be the right
hemisphere (North or right of track) and C<MeanHemi2> the left (South or
left of the motion).  The quadrants are defined NE,SE,SW,NW relative to
I<BEARING> (0 deg or North)

=back


=head2 B<mfhilo> I<EXPR> [I<METHOD>[,I<OPTIONS>] 

=over 4

This command finds maximun/minimum values in a 2-D lat/lon field, returning
the location and value between grid points via the GrADS script variable 
I<result>.

=over 4

I<EXPR> is a GrADS valid expression

I<METHOD> - can be either I<CL> or I<GR>

For method B<CL> -- Contour-Level based from NCEP -- the I<OPTIONS> are

=over 4

I<MaXMin> - specify I<L> for lows only, I<H> for highs only; A|B for both (case insensitive)

I<RADIUS> - radius in nm, default is 120, bigger radius leads to finding fewer H/L

I<CINTINF> - contour interval between max and nearby point; smaller contour interval leads to finding more negative extrema.

=back

For method B<GR> -- GRid-based written by Mike Firoino -- the I<OPTIONS> are

=over 4

I<MaXMin> - specify I<L> for lows only, I<H> for highs only; A|B for both (case insensitive)

I<SORTby> - I<d> distance from either center of grid or a specified
lat/lon; I<m> order by mag/value at extrema; I<g> by gradient; I<l> by
laplacian

I<PCNTILE> - set percentile (0-100) of top N points sorted by gradient

I<LATC> - set latitude of the center point of the search; default is center of grid

I<LONC> - set longitude (deg E) of the search center point; defaul is grid center

I<RADINF> - radius from I<LATC>,I<LONC> to search; default is -999 or do not use I<RADINF>

I<NDIGSIG> - # of significant digits to determine if extrema is 'significant'; default is 6 and can only be set from 2-8, if outside this bound then precision checking is disabled...


=back

Example + output:


 vt=hcurl(u,v)*1e5

 mfhilo vt gr b d 30 16.8 129.7

output in script variable I<result>:

 mfhilo GR method N: 7  maxmin: 0  pcntile:     30  latc:  16.80 lonc:  129.70
 H    16.86  129.90  M:    20.3321  G:  100.0  L:  100.0  D:   12.22  GV:     2.2897  LV:    4.82068 
 H    17.03  127.69  M:    10.4799  G:   38.7  L:   28.8  D:  115.96  GV:    0.88574  LV:    1.38745 
 L    20.56  129.96  M:    -5.2466  G:  100.0  L:  100.0  D:  226.19  GV:    1.09352  LV:    4.55874 
 L    17.94  125.36  M:  -0.435871  G:   43.6  L:   23.0  D:  257.63  GV:    0.47636  LV:    1.04821 
 L    18.10  134.23  M: 0.00436844  G:   60.2  L:   25.6  D:  270.80  GV:   0.658458  LV:    1.16751 
 L    16.06  124.95  M:  -0.726843  G:   26.8  L:   32.2  D:  276.80  GV:   0.293564  LV:    1.46846 
 L    14.40  134.67  M:   -5.43262  G:   88.4  L:   41.8  D:  321.18  GV:   0.966466  LV:    1.90383 
 
for the first output line (in grads script: sublin(result,2)):

 H                     -- type of extrema High or Low
 16.86                 -- latitude
 129.90                -- longitude
 M:    20.3321         -- magnitude
 G:  100.0             -- percentile of gradient
 L:  100.0             -- percentile of laplacian
 D:   12.22            -- great-circle distance [nm] away from latc (16.8) and lonc (129.7)
 GV:     2.2897        -- value of gradient in grid units (dx=i+1-i = 1)
 LV:    4.82068        -- value of laplacian in grid units

=over 4

=back

For method B<TM> -- Tim Marchok, GFDL barnes-analysis scheme to locate extrema between grid points -- the I<OPTIONS> are

=over 4

I<LATC> - first-guess latitude

I<LONC> - first-guess longitude (deg E)

I<RADINF> - search radiusm [nm]

I<ICTYPE> - 0 -> cparm='nothing special'; 1 -> cparm='vmag' (special
wind speed reatment); 2 -> cparm='zeta' (special vorticity treatment)

=back

Example + Output:

 mfhilo vt tm 16.8 129.7

 mfhilo TM method for expr: vt  Radinf:   250 [nm] ictype: 0 (0=> cparm='nothing special', 1=> cparm='vmag'; 2=> cparm='zeta'
 GuessLon:  129.70  GuessLat:   16.80
 FinalLon:  129.90  FinalLat:   16.85  FinalVal:     17.665  FinalValBesselIntrp:    20.3253 



=back 

=back 

=head1 FUNCTIONS PROVIDED

=head2 B<smth2d>(I<EXPR>[,I<N>,I<nu1>,...,I<nuN>])

=over 4

This function smooths a 2D field according to the Shuman (1957, MWR 357-361)
smoother-desmoother algorithm; defaults I<N>=1 I<nu1>=0.5 I<nu2>=-0.5 (light).
For exampe,  for a 3 passes, heavy filter enter

      ga-> d smth2d(psl,3,0.5)   

=over 4

I<EXPR> is a GrADS valid expression

I<N> - number of fiter passes

I<ni1>,...I<nuN> - smoothing coefficients

=back 


=back

=head2 B<uv2trw>(I<U,V>,[I<OPT>],[I<latc,lonc>]) 

=over 4

This function returns the tangential (I<OPT>=1, default) or radial (I<OPT>=-1)
velocity relative to a fixed I<latc,lonc>. If no I<latc,lonc> is provided, 
then the center of the grid is assumed as the fixed point. For example, 

      ga-> d uv2trw(ua,va,latc,lonc,-1) 

displays the I<radial> velocity, while

      ga-> d uv2trw(ua,va,latc,lonc) 

displays the I<tangential> velocity.

I<OPT> = 2 -- return the u(x,y) component of the tangential wind (Vt(r,theta))
I<OPT> = 3 -- return the v(x,y) component of the tangential wind (Vt(r,theta))

I<OPT> = -3 -- return the v(x,y) component of the radial wind (Vr(r,theta))
I<OPT> = -2 -- return the u(x,y) component of the radial wind (Vr(r,theta))

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

=back


=head1 AUTHOR 

Michael Fiorino (mfiorino@gmail.com)

=head1 COPYRIGHT

Copyright (C) 2009 Michael Fiorino

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif




