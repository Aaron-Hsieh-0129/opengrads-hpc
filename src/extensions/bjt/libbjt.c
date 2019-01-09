/*

    Copyright (c) 2003-2008 by Ben-Jei Tsuang <btsuang@yahoo.com>
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

% cpp -DPOD -P < libbjt.c > libbjt.pod

and place libbjt.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < libbjt.pod > libbjt.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < libbjt.pod > libbjt.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make libbjt.html
% make libbjt.wiki

*/

#ifndef POD

#define LW
#undef lDiag
//#define lDiag

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "grads.h"
#include "gx.h"		// bjt: to have" pi"
#include "nr.h"		// for using a modified version of Numerical Recepies routines

#define float  double

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif
#ifndef SIGN
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
#endif
/*mf 971022 --- expose Mike Fiorino's global struct to these routines for warning level setting mf*/
extern struct gamfcmn mfcmn;
/*mf 971022 --- expose Mike Fiorino's global struct to these routines for warning level setting mf*/

static struct gaufb *ufba;  /* Anchor for user function defs */
char *gxgnam(char *);  /* This is also in gx.h */
static char pout[256];   /* Build error msgs here */
static char *mon[12] = {"jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"};

static int verbose = 1;   /*ams by default only print error messages and help ams*/

/* Function routine names.  Add a new function by putting the
prototype here and adding to the if tests below.  */
//modified from numerical recipes
int julianDate(int, int, int);
//void calday(int, int *, int *, int *);
float ellf(float phi, float ak);
float rf(float x, float y, float z);
float bisdew(float vap, float null);
float calcAirVaporPressure(float ta, float relativeHumidity);
float calcAtmosphericLongWaveRadiation(float ta, float relativeHumidity, float cloudAmount);
int calcJulianDate(int year,int	month,int day);
int calcMonthDay(int year,int julianDate);
float calcSaturatedVaporPressure(float ta);
float calcSolarRadiation(float zenith, float cloudAmount);
float calcTerrestrialLongWaveRadiation(float tg, float emiss);
float calcZenithAngle(int julianDate, float hour, float latitude);
float sign(float val);
int tmaskf  (struct gafunc *, struct gastat *, int);

/* GrADS v1.x compatibility functions */
int gex_expr1(char *expr, struct gastat *pst);
int gex_setUndef (struct gastat *pst);
#define gaexpr gex_expr1
#define valprs getdbl

//
// bjt >>
/*  in fflt, ffjd, ffcosz funtion :
data must be grid data
pgr->isiz : X dimention size
pgr->jsiz : Y dimention size
isiz      : longitude size
jsiz      : latitude size
two dimention : lon-lat, lon-T, lat-T
one dimention : lon, lat, T
zero dimention: value
jd was calculating from 1 A.D.
*/

struct gafile *getfile(char *ch, struct gastat *pst) {
char name[20], vnam[20], cc, *pos;
int i,fnum,ii,jj,dval,rc,dotflg,idim,jdim,dim;
int id[4];
float dmin[4],dmax[4],d1,d2;
int gmin[4],gmax[4];
struct gafile *pfi;
float (*conv) (float *, float);
float *cvals,*r,*r2,lonr,rlon,dir,spd,wrot,a,b;
int size,j,dotest;

  /* Get the variable or function name.  It must start with a
     letter, and consist of letters or numbers or underscore.  */

  i=0;
  while ( (*ch>='a' && *ch<='z') || (*ch>='0' && *ch<='9' ) || (*ch == '_') ) {
    name[i] = *ch;
    vnam[i] = *ch;
    ch++; i++;
    if (i>16) break;
  }
  name[i] = '\0';
  vnam[i] = '\0';  /* Save 'i' for next loop */

  /* Check for the data set number in the variable name.  If there,
     then this has to be a variable name.                            */

  fnum = pst->fnum;
  dotflg=0;
  if (*ch == '.') {
    dotflg=1;
    ch++;
    pos = intprs(ch,&fnum);
    if (pos==NULL || fnum<1) {
      sprintf (pout,"Syntax error: Bad file number for variable %s \n",
                 name);
      gaprnt (0,pout);
      return (NULL);
    }
    vnam[i] = '.';
    i++;
    while (ch<pos) {
      vnam[i] = *ch;
      ch++; i++;
    }
    vnam[i] = '\0';
  }

  /* Check for a predefined data object. */

  pfi = NULL;
  if (!dotflg) pfi = getdfn(name,pst);

  /* If not a defined grid, get a pointer to a file structure    */

  if (pfi==NULL) {
    if (!dotflg) pfi = pst->pfid;
    else {
      pfi = pst->pfi1;
      for (i=1; i<fnum && pfi!=NULL; i++) pfi = pfi->pforw;
      if (pfi==NULL) {
        gaprnt (0,"Data Request Error:  File number out of range \n");
        sprintf (pout,"  Variable = %s \n",vnam);
        gaprnt (0,pout);
        return (NULL);
      }
    }
  }

  return (pfi);
}



int fflt  (struct gafunc *pfc, struct gastat *pst) {
	int i,j,rc;
	int isiz;
	int dt, dy, dm, dd, dh;//an interval of time
	float *val;
	struct gagrid *pgr;
	struct dt tim;
	
	if (pfc->argnum!=1) {
		gaprnt (0,"Error from Local Time: Too many or too few args \n");
		gaprnt (0,"                 One argument expected \n");
		return (1);
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	
	if (pst->type==1) {
		pgr = pst->result.pgr;
		if (!pgr->ilinr || !pgr->jlinr) {
			gaprnt (0,"Error from Local Time: Grid must have linear scaling\n");
			gafree (pst);
			return (1);
		}
		if (0==pst->idim && 2!=pst->jdim)
			isiz = pgr->isiz;  //X-Y:lon-lat,lon-Time,lon,lat,T dimention
		else if ((1==pst->idim && 3==pst->jdim) || (2!=pst->idim && -1==pst->jdim))
			isiz=1;         //X-Y:lat-Time,
		else {
			gaprnt (0,"X or Y dimension must be lon, lat, or time scaling.\n");
			gafree (pst);
			return (1);
		}
		tim = pst->tmin;
		val = pgr->grid;
		
		if(0!=pgr->dimmax[3]-pgr->dimmin[3]) {
			dt = pgr->dimmax[3]-pgr->dimmin[3]+1;
			dy = pst->tmax.yr - pst->tmin.yr+1;
			dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
			dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
				-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
			dh = (dd-1)*24 + pst->tmax.hr - pst->tmin.hr +1;
			if (dt==dh){ // t is increasing by hour
				if(3==pgr->jdim){
					for (j=0; j<pgr->jsiz; j++) {
						for (i=0; i<pgr->isiz; i++) {
							if (1!=isiz) {
								*val=pst->dmin[0]+
									(pst->dmax[0]-pst->dmin[0])/((float)isiz-1.)*(float)i;
							} else 	*val=pst->dmin[0];
							tim.hr = pst->tmin.hr+(int)(floor((*val+7.5)/15))+j;
							if (tim.hr < 0) tim.hr +=24;
							*val=(float)(tim.hr%24)+(float)pst->tmin.mn/60;
							val++;
						}
					}
				}else if(3==pgr->idim){
					for (j=0; j<pgr->jsiz; j++) {
						for (i=0; i<pgr->isiz; i++) {
							*val=pst->dmin[0];
							tim.hr = pst->tmin.hr+(int)(floor((*val+7.5)/15))+i;
							if (tim.hr < 0) tim.hr +=24;
							*val=(float)(tim.hr%24)+(float)pst->tmin.mn/60;
							val++;
						}
					}
				}
			}
		}else{
			//X-Y:lon-lat, lon, lat dimention, value, and lon-T, lat-T, T increasing by yr,mo,dy.
			for (j=0; j<pgr->jsiz; j++) {
				for (i=0; i<pgr->isiz; i++) {
					if (1!=isiz) {
						*val=pst->dmin[0]+
							(pst->dmax[0]-pst->dmin[0])/((float)isiz-1.)*(float)i;
					} else 	*val=pst->dmin[0];
					tim.hr = pst->tmin.hr+(int)(floor((*val+7.5)/15));
					if (tim.hr < 0) tim.hr +=24;
					*val=(float)(tim.hr%24)+(float)pst->tmin.mn/60;
					val++;
				}
			} 
		}
	}else {
		gaprnt (0,"Error from LOCAL TIME : Argument must be a grid \n");
		return (1);
    }
	return (0);
}

int ffjd  (struct gafunc *pfc, struct gastat *pst) {  //calc from 1 A.D.
	int i,k,rc,cnt;
	int isiz;
	int dt,dy,dm,dd,dh;//an interval of time
	int juliandate,jd;
	float *val;
	float *lon;
	struct gagrid *pgr;
	struct gafile *pfi;
	struct dt dtim, tim;
	//	int M[12]={31,28,31,30,31,30,31,31,30,31,30,31};  //days of one month
	
	if (pfc->argnum!=1) {
		gaprnt (0,"Julian day since 1 A.D.\n");
		gaprnt (0,"Error from JulianDate: Too many or too few args \n");
		gaprnt (0,"                 One argument expected \n");
		return (1);
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	pfi = pst->pfid;
	if (rc) return(rc);
//	if (valprs(pfc->argpnt[0],pst)==NULL) { rc=1; return (rc); }
	
	if (pst->type==1) {
		pgr = pst->result.pgr;
		gr2t (pfi->grvals[3],pgr->dimmin[3],&dtim);
//                gr2t (pgr->ivals,pgr->dimmin[3],&dtim);
/*		
		if (!pgr->ilinr || !pgr->jlinr) {
			gaprnt (0,"Error from JulianDate: Grid must have linear scaling\n");
			gafree (pst);
			return (1);
		}
*/
		if (0==pst->idim && 2!=pst->jdim)
			isiz = pgr->isiz;  //X-Y:lon-lat,lon-Time,lon,lat,T dimention
		else if ((1==pst->idim && 3==pst->jdim) || (2!=pst->idim && -1==pst->jdim))
			isiz=1;         //X-Y:lat-Time,
		else {
			gaprnt (0,"X or Y dimension must be lon, lat, or time scaling.\n");
			gafree (pst);
			return (1);
		}
		val = pgr->grid;
		lon = malloc(isiz*sizeof(float));
		if (!lon) { sprintf(pout,"Out of memory\n");gaprnt (1,pout);return(1);}	
		cnt = pgr->isiz * pgr->jsiz;
		
		//calc lon for each grid
		if (1!=isiz) {
			for (i=0 ; i<pgr->isiz ; i++)
				lon[i] = pst->dmin[0]+(pst->dmax[0]-pst->dmin[0])/((float)isiz-1)*(float)i;
		}
		else    lon[0] = pst->dmin[0];
		//calc juliandate at 0Z
		if ((0==pst->idim && 1==pst->jdim) || (3!=pst->idim && -1==pst->jdim)){  
//		juliandate = julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)-julianDate(1,1,1)+1;
		juliandate = julianDate(dtim.mo, dtim.dy, dtim.yr)-julianDate(1,1,1)+1;
			//X-Y:lon-lat, lon, lat dimention, value
			for (k=0; k<cnt; k++) {
				jd = juliandate;  //local juliandate
				if (lon[k%isiz]<180.0) tim.hr = dtim.hr+(int)(floor((lon[k%isiz]+7.5)/15));
				else tim.hr = dtim.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
				*val = jd+(int)(floor((float)tim.hr/24));
				val++;
			}
		}else if(3==pgr->jdim) {  //lon-T,lat-T
			dt = pgr->dimmax[3]-pgr->dimmin[3]+1;
			dy = pst->tmax.yr - pst->tmin.yr+1;
			dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
			dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
				-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
			dh = (dd-1)*24 + pst->tmax.hr - pst->tmin.hr +1;
			if (dt==dm) { // t is increasing by month
				for (k=0; k<cnt; k++) {
					tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
					if (0==tim.mo) tim.mo=12;
					jd = julianDate(tim.mo, pst->tmin.dy,
						pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if (dt==dd){  // t is increasing by day
				for (k=0; k<cnt; k++) {
					jd = juliandate +k/pgr->isiz;
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if(dt==dh){  // t is increasing by hour
				for (k=0; k<cnt; k++) {
					jd = juliandate + (k/pgr->isiz)/24;
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if (dt==dy){
				for (k=0; k<cnt; k++) {
					jd = julianDate(pst->tmin.mo, pst->tmin.dy,
						pst->tmin.yr+k/pgr->isiz)-julianDate(1,1,1)+1; //local juliandate
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}
		}else if (3==pgr->idim && -1==pgr->jdim) {  //T dimension
			dt = pgr->dimmax[3]-pgr->dimmin[3]+1;
			dy = pst->tmax.yr - pst->tmin.yr+1;
			dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
			dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
				-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
			dh = (dd-1)*24 + pst->tmax.hr - pst->tmin.hr +1;
			if (dt==dm) { // t is increasing by month
				for (k=0; k<cnt; k++) {
					tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
					if (0==tim.mo) tim.mo=12;
					jd = julianDate(tim.mo, pst->tmin.dy,
						pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
					if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if (dt==dd){  // t is increasing by day
				for (k=0; k<cnt; k++) {
					jd = juliandate +k;
					if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if(dt==dh){  // t is increasing by hour
				for (k=0; k<cnt; k++) {
					jd = juliandate + k/24;
					if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}else if (dt==dy){
				for (k=0; k<cnt; k++) {
					jd = julianDate(pst->tmin.mo, pst->tmin.dy,
						pst->tmin.yr+k)-julianDate(1,1,1)+1; //local juliandate
					if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
					*val = jd+(int)(floor((float)tim.hr/24));
					val++;
				}
			}  
		}
		free (lon);
	}else {
		gaprnt (0,"Error from JulianDate : Argument must be a grid \n");
		return (1);
	}
	return (0);
}
// cos zenith angle
int ffcosz  (struct gafunc *pfc, struct gastat *pst) {
	int rc,cnt;
	int i,j,k;     // (i<=isiz, j<=jsiz, k<=cnt, cnt=isiz*jsiz)
	int isiz,jsiz; // lon and lat size, it is not necessarily to be equal to X and Y dimension size.
	int dt, dy, dm, dd, dh; //an interval of time
	struct gagrid *pgr;
	//struct gastn *stn;  //for stn
	//struct garpt *rpt;  //for stn
	struct dt tim;
	float *val;
	float *lon,*lat;
	const float RAD = pi/180.;        // radian conversion factor
	float theta;          // latidude
	float delta;          // earth's declination angle
	float dayZenith,monthZenith;
	int jd,jb,je,juliandate,julianend;
	//	int hr,mo,jd;            // local hour, month and juliandate
	//	int M[12]={31,28,31,30,31,30,31,31,30,31,30,31};  //days of one month
	
	
	if (pfc->argnum<2) {
		gaprnt (0,"Error from COSZ:  2 arguments expected \n");
		gaprnt (0,"cosz(exp,h|d|m) \n");
		gaprnt (0,"usage:\n");
		gaprnt (0,"Function returns: cosine of the solar zenith angle\n");
		gaprnt (0,"Arguments:\n");
		gaprnt (0,"   exp = any grads vaild expression, e.g., lat\n");
		gaprnt (0,"   h = hourly value of the cosine of solar zenith angle\n");
		gaprnt (0,"   d = daily mean of the cosine of solar zenith angle\n");
		gaprnt (0,"   m = monthly mean of the cosine of solar zenith angle\n");
		return (1);		
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	
	if (pst->type==1) {	
		pgr = pst->result.pgr;
		if (!pgr->ilinr || !pgr->jlinr) {
			gaprnt (0,"Error from COSZ: Grid must have linear scaling.\n");
			gafree (pst);
			return (1);
		}
		if (0==pst->idim && 1==pst->jdim){  //X-Y:lon-lat
			isiz = pgr->isiz;
			jsiz = pgr->jsiz;
		}else if (0==pst->idim && (3==pst->jdim || -1==pst->jdim)) {  //X-Y:lon-Time,lon
			isiz = pgr->isiz;
			jsiz = 1;
		}else if (1==pst->idim && (3==pst->jdim || -1==pst->jdim)) {  //X-Y:lat-Time,lat
			isiz = 1;
			jsiz = pgr->isiz;
		}else if ((3==pst->idim || -1==pst->idim) && -1==pst->jdim) {  //X-Y:T,value
			isiz = 1;
			jsiz = 1;
		}else {
			gaprnt (0,"X or Y dimension must be lon, lat, or time scaling.\n");
			gafree (pst);
			return (1);
		}
		val = pgr->grid;
		cnt = pgr->isiz * pgr->jsiz;
		lon = malloc(isiz * sizeof(float));
		if (!lon) { sprintf(pout,"Out of memory\n");gaprnt (1,pout);return(1);}	
		lat = malloc(jsiz * sizeof(float));
		if (!lat) { sprintf(pout,"Out of memory\n");gaprnt (1,pout);return(1);}	
		
		//calc lat for each grid	
		for (j=0; j<jsiz; j++) {
			if (1!=jsiz) 
				lat[j] = pst->dmin[1]+(pst->dmax[1]-pst->dmin[1])/((float)jsiz-1)*(float)j;
			else 	lat[j] = pst->dmin[1];
		}
		//calc lon for each grid
		for (i=0; i<isiz; i++) {
			if (1!=isiz) 
				lon[i] = pst->dmin[0]+(pst->dmax[0]-pst->dmin[0])/((float)isiz-1)*(float)i;
			else    lon[i] = pst->dmin[0];
		}
		tim=pst->tmin;
		// calc zenith angle for one hour
		if (cmpwrd("h",pfc->argpnt[1])){
			juliandate = julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)-julianDate(1,1,1)+1;
			//calc hour zenith angle for each grid
			if (!(3==pst->idim || 3==pst->jdim)) {  //lon-lat,lon,lat,v
				for(k=0; k<cnt; k++) {	
					jd = juliandate;//julianday at 0Z
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
					if (tim.hr < 0) tim.hr +=24;
					tim.hr = tim.hr%24; //local hour
					if (0==pst->idim && 1==pst->jdim) {
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k/isiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k/isiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
					}else{
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
					}
					if (*val<0.0) *val=0.0;				
					val++;
				}
			}else if(3==pgr->jdim) {  //lon-T,lat-T
				dt = pgr->jsiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
					-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
				dh = (dd-1)*24 + pst->tmax.hr - pst->tmin.hr +1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
						if (0==tim.mo) tim.mo=12;
						jd = julianDate(tim.mo, pst->tmin.dy,
							pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
						
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if (dt==dd){  // t is increasing by day
					for (k=0; k<cnt; k++) {
						jd = juliandate +k/pgr->isiz;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if(dt==dh){  // t is increasing by hour
					for (k=0; k<cnt; k++) {
						jd = juliandate + (k/pgr->isiz)/24;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if (dt==dy){
					for (k=0; k<cnt; k++) {
						jd = julianDate(pst->tmin.mo, pst->tmin.dy,
							pst->tmin.yr+k/pgr->isiz)-julianDate(1,1,1)+1;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}
			}else{  //T dimention
				dt = pgr->isiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
					-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
				dh = (dd-1)*24 + pst->tmax.hr - pst->tmin.hr +1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
						if (0==tim.mo) tim.mo=12;
						jd = julianDate(tim.mo, pst->tmin.dy,
							pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
						
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if (dt==dd){  // t is increasing by day
					for (k=0; k<cnt; k++) {
						jd = juliandate +k;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if(dt==dh){  // t is increasing by hour
					for (k=0; k<cnt; k++) {
						jd = juliandate + k/24;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}else if (dt==dy){  // t is increasing by year
					for (k=0; k<cnt; k++) {
						jd = julianDate(pst->tmin.mo, pst->tmin.dy,pst->tmin.yr+k)-julianDate(1,1,1)+1;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						if (tim.hr < 0) tim.hr +=24;
						tim.hr = tim.hr%24; //local hour
						*val=cos(15.*(12.-(float)tim.hr)*RAD)*cos(lat[k%jsiz]*RAD)*cos(-23.4*
							cos((float)(jd+10)*2*pi/365.2425)*RAD)+sin(lat[k%jsiz]*RAD)
							*sin(-23.4*cos((float)(jd+10)*2*pi/365.2425)*RAD);
						if (*val<0.0) *val=0.0;				
						val++;
					}
				}
			}
		}
		// calc zenith angle for day average
		else if (cmpwrd("d",pfc->argpnt[1])){
			juliandate = julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)-julianDate(1,1,1)+1;
			//calc hour zenith angle for each grid
			if (!(3==pst->idim || 3==pst->jdim)) {  //lon-lat,lon,lat,v
				for(k=0; k<cnt; k++) {	
					jd = juliandate;//julianday at 0Z
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
					
					if (0==pst->idim && 1==pst->jdim)	theta=lat[k/isiz]*RAD;   //lon-lat
					else	theta=lat[k%jsiz]*RAD;  //lat-T, lon-T
					delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
					if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
					else if (-tan(theta)*tan(delta) <-1.0)
						*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
					else{
						*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
							-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
							-sin(delta-theta-acos(-tan(delta)*tan(theta)))
							-sin(delta+theta-acos(-tan(delta)*tan(theta)))
							+sin(delta-theta+acos(-tan(delta)*tan(theta)))
							+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
					}
					val++;
				}
			}else if(3==pgr->jdim) {  //lon-T,lat-T
				dt = pgr->jsiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
					-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
						if (0==tim.mo) tim.mo=12;
						jd = julianDate(tim.mo, pst->tmin.dy,
							pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
						
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta = lat[k%jsiz]*RAD;  //lat-T, lon-T
						delta = -23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else if (dt==dd){  // t is increasing by day
					for (k=0; k<cnt; k++) {
						jd = juliandate +k/pgr->isiz;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta=lat[k%jsiz]*RAD;  //lat-T, lon-T
						delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else if (dt==dy){
					for (k=0; k<cnt; k++) {
						jd = julianDate(pst->tmin.mo, pst->tmin.dy,
							pst->tmin.yr+k/pgr->isiz)-julianDate(1,1,1)+1;
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta=lat[k%jsiz]*RAD;  //lat-T, lon-T
						delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else{
					gaprnt (0,"Error from COSZ: Interval of Time have to day, month, or year for calculating daily average.\n");
					gafree (pst);
					return (1);
				}
			}else{  //T dimention
				dt = pgr->isiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				dd = julianDate(pst->tmax.mo, pst->tmax.dy, pst->tmax.yr)
					-julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)+1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
						if (0==tim.mo) tim.mo=12;
						jd = julianDate(tim.mo, pst->tmin.dy,
							pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12)-julianDate(1,1,1)+1;
						
						if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta=lat[0]*RAD;  //lat-T, lon-T
						delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else if (dt==dd){  // t is increasing by day
					for (k=0; k<cnt; k++) {
						jd = juliandate +k;
						if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta=lat[0]*RAD;  //lat-T, lon-T
						delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else if (dt==dy){  // t is increasing by year
					for (k=0; k<cnt; k++) {
						jd = julianDate(pst->tmin.mo, pst->tmin.dy,pst->tmin.yr+k)-julianDate(1,1,1)+1;
						if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
						jd = jd + (int)(floor((float)tim.hr/24)); //locat juliandate
						theta=lat[0]*RAD;  //lat-T, lon-T
						delta=-23.4*RAD*cos((float)(jd+10)*360.*RAD/365.2425);
						if (-tan(theta)*tan(delta) > 1.0)	*val=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							*val=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							*val=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						val++;
					}
				}else{
					gaprnt (0,"Error from COSZ: Interval of Time have to day, month, or year for calculating daily average.\n");
					gafree (pst);
					return (1);
				}
			}
		} 
		// calc zenith angle for month average
		else if (cmpwrd("m",pfc->argpnt[1])){
			juliandate = julianDate(pst->tmin.mo, 1, pst->tmin.yr)-julianDate(1,1,1)+1;        //the first day in a month
			if (12==pst->tmin.mo) julianend = julianDate(1,1,pst->tmin.yr+1)-julianDate(1,1,1);//the last day in a month
            else julianend = julianDate(pst->tmin.mo+1,1,pst->tmin.yr)-julianDate(1,1,1);
			
			if (!(3==pst->idim || 3==pst->jdim)) {  //lon-lat,lon,lat,v
				for(k=0; k<cnt; k++) {	
					jb = juliandate;  //beginning of a month
					je = julianend;   //ending of a month
					if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
					else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
					//local jliandate
					jb += (int)(floor((float)tim.hr/24.));
					je += (int)(floor((float)tim.hr/24.));
					monthZenith=0.0;
					for(i=jb; i<=je; i++){
						if (0==pst->idim && 1==pst->jdim)	theta=lat[k/isiz]*RAD;   //lon-lat
						else	theta=lat[k%jsiz]*RAD;  //lat-T, lon-T, v
						delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);
						
						if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
						else if (-tan(theta)*tan(delta) <-1.0)
							dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
						else{
							dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
								-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
								-sin(delta-theta-acos(-tan(delta)*tan(theta)))
								-sin(delta+theta-acos(-tan(delta)*tan(theta)))
								+sin(delta-theta+acos(-tan(delta)*tan(theta)))
								+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
						}
						monthZenith += dayZenith;
					}
					*val=monthZenith/(float)(je-jb+1); //month's average					
					val++;
				}
			}else if(3==pgr->jdim) {  //lon-T,lat-T
				dt = pgr->jsiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k/pgr->isiz)%12;
						if (0==tim.mo) tim.mo=12;
						tim.yr = pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12;
						jb = julianDate(tim.mo,1,tim.yr)-julianDate(1,1,1)+1;
						if (12==tim.mo) je = julianDate(1,1,tim.yr+1)-julianDate(1,1,1);
						else je = julianDate(tim.mo+1,1,tim.yr)-julianDate(1,1,1);
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						//local jliandate
						jb += (int)(floor((float)tim.hr/24.));
						je += (int)(floor((float)tim.hr/24.));
						monthZenith=0.0;
						for(i=jb; i<=je; i++){
							theta = lat[k%jsiz]*RAD;
							delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);
							
							if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
							else if (-tan(theta)*tan(delta) <-1.0)
								dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
							else{
								dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
									-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
									-sin(delta-theta-acos(-tan(delta)*tan(theta)))
									-sin(delta+theta-acos(-tan(delta)*tan(theta)))
									+sin(delta-theta+acos(-tan(delta)*tan(theta)))
									+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
							}
							monthZenith += dayZenith;
						}
						*val=monthZenith/(float)(je-jb+1); //month's average					
						val++;
					}
				}else if (dt==dy){
					for (k=0; k<cnt; k++) {
						tim.yr = pst->tmin.yr+(pst->tmin.mo+k/pgr->isiz)/12;
						jb = julianDate(pst->tmin.mo,1,tim.yr)-julianDate(1,1,1)+1;
						if (12==pst->tmin.mo) je = julianDate(1,1,tim.yr+1)-julianDate(1,1,1);
						else je = julianDate(pst->tmin.mo,1,tim.yr)-julianDate(1,1,1);
						
						if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
						//local jliandate
						jb += (int)(floor((float)tim.hr/24.));
						je += (int)(floor((float)tim.hr/24.));
						monthZenith=0.0;
						for(i=jb; i<=je; i++){
							theta = lat[k%jsiz]*RAD;
							delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);
							if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
							else if (-tan(theta)*tan(delta) <-1.0)
								dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
							else{
								dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
									-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
									-sin(delta-theta-acos(-tan(delta)*tan(theta)))
									-sin(delta+theta-acos(-tan(delta)*tan(theta)))
									+sin(delta-theta+acos(-tan(delta)*tan(theta)))
									+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
							}
							monthZenith += dayZenith;
						}
						*val=monthZenith/(float)(je-jb+1); //month's average					
						val++;
					}
				}else{
					gaprnt (0,"Error from ZENITH: Interval of Time have to month, or year for calculating monthly average.\n");
					gafree (pst);
					return (1);
				}
			}else{  //T dimention
				dt = pgr->jsiz;
				dy = pst->tmax.yr - pst->tmin.yr+1;
				dm = (dy-1)*12+(pst->tmax.mo - pst->tmin.mo)+1;
				if (dt==dm) { // t is increasing by month
					for (k=0; k<cnt; k++) {
						tim.mo = (pst->tmin.mo+k)%12;
						if (0==tim.mo) tim.mo=12;
						tim.yr = pst->tmin.yr+(pst->tmin.mo+k)/12;
						jb = julianDate(tim.mo,1,tim.yr)-julianDate(1,1,1)+1;
						if (12==tim.mo) je = julianDate(1,1,tim.yr+1)-julianDate(1,1,1);
						else je = julianDate(tim.mo+1,1,tim.yr)-julianDate(1,1,1);
						if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
						//local jliandate
						jb += (int)(floor((float)tim.hr/24.));
						je += (int)(floor((float)tim.hr/24.));
						monthZenith=0.0;
						for(i=jb; i<=je; i++){
							theta = lat[0]*RAD;
							delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);
							
							if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
							else if (-tan(theta)*tan(delta) <-1.0)
								dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
							else{
								dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
									-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
									-sin(delta-theta-acos(-tan(delta)*tan(theta)))
									-sin(delta+theta-acos(-tan(delta)*tan(theta)))
									+sin(delta-theta+acos(-tan(delta)*tan(theta)))
									+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
							}
							monthZenith += dayZenith;
						}
						*val=monthZenith/(float)(je-jb+1); //month's average					
						val++;
					}
				}else if (dt==dy){
					for (k=0; k<cnt; k++) {
						tim.yr = pst->tmin.yr+(pst->tmin.mo+k)/12;
						jb = julianDate(pst->tmin.mo,1,tim.yr)-julianDate(1,1,1)+1;
						if (12==pst->tmin.mo) je = julianDate(1,1,tim.yr+1)-julianDate(1,1,1);
						else je = julianDate(pst->tmin.mo,1,tim.yr)-julianDate(1,1,1);
						
						if (lon[0]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15));
						else tim.hr = pst->tmin.hr+(int)(floor((lon[0]+7.5)/15))-24;
						//local jliandate
						jb += (int)(floor((float)tim.hr/24.));
						je += (int)(floor((float)tim.hr/24.));
						monthZenith=0.0;
						for(i=jb; i<=je; i++){
							theta = lat[0]*RAD;
							delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);
							if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
							else if (-tan(theta)*tan(delta) <-1.0)
								dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
							else{
								dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
									-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
									-sin(delta-theta-acos(-tan(delta)*tan(theta)))
									-sin(delta+theta-acos(-tan(delta)*tan(theta)))
									+sin(delta-theta+acos(-tan(delta)*tan(theta)))
									+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
							}
							monthZenith += dayZenith;
						}
						*val=monthZenith/(float)(je-jb+1); //month's average					
						val++;
					}
				}else{
					gaprnt (0,"Error from COSZ: Interval of Time have to month, or year for calculating monthly average.\n");
					gafree (pst);
					return (1);
				}
			}
			/*		}else if (cmpwrd("m2",pfc->argpnt[1])){
			float deltab,deltae;
			juliandate = julianDate(pst->tmin.mo, 1, pst->tmin.yr)-julianDate(1,1,1)+1;
			if (12==pst->tmin.mo) julianend = julianDate(1,1,pst->tmin.yr+1)-julianDate(1,1,1)+1-1;
            else julianend = julianDate(pst->tmin.mo+1,1,pst->tmin.yr)-julianDate(1,1,1)+1-1;
			//calc monthly average zenith angle for each grid
			for(k=0; k<cnt; k++){
			tim= pst->tmin;
			jb = juliandate;  //beginning of a month
			je = julianend;   //ending of a month
			if (lon[k%isiz]<180.0) tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15));
			else tim.hr = pst->tmin.hr+(int)(floor((lon[k%isiz]+7.5)/15))-24;
			jb += (int)(floor((float)tim.hr/24.));
			je += (int)(floor((float)tim.hr/24.));
			monthZenith=0.0;
			if (lat[k/isiz] > 50. || lat[k/isiz] < -50.){
			for(i=jb;i<=je;i++){
			theta = lat[k/isiz]*RAD;
			delta = -23.4*RAD*cos((float)(i+10)*360.*RAD/365.2425);		
			if (-tan(theta)*tan(delta) > 1.0)	dayZenith=0.0;
			else if (-tan(theta)*tan(delta) <-1.0)
			dayZenith=(pi*cos(delta-theta)-pi*cos(delta+theta))/(2.*pi);
			else{
			dayZenith=(0.5*(2*acos(-tan(delta)*tan(theta))*cos(delta-theta)
			-2*acos(-tan(delta)*tan(theta))*cos(delta+theta)
			-sin(delta-theta-acos(-tan(delta)*tan(theta)))
			-sin(delta+theta-acos(-tan(delta)*tan(theta)))
			+sin(delta-theta+acos(-tan(delta)*tan(theta)))
			+sin(delta+theta+acos(-tan(delta)*tan(theta)))))/(2.*pi); //day's average
			}
			monthZenith += dayZenith;
			}
			*val=monthZenith/(float)(je-jb+1); //month's average
			}else{
			theta = lat[k/isiz]*RAD;
			deltab = -23.4*RAD*cos((float)(jb-1+10)*360.*RAD/365.2425);
			deltae = -23.4*RAD*cos((float)(je+10)*360.*RAD/365.2425);
			//Integrate for delta
			*val =( (1./sqrt(8.*(1.+cos(2.*theta))) * ellf(deltae,2./(1.+cos(2.*theta)))* 
			pow(1/cos(deltae),3)/cos(theta) * (3.*cos(deltae-theta) +3.*cos(deltae+theta)
			+cos(3.*deltae-theta) +cos(3.*deltae+theta))-2.*acos(-tan(deltae)*tan(theta))
			*cos(deltae)*sin(theta))
			-(1./sqrt(8.*(1.+cos(2.*theta))) * ellf(deltab,2/(1+cos(2.*theta)))* 
			pow(1/cos(deltab),3)/cos(theta) * (3.*cos(deltab-theta) +3.*cos(deltab+theta)
			+cos(3.*deltab-theta) +cos(3.*deltab+theta))-2.*acos(-tan(deltab)*tan(theta))
			*cos(deltab)*sin(theta)) )/ ((deltae-deltab)*2.*pi);
			}		
			val++;
			}
			*/			
		}else{
			gaprnt (0,"Error from COSZ : 2nd argument must be 'h' or 'd' or 'm' \n");
			return (1);	
		}
		free (lon);
		free (lat);
	} else {
		//**Do nothing on stn data now! If you want to add stn function, please write under line.
		gaprnt (0,"Error from COSZ : First argument must be a grid \n");
		return (1);
		/*	  stn = pst->result.stn; 
		rpt = stn->rpt;
		while (rpt!=NULL) {
		if (rpt->val!=stn->undef) rpt->val = sin(rpt->val);
		rpt=rpt->rpt;
		}
		*/
	}
	return (0);
}
//calc the daylight hours ratio of a day
int ffdayratio  (struct gafunc *pfc, struct gastat *pst) {
	int rc, i, j, k;
	int jb, je;
	int juliandate;
	struct gagrid *pgr;
	float *val;
	float lat, rj;
	float *ltvals; //*lnvals
	//float (*lnconv) (float *, float);
	float (*ltconv) (float *, float);
	float dhour, mhour;
	float delta;          // earth's declination angle
	const float RAD = pi/180.;        // radian conversion factor
	
	if (pfc->argnum!=1) {
		gaprnt (0,"Error from dayratio:  One argument expected \n");
		gaprnt (0,"dayratio(exp) \n");
		gaprnt (0,"usage:\n");
		gaprnt (0,"Function returns: daylight ratio of a day\n");
		gaprnt (0,"Arguments:\n");
		gaprnt (0,"   exp = any grads vaild expression, e.g., lat\n");
		return (1);
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	
	if (pst->type==1) {
		pgr = pst->result.pgr;
		val = pgr->grid;
		//lnvals = pgr->ivals;
		ltvals = pgr->jvals;
		//lnconv = pgr->igrab;
		ltconv = pgr->jgrab;
		
		/*	sprintf (pout,"pgr->dimmin[1]=%d \n",pgr->dimmin[1]);gaprnt (1,pout);
		sprintf (pout,"pgr->dimmax[1]=%d \n",pgr->dimmax[1]);gaprnt (1,pout);
		sprintf (pout,"pgr->dimmin[0]=%d \n",pgr->dimmin[0]);gaprnt (1,pout);
		sprintf (pout,"pgr->dimmax[0]=%d \n",pgr->dimmax[0]);gaprnt (1,pout);
		rj=(float)(pgr->dimmin[1]+1);
		lat  = ltconv(ltvals,rj) * RAD;
		sprintf (pout,"lat=%d \n",lat);gaprnt (1,pout);
		*/
		juliandate = julianDate(pst->tmin.mo, pst->tmin.dy, pst->tmin.yr)-julianDate(1,1,1)+1;
		for (j=(pgr->dimmin[1]); j<=pgr->dimmax[1]; j++) {
			rj = (float)j;
			lat  = ltconv(ltvals,rj) * RAD;
			for (i=(pgr->dimmin[0]); i<=pgr->dimmax[0]; i++) {
				jb = julianDate(pst->tmin.mo,1,pst->tmin.yr)-julianDate(1,1,1)+1;
				je = julianDate(pst->tmin.mo+1,1,pst->tmin.yr)-julianDate(1,1,1);
				mhour=0.0;
				for(k=jb; k<=je; k++){
					delta=-23.4*RAD*cos((float)(k+10)*360.*RAD/365.2425);
					if (-tan(lat)*tan(delta) > 1.0)	dhour=0.0;
					else if (-tan(lat)*tan(delta) <-1.0) dhour=1.0;
					else dhour = 2*acos(-tan(lat)*tan(delta))/(2*pi); //daylight ratio
					mhour += dhour;
				}
				*val = mhour/(float)(je-jb+1); //month's average					
				val++;
			}
		}
	} else {
		gaprnt (0,"Error from dayratio: First argument must be a grid \n");
		return (1);
	}
	
	return (0);
}

int ffif (struct gafunc *pfc, struct gastat *pst) {
	int i,rc,rflg,returnGrid,isGridCompatible;
	int size[5];
	struct gagrid *pgr[5];
	int isArgundef[5];
	//  pgr[1]: var, pgr[2]: standard, pgr[3]: true, pgr[4]: false
	float *p[5];
	char ch[80];
	// initialization
	for (i=0; i<5; i++) isArgundef[i]=0;
	/* Check for user errors */
	if (pfc->argnum!=5) {
		gaprnt (0,"Error from IF:  5 arguments expected \n");
		gaprnt (0,"usage:\n");
		gaprnt (0,"  d if(exp,>,exp2,tru_exp,false_exp)\n");
		gaprnt (0,"  2nd arg: ==, =, >, <, >=, <=, !=\n");
		gaprnt (0,"  exp2,true_exp,false_exp can be expression, constant or -u.\n");
		return (1);
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	if (pst->type==0) {
		gaprnt (0,"Error from IF : First argument must be a grid \n");
		gafree (pst);
		return (-1);
	}		
	pgr[1] = pst->result.pgr;
	size[1] = pgr[1]->isiz * pgr[1]->jsiz;
#ifdef lDiag
	sprintf (pout,"1.0\n");gaprnt (0, pout);
#endif	
	rflg = 0;
	if (pfc->argnum>2) {
		getwrd(ch,pfc->argpnt[1],80);
		lowcas(ch);
		if (cmpwrd(">",ch)) rflg = 1;
		else if (cmpwrd("==",ch)||cmpwrd("=",ch)) rflg = 2;	// 20031206
		else if (cmpwrd("<",ch)) rflg = 3;
		else if (cmpwrd(">=",ch)) rflg = 4;
		else if (cmpwrd("<=",ch)) rflg = 5;
		else if (cmpwrd("!=",ch)) rflg = 6;
		else {
			sprintf (pout,"Warning from IF: Invalid flag: %s.\n", ch);
			gaprnt (0, pout);
			gafree (pst);
			return (1);			
		}
	}
	// 20031206
#ifdef lDiag
	sprintf (pout,"2.0\n");gaprnt (0, pout);
#endif	
	getwrd(ch,pfc->argpnt[2],80);
	if (cmpwrd("-u",ch)) {
		isArgundef[2] = 1;
		size[2] = 1;
	} else {
		rc = gaexpr(pfc->argpnt[2],pst);
		if (rc) {
			gagfre(pgr[1]);
			return (rc);
		}
		if (pst->type==0) {
			gafree (pst);
			gagfre(pgr[1]);
			return (-1);
		}
		pgr[2] = pst->result.pgr;
		size[2] = pgr[2]->isiz * pgr[2]->jsiz;
	}
#ifdef lDiag
	sprintf (pout,"3.0\n");gaprnt (0, pout);
#endif	
	getwrd(ch,pfc->argpnt[3],80);
	if (cmpwrd("-u",ch)) {
		isArgundef[3] = 1;
		size[3] = 1;
	} else {
		rc = gaexpr(pfc->argpnt[3],pst);
		if (rc) {
			gagfre(pgr[1]);
			if (0==isArgundef[2]) gagfre(pgr[2]);
			return (rc);
		}
		if (pst->type==0) {
			gafree (pst);
			gagfre(pgr[1]);
			if (0==isArgundef[2]) gagfre(pgr[2]);
			return (-1);
		}
		pgr[3] = pst->result.pgr;
		size[3] = pgr[3]->isiz * pgr[3]->jsiz;
	}
#ifdef lDiag
	sprintf (pout,"4.0\n");gaprnt (0, pout);
#endif	
	getwrd(ch,pfc->argpnt[4],80);
	if (cmpwrd("-u",ch)) {
		isArgundef[4] = 1;
		size[4] = 1;
	} else {
		rc = gaexpr(pfc->argpnt[4],pst);
		if (rc) {
			gagfre(pgr[1]);
			if (0==isArgundef[2]) gagfre(pgr[2]);
			if (0==isArgundef[3]) gagfre(pgr[3]);
			return (rc);
		}
		if (pst->type==0) {
			gafree (pst);
			gagfre(pgr[1]);
			if (0==isArgundef[2]) gagfre(pgr[2]);
			if (0==isArgundef[3]) gagfre(pgr[3]);
			return (-1);
		}
		pgr[4] = pst->result.pgr;
		size[4] = pgr[4]->isiz * pgr[4]->jsiz;
	}
//	5.0 determine return grid and check whether the size is compatible	
#ifdef lDiag
	sprintf (pout,"5.0\n");gaprnt (0, pout);
#endif	
	pgr[0]=pgr[1];
	size[0]=size[1];
	returnGrid=1;
	for (i=1; i<=4; i++) {
		if (size[i]>size[0]) {
			size[0]=size[i];
			pgr[0]=pgr[i];
			returnGrid=i;
		}
	}

	isGridCompatible=1;
	for (i=1; i<=4; i++) {
		if (size[0]!= size[i] && 1!= size[i]) {
			isGridCompatible=0;
			sprintf (pout,"grid[%i]'s size = %i\n",i,size[i]);gaprnt (0, pout);
			sprintf (pout,"max grid[%i]'s size = %i\n",returnGrid,size[0]);gaprnt (0, pout);
		}
	}

	if (!isGridCompatible ) {
		gaprnt (0,"size incompatible\n");
		gagfre(pgr[1]);
		if (0==isArgundef[2]) gagfre(pgr[2]);
		if (0==isArgundef[3]) gagfre(pgr[3]);
		if (0==isArgundef[4]) gagfre(pgr[4]);
		return (1);
	}
	
//	6.0 start to work
#ifdef lDiag
	sprintf (pout,"6.0\n");gaprnt (0, pout);
#endif	
	p[0] = pgr[0]->grid;
	p[1] = pgr[1]->grid;
	if (0==isArgundef[2]) p[2] = pgr[2]->grid;
	if (0==isArgundef[3]) p[3] = pgr[3]->grid;
	if (0==isArgundef[4]) p[4] = pgr[4]->grid;
	
	for (i=0; i<size[0]; i++) {
		if (0!=isArgundef[2] ){
			if (2==rflg) {	// p[1]==undef ?
				if (*p[1]==pgr[1]->undef) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (6==rflg) {	// p[1]!=undef ?
				if (*p[1]!=pgr[1]->undef) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else {
				*p[0]=pgr[0]->undef;
			}
		} else if (*p[1] != pgr[1]->undef && *p[2]!=pgr[2]->undef) {
			if (1==rflg) {
				if (*p[1] > *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (2==rflg) {
				if (*p[1] == *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (3==rflg) {
				if (*p[1] < *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (4==rflg) {
				if (*p[1] >= *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (5==rflg) {
				if (*p[1] <= *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			} else if (6==rflg) {
				if (*p[1] != *p[2]) {
					if (1==isArgundef[3]) *p[0]=pgr[0]->undef;
					else if (*p[3]==pgr[3]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[3];
				} else {
					if (1==isArgundef[4]) *p[0]=pgr[0]->undef;
					else if (*p[4]==pgr[4]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[4];
				}
			}
		} else {
			*p[0]=pgr[0]->undef;
		}
		p[0]++;
		if (1!=size[1]) p[1]++;
		if (0==isArgundef[2]) {if (1!=size[2]) p[2]++;} 
		if (0==isArgundef[3]) {if (1!=size[3]) p[3]++;} 
		if (0==isArgundef[4]) {if (1!=size[4]) p[4]++;} 
	}
#ifdef lDiag
	sprintf (pout,"7.0\n");gaprnt (0, pout);
#endif	
	for (i=1; i<=4; i++) { if (i!=returnGrid && !isArgundef[i]) gagfre(pgr[i]); }
	pst->result.pgr = pgr[0];
	return (0);
}

int maxminv (struct gafunc *pfc, struct gastat *pst, int flg) {
	int i,rc;
	int size1, size2;
	struct gagrid *pgr1, *pgr2;
	float *p1, *p2;
	
	/* Check for user errors */
	if (pfc->argnum!=2) {
		gaprnt (0,"Error from maxv/minv: 2 arguments expected \n");
		return (1);
	}
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	if (pst->type==0) {
		gaprnt (0,"Error from maxv/minv : First argument must be a grid \n");
		gafree (pst);
		return (-1);
	}		
	pgr1 = pst->result.pgr;
	rc = gaexpr(pfc->argpnt[1],pst);
	if (rc) {
		gagfre(pgr1);
		return (rc);
	}
	if (pst->type==0) {
		gafree (pst);
		gagfre(pgr1);
		return (-1);
	}
	pgr2 = pst->result.pgr;
	size1 = pgr1->isiz * pgr1->jsiz;
	size2 = pgr2->isiz * pgr2->jsiz;
	if (size1 != size2 && 1!= size2){
		gafree (pst);
		gagfre(pgr1);
		gagfre(pgr2);
		return (-1);
	}
	
	p1 = pgr1->grid;
	p2 = pgr2->grid;
	
	for (i=0; i<size1; i++) {
		if (*p1 != pgr1->undef && *p2!=pgr2->undef) {
			if (4==flg) {	// max
				if (*p1 >= *p2) {
//					if (*p1==pgr1->undef) *p1=pgr1->undef;
//					else *p1=*p1;
					*p1=*p1;
				}
				else {
//					if (*p2==pgr2->undef) *p1=pgr1->undef;
//					else *p1=*p2;
					*p1=*p2;
				}
			} else if (5==flg) {	// min
				if (*p1 <= *p2) {
//					if (*p1==pgr1->undef) *p1=pgr1->undef;
//					else *p1=*p1;
					*p1=*p1;
				}
				else {
//					if (*p2==pgr2->undef) *p1=pgr1->undef;
//					else *p1=*p2;
					*p1=*p2;
				}
			}
		} else {
			*p1=pgr1->undef;
		}
		p1++;
		if (1!=size2) p2++; 
	}
	gagfre(pgr2);
	pst->result.pgr = pgr1;
	return (0);
}
int ffmaxv (struct gafunc *pfc, struct gastat *pst) {
	return (maxminv (pfc, pst,4));	// 4 == max flag
}
int ffminv (struct gafunc *pfc, struct gastat *pst) {
	return (maxminv (pfc, pst,5));	// 5 == min flag
}

int ffwhich (struct gafunc *pfc, struct gastat *pst) {
	int i,j,rc,lSet;
	int * size;
	int * pgrUndef;
	struct gagrid **pgr;
	float **p;
	
	/* Check for user errors */
	if (0==pfc->argnum || pfc->argnum%2!=0) {
		gaprnt (0,"which(exp,con1,exp1,con2,exp2, ..., else_exp)\n");
		gaprnt (0,"Error from WHICH:  Too many or too few args \n");
		gaprnt (0,"                  even arguments expected \n");
		return (1);
	}
	size = (int *)malloc(sizeof(int)*pfc->argnum);
	pgrUndef = (int *)malloc(sizeof(int)*pfc->argnum);
	pgr = (struct gagrid **)malloc(sizeof(struct gagrid *)*pfc->argnum);
	p = (float **)malloc(sizeof(float *)*pfc->argnum);
	if (NULL==(size)||NULL==(pgrUndef)||NULL==(pgr)||NULL==(p)) {
		gaprnt (0,"Error from WHICH:  memory allocation error\n");
		if (NULL!=size) free (size);
		if (NULL!=pgrUndef) free (pgrUndef);
		if (NULL!=pgr) free (pgr);
		if (NULL!=p) free (p);
		return (1);
	}
	for (i=0; i<pfc->argnum; i++) {
		pgrUndef[i]=0;
		if (0!=i && cmpwrd("-u",pfc->argpnt[i])) {
			pgrUndef[i] = 1;
			size[i]=1;
		} else {			
			rc = gaexpr(pfc->argpnt[i],pst);
			if (rc) {
				for (j=0; j<i; j++) {if(!pgrUndef[i]) gagfre(pgr[j]);}
				if (NULL!=size) free (size);
				if (NULL!=pgrUndef) free (pgrUndef);
				if (NULL!=pgr) free (pgr);
				if (NULL!=p) free (p);
				return (rc);
			}
			if (pst->type==0) {
				gaprnt (0,"Error from WHICH : Arguments must be grid \n");
				gafree (pst);
				for (j=0; j<i; j++) {if(!pgrUndef[i]) gagfre(pgr[j]);}
				if (NULL!=size) free (size);
				if (NULL!=pgrUndef) free (pgrUndef);
				if (NULL!=pgr) free (pgr);
				if (NULL!=p) free (p);
				return (-1);
			}		
			pgr[i] = pst->result.pgr;
			size[i] = pgr[i]->isiz * pgr[i]->jsiz;
		}
	}
	for (i=0; i<pfc->argnum; i++) {
		if (size[i] != size[0] && 1!= size[i]){
			gaprnt (0,"Error from WHICH: Size incompatible\n");
			gafree (pst);
			for (j=0; j<pfc->argnum; j++) {if(!pgrUndef[i]) gagfre(pgr[j]);}
			return (-1);
		}
	}
	
	for (i=0; i<pfc->argnum; i++) {
		if(!pgrUndef[i]) p[i]=pgr[i]->grid;
	}
	for (j=0; j<size[0]; j++) {
		lSet=0;	// false
		if (*p[0] != pgr[0]->undef) {
			for (i=1; i<pfc->argnum-1; i+=2) {
				if(pgrUndef[i]) continue;
				if (*p[0]==*p[i]) {
					if (*p[i+1]==pgr[i+1]->undef) *p[0]=pgr[0]->undef;
					else *p[0]=*p[i+1];
					lSet=1;	// true
					break;
				}
			}
		} else {
			for (i=1; i<pfc->argnum-1; i+=2) {
				if(pgrUndef[i]) {
					if(pgrUndef[i+1]) {
						*p[0]=pgr[0]->undef;
						lSet=1;	// true
						break;
					} else {
						if (*p[i+1]==pgr[i+1]->undef) *p[0]=pgr[0]->undef;
						else *p[0]=*p[i+1];
						lSet=1;	// true
						break;
					}					
				}
			}
		}
		if (!lSet) {
			if(pgrUndef[pfc->argnum-1]) {
				*p[0]=pgr[0]->undef;
			} else {
				if (*p[pfc->argnum-1]==pgr[pfc->argnum-1]->undef) *p[0]=pgr[0]->undef;
				else *p[0]=*p[pfc->argnum-1];
			}				
		
		}
		for (i=0; i<pfc->argnum; i++) {
			if (1!=size[i]) {if(!pgrUndef[i]) p[i]++; } 
		}
	}
	for (i=1; i<pfc->argnum; i++) {
		if(!pgrUndef[i]) gagfre(pgr[i]);
	}
	pst->result.pgr = pgr[0];
	if (NULL!=size) free (size);
	if (NULL!=pgrUndef) free (pgrUndef);
	if (NULL!=pgr) free (pgr);
	if (NULL!=p) free (p);
	return (0);
}
char *testnam[2] = {"FTEST","TTEST"};

int testprob(struct gafunc *pfc, struct gastat *pst, int sel) {
	struct gagrid *pgr[3];
	int i,j,rc,cnt[3],lsiz,ldef,nexp;
	float *f, *df1, *df2, *res;
	
//	sprintf(pout,"Error from sel==%i\n",sel);gaprnt (0,pout);
	if (0==sel) nexp=3;	// ftest
	else if (1==sel) nexp=2;	// ttest	
	if (pfc->argnum!=nexp) {
		if(sel ==0) {
			sprintf(pout,"\t%s(ratio,df1,df2)\n",testnam[sel]);gaprnt (0,pout);
			sprintf(pout,"F test:\n");gaprnt (0,pout);
			sprintf(pout,"probability for ratio=var1/var2>1,\n");gaprnt (0,pout);
			sprintf(pout,"where var1 has df1 degree of freedom and\n");gaprnt (0,pout);
			sprintf(pout,"      var2 has df2 degree of freedom\n");gaprnt (0,pout);
		} else if(sel ==1) {
			sprintf(pout,"\t%s(diff,df)\n",testnam[sel]);gaprnt (0,pout);
			sprintf(pout,"T test:\n");gaprnt (0,pout);
			sprintf(pout,"probability for two means (m1, m2) to be different.\n");gaprnt (0,pout);
			sprintf(pout,"where diff=(m1-m2)/std\n");gaprnt (0,pout);
			sprintf(pout,"      std = standard deviation\n");gaprnt (0,pout);
			sprintf(pout,"      df is the degree of freedom\n");gaprnt (0,pout);
		}
		sprintf(pout,"Error from %s:  Too many or too few args \n",testnam[sel]);gaprnt (0,pout);
		sprintf(pout,"\t%i arguments expected \n",nexp);gaprnt (0,pout);
		return (1);
	}
	
	for(i=nexp-1;i>=0;i--) {
		rc = gaexpr(pfc->argpnt[i],pst);
		if (rc) {
			for (j=nexp-1; j>i; j--) if (NULL!=pgr[j]) gagfre (pgr[j]);
			return (rc);
		}
		if (rc||pst->type!=1) {
			sprintf(pout,"Error from %s: %i argument must be a grid \n",testnam[sel],i+1);
			gaprnt (0,pout);
			for (j=nexp-1; j>i; j--) if (NULL!=pgr[j]) gagfre (pgr[j]);
			gafree (pst);
			return (1);
		}
		pgr[i] = pst->result.pgr;
		cnt[i] = pgr[i]->isiz * pgr[i]->jsiz;
	}
	// chk size compatible
	lsiz=1;
	for (i=1; i<nexp-1; i++) {
		if (cnt[i]!=1) lsiz=(lsiz&&(cnt[0]==cnt[i]));
	}	
	if(!lsiz) {
		sprintf(pout,"Error from %s: size incompatible!\n",testnam[sel]);
		gaprnt (0,pout);
		for (j=nexp-1; j>0; j--) if (NULL!=pgr[j]) gagfre (pgr[j]);
		gafree (pst);
		return (1);
	}	 
	// start to work
	f = pgr[0]->grid;
	df1 = pgr[1]->grid;
	if (sel==0) df2 = pgr[2]->grid;

	for (i=0; i<cnt[0]; i++) {
		if (0==sel) ldef=(*f!=pgr[0]->undef && *df1!=pgr[1]->undef && *df2!=pgr[2]->undef);
		else if (1==sel) ldef=(*f!=pgr[0]->undef && *df1!=pgr[1]->undef);
		if (ldef) {
			if (0==sel) *f=ftestb(*f, *df1, *df2);
			else if (1==sel) *f=ttestb(*f, *df1);
		} else {
			*f=pgr[0]-> undef;
		}	
		f++;
		if (1!=cnt[1]) df1++;
		if (0==sel&&1!=cnt[2]) df2++;
	}
	for (j=nexp-1; j>0; j--) if (NULL!=pgr[j]) gagfre (pgr[j]);
	return (0);
}

int ffftest (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = testprob (pfc, pst, 0);
	return (rc);
}

int ffttest (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = testprob (pfc, pst, 1);
	return (rc);
}

/* fit */  /* bjt */
int fftfit (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = tmaskf (pfc,pst,5);
	return (rc);
}


int fffit (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = tmaskf (pfc,pst,4);
	return (rc);
}

/* Time correlation */

int fftcorr2 (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = tmaskf (pfc, pst, 2);
	return (rc);
}

/* Time regression */  /* Timlin */

int fftregr2 (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = tmaskf (pfc,pst,3);
	return (rc);
}

/* Time mean, masked and arbitrarily weighted. */

int fftmave2 (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = tmaskf (pfc, pst, 1);
	return (rc);
}

char *tmnam[5] = {"TMAVE2","TCORR2","TREGR2","FIT","TFIT"};

/* Following function does time series operations that
involve a mask grid, where if the first mask grid
is undefined, the 2nd grid is not even accessed.
This saves processing time for composite means and
such.  Variable sel determines the function operation:

  sel = 1:  time mean.
  sel = 2:  time correlation.
  sel = 3:  time regression.
  sel = 4:  fit
  sel = 5:  tfit    */
// 2003/12/16: add time incr option (bjt)
int covindex(int ii,int jj,int n) {
	//      j	
	//   0  1  2  3  4
	//   1  5  6  7  8
	// i 2  6  9 10 11	
	//   3  7 10 12 13
	//   4  8 11 13 14
	// e.g. covindex(0,0,5)=0, covindex(0,1,5)=covindex(1,0,5)=1, covindex(1,1,5)=5,  
	//      covindex(2,3,5)=covindex(3,3,5)=10, covindex(4,4,5)=14
	int i, j, index, tmp;
	
	if(ii>jj) {tmp=ii;ii=jj;jj=tmp;}	// swap
	index=0;
	for (i=0; i<=ii; i++) {
		for (j=i; j<n; j++) {
			if (i==ii && j == jj) break;
			index++;
		}
	}
	return (index);	
}

/*mf 971020
-- new version of ave, mean ans sum using tmaskf technique 
*/
int tmaskf (struct gafunc *pfc, struct gastat *pst, int sel) {
	// ref:
	// Hays, W., 1980?: Statistics, 3rd Ed., 713 pp. + index
	// 吳宗正, 1993: 迴歸分析，三民書局, 476 pp.  
	const float TINY=1.0e-20;
	struct gafile *pfi;
	float t1,t2,res;
	int dim,wflag,d1,d2,d,rc,i,j,k,size;
	int sizeOut;	// size of output domain (=1 for sel=4, others =size)
	char *ch,*fnam;
	struct dt tinc;
	int mos,mns,incr;
	float *val;
	// y=my+b1*(x1-mx1)+b2*(x2-mx2)+..+b(nexp-1)*(x(nexp-1)-mx(nexp-1)
	int nexp;	// number of expresssion variables.
	int nskip;	// number of grid can not be fit by normal equation (lu solver)
	int ldef;	// define logical.
	int * siz;
	int * skipIndex;	// id vector for grid can not be fit by normal equation
	float * udef;
	struct gagrid **pgr;
	float **vv;
	float **mn, **cov, *wt;	//mean, cnt, covariance, weight
	int idim;
	int ncov;	// number of covariance (including variance)
	// variable for fit & tfit
	float *cnt;	// pairs of defined data  
	float *kexp;	// number of dependent variables  
	float **r;	// correlation coefficient among variables
	float **z;	// sy=z0*sx0+z1*sx1+   standardized form
	float **b;	// y=my+b1*(x1-mx1)+b2*(x2-mx2)+..+b(nexp-1)*(x(nexp-1)-mx(nexp-1) coefficient
//	float *ssr;	// sum square regression (b'X'Y-nYm^2)   
//	float *msr;	// mean square regression =ssr/(nexp-1)   
	float *corr;	// corr of y=my+b1*(x1-mx1)+b2*(x2-mx2)+..   
	float *vepl;	// variance explanied (=corr*corr)   
	float *sse;	// sum square error (sst0-ssr)   
	float *mse;	// mean square error =sse/(n-nexp)   
	float *err;	// unbias stderr of y=my+b1*(x1-mx1)+b2*(x2-mx2)+.. 
	float *corra;	// adjusted corr = sqrt(1-(n-1)/(n-nexp)*(1-vepl)), Wu (1993: p. 300)   
	float *fratio;	// f=msr/mse=(corr^2/(nexp-1))/((1-corr^2)/(n-nexp))=(vepl/(nexp-1))/((1-vepl)/(n-nexp))  				
	float *pc;	// probability of confidence level for multiple linear reg.: ftestb(fratio,nexp-1,n-nexp)  				
	float **sz;	// standard deviation of z coefficients (std form)
	float **sb;	// standard deviation of b coefficients (raw form)
	float **pcb;	// probability of confidence level for variable b: ftestb(sb,1,n-1)  				
	float tmp;	// temporary working variable

	float ** xxi;	// inverse of standardized normal matrix 
	int lskip;	// logical of skip
	FILE *fpdata, *fpctl;	// data file & ctl file handle 
	char filename[80]="null", datafilename[80], ctlfilename[80];
	int isiz, jsiz;
	// ludcmp
	const float TOL=1.0e-5;
	float p;	// ludcmp: whether the number of row interchanges was even or odd, respectively.
	float **c, *x;	// ludcmp:
	int *luindx;	// ludcmp:
	// svdcmp
	float ***u;	// svdcmp:
	float **v;	// svdcmp:
	float **y;	// svdcmp:
	float *w;	// svdcmp:
	int ndata;
	
	// 1.0 Prepare
	fnam = tmnam[sel-1];
	if (pfc->argnum<4) {
		sprintf (pout,"Error from %s:  Syntax error\n",fnam);
		gaprnt (0,pout);
		if(sel==4||sel==5) sprintf (pout,"%s(y1,x1,x2,x3,..,xn,time=tbeg,time=tend[,dt,'filename'])\n",fnam);
		else sprintf (pout,"%s(x1,x2,time=tbeg,time=tend [,dt])\n",fnam);
		gaprnt (0,pout);
		return 1;
	}
	
	/* Check for valid number of args       */
	if (sel==1 || sel==2 || sel==3) { nexp=2;
	} else if (sel==4 || sel==5) {
		nexp=0;
		for (i=0; i<pfc->argnum; i++) {
			rc = gaexpr(pfc->argpnt[i],pst);
			gafree (pst);
			if (rc) break;
			nexp++;
		}
	}
	//	sprintf (pout,"%s:  0.0 nexp=%i\n",fnam,nexp);
	//	gaprnt (0,pout);
	ncov=(nexp*nexp-nexp*(nexp-1)/2);	// number of covariance (including variance)
	// n^2-C(n,2)=n^2-n*(n-1)/2
	siz = (int *)malloc(sizeof(int)*nexp);
	udef = (float *)malloc(sizeof(float)*nexp);
	pgr = (struct gagrid **)malloc(sizeof(struct gagrid *)*nexp);
	vv = (float **)malloc(sizeof(float *)*nexp);
	mn = (float **)malloc(sizeof(float *)*nexp);
	cov = (float **)malloc(sizeof(float *)*ncov);	
	r = (float **)malloc(sizeof(float *)*ncov);	
	z = (float **)malloc(sizeof(float *)*(nexp-1));	
	b = (float **)malloc(sizeof(float *)*(nexp-1));	
	sz = (float **)malloc(sizeof(float *)*(nexp-1));	
	sb = (float **)malloc(sizeof(float *)*(nexp-1));	
	pcb = (float **)malloc(sizeof(float *)*(nexp-1));	
	if (NULL==(siz)||NULL==(udef)||NULL==(pgr)||NULL==(vv)||NULL==(mn)||NULL==(cov)||NULL==(r)||NULL==(z)
		||NULL==(b)||NULL==(sz)||NULL==(sb)||NULL==(pcb)) {
		sprintf (pout,"Error from %s:  1.0 memory allocation error\n",fnam);
		gaprnt (0,pout);
		goto err0;
	}
	if (pfc->argnum<nexp+2) {
		sprintf (pout,"Error from %s:  Too many or too few args\n",fnam);
		gaprnt (0,pout);
		sprintf (pout,"    >=%i arguments expected \n",nexp+2);
		gaprnt (0,pout);
		goto err0;
	}
	/* Parse the begining time dimension expression       */
	
	pfi = pst->pfid;
	ch = dimprs (pfc->argpnt[nexp], pst, pfi, &dim, &t1, 1, &wflag);
	if (ch==NULL || dim!=3) {
		sprintf (pout,"Error from %s:  1st time dimension expr invalid\n",fnam);
		gaprnt (0,pout);
		goto err0;
	}
	
	/* Now parse the ending time dimension expression.  */
	
	ch = dimprs (pfc->argpnt[nexp+1], pst, pfi, &dim, &t2, 1, &wflag);
	if (ch==NULL || dim!=3) {
		sprintf (pout,"Error from %s:  2nd time dimension expr invalid\n",fnam);
		gaprnt (0,pout);
		goto err0;
	}
	
	/* Check for 5th argument.  Could be a time increment, or it may
	be option flags.  Time increment only valid for time averaging*/
	
	incr = 1;
	if (pfc->argnum >= nexp+3) {	
		/* Assume time increment */
		if (dim!=3) {
			sprintf(pout,"Error from %s: Invalid usage of increment time value\n",fnam);
			gaprnt(0,pout);
			gaprnt (0,"                Can only be used with time averaging\n");
			goto err0;
		}
		ch = intprs(pfc->argpnt[nexp+2],&incr);
		if (ch==NULL) goto err1;
		
		/* If a relative date/time was given, the increment is obtained
		by looking at the default file structure (which assumes
		knowledge of how date/time conversions are done) */
		
		if (*ch!='\0') {
			ch = rdtprs(pfc->argpnt[nexp+2],&tinc);
			if (ch==NULL) goto err1;
			mos = tinc.yr*12 + tinc.mo;
			mns = tinc.dy*1140 + tinc.hr*60 + tinc.mn;
			val = pfi->grvals[3];
			if (mos>0 && *(val+5)>0) {
				incr = mos / (*(val+5));
				if (mos!=incr*(*(val+5))) goto err1;
			}
			else if (mns>0 && *(val+6)>0) {
				incr = mns / (*(val+6));
				if (mns!=incr*(*(val+6))) goto err1;
			}
			else goto err1;
		}
	}
	
	/* Check for 6th argument: get working directory and filename */	
	if (pfc->argnum >= nexp+4) {	
		getwrd(filename,pfc->argpnt[nexp+3],80);
	}
		
	/* 2.0 Loop through all times and get means */
	
	if (pst->jdim==dim) pst->jdim = -1;
	d1 = ceil(t1-0.001);          /* Loop limits are integers    */
	d2 = floor(t2+0.001);         /* No weighting  */
	sprintf (pout,"%s:  dim = %i, start = %i, end = %i, step= %i\n",fnam,
		dim, d1, d2, incr);
	gaprnt (0,pout);
	
	rc = 0;
	mn[0] = NULL;
	for (d=d1; d<=d2 && !rc; d+=incr) {
		gr2t (pfi->grvals[3],d,&(pst->tmin));
		pst->tmax = pst->tmin;
		for (i=0; i<nexp; i++) {
			rc = gaexpr(pfc->argpnt[i],pst);
			if (rc) {
				for (j=0; j<i-2; j++) if (NULL!=pgr[j]) gagfre (pgr[j]); 
				goto err2;
			}	
			if (!rc && pst->type==0) {
				for (j=0; j<i-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]); 
				goto err2;
			}
			pgr[i] = pst->result.pgr;
			siz[i] = pgr[i]->isiz * pgr[i]->jsiz;
			vv[i] = pgr[i]->grid;
			udef[i] = pgr[i]->undef;
		}
		if (sel==1 || sel==2 || sel==3) {
			if (siz[0]!=1 && siz[0]!=siz[1]) {
				sprintf (pout,"Error from %s:  1st arg must be 0-D or size incompatible!\n",fnam);
				gaprnt (0,pout);
				goto err3;
			}
			size=siz[1];
		} else if (sel==4 || sel==5) {
			for (i=0; i<nexp; i++) {
				if (siz[0]!=siz[i]) {
					sprintf (pout,"Error from %s:  %ist arg size incompatible!\n",fnam,i);
					gaprnt (0,pout);
					goto err3;
				}
			}
			size=siz[1];
		}
		
		if (mn[0]==NULL) {
			if (sel==1) {idim = 2; sizeOut=size;}	// mean*1 + wt*1
			//  nexp=2: mean(2)+cnt(1)+cov(3) =6
			if (sel==2 || sel==3) {idim = nexp+1+ncov; sizeOut=size;}
			// if (sel==4) {idim = 1;}	// dummy
			//  mean(nexp)+cnt(1)+k(1)+cov(ncov)+r(ncov)+z(nexp-1)+b(nexp-1)+corr(1)
			//    +vepl(1)+sse(1)+mse(1)+err(1)+corra(1)+fratio(1)+sz(nexp-1)+sb(nexp-1)+pc(1)
			//    +pcb(nexp-1)
			if (sel==4) {idim = nexp+1+1+ncov+ncov+(nexp-1)+(nexp-1)+1+1+1+1+1+1+1+(nexp-1)+(nexp-1)+1+(nexp-1); sizeOut=1;}
			if (sel==5) {idim = nexp+1+1+ncov+ncov+(nexp-1)+(nexp-1)+1+1+1+1+1+1+1+(nexp-1)+(nexp-1)+1+(nexp-1); sizeOut=size;}
			mn[0] = (float *)malloc(sizeof(float)*sizeOut*idim);
			skipIndex = (int *)malloc(sizeof(int)*sizeOut);
			if (mn[0]==NULL) {
				sprintf (pout,"Error from %s:  Memory allocation error\n",fnam);
				gaprnt (0,pout);			
				goto err3;
			}
			// initialization
			for (i=0; i<sizeOut*idim; i++) *(mn[0]+i) = 0.0;	// setting everything to zero!
			// set pointers
			if (sel==1) {
				wt = mn[0] + sizeOut;
			}
			if (sel==2 || sel==3) {
				for (j=1; j<nexp; j++) mn[j] = mn[0]+j*sizeOut;
				cnt = mn[0]+(nexp)*sizeOut;
				for (j=0; j<ncov; j++) cov[j] = cnt+(1+j)*sizeOut;
			}
			if (sel==4 || sel==5) {
				for (j=1; j<nexp; j++) mn[j] = mn[0]+j*sizeOut;
				cnt = mn[0]+(nexp)*sizeOut;
				{  kexp = cnt+sizeOut;
				   // n. of dependent variables (initialize)
				   for (i=0; i<sizeOut; i++) *(kexp+i) = (float)(nexp-1);
				}
				for (j=0; j<ncov; j++) cov[j] = kexp+(1+j)*sizeOut;
				for (j=0; j<ncov; j++) r[j] = cov[0]+(ncov+j)*sizeOut;
				for (j=0; j<nexp-1; j++) z[j] = r[0]+(ncov+j)*sizeOut;
				for (j=0; j<nexp-1; j++) b[j] = z[0]+((nexp-1)+j)*sizeOut;
				corr = b[0]+(nexp-1)*sizeOut;
				vepl = corr+sizeOut;
				sse = vepl+sizeOut;
				mse = sse+sizeOut;
				err = mse+sizeOut;
				corra = err+sizeOut;
				fratio = corra+sizeOut;
				for (j=0; j<nexp-1; j++) sz[j] = fratio+(1+j)*sizeOut;
				for (j=0; j<nexp-1; j++) sb[j] = sz[0]+((nexp-1)+j)*sizeOut;
				pc = sb[0]+(nexp-1)*sizeOut;
				for (j=0; j<nexp-1; j++) pcb[j] = pc+(1+j)*sizeOut;
			}
			//	sprintf (pout,"3.1: %s allocate memory\n",fnam);
			//	gaprnt (0,pout);
		}
		for (i=0; i<size; i++) {
			// discard undef data
			ldef=1;
			for (j=0; j<nexp; j++) ldef=ldef && *vv[j]!=udef[j];
			//	sprintf (pout,"3.2: i=%i,ldef=%i\n",i,ldef);
			//	gaprnt (0,pout);
			if (ldef) {
				if (sel==1) {
					*(mn[0]+i) += *vv[1] * (*vv[0]);
					*(wt+i) += (*vv[0]);
				}
				if (sel==2 || sel==3|| sel==5) {
					for (j=0; j<nexp; j++) *(mn[j]+i) += *vv[j];
					*(cnt+i) += 1.0;
				}
				if (sel==4) {
					for (j=0; j<nexp; j++) *(mn[j]+0) += *vv[j];
					*(cnt+0) += 1.0;
				}
			}
			if(siz[0]!=1) vv[0]++;
			for (j=1; j<nexp; j++) vv[j]++;
		}
		//	sprintf (pout,"3.3: gafree=%i\n",d);
		//	gaprnt (0,pout);
		for (j=0; j<nexp-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]);
		gafree (pst);
		//	sprintf (pout,"3.4: gafree=%i\n",d);
		//	gaprnt (0,pout);
	}
	
	//	sprintf (pout,"4.0: %s start calculating mean of each time series\n",fnam);
	//	gaprnt (0,pout);
	/* Calculate mean of each time series */
	
	if (mn[0]) {
		if (sel==1) {
			for (i=0; i<sizeOut; i++) {
				if (*(wt+i)>0.0) {
					*(mn[0]+i) /= *(wt+i);
				} else {
					*(mn[0]+i) = udef[1];
				}
			}
		}
		if (sel==2 || sel==3 || sel==4 ||sel==5) {
			for (i=0; i<sizeOut; i++) {
				if (*(cnt+i)>0.0) {
					for (j=0; j<nexp; j++) *(mn[j]+i) /= *(cnt+i);
				} else {
					for (j=0; j<nexp; j++) *(mn[j]+i) = udef[1];
				}
			}
		}
	}
	// sprintf (pout,"4.1: %s claculated mean\n",fnam);
	// gaprnt (0,pout);
	
	/* 3.0 Loop through time again if needed; do squares and cov.
	Less error checking this time through. */
	
	if ((sel==2 || sel==3 || sel==4|| sel==5) && mn[0]) {
		rc = 0;
		for (d=d1; d<=d2 && !rc; d+=incr) {
			gr2t (pfi->grvals[3],d,&(pst->tmin));
			pst->tmax = pst->tmin;
			for (i=0; i<nexp; i++) {
				rc = gaexpr(pfc->argpnt[i],pst);
				if (rc) {
					for (j=0; j<i-2; j++) if (NULL!=pgr[j]) gagfre (pgr[j]); 
					goto err2;
				}	
				pgr[i] = pst->result.pgr;
				vv[i] = pgr[i]->grid;
			}
			for (i=0; i<size; i++) {
				// discard undef data
				ldef=1;
				for (j=0; j<nexp; j++) ldef=(ldef&&(*vv[j]!=udef[j]));
				if (ldef) {
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) {
							if (sel==2 || sel==3 ||sel==5) {
								*(cov[covindex(j,k,nexp)]+i) +=(*vv[j] - *(mn[j]+i))*(*vv[k] - *(mn[k]+i));
							} else if (sel==4){
								*(cov[covindex(j,k,nexp)]+0) +=(*vv[j] - *(mn[j]+0))*(*vv[k] - *(mn[k]+0));
							}
						}
					}
				}
				if(siz[0]!=1) vv[0]++;
				for (j=1; j<nexp; j++) vv[j]++;
			}
			for (j=0; j<nexp-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]);
			gafree (pst);
		}
		if (sel==2 || sel==3) {		
			for (i=0; i<sizeOut; i++) {
				// chk paris of data > degree of freedom
				if (*(cnt+i) > (float)nexp) {
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(cov[covindex(j,k,nexp)]+i) /= *(cnt+i);
					}
					if (sel==2) res = sqrt(*(cov[covindex(0,0,nexp)]+i) * *(cov[covindex(1,1,nexp)]+i));
					if (sel==3) res = sqrt(*(cov[covindex(0,0,nexp)]+i) * *(cov[covindex(0,0,nexp)]+i));
					if (res==0.0) res = udef[1];
					else res = *(cov[covindex(0,1,nexp)]+i)/res;
				} else res = udef[1];
				*(cov[covindex(0,0,nexp)]+i) = res;
			}
		}
		if (sel==4 || sel==5) {		
				// sprintf(pout,"grid unable to fit:\n "); gaprnt(0,pout);
			for (i=0; i<sizeOut; i++) {
				// chk paris of data > degree of freedom
				nskip=0;
				if (*(cnt+i) > (float)nexp) {
					lskip=0;
					luindx= (int *)malloc(sizeof(int)*(nexp-1));
					x= (float *)malloc(sizeof(float)*(nexp-1));
					c=matrix((nexp-1),(nexp-1));
					// covariance and variance
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(cov[covindex(j,k,nexp)]+i) /= *(cnt+i);
					}
					// corr. coeff.
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) {
							tmp=sqrt(*(cov[covindex(j,j,nexp)]+i)**(cov[covindex(k,k,nexp)]+i));
							if (tmp>TINY) *(r[covindex(j,k,nexp)]+i) = *(cov[covindex(j,k,nexp)]+i)/tmp;
							else {lskip=1; *(r[covindex(j,k,nexp)]+i)=udef[1];}
						}
					}
					if (!lskip) {
						// prepare normal equation (Hays, p476)
						for (j=0;j<nexp-1;j++) {
							for (k=0;k<nexp-1;k++) {
								c[j][k]=*(r[covindex(j+1,k+1,nexp)]+i); // note that r[k][j]=r[j][k] 
							}
						}
						// Do LU decomposition
						rc=ludcmpb(c,nexp-1,luindx,&p);
					}
					if (lskip || rc) {
						skipIndex[nskip]=i;
						nskip++;
						// singular matrix	
						free_matrix(c);
						free (x);
						free (luindx);				
						// unable to fit!
						sprintf(pout," %i",i);
						gaprnt(0,pout);
						// z: standardized form (z-score) partial regression coeff.
						for (j=0;j<nexp-1;j++) *(z[j]+i)=udef[1];
						// b: unstandardized partial regression coeff.
						for (j=0;j<nexp-1;j++) *(b[j]+i)=udef[1];
						// multiple correlation coefficient
						*(corr+i)=udef[1];
						// variance explained
						*(vepl+i)=udef[1];
						*(sse+i)=udef[1];
						*(mse+i)=udef[1];
						// err
						*(err+i)=udef[1];
						// adjuested correlation coefficient
						*(corra+i)=udef[1];
						// fratio
						*(fratio+i)=udef[1];
						*(pc+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(sz[j]+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(sb[j]+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(pcb[j]+i)=udef[1];
						continue;
					}
					// Solve equations for each right-hand vector
					for (j=0;j<nexp-1;j++) x[j]=*(r[covindex(0,j+1,nexp)]+i);
					lubksb(c,nexp-1,luindx,x);
					// z: standardized form (z-score) partial regression coeff.
					for (j=0;j<nexp-1;j++) *(z[j]+i)=x[j];
					// b: unstandardized partial regression coeff. (Hays, 1980?, 13.18.7, p: 476)
					for (j=0;j<nexp-1;j++) *(b[j]+i)=x[j]*sqrt(*(cov[covindex(0,0,nexp)]+i)/(*(cov[covindex(j+1,j+1,nexp)]+i)));
					// multiple correlation coefficient
					*(corr+i)=0.;
					for (j=0;j<nexp-1;j++) *(corr+i)+=x[j]*(*(r[covindex(0,j+1,nexp)]+i));
					*(corr+i)=sqrt(*(corr+i));
					// variance explained
					*(vepl+i)=pow(*(corr+i),2);
					// sum square error (sse) (Hays, 1980?, 13.20.2, p: 484)
					*(sse+i)=*(cov[covindex(0,0,nexp)]+i)*(1-*(vepl+i));
					// mean square error (mse) (Hays, 1980?, 13.20.3, p: 484)
					*(mse+i)=*(sse+i)/(*(cnt+i)-nexp);
					// unbiased std error (Hays, 1980?, 13.18.17, p: 480)
					*(err+i)=sqrt(*(cnt+i)/(*(cnt+i)-nexp)**(sse+i));
//					*(err+i)=sqrt(*(cnt+i)/(*(cnt+i)-nexp)* *(cov[covindex(0,0,nexp)]+i))*sqrt(1-*(vepl+i));
					// adjusted correlation (Hays, 1980?, 13.18.18, p: 480)
					*(corra+i)=sqrt(1.-(*(cnt+i)-1.)/(*(cnt+i)-nexp)*(1.-*(vepl+i)));
					// fratio (Hays, 1980?, 13.20.5, p: 484)
					// f=msr/mse=(corr^2/(nexp-1))/((1-corr^2)/(n-nexp))
					//  =(vepl/(nexp-1))/((1-vepl)/(n-nexp))  				
					*(fratio+i)=(*(vepl+i)/(nexp-1))/((1-*(vepl+i))/(*(cnt+i)-nexp));
					*(pc+i)=ftestb(*(fratio+i),(float)(nexp-1),*(cnt+i)-(float)nexp);
					// std of a and b coeff. (Rice, 1988, p. 488)
					// Inverse of normal maxtrix (correlation matrix)
					for (j=0;j<nexp-1;j++) {
						for (k=0;k<nexp-1;k++) {
							if (j==k) x[k]=1.;
							else x[k]=0.;
						}	
						lubksb(c,nexp-1,luindx,x);
						*(sz[j]+i)=sqrt(x[j]*(1-*(vepl+i))/(*(cnt+i)-nexp));
						*(sb[j]+i)=*(sz[j]+i)*sqrt(*(cov[covindex(0,0,nexp)]+i)/(*(cov[covindex(j+1,j+1,nexp)]+i)));
						*(pcb[j]+i)=ttestb(*(b[j]+i)/(*(sb[j]+i)),*(cnt+i)-(float)nexp);
					}
					free_matrix(c);
					free (x);
					free (luindx);				
//					sprintf(pout,"i=%i, r=%g\n",i,*(corra+i));
//					gaprnt(0,pout);
				} else {
					// all missing data, unable to fit!
					// covariance and vaiance
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(cov[covindex(j,k,nexp)]+i) = udef[1];
					}
					// corr. coeff.
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(r[covindex(j,k,nexp)]+i) = udef[1];
					}
					// z: standardized form (z-score) partial regression coeff.
					for (k=0;k<nexp-1;k++) *(z[k]+i)=udef[1];
					// b: unstandardized partial regression coeff.
					for (k=0;k<nexp-1;k++) *(b[k]+i)=udef[1];
					// multiple correlation coefficient
					*(corr+i)=udef[1];
					// variance explained
					*(vepl+i)=udef[1];
					*(sse+i)=udef[1];
					*(mse+i)=udef[1];
					// err
					*(err+i)=udef[1];
					// adjusted correlation coefficient
					*(corra+i)=udef[1];
					// fratio
					*(fratio+i)=udef[1];
					*(pc+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(sz[k]+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(sb[k]+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(pcb[k]+i)=udef[1];
				}
			}	
			sprintf(pout,"\n");
			gaprnt(0,pout);
			if (sel==4) {		
				// print
				sprintf(pout,"Pairs of defined data: %i\n",(int)*(cnt+0));
				gaprnt(0,pout);
				sprintf(pout,"Covariance matrix:\n");
				gaprnt(0,pout);
				for (k=0;k<nexp;k++) {
					if (k==0) sprintf(pout,"\tY\t");
					else sprintf(pout,"X%i\t",k);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				for (j=0;j<nexp;j++) {
					for (k=0;k<nexp;k++) {
						if (k==0) {
							if (j==0) sprintf(pout,"Y\t");
							else sprintf(pout,"X%i\t",j);
							gaprnt(0,pout);
						}
						sprintf(pout,"%g\t",*(cov[covindex(j,k,nexp)]+0));
						gaprnt(0,pout);
					}
					gaprnt(0,"\n");
				}		
				// print					
				sprintf(pout,"Correlation matrix:\n");
				gaprnt(0,pout);
				for (k=0;k<nexp;k++) {
					if (k==0) sprintf(pout,"\tY\t");
					else sprintf(pout,"X%i\t",k);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				for (j=0;j<nexp;j++) {
					for (k=0;k<nexp;k++) {
						if (k==0) {
							if (j==0) sprintf(pout,"Y\t");
							else sprintf(pout,"X%i\t",j);
							gaprnt(0,pout);
						}
						sprintf(pout,"%g\t",*(r[covindex(j,k,nexp)]+0));
						gaprnt(0,pout);
					}
					gaprnt(0,"\n");
				}		
				sprintf(pout,"Standardized form (z-score):\nZ = ");
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g [ %g ]*X%i%",*(z[k]+0),*(sz[k]+0),k+1);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				// gb: unstandardized partial regression coeff.
				sprintf(pout,"Raw score regression eq.:\nY = %g ",*(mn[0]+0));
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g [ %g , %g% ]*(X%i %+g )",*(b[k]+0),*(sb[k]+0),*(pcb[k]+0)*100,k+1,-(*mn[k+1]+0));
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				// multiple correlation coefficient
				sprintf(pout,"Correlation coefficient = %g\n",*(corr+0));
				gaprnt(0,pout);
				// adjusted correlation coefficient
				sprintf(pout,"Adjusted correlation coefficient = %g\n",*(corra+0));
				gaprnt(0,pout);
				sprintf(pout,"Variance explained= ");
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g*( %+g )",*(z[k]+0),*(r[covindex(0,k+1,nexp)]+0));
					gaprnt(0,pout);
				}
				sprintf(pout,"= %g\n",*(vepl+0));
				gaprnt(0,pout);
				sprintf(pout,"unbiased std err. = %g\n",*(err+0));
				gaprnt(0,pout);
				// F ratio
				sprintf(pout,"F ratio = %g\n",*(fratio+0));
				gaprnt(0,pout);
				sprintf(pout,"Probability of confidence level = %g%\n",*(pc+0)*100);
				gaprnt(0,pout);
								
			}
			if (!cmpwrd("null",filename)) {
				strcpy(datafilename,filename);
				strcat(datafilename,".bin");			
				fpdata=fopen(datafilename,"w+b");
				if(!fpdata) {
					sprintf (pout,"Error from %s:  open %s file error!\n",fnam,datafilename);
					gaprnt (0,pout);			
				}			
				// write bin file
				sprintf (pout,"%s:  write %s file!\n",fnam,datafilename);gaprnt (0,pout);			
				if (0==fwrite(mn[0],sizeof(float),size*idim,fpdata)) {
					sprintf (pout,"Error from %s:  write %s file error!\n",fnam,datafilename);
					gaprnt (0,pout);			
				}
				fclose(fpdata);
			}
		}
	}
	// sprintf (pout,"5.1: %s claculated covariace\n",fnam);
	// gaprnt (0,pout);
	
	/* 4.0 Loop through time again for grid can not fit by normal equation;
	   do sigular value decomposition. */
	   
	// read data
	if (sel==4 && mn[0] && nskip) {
		u=tensor(1,nskip*(d2-d1+1),(nexp-1));
		y=matrix(1,nskip*(d2-d1+1));
		rc = 0;
		for (ndata=0, d=d1; d<=d2 && !rc; d+=incr) {
			gr2t (pfi->grvals[3],d,&(pst->tmin));
			pst->tmax = pst->tmin;
			for (i=0; i<nexp; i++) {
				rc = gaexpr(pfc->argpnt[i],pst);
				if (rc) {
					for (j=0; j<i-2; j++) if (NULL!=pgr[j]) gagfre (pgr[j]); 
					goto err2;
				}	
				pgr[i] = pst->result.pgr;
				vv[i] = pgr[i]->grid+skipIndex[0];	// first date
			}
			for (i=0; i<nskip; i++, ndata++) {
				y[0][ndata]= *vv[0];
				for (j=1; j<nexp; j++) u[0][ndata][j]= *vv[j];					
				for (j=0; j<nexp; j++) if(siz[j]!=1) vv[j] = pgr[j]->grid+skipIndex[i];
			}
			for (j=0; j<nexp-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]);
			gafree (pst);
		}
	}	
	if (sel==5 && mn[0] && nskip) {
		u=tensor(nskip,(d2-d1+1),(nexp-1));
		y=matrix(nskip,(d2-d1+1));
		rc = 0;
		for (ndata=0, d=d1; d<=d2 && !rc; d+=incr, ndata++) {
			gr2t (pfi->grvals[3],d,&(pst->tmin));
			pst->tmax = pst->tmin;
			for (i=0; i<nexp; i++) {
				rc = gaexpr(pfc->argpnt[i],pst);
				if (rc) {
					for (j=0; j<i-2; j++) if (NULL!=pgr[j]) gagfre (pgr[j]); 
					goto err2;
				}	
				pgr[i] = pst->result.pgr;
				vv[i] = pgr[i]->grid+skipIndex[0];	// first date
			}
			for (i=0; i<nskip; i++) {
				y[i][ndata]= *vv[0];
				for (j=1; j<nexp; j++) u[i][ndata][j]= *vv[j];					
				for (j=0; j<nexp; j++) if(siz[j]!=1) vv[j] = pgr[j]->grid+skipIndex[i];
			}
			for (j=0; j<nexp-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]);
			gafree (pst);
		}
	}	
	if ((sel==4|| sel==5) && mn[0] && nskip) {
		if (sel==4) {
			/*
			rc=svdcmpb(u[0],ndata,nexp-1,w,v);
			wmax=0.0;
			for (j=0;j<ma;j++)
				if (w[j] > wmax) wmax=w[j];
			thresh=TOL*wmax;
			for (j=0;j<ma;j++)
				if (w[j] < thresh) w[j]=0.0;
			svbksb(u[0],w,v,ndata,ma,b,a);
			*/
			/*
			*chisq=0.0;
			for (i=0;i<ndata;i++) {
				(*funcs)(x[i],afunc,ma);
				sum=0.0;
				for (j=0;j<ma;j++) sum += a[j]*afunc[j];
				*chisq += (tmp=(y[i]-sum)/sig[i],tmp*tmp);
			}
			free_vector(afunc);
			free_vector(b);
			*/
			
		} else if (sel==5) {
			for (i=0; i<nskip; i++) {
				rc=svdcmpb(u[i],ndata,nexp-1,w,v);
			}
		}
			
		 {		
		// 	sprintf(pout,"grid unable to fit:\n ");	gaprnt(0,pout);
			for (i=0; i<sizeOut; i++) {
				// chk paris of data > degree of freedom
				nskip=0;
				if (*(cnt+i) > (float)nexp) {
					lskip=0;
					luindx= (int *)malloc(sizeof(int)*(nexp-1));
					x= (float *)malloc(sizeof(float)*(nexp-1));
					c=matrix((nexp-1),(nexp-1));
					// covariance and variance
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(cov[covindex(j,k,nexp)]+i) /= *(cnt+i);
					}
					// corr. coeff.
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) {
							tmp=sqrt(*(cov[covindex(j,j,nexp)]+i)**(cov[covindex(k,k,nexp)]+i));
							if (tmp>TINY) *(r[covindex(j,k,nexp)]+i) = *(cov[covindex(j,k,nexp)]+i)/tmp;
							else {lskip=1; *(r[covindex(j,k,nexp)]+i)=udef[1];}
						}
					}
					if (!lskip) {
						// prepare normal equation (Hays, p476)
						for (j=0;j<nexp-1;j++) {
							for (k=0;k<nexp-1;k++) {
								c[j][k]=*(r[covindex(j+1,k+1,nexp)]+i); // note that r[k][j]=r[j][k] 
							}
						}
						// Do LU decomposition
						rc=ludcmpb(c,nexp-1,luindx,&p);
					}
					if (lskip || rc) {
						skipIndex[nskip]=i;
						nskip++;
						// singular matrix	
						free_matrix(c);
						free (x);
						free (luindx);				
						// unable to fit!
						sprintf(pout," %i",i);
						gaprnt(0,pout);
						// z: standardized form (z-score) partial regression coeff.
						for (j=0;j<nexp-1;j++) *(z[j]+i)=udef[1];
						// b: unstandardized partial regression coeff.
						for (j=0;j<nexp-1;j++) *(b[j]+i)=udef[1];
						// multiple correlation coefficient
						*(corr+i)=udef[1];
						// variance explained
						*(vepl+i)=udef[1];
						*(sse+i)=udef[1];
						*(mse+i)=udef[1];
						// err
						*(err+i)=udef[1];
						// adjuested correlation coefficient
						*(corra+i)=udef[1];
						// fratio
						*(fratio+i)=udef[1];
						*(pc+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(sz[j]+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(sb[j]+i)=udef[1];
						for (j=0;j<nexp-1;j++) *(pcb[j]+i)=udef[1];
						continue;
					}
					// Solve equations for each right-hand vector
					for (j=0;j<nexp-1;j++) x[j]=*(r[covindex(0,j+1,nexp)]+i);
					lubksb(c,nexp-1,luindx,x);
					// z: standardized form (z-score) partial regression coeff.
					for (j=0;j<nexp-1;j++) *(z[j]+i)=x[j];
					// b: unstandardized partial regression coeff. (Hays, 1980?, 13.18.7, p: 476)
					for (j=0;j<nexp-1;j++) *(b[j]+i)=x[j]*sqrt(*(cov[covindex(0,0,nexp)]+i)/(*(cov[covindex(j+1,j+1,nexp)]+i)));
					// multiple correlation coefficient
					*(corr+i)=0.;
					for (j=0;j<nexp-1;j++) *(corr+i)+=x[j]*(*(r[covindex(0,j+1,nexp)]+i));
					*(corr+i)=sqrt(*(corr+i));
					// variance explained
					*(vepl+i)=pow(*(corr+i),2);
					// sum square error (sse) (Hays, 1980?, 13.20.2, p: 484)
					*(sse+i)=*(cov[covindex(0,0,nexp)]+i)*(1-*(vepl+i));
					// mean square error (mse) (Hays, 1980?, 13.20.3, p: 484)
					*(mse+i)=*(sse+i)/(*(cnt+i)-nexp);
					// unbiased std error (Hays, 1980?, 13.18.17, p: 480)
					*(err+i)=sqrt(*(cnt+i)/(*(cnt+i)-nexp)**(sse+i));
//					*(err+i)=sqrt(*(cnt+i)/(*(cnt+i)-nexp)* *(cov[covindex(0,0,nexp)]+i))*sqrt(1-*(vepl+i));
					// adjusted correlation (Hays, 1980?, 13.18.18, p: 480)
					*(corra+i)=sqrt(1.-(*(cnt+i)-1.)/(*(cnt+i)-nexp)*(1.-*(vepl+i)));
					// fratio (Hays, 1980?, 13.20.5, p: 484)
					// f=msr/mse=(corr^2/(nexp-1))/((1-corr^2)/(n-nexp))
					//  =(vepl/(nexp-1))/((1-vepl)/(n-nexp))  				
					*(fratio+i)=(*(vepl+i)/(nexp-1))/((1-*(vepl+i))/(*(cnt+i)-nexp));
					*(pc+i)=ftestb(*(fratio+i),(float)(nexp-1),*(cnt+i)-(float)nexp);
					// std of a and b coeff. (Rice, 1988, p. 488)
					// Inverse of normal maxtrix (correlation matrix)
					for (j=0;j<nexp-1;j++) {
						for (k=0;k<nexp-1;k++) {
							if (j==k) x[k]=1.;
							else x[k]=0.;
						}	
						lubksb(c,nexp-1,luindx,x);
						*(sz[j]+i)=sqrt(x[j]*(1-*(vepl+i))/(*(cnt+i)-nexp));
						*(sb[j]+i)=*(sz[j]+i)*sqrt(*(cov[covindex(0,0,nexp)]+i)/(*(cov[covindex(j+1,j+1,nexp)]+i)));
						*(pcb[j]+i)=ttestb(*(b[j]+i)/(*(sb[j]+i)),*(cnt+i)-(float)nexp);
					}
					free_matrix(c);
					free (x);
					free (luindx);				
//					sprintf(pout,"i=%i, r=%g\n",i,*(corra+i));
//					gaprnt(0,pout);
				} else {
					// all missing data, unable to fit!
					// covariance and vaiance
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(cov[covindex(j,k,nexp)]+i) = udef[1];
					}
					// corr. coeff.
					for (j=0; j<nexp; j++) {
						for (k=j; k<nexp; k++) *(r[covindex(j,k,nexp)]+i) = udef[1];
					}
					// z: standardized form (z-score) partial regression coeff.
					for (k=0;k<nexp-1;k++) *(z[k]+i)=udef[1];
					// b: unstandardized partial regression coeff.
					for (k=0;k<nexp-1;k++) *(b[k]+i)=udef[1];
					// multiple correlation coefficient
					*(corr+i)=udef[1];
					// variance explained
					*(vepl+i)=udef[1];
					*(sse+i)=udef[1];
					*(mse+i)=udef[1];
					// err
					*(err+i)=udef[1];
					// adjusted correlation coefficient
					*(corra+i)=udef[1];
					// fratio
					*(fratio+i)=udef[1];
					*(pc+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(sz[k]+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(sb[k]+i)=udef[1];
					for (k=0;k<nexp-1;k++) *(pcb[k]+i)=udef[1];
				}
			}	
			sprintf(pout,"\n");
			gaprnt(0,pout);
			if (sel==4) {		
				// print
				sprintf(pout,"Pairs of defined data:	%i\n",(int)*(cnt+0));
				gaprnt(0,pout);
				sprintf(pout,"Covariance matrix:\n");
				gaprnt(0,pout);
				for (k=0;k<nexp;k++) {
					if (k==0) sprintf(pout,"\tY\t");
					else sprintf(pout,"X%i\t",k);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				for (j=0;j<nexp;j++) {
					for (k=0;k<nexp;k++) {
						if (k==0) {
							if (j==0) sprintf(pout,"Y\t");
							else sprintf(pout,"X%i\t",j);
							gaprnt(0,pout);
						}
						sprintf(pout,"%g\t",*(cov[covindex(j,k,nexp)]+0));
						gaprnt(0,pout);
					}
					gaprnt(0,"\n");
				}		
				// print					
				sprintf(pout,"Correlation matrix:\n");
				gaprnt(0,pout);
				for (k=0;k<nexp;k++) {
					if (k==0) sprintf(pout,"\tY\t");
					else sprintf(pout,"X%i\t",k);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				for (j=0;j<nexp;j++) {
					for (k=0;k<nexp;k++) {
						if (k==0) {
							if (j==0) sprintf(pout,"Y\t");
							else sprintf(pout,"X%i\t",j);
							gaprnt(0,pout);
						}
						sprintf(pout,"%g\t",*(r[covindex(j,k,nexp)]+0));
						gaprnt(0,pout);
					}
					gaprnt(0,"\n");
				}		
				sprintf(pout,"Standardized form (z-score):\nZ = ");
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g[%g]*X%i%",*(z[k]+0),*(sz[k]+0),k+1);
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				// gb: unstandardized partial regression coeff.
				sprintf(pout,"Raw score regression eq.:\nY = %g",*(mn[0]+0));
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g[%g,%g%]*(X%i%+g)",*(b[k]+0),*(sb[k]+0),*(pcb[k]+0)*100,k+1,-(*mn[k+1]+0));
					gaprnt(0,pout);
				}
				gaprnt(0,"\n");
				// multiple correlation coefficient
				sprintf(pout,"Correlation coefficient = %g\n",*(corr+0));
				gaprnt(0,pout);
				// adjusted correlation coefficient
				sprintf(pout,"Adjusted correlation coefficient = %g\n",*(corra+0));
				gaprnt(0,pout);
				sprintf(pout,"Variance explained=");
				gaprnt(0,pout);
				for (k=0;k<nexp-1;k++) {
					sprintf(pout,"%+g*(%+g)",*(z[k]+0),*(r[covindex(0,k+1,nexp)]+0));
					gaprnt(0,pout);
				}
				sprintf(pout,"=%g\n",*(vepl+0));
				gaprnt(0,pout);
				sprintf(pout,"unbiased std err. = %g\n",*(err+0));
				gaprnt(0,pout);
				// F ratio
				sprintf(pout,"F ratio = %g\n",*(fratio+0));
				gaprnt(0,pout);
				sprintf(pout,"Probability of confidence level = %g%\n",*(pc+0)*100);
				gaprnt(0,pout);
								
			}
			if (!cmpwrd("null",filename)) {
				strcpy(datafilename,filename);
				strcat(datafilename,".bin");			
				fpdata=fopen(datafilename,"w+b");
				if(!fpdata) {
					sprintf (pout,"Error from %s:  open %s file error!\n",fnam,datafilename);
					gaprnt (0,pout);			
				}			
				// write bin file
				sprintf (pout,"%s:  write %s file!\n",fnam,datafilename);gaprnt (0,pout);			
				if (0==fwrite(mn[0],sizeof(float),size*idim,fpdata)) {
					sprintf (pout,"Error from %s:  write %s file error!\n",fnam,datafilename);
					gaprnt (0,pout);			
				}
				fclose(fpdata);
			}
		}
		free_matrix(y);
		free_tensor(u);
	}
	// sprintf (pout,"5.1: %s claculated covariace\n",fnam);
	// gaprnt (0,pout);

	/* 5.0 Get one final grid, and use it to return the result. */
	gr2t (pfi->grvals[3],d1,&(pst->tmin));
	pst->tmax = pst->tmin;
	rc = gaexpr(pfc->argpnt[1],pst);
	if (rc) goto err2;
	if (!rc && pst->type==0) {
		gafree (pst);
		goto err2;
	}
	if ((sel==4||sel==5)&&!cmpwrd("null",filename)) {
		strcpy(ctlfilename,filename);
		strcat(ctlfilename,".ctl");
		fpctl=fopen(ctlfilename,"w");
		if(!fpctl) {
			sprintf (pout,"Error from %s:  open %s file error!\n",fnam,ctlfilename);
			gaprnt (0,pout);			
		}			
		// write ctl file
		sprintf (pout,"%s:  write %s file!\n",fnam,ctlfilename);gaprnt (0,pout);			
		fprintf(fpctl,"dset %s\n",datafilename);			
		fprintf(fpctl,"undef %e\n",udef[1]);
//		fprintf(fpctl,"title %s: dim = %i, start = %i, end = %i, step= %i\n",fnam,dim,d1,d2,incr);
		fprintf(fpctl,"title %s(",fnam);		
		for (i=0; i<pfc->argnum; i++) {
			if (i!= pfc->argnum-1) fprintf(fpctl,"%s,",pfc->argpnt[i]);
			else fprintf(fpctl,"%s",pfc->argpnt[i]);
		}
		fprintf(fpctl,")\n");
		if (sel==4) {
			fprintf(fpctl,"xdef %d linear %g %g\n",1,1.,1.);
			fprintf(fpctl,"ydef %d linear %g %g\n",1,1.,1.);
		} else if (sel==5) {			
			fprintf(fpctl,"xdef %d linear %g %g\n",pst->result.pgr->isiz,
				pst->result.pgr->igrab(pst->result.pgr->ivals,pst->result.pgr->dimmin[pst->result.pgr->idim])
				,*(pst->result.pgr->ivals));
			if(pst->result.pgr->jlinr==1){
				// linear scaling info
				fprintf(fpctl,"ydef %d linear %g %g\n",pst->result.pgr->jsiz,
					pst->result.pgr->igrab(pst->result.pgr->jvals,pst->result.pgr->dimmin[pst->result.pgr->jdim])
					,*(pst->result.pgr->jvals));
			} else {
				// non-linear scaling info		
				fprintf(fpctl,"ydef %d levels  ",pst->result.pgr->jsiz);
				for (j=0; j<pst->result.pgr->jsiz; j++) {
					fprintf(fpctl,"%g ",pst->result.pgr->jvals[j+1]);
					if (0==(j+1)%10) {
						fprintf(fpctl,"\n");
					}
				}
				fprintf(fpctl,"\n");
			}
		}
		fprintf(fpctl,"zdef 1 linear 0 1\n");
		if (*(pfi->grvals[3]+5)>0) {
//			sprintf(pout,"dmon=%g\n",*(pfi->grvals[3]+5));gaprnt(0,pout);
//			sprintf(pout,"dmn=%g\n",*(pfi->grvals[3]+6));gaprnt(0,pout);
			fprintf(fpctl,"tdef %d linear %s%i %imo\n",1,mon[pst->tmin.mo-1],pst->tmin.yr,(int)(((float)*(pfi->grvals[3]+5))*(t2-t1+1)));
		} else {
			fprintf(fpctl,"tdef %d linear %i:%iZ%i%s%i %imn\n",1,pst->tmin.hr,pst->tmin.mn,
				pst->tmin.dy,mon[pst->tmin.mo-1],pst->tmin.yr,(int)(((float)*(pfi->grvals[3]+6))*(t2-t1+1)));
		}						
		fprintf(fpctl,"vars %i\n",idim);
		// write covariance and variance
		for (j=0; j<nexp; j++) {
			if (j==0) fprintf(fpctl,"y 0 99 mean of y\n");
			else fprintf(fpctl,"x%i 0 99 mean of x\n",j);
		}
		// pairs of defined data
		fprintf(fpctl,"n 0 99 pairs of defined data\n");
		fprintf(fpctl,"k 0 99 number of dependent variables\n");
		for (i=0; i<nexp; i++) {
			for (j=i; j<nexp; j++) {
				if(i==0 && j==0) {fprintf(fpctl,"varyy 0 99 variance of y\n");}
				else if(i==0 && j!=0) {fprintf(fpctl,"covy%i 0 99 covariance between y and x%i\n",j,j);}
				else if(i==j) {fprintf(fpctl,"var%i 0 99 variance of x%i\n",i,i);}
				else {fprintf(fpctl,"cov%i%i 0 99 covariance between x%i and x%i\n",i,j,i,j);}
			}
		}
		// corr. coeff.
		for (i=0; i<nexp; i++) {
			for (j=i; j<nexp; j++) {
				if(i==0 && j==0) {fprintf(fpctl,"ryy 0 99 corr coeff of y (=1)\n");}
				else if(i==0 && j!=0) {fprintf(fpctl,"ry%i 0 99 corr coeff between y and x%i [-1,1]\n",j,j);}
				else {fprintf(fpctl,"r%i%i 0 99 corr coeff between x%i and x%i [-1,1]\n",i,j,i,j);}
			}
		}
		// z: standardized form (z-score) partial regression coeff.
		for (k=1;k<nexp;k++)  {fprintf(fpctl,"z%i 0 99 standardized partial regression coeff x%i\n",k,k);}
		// b: unstandardized partial regression coeff.
		for (k=1;k<nexp;k++)  {fprintf(fpctl,"b%i 0 99 unstandardized partial regression coeff x%i\n",k,k);}
		// multiple correlation coefficient
		fprintf(fpctl,"corr 0 99 corr coeff of multiple linear regression [-1,1]\n");
		// variance explained
		fprintf(fpctl,"vepl 0 99 variance explained by multiple linear regression (0-1)\n");
		// sse
		fprintf(fpctl,"sse 0 99 sum of square error\n");
		// mse
		fprintf(fpctl,"mse 0 99 mean square error\n");
		// err
		fprintf(fpctl,"stderr 0 99 unbiased stderr of multiple linear regression\n");
		// adjusted correlation coefficient
		fprintf(fpctl,"corra 0 99 adjusted corr coeff of multiple linear regression [-1,1]\n");
		// fratio
		fprintf(fpctl,"fratio 0 99 F ratio of multiple linear regression\n");
		for (k=1;k<nexp;k++)  {fprintf(fpctl,"sz%i 0 99 std dev of partial regression coeff z%i\n",k,k);}
		for (k=1;k<nexp;k++)  {fprintf(fpctl,"sb%i 0 99 std dev of partial regression coeff x%i\n",k,k);}
		fprintf(fpctl,"pc 0 99 prob of confidence level for the multiple linear regression\n");
		for (k=1;k<nexp;k++)  {fprintf(fpctl,"pcb%i 0 99 prob of confidence level for b%i\n",k,k);}
		// end of ctl file
		fprintf(fpctl,"endvars\n");
		fclose(fpctl);
	}		
	if (mn[0]) {
		if (size != siz[1]) {
			gafree (pst);
			goto err2;
		}
		if (sel==1) {
			for (i=0; i<size; i++) *(pst->result.pgr->grid+i) = *(mn[0]+i);
		}
		if (sel==2 || sel==3) {
			for (i=0; i<size; i++) *(pst->result.pgr->grid+i) = *(cov[covindex(0,0,nexp)]+i);
		}
		if (sel==5) {
			for (i=0; i<size; i++) *(pst->result.pgr->grid+i) = *(corr+i);
		}
		free (mn[0]);
	} else {
		for (i=0; i<size; i++) *(pst->result.pgr->grid+i) = pst->result.pgr->undef;
	}
	if (NULL!=pcb) free (pcb);
	if (NULL!=sb) free (sb);
	if (NULL!=sz) free (sz);
	if (NULL!=b) free (b);
	if (NULL!=z) free (z);
	if (NULL!=r) free (r);
	if (NULL!=cov) free (cov);
	if (NULL!=mn) free (mn);
	if (NULL!=vv) free (vv);
	if (NULL!=pgr) free (pgr);
	if (NULL!=udef) free (udef);
	if (NULL!=siz) free (siz);
	//	sprintf (pout,"6.1: %s return\n",fnam);
	//	gaprnt (0,pout);
	return (0);
	
err0:
	if (NULL!=pcb) free (pcb);
	if (NULL!=sb) free (sb);
	if (NULL!=sz) free (sz);
	if (NULL!=b) free (b);
	if (NULL!=z) free (z);
	if (NULL!=r) free (r);
	if (NULL!=cov) free (cov);
	if (NULL!=mn) free (mn);
	if (NULL!=vv) free (vv);
	if (NULL!=pgr) free (pgr);
	if (NULL!=udef) free (udef);
	if (NULL!=siz) free (siz);
	return (1);
err1:
	sprintf(pout,"Error from %s: Invalid time increment argument\n",fnam);
	gaprnt(0,pout);
	goto err0;
err2:
	sprintf (pout,"Error from %s:  Error getting grids\n",fnam);
	gaprnt (0,pout);
	if (NULL!=pst) gafree (pst);
	if (mn[0]) free(mn[0]);
	goto err0;
err3:
	for (j=0; j<nexp-1; j++) if (NULL!=pgr[j]) gagfre (pgr[j]);
	if (NULL!=pst) gafree (pst);
	if (mn[0]) free(mn[0]);
	goto err0;
}


float sign(float val) {
	if (0.==val) {
		return (0.);
	} else {
		return (val/fabs(val));
	}	
}


int bottPsiI(int j, int n, float *cn, float *conc, float undef, float *iMinus, float *iPlus, float *w) {
	float a[5];
	float cPlus;
	float cMinus;
	int k;
	int lorder;
	if (undef==conc[j]||undef==cn[j-1]||undef==cn[j]) {
		iMinus[j-1]=undef;
		iPlus[j]=undef;
		w[j]=undef;
		return 0;
	} else if (undef==conc[j-1]&&undef==conc[j+1]) {
		// Bott, 1989a: Table 1
		lorder=0;
		a[0]=conc[j];
	} else if (undef==conc[j-1]) {
		// Bott, 1989a: Table 1
		lorder=1;
		a[0]=conc[j];
		a[1]=(conc[j+1]-conc[j]);
	} else if (undef==conc[j+1]) {
		// Bott, 1989a: Table 1
		lorder=1;
		a[0]=conc[j];
		a[1]=(conc[j]-conc[j-1]);
	} else if (-1==j||n==j||undef==conc[j+2]||undef==conc[j-2]) {
		// Bott, 1989b: Table 1
		lorder=2;
		a[0]=-(conc[j+1]-26.*conc[j]+conc[j-1])/24.;
		a[1]=(conc[j+1]-conc[j-1])/2.;
		a[2]=(conc[j+1]-2*conc[j]+conc[j-1])/2.;
	} else {
		// Bott, 1989b: Table 1
		lorder=4;
		a[0]=(9.*(conc[j+2]+conc[j-2])-116.*(conc[j+1]+conc[j-1])+2134.*conc[j])/1920.;
		a[1]=(-5.*(conc[j+2]-conc[j-2])+34.*(conc[j+1]-conc[j-1]))/48.;
		a[2]=(-3*(conc[j+2]+conc[j-2])+36.*(conc[j+1]+conc[j-1])-66.*conc[j])/48.;
		a[3]=((conc[j+2]-conc[j-2])-2.*(conc[j+1]-conc[j-1]))/12.;
		a[4]=((conc[j+2]+conc[j-2])-4.*(conc[j+1]+conc[j-1])+6.*conc[j])/24.;
	}
	// cjPlus & cjMinus: B89a: Eq (4+)
	cPlus=(cn[j]+fabs(cn[j]))/2.;
	cMinus=-(cn[j-1]-fabs(cn[j-1]))/2.;
	// B89a: Eq (7)
	iPlus[j]=0.;
	for (k=0; k<=lorder; k++) {
		iPlus[j]+=(a[k]/(k+1)/pow(2,k+1)*(1.-pow(1.-2.*cPlus,k+1)));
	}
	iPlus[j]=max(0.,iPlus[j]);	// Plus conc only
	// B89a: Eq (8)
	iMinus[j-1]=0.;	// Plus conc only
	for (k=0; k<=lorder; k++) {
		iMinus[j-1]+=(a[k]/(k+1)/pow(2,k+1)*pow(-1,k)*(1.-pow(1.-2.*cMinus,k+1)));
	}
	iMinus[j-1]=max(0.,iMinus[j-1]);	// Plus conc only
	//
	// B89a: Eq (11)
	w[j]=0.;
	for (k=0; k<=lorder; k++) {
		w[j]+=(a[k]/(k+1)/pow(2,k+1)*(pow(-1,k)+1.));
	}
	w[j]=max(max(w[j],iPlus[j]+iMinus[j-1]),1.e-10);
	w[j]=conc[j]/w[j];
	return 1;
}
int bott(float *cn, int n, int wrap, float undef, float *conc, float *dcdt, int iDir) {
//           BOTT scheme: calculate advected conc increment
//
//  Bott, A., 1989a: A positive definite advection schme obtained by nonlinear renormalization
//    of the advection flux. Monthly Weather Review, 117:1006-1015. 
//  Bott, A., 1989b: Notes and correspondance. Monthly Weather Review, 117: 2633-2636.
//  where
//   cn = Courant number;
//   conc = a nondifusive quantity;
//   n = number of the dimension;
//   wrap = the grid globally 'wraps' in X
//  Original from Ken Chang for Taiwan Air Quality Model (TAQM)
//  Convert to C by b-j. tsuang 2003/6/15
//
	float *cn4;
	float *conc4;
	float *iMinus;
	float *iPlus;
	float *w;
	float *array[5];
	int i,j;
	//	0.0  allocatate working space
	for (i=0; i<5; i++) {
		array[i]=NULL;
		array[i] = (float *)malloc((n+4)*sizeof(float));
		if (NULL==array[i]) goto merr;
	}
	//  1.0 shift index from [0, n+3] to [-2, n+1]
	iPlus=array[0]+2;
	iMinus=array[1]+2;
	w=array[2]+2;
	conc4=array[3]+2;
	cn4=array[4]+2;
	// 2.0 setup boundary conditions
	if (wrap) {
		// Conc
		conc4[-2]=conc[n-2];
		conc4[-1]=conc[n-1];
		for (i=0; i<n; i++) {
			conc4[i]=conc[i];
		}
		conc4[n]=conc[0];
		conc4[n+1]=conc[1];
		// Courant number
		cn4[-2]=cn[n-2];
		cn4[-1]=cn[n-1];
		for (i=0; i<n; i++) {
			cn4[i]=cn[i];
		}
		cn4[n]=cn[0];
		cn4[n+1]=cn[1];
	} else {	// Outflow Boundary Conditions
		// Conc
		conc4[-2]=conc[0];
		conc4[-1]=conc[0];
		for (i=0; i<n; i++) {
			conc4[i]=conc[i];
		}
		conc4[n]=conc[n-1];
		conc4[n+1]=conc[n-1];
		// Courant number
		cn4[-2]=cn[0];
		cn4[-1]=cn[0];
		for (i=0; i<n; i++) {
			cn4[i]=cn[i];
		}
		cn4[n]=cn[n-1];
		cn4[n+1]=cn[n-1];
	}
	if (0==iDir||1==iDir) {
		for (j=-1; j<=n; j++) {
			bottPsiI(j, n, cn4, conc4, undef, iMinus, iPlus, w);
		}
		// 5.0  calc dconc
		for (j=0; j<n; j++) {
			if (iMinus[j-1]!=undef && iMinus[j]!=undef && iPlus[j-1]!=undef
				&& iPlus[j]!=undef && w[j-1]!=undef && w[j]!=undef
				&& w[j+1]!=undef) {
				dcdt[j]=-((iPlus[j]*w[j]-iMinus[j]*w[j+1])
					-(iPlus[j-1]*w[j-1]-iMinus[j-1]*w[j]));
			} else if (w[j]!=undef) {
				dcdt[j]=0.;	// assume no flux
			} else {
				dcdt[j]=undef;
			}
		}
	} else {
		for (j=0; j<n; j++) {
			if (cn4[j]!=undef && conc4[j-1]!=undef&& conc4[j]!=undef
				&& conc4[j+1]!=undef) {	// upwind scheme
				dcdt[j]=-(cn4[j]*(1.+ sign(cn4[j]))/2.*(conc4[j]-conc4[j-1])+
					cn4[j]*(1.- sign(cn4[j]))/2.*(conc4[j+1]-conc4[j]));
			} else if (conc4[j]!=undef) {
				dcdt[j]=0.;	// assume no flux
			} else {
				dcdt[j]=undef;
			}
		}
	}	
	for (i=0; i<5; i++) {
		if (NULL!=array[i]) free(array[i]);
	}
	return (0);
	
merr:
	sprintf (pout,"Error from %s: Memory Allocation Error\n","bott");
	gaprnt (0,pout);
	for (i=0; i<5; i++) {
		if (NULL!=array[i]) free(array[i]);
	}
	return (1); 
}

int mirror (float * data, int ni, int nj) {
	float * tmp;
	int i, j, size;
	size=ni*nj;
	// mirror data
	tmp = (float *)malloc(size*sizeof(float));
	if (tmp==NULL) {
		gaprnt (0,"Memory Allocation Error:  MIRROR function \n");
		return 0;
	}
	for (j=0; j<nj; j++) {
		for (i=0; i<ni; i++) {
			*(tmp+i*nj+j) = *(data+i+j*ni);
		}
	}
	for (i=0; i<size; i++) *(data+i) = *(tmp+i);
	free (tmp);
	return 1;
}


int ffadv (struct gafunc *pfc, struct gastat *pst, int iDir) {
	int rc,size,i,j,lvt;
	struct gagrid *pgru, *pgrc;
	struct gagrid *pgrum, *pgrup, *pgrcm, *pgrcp;
	float *result, *cnResult;
	float *u1, *u2, *cn, *conc;
	float lat, ri, rj, temp, dx;
	float *lnvals, *ltvals, *lvvals;
	float (*lnconv) (float *, float);
	float (*ltconv) (float *, float);
	float (*lvconv) (float *, float);
	float (*conv) (float *, float);
	int isiz, jsiz, idim, jdim, wrap;
        int iLonLat=1;                                 /*ams to avoid UTM stuff */
	//
	struct gafile *pfi;
	float levp, lev, levm, levu; 
	float z, zu;
	float *cm, *c, *cp, *wm, *w, *wp, *dcdt;
	float cnw, cnwm;

	pfi = pst->pfid;
	lvt = pfi->dnum[2];	// size of levels
	lvvals = pfi->grvals[2];
	lvconv = pfi->gr2ab[2];
	conv = pfi->ab2gr[2];

	pgru=pgrc=NULL;
	pgrum=pgrup=pgrcm=pgrcp=NULL;
	result = NULL;
	cnResult = NULL;
	
	/* Check for user errors */
	
	if (pfc->argnum!=2) {
		gaprnt (0,"Error from UADV (VADV):  Too many or too few args \n");
		gaprnt (0,"                   Two arguments expected \n");
		return (1);
	}
	if (pst->idim!=0 || pst->jdim!=1) {
		gaprnt (0,"Error from UADV (VADV):  Invalid dimension environment\n");
		gaprnt (0,"  Horizontal environment (X, Y Varying) is required\n");
		return (1);
	}
	
	/* Get the u and c fields.  User responsible for validity. */
	
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	if (pst->type==0) {
		gafree (pst);
		return (-1);
	}
	pgru = pst->result.pgr;
	if (pfi->type==2) {	// station
		levu = pst->dmin[2];
	} else {	// grid
		levu=pst->dmin[2];
		zu = conv(pfi->abvals[2],pst->dmin[2]);
	}

	rc = gaexpr(pfc->argpnt[1],pst);
	if (rc) {
		gagfre(pgru);
		return (rc);
	}
	if (pst->type==0) {
		gafree (pst);
		gagfre(pgru);
		return (-1);
	}
	pgrc = pst->result.pgr;
	
	if (pfi->type==2) {	// station
		lev = pst->dmin[2];
	} else {	// grid
		lev=pst->dmin[2];
		z = conv(pfi->abvals[2],pst->dmin[2]);
	}
	/* Check that an operation between these grids is valid */
	
	if (gagchk(pgru,pgrc,pst->idim) ||
		gagchk(pgru,pgrc,pst->jdim) ) {
		gaprnt (0,"Error from UADV (VADV, WADV):  Incompatable grids \n");
		gaprnt (0,"                   Dimension ranges unequal \n");
		goto erret;
	}
#ifdef lDiag
	sprintf (pout,"iLonLat=%d\n",pfi->iLonLat);gaprnt (0,pout);
#endif 
	if (0==iDir||1==iDir||3==iDir||4==iDir) {	// muadv=d(uc)/dx, or mvadv=d(vc)/dy
		if (1!=pgru->ilinr||1!=pgru->jlinr) {
			gaprnt (0,"Error from UADV (VADV):  Nonlinear U/V grids \n");
			gaprnt (0,"                   Both xdef and ydef should be linear \n");
			goto erret;
		}
		
		if (1!=pgrc->ilinr||1!=pgrc->jlinr) {
			gaprnt (0,"Error from UADV (VADV):  Nonlinear C grids \n");
			gaprnt (0,"                   Both xdef and ydef should be linear \n");
			goto erret;
		}
	} else if (2==iDir||5==iDir) {	// mwadv
		if (fabs(lev-levu)>1.||fabs(z-zu)>0.5) {
			gaprnt (0,"Error from WADV:  Incompatable W/C grids \n");
			gaprnt (0,"                   vertical dimension ranges unequal \n");
			goto erret;
		}
	}
	
	/* Get memory for result grid. */
	
	size = pgrc->isiz * pgrc->jsiz;
	result = (float *)malloc(size*sizeof(float));
	if (result==NULL) {
		gaprnt (0,"Memory Allocation Error:  UADV/VADV/WADV function \n");
		goto erret;
	}
	for (i=0; i<size; i++) *(result+i) = pgrc->undef;
	if (0==iDir||3==iDir) {	// xDir
		isiz=pgrc->isiz;
		jsiz=pgrc->jsiz;
		idim=0;
		jdim=1;
		wrap=pgrc->pfile->wrap;
	} else if (1==iDir||4==iDir)	{	// yDir
		isiz=pgrc->jsiz;
		jsiz=pgrc->isiz;
		idim=1;
		jdim=0;
		wrap=0;	// FALSE
		// mirror data
		if (!mirror (result,pgrc->isiz,pgrc->jsiz)) {
			gaprnt (0,"Memory Allocation Error:  VADV function \n");
			goto erret;
		}
		if (!mirror (pgrc->grid,pgrc->isiz,pgrc->jsiz)) {
			gaprnt (0,"Memory Allocation Error:  VADV function \n");
			goto erret;
		}
		if (!mirror (pgru->grid,pgru->isiz,pgru->jsiz)) {
			gaprnt (0,"Memory Allocation Error:  VADV function \n");
			goto erret;
		}
	} else if (2==iDir||5==iDir) {	//mwadv
		isiz=1;
		idim=0;
		jdim=1;
		wrap=0;	// FALSE
	}
	
	cnResult = (float *)malloc(isiz*sizeof(float));
	if (cnResult==NULL) {
		gaprnt (0,"Memory Allocation Error:  UADV/VADV function \n");
		goto erret;
	}
	for (i=0; i<isiz; i++) *(cnResult+i) = pgrc->undef;
	
	/* Perform the advection calculation except at grid borders */
	//           
	//          cn1       cn2
	//    conc1--|--conc2--|--conc3
	//    u1-----|----u2---|----u3
	//
	
	if (0==iDir||1==iDir||3==iDir||4==iDir) {	// muadv or mvadv
		lnvals = pgrc->ivals;
		ltvals = pgrc->jvals;
		lnconv = pgrc->igrab;
		ltconv = pgrc->jgrab;
		for (j=(pgrc->dimmin[jdim]); j<=pgrc->dimmax[jdim]; j++) {
			rj = (float)j;
			conc = pgrc->grid+(j-pgrc->dimmin[jdim])*isiz;
			dcdt = result+(j-pgrc->dimmin[jdim])*isiz;
			u1 = pgru->grid+(j-pgrc->dimmin[jdim])*isiz;
			cn = cnResult;
			for (i=(pgru->dimmin[idim]); i<=pgru->dimmax[idim]; i++) {
				if (i==pgru->dimmax[idim]) {
					if (1==wrap) {
						u2 = pgru->grid+(j-pgrc->dimmin[jdim])*isiz;
					} else {
						u2 = u1;	// last one: outflow cond.
					}
				} else {
					u2 = u1+1;
				}
				ri = (float)i;
				if (0==iDir||3==iDir) {
					dx = *pgru->ivals;
				} else if (1==iDir||4==iDir) {
					dx = *pgru->jvals;
				}
				if (0==iDir||1==iDir) {
					if (*u1!=pgru->undef && *u2!=pgru->undef) {
						*cn = (*u2 + *u1)/2./dx*1.;	// set dt = 1 sec
					} else if (*u1!=pgru->undef) {
						*cn = *u1/dx*1.;	// set dt = 1 sec
					} else if (*u2!=pgru->undef) {
						*cn = *u2/dx*1.;	// set dt = 1 sec
					}
				} else {
					if (*u1!=pgru->undef) {
						*cn = *u1/dx*1.;	// set dt = 1 sec
					}
				}			
#ifdef USEUTM
                                iLonLat = pfi->iLonLat;
#endif
				if (*cn!=pgrc->undef) {	
					if (2==iLonLat) {
						// if (2==pgrc->pfile->iLonLat) {
						*cn /= 1000.;	// utm coordinate (convert km to m)
					}
					else {
						// 1==pfi->iLonLat (LonLat Coord.) 
						// or 1==pgrc->pfile->iLonLat
						if (0==iDir|3==iDir) {
							lat  = ltconv(ltvals,rj) * 3.1416/180.0;
							temp = 6.37E6 * cos(lat) * 3.1416/180.0;	// m/deg
						} else if (1==iDir||4==iDir) {
							temp = 6.37E6 * 3.1416/180.0;	// m/deg
						}
						if (temp>1E-10) {*cn /= temp;}
						else {*cn = pgrc->undef;}
					}
				}
				u1++; cn++;
			}
			bott(cnResult, isiz, wrap, pgrc->undef, conc, dcdt, iDir);
		}
		if (1==iDir||4==iDir) {
			// mirror data back
			if (!mirror (result,pgrc->jsiz,pgrc->isiz)) {
				gaprnt (0,"Memory Allocation Error:  VADV function \n");
				goto erret;
			}
		}
	} else if (2==iDir||5==iDir) {	// mwadv
		// -w*dC/dz
		// usage: d mwadv(w,C)
		// no-slip boundary conditions are assumed. i.e.,
		//    w(lev=0)=0
		//    C(lev=0)=C(lev=1)
		// numerical method: upwind scheme
		//
		//    concp, wp
		//         |---cn
		//    conc, w
		//         |---cnm
		//    concm, wm
		//
		// level -1 (levm)
		if (1!=(int)z) {
			levm = lvconv(lvvals, z-1.);
			pst->dmin[2] = levm;
			pst->dmax[2] = levm;
			// conc
			rc = gaexpr(pfc->argpnt[1],pst);
			if (rc) {
				goto erret;
			}
			pgrcm = pst->result.pgr;
			// w
			rc = gaexpr(pfc->argpnt[0],pst);
			if (rc) {
				goto erret;
			}
			pgrum = pst->result.pgr;
		} else {	// no slip boundary condition
			levm = lvconv(lvvals, z);
			// levm = pgrc->undef;
			pst->dmin[2] = levm;
			pst->dmax[2] = levm;
			// conc
			rc = gaexpr(pfc->argpnt[1],pst);
			if (rc) {
				goto erret;
			}
			pgrcm = pst->result.pgr;
			// w
			rc = gaexpr(pfc->argpnt[0],pst);
			if (rc) {
				goto erret;
			}
			pgrum = pst->result.pgr;
			for (i=0; i<size; i++) *(pgrum->grid+i) = 0.;	// no-slip conditions
		}
		// level +1 (levp)
		if (lvt!=(int)z) {
			levp = lvconv(lvvals, z+1.);
			pst->dmin[2] = levp;
			pst->dmax[2] = levp;
			// conc
			rc = gaexpr(pfc->argpnt[1],pst);
			if (rc) {
				goto erret;
			}
			pgrcp = pst->result.pgr;
			// w
			rc = gaexpr(pfc->argpnt[0],pst);
			if (rc) {
				goto erret;
			}
			pgrup = pst->result.pgr;
		} else {
			// outflow conditions
			levp = lvconv(lvvals, z);
			// levp = pgrc->undef;
			pst->dmin[2] = levp;
			pst->dmax[2] = levp;
			// conc
			rc = gaexpr(pfc->argpnt[1],pst);
			if (rc) {
				goto erret;
			}
			pgrcp = pst->result.pgr;
			// w
			rc = gaexpr(pfc->argpnt[0],pst);
			if (rc) {
				goto erret;
			}
			pgrup = pst->result.pgr;
		}
#ifdef lDiag
		sprintf (pout,"levm=%f\n",levm);gaprnt (0,pout);
		sprintf (pout,"lev=%f\n",lev);gaprnt (0,pout);
		sprintf (pout,"levp=%f\n",levp);gaprnt (0,pout);
#endif 	
		cm = pgrcm->grid;
		c  = pgrc->grid;
		cp = pgrcp->grid;
		wm = pgrum->grid;
		w  = pgru->grid;
		wp = pgrup->grid;
		dcdt = result;
		for (j=0; j<size; j++) {
			if ( (*cm!=pgrcm->undef)&&(*c!=pgrc->undef)&&(*cp!=pgrcp->undef)&&
				(*wm!=pgrum->undef)&&(*w!=pgru->undef)&&(*wp!=pgrup->undef) ) {
				if (2==iDir) {	// madvw: donor-cell scheme
					if (lvt==(int)z) {	// top boundary
						cnwm=((*w)+(*wm))/2./(lev-levm);
						cnw=(*w)/(lev-levm);
					} else if (1==(int)z) {	// bottom boundary
						cnwm=(*w)/(levp-lev);
						cnw=((*w)+(*wp))/2./(levp-lev);
					} else {
						cnwm=((*w)+(*wm))/2./(lev-levm);
						cnw=((*w)+(*wp))/2./(levp-lev);
					}
					if ((levp-levm)>0.) {
						*dcdt=-1./2.*(cnw*((*c)+(*cp))-cnwm*((*cm)+(*c))+
							fabs(cnw)*((*c)-(*cp))-fabs(cnwm)*((*cm)-(*c)));
					} else {
						*dcdt=-1./2.*(cnw*((*c)+(*cm))-cnwm*((*cp)+(*c))+
							fabs(cnw)*((*c)-(*cm))-fabs(cnwm)*((*cp)-(*c)));
					}						
				} else { // mwadv: upwind scheme.
					if (lvt==(int)z) {	// top boundary
						cnwm=(*w)/(lev-levm);
						cnw=(*w)/(lev-levm);
					} else if (1==(int)z) {	// bottom boundary
						cnwm=(*w)/(levp-lev);
						cnw=(*w)/(levp-lev);
					} else {
						cnwm=(*w)/(lev-levm);
						cnw=(*w)/(levp-lev);
					}
					if ((levp-levm)>0.) {
						*dcdt=-(cnw*(1.+ sign(cnw))/2.*( *cp - *c )+
							cnwm*(1.- sign(cnwm))/2.*( *c - *cm ));
					} else {
						*dcdt=-(cnw*(1.+ sign(cnw))/2.*( *c - *cm )+
							cnwm*(1.- sign(cnwm))/2.*( *cp - *c ));
					}
				}
				// *dcdt=-(cnw+cnwm)/2.*((*cp)-(*c))/2.;
#ifdef lDiag
				if (0==(j%500)) {
					sprintf (pout,"dcdt=%f\n",*dcdt);gaprnt (0,pout);
					sprintf (pout,"cnwm=%f, cnw=%f\n",cnwm,cnw);gaprnt (0,pout);
					sprintf (pout,"cm=%f, c=%f, cp=%f\n",*cm,*c,*cp);gaprnt (0,pout);
					sprintf (pout,"wm=%f, w=%f, wp=%f\n",*wm,*w,*wp);gaprnt (0,pout);
				}
#endif 	
			} else {
				*dcdt=pgrc->undef;
			}
			cm++; c++; cp++; wm++; w++; wp++; dcdt++;
		}
	}
	if (cnResult!=NULL) free(cnResult);
	free (pgrc->grid);
	gagfre(pgru);
	gagfre(pgrum);
	gagfre(pgrup);
	gagfre(pgrcm);
	gagfre(pgrcp);
	pgrc->grid = result;
	pst->type = 1;
	pst->result.pgr = pgrc;
	return (0);
	
erret:
	if (result!=NULL) free(result);
	if (cnResult!=NULL) free(cnResult);
	gagfre(pgru);
	gagfre(pgrum);
	gagfre(pgrup);
	gagfre(pgrc);
	gagfre(pgrcm);
	gagfre(pgrcp);
	return (1);
	
}

int ffmadvu (struct gafunc *pfc, struct gastat *pst) {
	// -d(uC)/dx
	// usage: d muadv(u,C)
	return (ffadv (pfc, pst, 0));
}

int ffmadvv (struct gafunc *pfc, struct gastat *pst) {
	// -d(vC)/dy
	// usage: d mvadv(v,C)
	return (ffadv (pfc, pst, 1));
}

int ffmadvw (struct gafunc *pfc, struct gastat *pst) {
	// -d(wC)/dz
	// usage: d mwadv(w,C)
	// no-slip boundary conditions are assumed. i.e.,
	//    w(lev=0)=0
	//    C(lev=0)=C(lev=1)
	// numerical method: upwind scheme
	//
	//    concp, wp
	//         |---cn
	//    conc, w
	//         |---cnm
	//    concm, wm
	//
	return (ffadv (pfc, pst, 2));
}
int ffmuadv (struct gafunc *pfc, struct gastat *pst) {
	// -u*dC/dx
	// usage: d muadv(u,C)
	return (ffadv (pfc, pst, 3));
}

int ffmvadv (struct gafunc *pfc, struct gastat *pst) {
	// -v*dC/dy
	// usage: d mvadv(v,C)
	return (ffadv (pfc, pst, 4));
}

int ffmwadv (struct gafunc *pfc, struct gastat *pst) {
	// -w*dC/dz
	// usage: d mwadv(w,C)
	// no-slip boundary conditions are assumed. i.e.,
	//    w(lev=0)=0
	//    C(lev=0)=C(lev=1)
	// numerical method: upwind scheme
	//
	//    concp, wp
	//         |---cn
	//    conc, w
	//         |---cnm
	//    concm, wm
	//
	return (ffadv (pfc, pst, 5));
}

// bjt >>

char *nam1var[2] = {"SATVAP","DEW"};
char *usg1var[2] = {"calc saturated vapor pressure\nusage: d satvap(T)\nwhere T in K\n",
	"calc dew point:\nusage: d dew(vap)\nwhere vap in Pa\n"};

int ff1var  (struct gafunc *pfc, struct gastat *pst, int ifun) {
	int i,rc,cnt;
	struct gagrid *pgr;
	struct gastn *stn;
	struct garpt *rpt;
	float *val;
	

	if (pfc->argnum!=1) {
		sprintf (pout,"Error from %s:  One argument expected\n",nam1var[ifun-1]);gaprnt (0,pout);
		gaprnt (0,usg1var[ifun-1]);
		return (1);
	}
	
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	
	if (pst->type==1) {
		pgr = pst->result.pgr;
		cnt = pgr->isiz * pgr->jsiz;
		val = pgr->grid;
		for (i=0; i<cnt; i++) {
			if (*val!=pgr->undef) {
				if (1==ifun) *val = calcSaturatedVaporPressure(*val);
				if (2==ifun) {
					if (*val!=pgr->undef && *val > 0.0) *val = bisdew(*val,pgr->undef);
					else *val = pgr->undef;
				}
			}
			val++;
		}
	} else {
		stn = pst->result.stn;
		rpt = stn->rpt;
		while (rpt!=NULL) {
			if (rpt->val!=stn->undef) {
				if (1==ifun) rpt->val = calcSaturatedVaporPressure(rpt->val);
				if (2==ifun) {
					if (rpt->val!=stn->undef && rpt->val > 0.0) rpt->val = bisdew(rpt->val,stn->undef);
					else rpt->val = stn->undef;
				}
			}	
			rpt=rpt->rpt;
		}
	}
	
	return (0);
}

int ffsatvap (struct gafunc *pfc, struct gastat *pst) {
	// saturated vapor pressure 
	// usage: d satvap(T)
	// where T in K
	return (ff1var (pfc, pst, 1));
}

int ffdew (struct gafunc *pfc, struct gastat *pst) {
	// dew point 
	// usage: d dew(vap)
	// where vap in Pa
	return (ff1var (pfc, pst, 2));
}

/*
int ffdew  (struct gafunc *pfc, struct gastat *pst) {
	int i,rc,cnt;
	struct gagrid *pgr1;
	float *vap;//,*RH;
	
	if (pfc->argnum!=1) {
		gaprnt (0,"Error from Dew:  Too many or too few args \n");
		gaprnt (0,"                  One arguments expected \n");
		return (1);
	}
	
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	
	if (pst->type!=1) {
		gaprnt (0,"Error from Dew : first argument must be a grid \n");
		return (1);
	}
	pgr1 = pst->result.pgr;
	cnt = pgr1->isiz * pgr1->jsiz;
	vap = pgr1->grid;
	for (i=0; i<cnt; i++) {
		if (*vap!=pgr1->undef && *vap > 0.0) *vap = bisdew(*vap,pgr1-> undef);
		else *vap = pgr1-> undef;
		vap++;
	}
	pst->result.pgr = pgr1;
	return (0);
}
*/
float calcAirVaporPressure(float ta, float relativeHumidity)
{
	float ea;
	ea= calcSaturatedVaporPressure(ta)*relativeHumidity;
	return(ea);
} 

float calcAtmosphericLongWaveRadiation(float ta, float relativeHumidity, float cloudAmount) {
	float longWaveRadiation;
	float sTB;
	sTB=5.6697e-8;	// [W.m-2.K-4]  Stefan-Boltzmann constant. 
	longWaveRadiation = 1.147282*(1-exp(-pow(calcAirVaporPressure(ta, relativeHumidity),
		ta/2016)))*sTB*pow(	ta,4)*
		(1+0.072467*pow(cloudAmount,1.263964));	
	return(longWaveRadiation);
}

int calcJulianDate(int year,int	month,int day)
{
	int dayInMonth[12]={0,31,59,90,120,151,181,212,243,273,304,334};
	/* specical to deal with the leap year (year%4)	= 0 */
	if ((year%4)==0	&& (!(year%100)==0 || (year%400)==0))
	{
		if ( month == 1	) day  = day;
		if ( month == 2	) day += dayInMonth[month-1] ;
		if ( month > 2 )  day += dayInMonth[month-1] + 1 ;
	}
	else
	{
		if ( month == 1	) day  = day;
		if ( month > 1 )  day += dayInMonth[month-1];
	}
	return (day);
}

int calcMonthDay(int year,int julianDate)
{	
	int days[12]={31,59,90,120,151,181,212,243,273,304,334,365};
	int day=0, i=0, j=0;
	if ((year%4)==0	&& (!(year%100)==0 || (year%400)==0))
	{								 
		for (i=1;i<12;i++) days[i]++;
	}
	while (	julianDate > days[j] ) j++;
	
	if ( j == 0 )	day = julianDate;
	else day=julianDate-days[j-1];
	return(day);
}

float calcSaturatedVaporPressure(float ta)
{
	float vaporPressure, tr;
	tr= 1.00 - 373.15/ta;
	vaporPressure= 101325.*exp(13.3185*tr-1.976*(pow(tr,2.))
		-0.6445*(pow(tr,3.))-0.1299*(pow(tr,4.)));
	return(vaporPressure);
} 

float calcSolarRadiation(float zenith, float cloudAmount) {   
    float solarRadiation;
	float solarConstant;
	solarConstant= 1367.;	// [W.m-2]      Solar constant 
	if (zenith <= 0.)	{
		solarRadiation=0.;
	}
	else {
		solarRadiation = solarConstant*zenith*(0.7288837-0.5050691*cloudAmount);
	}
    return(solarRadiation);
}   

float calcTerrestrialLongWaveRadiation(float tg, float emiss) {
	float terrestrialLongWaveRadiation;
	float sTB;
	sTB=5.6697e-8;	// [W.m-2.K-4]  Stefan-Boltzmann constant. 
	terrestrialLongWaveRadiation = emiss*sTB*pow(tg, 4.);
	return(terrestrialLongWaveRadiation);
}

float calcZenithAngle(int julianDate, float hour, float latitude) {   
    float zenith;
	float rad;
	rad = pi/180.;	// radian conversion factor
    zenith = cos(15.*(12.-hour)*rad)*cos(latitude*rad)*cos(-23.4*cos((360.*
		julianDate+10.)/365.*rad)*rad)+sin(latitude*rad)
		*sin(-23.4*cos((360.*julianDate+10.)/365.*rad)*rad);
    return(zenith);
}

// << bjt
int julianDate(int mm, int id, int iyyy){
	const int IGREG=15+31*(10+12*1582);
	int ja, jul,jy=iyyy,jm;
	if (0==jy){
		sprintf(pout,"Julian: There is no year zero.\n");
		gaprnt (1,pout);
	}
	if (jy < 0) ++jy;
	if (mm > 2){
		jm = mm+1;
	}else{
		--jy;
		jm = mm+13;
	}
	jul=(int)(floor(365.25*jy)+floor(30.6001*jm)+id+1720995.);
	if (id+31*(mm+12*iyyy) >= IGREG){
		ja = (int)(0.01*jy);
		jul += 2-ja+(int)(0.25*ja);
	}
	return jul;
}
/*
void calday(int julian, int &mm, int &id, int &iyyy){
const int IGREG=2299161;
int ja,jalpha,jb,jc,jd,je;
if (julian>=IGREG){
jalpha=(int)(((float)(julian-1867216)-0.25)/36524.25);
ja=julian+1+jalpha-(int)(0.25*jalpha);
}else if (julian<0){
ja=julian+36525*(1-julian/36525);
}else ja=julian;
jb=ja+1524;
jc=(int)(6680.0+((float)(jb-2439870)-122.1)/365.25);
jd=(int)(365*jc+(0.25*jc));
je=(int)((jb-jd)/30.6001);
id=jb-jd-(int)(30.6001*je);
mm=je-1;
if (mm > 2) mm-=12;
iyyy=jc-4715;
if (mm > 2) --iyyy;
if (iyyy <= 0) --iyyy;
if (julian < 0) iyyy-=100*(1-julian/36525);
}*/
float ellf(float phi, float ak){
	float s;
	s= sin(phi);
	return s*rf(cos(phi)*cos(phi),(1.0-s*ak)*(1.0+s*ak),1.0);
}
float rf(float x, float y, float z){
	const float ERRTOL=0.0025, TINY=1.5e-38, BIG=3.0e37, THIRD=(1.0/3.0);
	const float C1=(1.0/24.0), C2=0.1, C3=(3.0/44.0), C4=(1.0/14.0);
	float alamb, ave, delx, dely, delz, e2, e3, sqrtx, sqrty, sqrtz, xt, yt, zt;
	
	if ( min(min(x,y),z)<0.0 || min(min(x+y,x+z),y+z)<TINY || max(max(x,y),z)>BIG ){
		sprintf(pout,"invalid arguments in rf.\n");
		gaprnt (1,pout);
	}
	xt=x;
	yt=y;
	zt=z;
	do{
		sqrtx=sqrt(xt);
		sqrty=sqrt(yt);
		sqrtz=sqrt(zt);
		alamb=sqrtx*(sqrty+sqrtz)+sqrty*sqrtz;
		xt=0.25*(xt+alamb);
		yt=0.25*(yt+alamb);
		zt=0.25*(zt+alamb);
		ave=THIRD*(xt+yt+zt);
		delx=(ave-xt)/ave;
		dely=(ave-yt)/ave;
		delz=(ave-zt)/ave;
	}while(max(max(fabs(delx),fabs(dely)),fabs(delz))>ERRTOL);
	e2=delx*dely-delz*delz;
	e3=delx*dely*delz;
	return (1.0+(C1*e2-C2-C3*e3)*e2+C4*e3)/sqrt(ave);
}
/*
float min(float a, float b){
	if (a>b) a=b;
	return a;
}
float max(float a, float b){
	if (a<b) a=b;
	return a;
}
*/
//calc dew Temp. (ffdew) by bisection method
float bisdew(float vap, float null){
	const int JMAX=20;
	int j;
	float rtn;
	float tmpr,tmpl,fr,fl;
	const float xacc=0.01;
	tmpr=288.15;
	do {
		tmpr+=5.;
		if (tmpr > 373.15) {
			gaprnt (0,"Warning from Dew: out of range, set to undef \n");
			return null;
		}
		fr = calcSaturatedVaporPressure(tmpr)-vap;
	} while (fr<0.);
	tmpl=273.15;
	do {
		tmpl-=5.;
		if (tmpl <0.) {
			gaprnt (0,"Warning from Dew: out of range, set to undef \n");
			return null;
		}
		fl = calcSaturatedVaporPressure(tmpl)-vap;
	} while (fl>0.);
	if (fr*fl <= 0.0){
		for(j=0; j<JMAX; j++){
			rtn= 0.5*(tmpr+tmpl);
			fr = calcSaturatedVaporPressure(tmpr)-vap;
			fl = calcSaturatedVaporPressure(rtn)-vap;
			if (fl*fr<0) tmpl=rtn;
			else tmpr=rtn;
			if (fabs(tmpr-tmpl) < xacc) return (rtn);
		}
		gaprnt (0,"Error from Dew: it is not converge \n");
		return (1);
	}else{
		gaprnt (0,"Warning from Dew: out of range, it was been set null \n");
		return null;
	}
}

#ifdef LW	
char *lwnam[2] = {"LW","LW2"};
int lw (struct gafunc *pfc, struct gastat *pst, int sel) {
// modify by Ben-Jei Tsuang for using in GrADS, 2005/01/08
/* ********************************************************************* */
/* Following the NASA Technical Memorandum (NASA/TM-2001-104606, */
/*  Vol. 19) of Chou, Suarez, Liang, and Yan (2001). This NASA TM */
/*  has been revised a few timessince. It computes thermal infrared */
/*  fluxes due to emission by water vapor, ozone, co2, o2, minor */
/*  trace gases, clouds, and aerosols and due to scattering by */
/*  clouds and aerosols. */
/* ***** The computer code and documentation are accessible from */
/*      ftp://climate.gsfc.nasa.gov/pub/chou/clirad_lw/ */
/* ********************************************************************* */
/* CHANGES MADE IN SEPTEMBER 2003: */
/*  (1) A subroutine "cldreff" for computing the effective particle size */
/*      of cloud particles (reff) is created. The reff is an now an input */
/*      parameter of the subroutine "irrad". */
/*  (2) The int parameter "vege" is no longer an input parameter. */
/*      Subsequent changes were made to the the subroutine "sfcflux" */
/* ********************************************************************* */
/* The maximum-random assumption is applied for cloud overlapping. */
/*  Clouds are grouped into high, middle, and low clouds separated */
/*  by the level indices ict and icb.  Within each of the three groups, */
/*  clouds are assumed maximally overlapped.  Clouds among the three */
/*  groups are assumed randomly overlapped. The indices ict and icb */
/*  correspond approximately to the 400 mb and 700 mb levels. */
/* There are options for computing fluxes: */
/*   If high = .true., transmission functions in the co2, o3, and the */
/*   three water vapor bands with strong absorption are computed using */
/*   table look-up.  cooling rates are computed accurately from the */
/*   surface up to 0.01 mb. */
/*   If high = .false., transmission functions are computed using the */
/*   k-distribution method with linear pressure scaling for all spectral */
/*   bands except Band 5.  cooling rates are not accurately calculated */
/*   for pressures less than 10 mb. the computation is faster with */
/*   high=.false. than with high=.true. */
/*   If trace = .true., absorption due to n2o, ch4, cfcs, and the */
/*   two minor co2 bands in the window region is included. */
/*   Otherwise, absorption in those minor bands is neglected. */
/*   If overcast=.true., the layer cloud cover is either 0 or 1. */
/*   If overcast=.false., the cloud cover can be anywhere between 0 and 1. */
/*   Computation is faster for the .true. option than the .false. option. */
/*   If aerosol = .true., aerosols are included in calculating transmission */
/*   functions. Otherwise, aerosols are not included. */
/*   If roberts = .true., Roberts et al. (1976) water vapor continuum adsorption */
/*   is used. Otherwise, Clough et al. (1989) is used (suggested). */
/* ---- Input parameters                               units    size */
/*   number of bands (nb=10)                            --      1 */
/*   number of soundings (m)                            --      1 */
/*   number of atmospheric layers (lvt)                  --      1 */
/*   level pressure (pl)                               mb      m*(lvt+1) */
/*   layer temperature (ta)                            k       m*lvt */
/*   layer specific humidity (wa)                      g/g     m*lvt */
/*   layer ozone mixing ratio by mass (oa)             g/g     m*lvt */
/*   surface air temperature (tb)                      k        m */
/*   co2 mixing ratio by volume (co2)                  pppv     1 */
/*   option (high) (see explanation above)              --      1 */
/*   option (trace) (see explanation above)             --      1 */
/*   n2o mixing ratio by volume (n2o)                  pppv     1 */
/*   ch4 mixing ratio by volume (ch4)                  pppv     1 */
/*   cfc11 mixing ratio by volume (cfc11)              pppv     1 */
/*   cfc12 mixing ratio by volume (cfc12)              pppv     1 */
/*   cfc22 mixing ratio by volume (cfc22)              pppv     1 */
/*   number of sub-grid surface types (ns)              --      m */
/*   fractional cover of sub-grid regions (fs)       fraction  m*ns */
/*   land or ocean surface temperature (tg)            k       m*ns */
/*   land or ocean surface emissivity (eg)           fraction  m*ns*nb */
/*   vegetation temperature (tv)                       k       m*ns */
/*   vegetation emissivity (ev)                      fraction  m*ns*nb */
/*   vegetation reflectivity (rv)                    fraction  m*ns*nb */
/*   option for cloud fractional cover                  --      1 */
/*      (overcast)   (see explanation above) */
/*   cloud water mixing ratio (cwc)                   gm/gm   m*lvt*3 */
/*       index 1 for ice particles */
/*       index 2 for liquid drops */
/*       index 3 for rain drops */
/*   cloud amount (fcld)                             fraction  m*lvt */
/*   level index separating high and middle             --      1 */
/*       clouds (ict) */
/*   level index separating middle and low              --      1 */
/*       clouds (icb) */
/*   option for including aerosols (aerosol)            --      1 */
/*   number of aerosol types (na=3)                     --      1 */
/*   aerosol optical thickness (taual)                  --   m*lvt*nb*na */
/*   aerosol single-scattering albedo (ssaal)           --   m*lvt*nb*na */
/*   aerosol asymmetry factor (asyal)                   --   m*lvt*nb*na */
/* ---- output parameters */
/*   net downward flux, all-sky   (flx)              w/m**2  m*(lvt+1) */
/*   net downward flux, clear-sky (flc)              w/m**2  m*(lvt+1) */
/*   sensitivity of net downward flux */
/*       to surface temperature (dfdts)              w/m**2/k m*(lvt+1) */
/*   emission by the surface (sfcem)                 w/m**2     m */
/*   cooling rate (coolr)                            C/day     m*lvt */
/* *************************************************************************** */

	extern int irrad_ (int *,int *,int *,int *,int *,int *,int *,int *,float *,
	        float *,float *,float *,float *,float *,float *,
		int *,int *,float *,float *,float *,float *,float *,
		int *,int *,float *,float *,float *,int *,int *,float *,
		int *,float *,float *,float *,float *,float *,float *,
		int *,int *,float *,float *,float *,int *,
		float *,float *,float *,float *,float *,float *,float *,float *);
	    
	extern int cldreff_(int *, int *, int *, float *, float *,
	     float *, float *);


	struct gagrid *pgr;
	struct gagrid *pgrpl,*pgrta,*pgrwa,*pgrtaucl,*pgrfcld;
	struct gafile *pfi;
	int rc, size, lvt, lvt1;
	float *var, *res, *top;

	float *lnvals, *ltvals, *lvvals;
	float (*lnconv) (float *, float);
	float (*ltconv) (float *, float);
	float (*lvconv) (float *, float);
	float (*conv) (float *, float);
	float clev,ulev,blev,ulevi,blevi;

	/* 0. input parameters */
	float *pl,*ta,*wa,*oa;
	float *cwc,*taucl,*fcld;
	float *taual,*ssaal,*asyal;
	float *pm,*tm,*wm;
	float *ps,*tb,*wb;
	float *fs,*tg,*eg;
	float *tv,*ev,*rv;
	
	int *ict,*icb,m;
	int *np,*nm,*nta,*nwa,*noa,*ntaucl,*nfcld;
	static int ns = 1;
	static int na = 3;
	static int nb = 10;
	// surface or mixing level index
	int iend;
	
	float co2,n2o,ch4,cfc11,cfc12,cfc22;
	int high,trace,overcast,aerosol,lcwc,roberts;

// -----output parameters

	float *flx,*flxu,*flxd,*flc,*flcu,*flcd,*dfdts;
	float *coolr,*sfcem;

// -----temporay parameters

	int i,j,k,ib,*lsfc,*lml;
// 	static char hi[6],tr[6],ov[6],ar[6];
	float *reff,dp400,dp700,xx;
	float *ptrps,*ptrpl,*ptrta,*ptrwa,*ptrtaucl,*ptrfcld;
	float *ptrpm,*ptrtm,*ptrwm;
	FILE *fpdata, *fpctl;	// data file & ctl file handle 
	char fulldatafilename[80]="null",datafilename[80]="null",ctlfilename[80]="null";	
	char yymmddhh[16],date[80],dtstr[16],dsetstr[80];
	char ch[80];
	char seps[]   = "- ,\t\n\r";
        char *token;
        char buf[1024];

    /* Initialized data */
	float ta0[75] = { 209.9704f,210.2097f,210.7183f,
	    211.2577f,211.7986f,212.3405f,212.8834f,213.4273f,213.9723f,
	    214.5183f,215.0652f,215.6132f,216.1626f,216.7491f,218.4804f,
	    223.1608f,229.9159f,237.0121f,244.3231f,251.8528f,259.601f,
	    267.5217f,273.8894f,274.3845f,269.4364f,263.1005f,256.6132f,
	    250.3104f,244.4832f,239.1006f,233.8843f,228.875f,224.8026f,
	    221.8245f,219.1726f,216.8415f,215.788f,215.7518f,215.7807f,
	    216.4645f,219.2781f,223.7674f,228.2605f,232.4265f,236.2986f,
	    239.9184f,243.319f,246.5257f,249.5573f,252.4255f,255.1358f,
	    257.6929f,260.1064f,262.3929f,264.5716f,266.6589f,268.6667f,
	    270.6037f,272.4764f,274.2898f,276.0482f,277.7552f,279.4135f,
	    281.0244f,282.5865f,284.0929f,285.5257f,286.8554f,288.0568f,
	    289.1321f,290.1115f,291.0293f,291.9081f,292.7597f,293.5899f };
	float wa0[75] = { 4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,
	    4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,
	    4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,
	    4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,4e-6f,
	    4.003e-6f,4.17e-6f,6.396e-6f,1.702e-5f,3.855e-5f,7.691e-5f,
	    1.385e-4f,1.948e-4f,2.521e-4f,3.203e-4f,4.006e-4f,4.94e-4f,
	    6.017e-4f,7.101e-4f,8.255e-4f,9.556e-4f,.001102f,.001266f,.00145f,
	    .001736f,.00207f,.002438f,.002841f,.003276f,.003743f,.004241f,
	    .004767f,.005319f,.005895f,.006492f,.007106f,.007734f,.008373f,
	    .009021f,.009676f,.01034f,.011f };
	float oa0[75] = { 1.793e-7f,2.011e-7f,2.446e-7f,
	    2.884e-7f,3.324e-7f,3.767e-7f,4.212e-7f,4.659e-7f,5.108e-7f,
	    5.559e-7f,6.013e-7f,6.469e-7f,6.928e-7f,7.389e-7f,8.089e-7f,
	    1.003e-6f,1.306e-6f,1.628e-6f,1.968e-6f,2.328e-6f,2.707e-6f,
	    3.108e-6f,3.664e-6f,4.604e-6f,5.945e-6f,7.59e-6f,8.93e-6f,
	    9.585e-6f,9.906e-6f,1.008e-5f,9.725e-6f,8.573e-6f,7.141e-6f,
	    5.804e-6f,4.274e-6f,2.663e-6f,1.571e-6f,1.041e-6f,7.856e-7f,
	    6.023e-7f,4.495e-7f,3.569e-7f,3.016e-7f,2.518e-7f,2.143e-7f,
	    1.932e-7f,1.762e-7f,1.599e-7f,1.467e-7f,1.366e-7f,1.271e-7f,
	    1.181e-7f,1.096e-7f,1.028e-7f,9.749e-8f,9.252e-8f,8.829e-8f,
	    8.459e-8f,8.103e-8f,7.782e-8f,7.491e-8f,7.206e-8f,6.943e-8f,
	    6.707e-8f,6.481e-8f,6.258e-8f,6.073e-8f,5.928e-8f,5.789e-8f,
	    5.651e-8f,5.519e-8f,5.398e-8f,5.282e-8f,5.167e-8f,5.054e-8f };
	float pl0[76] = { 0.f,6.244e-4f,8.759e-4f,
	    .0012286f,.0017234f,.0024174f,.0033909f,.0047565f,.006672f,
	    .0093589f,.0131278f,.0184145f,.0258302f,.0362323f,.0508234f,
	    .0712906f,.1f,.140271f,.19676f,.275997f,.3871f,.5431f,.7617f,
	    1.0685f,1.4988f,2.1024f,2.949f,4.1366f,5.8025f,8.1392f,11.417f,
	    16.0147f,22.464f,31.5105f,44.2001f,62.f,85.775f,109.55f,133.325f,
	    157.1f,180.875f,204.65f,228.425f,252.2f,275.975f,299.75f,323.525f,
	    347.3f,371.075f,394.85f,418.625f,442.4f,466.175f,489.95f,513.725f,
	    537.5f,561.275f,585.05f,608.825f,632.6f,656.375f,680.15f,703.925f,
	    727.7f,751.475f,775.25f,799.025f,822.8f,846.575f,870.35f,894.125f,
	    917.9f,941.675f,965.45f,989.225f,1013.f };

	for (k =0; k < 75; ++k) pl0[k]=(pl0[k]+pl0[k+1])/2.;	// reset pl0 at the default ta, wa, oa profiles
#ifdef lDiag
	sprintf (pout,"3.1: reset pl0\n");gaprnt (0,pout);
#endif 	
//	0.0 Set options	
/* -----options for table look-up or k-dis methods 'high'  and */
/*     'trace' for trace gases (see ir.f) */
	high = 1; // table look-up
	trace = 0;
/* -----options for aerosols */
	aerosol = 1;
/* -----set overcast=.true. if cloud fractional cover can */
/*     only be either 0 or 1. */
/*     overcast=.true. */
	overcast = 0;
	lcwc=0;	// no cwc data
	roberts=0;  // using Clough et al. (1989) (default)
	// 1.0
	if(sel==0) {	                    	                                                 	
		if (pfc->argnum<10||pfc->argnum>13) {
			gaprnt (0,"Error from LW:  10-13 arguments expected \n");
			gaprnt (0,"lw(pl,ta,wa,taucl,fcld,ps,tg,eg,tb,wb [,'binfile','ctlfile',-r-c-h-l]) \n");
			gaprnt (0,"usage:\n");
			gaprnt (0,"Function returns: thermal infrared fluxes\n");
			gaprnt (0,"Following the NASA Technical Memorandum (NASA/TM-2001-104606,\n");
			gaprnt (0,"Vol. 19) of Chou, Suarez, Liang, and Yan (2001). This NASA TM\n");
			gaprnt (0,"has been revised a few timessince. It computes thermal infrared\n");
			gaprnt (0,"fluxes due to emission by water vapor, ozone, co2, o2, minor\n");
			gaprnt (0,"trace gases, clouds, and aerosols and due to scattering by\n");
			gaprnt (0,"clouds and aerosols.\n\n");
			gaprnt (0,"Arguments:\n");
			gaprnt (0,"   pl = level pressure (hPa)\n");
			gaprnt (0,"   ta = layer temperature (K)\n");
			gaprnt (0,"   wa = layer specific humidity (kg/kg)\n");
			gaprnt (0,"   taucl= cloud optical thickness (dimensionless)\n");
			gaprnt (0,"   fcld= cloud amount (fraction)\n");
			gaprnt (0,"         (low cloud >700 hPa >= middle cloud > 400 hPa >= high cloud \n");
			gaprnt (0,"   ps = surface pressure (hPa)\n");
			gaprnt (0,"   tg = land or ocean surface temperature (K)\n");
			gaprnt (0,"   eg = land or ocean surface emissivity (fraction)\n");
			gaprnt (0,"   tb = surface air temperature (K)\n");
			gaprnt (0,"   wb = surface air specific humidity (kg/kg)\n");
			gaprnt (0,"   binfile = binary file name ('null' or blank for no output)\n");
			gaprnt (0,"   ctlfile = ctl file name ('null' or blank for no output)\n");
			gaprnt (0,"   -r = using Roberts et al. (1976) water vaopr continuum\n");
			gaprnt (0,"   -c = using Clough et al. (1989) water vaopr continuum (default)\n");
			gaprnt (0,"   -h = using look-up table (high acc but slow) (default)\n");
			gaprnt (0,"   -l = using k-dist method (low acc but fast)\n");
			return (1);
		}
		/* Check for 11th argument: get working directory and binary data filename (bin) */	
		if (pfc->argnum >= 11) {	
			getwrd(datafilename,pfc->argpnt[10],80);
		}
		/* Check for 12th argument: get working directory and descriptor filename (ctl)*/	
		if (pfc->argnum >= 12) {	
			getwrd(ch,pfc->argpnt[11],80);
			if (!cmpwrd("null",ch)&&!cmpwrd("",ch)) {
				getwrd(ctlfilename,pfc->argpnt[11],80);
			}
		}
		if (pfc->argnum >= 13) {	
			strcpy(buf,pfc->argpnt[12]);
			// gaprnt (0,buf);
	                token=strtok(buf,seps);  // Value
	                while( token != NULL )   {
                        /* While there are tokens in "string" */
				getwrd(ch,token,80);
				if (cmpwrd("r",ch)) {
					roberts=1;  // using Roberts et al. (1976)
//					gaprnt (0,"   using Roberts et al. (1976) water vaopr continuum\n");
				} else if (cmpwrd("c",ch)) {
					roberts=0;  // using Clough et al. (1989) (default)
//					gaprnt (0,"   using Clough et al. (1989) water vaopr continuum\n");
				} else if (cmpwrd("h",ch)) {
					high=1; // table look-up 
//					gaprnt (0,"   using table look-up\n");
				} else if (cmpwrd("l",ch)) {
					high=0; // k-dis methods 
//					gaprnt (0,"   using  k-dis methods\n");
				}
			        token = strtok( NULL, seps );
			}
		}
	} else if (sel==1) {
		if (pfc->argnum<13||pfc->argnum>16) {
			gaprnt (0,"Error from LW2:  13-16 arguments expected \n");
			gaprnt (0,"lw2(pl,ta,wa,taucl,fcld,ps,tg,eg,tb,wb,pm,tm,wm [,'binfile','ctlfile',-r-c-h-l]) \n");
			gaprnt (0,"usage:\n");
			gaprnt (0,"Function returns: thermal infrared fluxes\n");
			gaprnt (0,"Following the NASA Technical Memorandum (NASA/TM-2001-104606,\n");
			gaprnt (0,"Vol. 19) of Chou, Suarez, Liang, and Yan (2001). This NASA TM\n");
			gaprnt (0,"has been revised a few timessince. It computes thermal infrared\n");
			gaprnt (0,"fluxes due to emission by water vapor, ozone, co2, o2, minor\n");
			gaprnt (0,"trace gases, clouds, and aerosols and due to scattering by\n");
			gaprnt (0,"clouds and aerosols.\n\n");
			gaprnt (0,"Arguments:\n");
			gaprnt (0,"   pl = level pressure (hPa)\n");
			gaprnt (0,"   ta = layer temperature (K)\n");
			gaprnt (0,"   wa = layer specific humidity (kg/kg)\n");
			gaprnt (0,"   taucl= cloud optical thickness (dimensionless)\n");
			gaprnt (0,"   fcld= cloud amount (fraction)\n");
			gaprnt (0,"         (low cloud >700 hPa >= middle cloud > 400 hPa >= high cloud \n");
			gaprnt (0,"   ps = surface pressure (hPa)\n");
			gaprnt (0,"   tg = land or ocean surface temperature (K)\n");
			gaprnt (0,"   eg = land or ocean surface emissivity (fraction)\n");
			gaprnt (0,"   tb = surface air temperature (K)\n");
			gaprnt (0,"   wb = surface air specific humidity (kg/kg)\n");
			gaprnt (0,"   pm = pressure at the top of mixed layer (hPa)\n");
			gaprnt (0,"   tm = temperature at the top of mixed layer (K)\n");
			gaprnt (0,"   wm = specific humidity at the top of mixed layer (kg/kg)\n");
			gaprnt (0,"   binfile = binary file name ('null' or blank for no output)\n");
			gaprnt (0,"   ctlfile = ctl file name ('null' or blank for no output)\n");
			gaprnt (0,"   -r = using Roberts et al. (1976) water vaopr continuum\n");
			gaprnt (0,"   -c = using Clough et al. (1989) water vaopr continuum (default)\n");
			gaprnt (0,"   -h = using look-up table (high acc but slow) (default)\n");
			gaprnt (0,"   -l = using k-dist method (low acc but fast)\n");
			return (1);
		}
		/* Check for 14th argument: get working directory and binary data filename (bin) */	
		if (pfc->argnum >= 14) {	
			getwrd(datafilename,pfc->argpnt[13],80);
		}		
		/* Check for 15th argument: get working directory and descriptor filename (ctl)*/	
		if (pfc->argnum >= 15) {	
			getwrd(ch,pfc->argpnt[14],80);
			if (!cmpwrd("null",ch)&&!cmpwrd("",ch)) {
				getwrd(ctlfilename,pfc->argpnt[14],80);
			}
		}
		if (pfc->argnum >= 16) {	
			strcpy(buf,pfc->argpnt[15]);
	                token=strtok(buf,seps);  // Value
	                while( token != NULL )   {
                        /* While there are tokens in "string" */
				getwrd(ch,token,80);
				if (cmpwrd("-r",ch)) {
					roberts=1;  // using Roberts et al. (1976)
//					gaprnt (0,"   using Roberts et al. (1976) water vaopr continuum\n");
				} else if (cmpwrd("-c",ch)) {
					roberts=0;  // using Clough et al. (1989) (default)
//					gaprnt (0,"   using Clough et al. (1989) water vaopr continuum\n");
				} else if (cmpwrd("-h",ch)) {
					high=1; // table look-up 
//					gaprnt (0,"   using table look-up\n");
				} else if (cmpwrd("-l",ch)) {
					high=0; // k-dis methods 
//					gaprnt (0,"   using  k-dis methods\n");
				}
			        token = strtok( NULL, seps );
			}
		}
	}

	/* Get the range of levels from the default file.  Set the
	level in the status block to the first level.  */
	
	pfi = pst->pfid;
	lvt = pfi->dnum[2];	// size of levels
	lvvals = pfi->grvals[2];
	lvconv = pfi->gr2ab[2];
	conv = pfi->ab2gr[2];

	if (lvt<3) {
		sprintf (pout,"Error from %s:  Too few levels in default file\n",lwnam[sel]);gaprnt (0,pout);
		return (1);
	}
	
	/* Get the surface pressure field (6th arg).  User is responsible
	for valid argument.  Then get the lowest level of the
	field to integrate.  */	
	if (pst->idim==2) {
		pst->idim = pst->jdim;
		pst->jdim = -1;
	}
	if (pst->jdim==2) pst->jdim = -1;
	rc = gaexpr(pfc->argpnt[5],pst);
	if (rc) {
		sprintf (pout,"Error from %s:  read %s error. \n",lwnam[sel],pfc->argpnt[5]);gaprnt (0,pout);
		return (1);
	}
	if (pst->type==0) {
		rc = -1;
		sprintf (pout,"Error from %s:  %s must be grid data. \n",lwnam[sel],pfc->argpnt[5]);gaprnt (0,pout);
		gafree (pst);
		return (-1);
	}

	pgr= pst->result.pgr;	
	m = pgr->isiz * pgr->jsiz;
	// date string
	sprintf(date,"%i:%iZ%i%s%i",pst->tmin.hr,pst->tmin.mn,pst->tmin.dy,mon[pst->tmin.mo-1],pst->tmin.yr);
	if (*(pst->pfid->grvals[3]+5)>0) {
		if (0==(int)*(pst->pfid->grvals[3]+5)%12) {
			sprintf(dtstr,"%iyr",(int)*(pst->pfid->grvals[3]+5)/12);
			sprintf(yymmddhh,"%04i",pst->tmin.yr);
			sprintf(dsetstr,"dset %s.%%y4.bin",datafilename);			
		} else {
			sprintf(dtstr,"%imo",(int)*(pst->pfid->grvals[3]+5));
			sprintf(yymmddhh,"%04i%02i",pst->tmin.yr,pst->tmin.mo);
			sprintf(dsetstr,"dset %s.%%y4%%m2.bin",datafilename);			
		}
	} else if (*(pst->pfid->grvals[3]+6)>0) {
		if (0==(int)*(pst->pfid->grvals[3]+6)%(24*60)) {
			sprintf(dtstr,"%idy",(int)*(pst->pfid->grvals[3]+6)/(24*60));
			sprintf(yymmddhh,"%04i%02i%02i",pst->tmin.yr,pst->tmin.mo,pst->tmin.dy);
			sprintf(dsetstr,"dset %s.%%y4%%m2%%d2.bin",datafilename);			
		} else if (0==(int)*(pst->pfid->grvals[3]+6)%60) {
			sprintf(dtstr,"%ihr",(int)*(pst->pfid->grvals[3]+6)/60);
			sprintf(yymmddhh,"%04i%02i%02i%02i",pst->tmin.yr,pst->tmin.mo,pst->tmin.dy,pst->tmin.hr);
			sprintf(dsetstr,"dset %s.%%y4%%m2%%d2%%h2.bin",datafilename);			
		} else {
			sprintf(dtstr,"%imn",(int)*(pst->pfid->grvals[3]+6));
			sprintf(yymmddhh,"%04i%02i%02i%02i%02i",pst->tmin.yr,pst->tmin.mo,pst->tmin.dy,pst->tmin.hr,
				pst->tmin.mn);
			sprintf(dsetstr,"dset %s.%%y4%%m2%%d2%%h2%%n2.bin",datafilename);			
		}
	}						
	strcpy(fulldatafilename,datafilename);
	strcat(fulldatafilename,".");
	strcat(fulldatafilename,yymmddhh);
	strcat(fulldatafilename,".bin");
#ifdef lDiag
	sprintf (pout,"finished 1.0 read arguments\n");gaprnt (0,pout);
#endif 	
//	2.0 allocate memory lvt+1 (ML)
	if (0==sel) lvt1=lvt;
	else if (1==sel) lvt1=lvt+1;	// extra one level for the mixed layer
	nta=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(nta+i) = 0;
	nwa=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(nwa+i) = 0;
	noa=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(noa+i) = 0;
	ntaucl=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(ntaucl+i) = 0;
	nfcld=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(nfcld+i) = 0;

	ict=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(ict+i) = -999;
	icb=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(icb+i) = -999;
	
	pl=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(pl+i) = pgr->undef;
	ta=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(ta+i) = pgr->undef;
	wa=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(wa+i) = pgr->undef;
	oa=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(oa+i) = pgr->undef;
//	taucl=(float *)malloc(sizeof(float)*m*lvt1*3); for (i=0; i<m*lvt1*3; i++) *(taucl+i) = 4.1881;
	taucl=(float *)malloc(sizeof(float)*m*lvt1*3); for (i=0; i<m*lvt1*3; i++) *(taucl+i) = pgr->undef;
//	fcld=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(fcld+i) = 0.;
	fcld=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(fcld+i) = pgr->undef;
	taual=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(taual+i) = pgr->undef;
	ssaal=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(ssaal+i) = pgr->undef;
	asyal=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(asyal+i) = pgr->undef;

	np=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(np+i) = 0;
	tb=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(tb+i) = pgr->undef;
	wb=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(wb+i) = pgr->undef;
	lsfc=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(lsfc+i) = 1;	// logical for setting ps

	fs=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(fs+i) = 1.;
	tg=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(tg+i) = pgr->undef;
	eg=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(eg+i) = 1.;
//	eg=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(eg+i) = pgr->undef;
//	tv=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(tv+i) = pgr->undef;
//	ev=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(ev+i) = pgr->undef;
	rv=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(rv+i) = 0.;
        if (sel==1) {
		nm=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(nm+i) = 0;
		pm=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(pm+i) = pgr->undef;
		tm=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(tm+i) = pgr->undef;
		wm=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(wm+i) = pgr->undef;
		lml=(int *)malloc(sizeof(int)*m); for (i=0; i<m; i++) *(lml+i) = 1;	// logical for setting mixed layer
		if (!nm||!pm||!tm||!wm||!lml) { sprintf(pout,"Out of memory\n");gaprnt (1,pout);rc=1;goto erret;}	
	}
// -----output parameters
	flx=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flx+i) = pgr->undef;
	flxu=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flxu+i) = pgr->undef;
	flxd=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flxd+i) = pgr->undef;
	flc=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flc+i) = pgr->undef;
	flcu=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flcu+i) = pgr->undef;
	flcd=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(flcd+i) = pgr->undef;
	dfdts=(float *)malloc(sizeof(float)*m*(lvt1+1)); for (i=0; i<m*(lvt1+1); i++) *(dfdts+i) = pgr->undef;
	coolr=(float *)malloc(sizeof(float)*m*lvt1); for (i=0; i<m*lvt1; i++) *(coolr+i) = pgr->undef;
	sfcem=(float *)malloc(sizeof(float)*m); for (i=0; i<m; i++) *(sfcem+i) = pgr->undef;

// -----temporay parameters
	reff=(float *)malloc(sizeof(float)*m*lvt1*3); for (i=0; i<m*lvt1*3; i++) *(reff+i) = pgr->undef;
	
	if (!nta||!nwa||!noa||!ntaucl||!nfcld||!ict||!icb||
	!pl||!ta||!wa||!oa||!taucl||!fcld||!taual||!ssaal||!asyal||
	!np||!tb||!wb||!lsfc||
	!fs||!tg||!eg||!rv||
	!flx||!flxu||!flxd||!flc||!flcu||!flcd||!dfdts||!coolr||!sfcem) {
		sprintf(pout,"Out of memory\n");gaprnt (1,pout);rc=1;goto erret;
	}	

#ifdef lDiag
	sprintf (pout,"finished 2.0 allocate memory\n");
#endif 	
//	3.0 Get surface (6-9) and mixed layer (ML) (6-12) data
	if (0==sel) iend=9;
	else if (1==sel) iend=12;	
	for (i=6; i<=iend; i++) {
		rc = gaexpr(pfc->argpnt[i],pst);
		if (rc) {
			sprintf (pout,"Error from %s:  read %s error.\n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
			goto erret;
		}
		if (pst->type==0) {
			rc = -1;
			sprintf (pout,"Error from %s:  %s must be grid data.\n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
			gafree (pst);
			goto erret;
		}
		/* Check that the two grids are equivalent.  */	
		if (((pst->result.pgr->isiz*pst->result.pgr->jsiz)!=1)&&
		(pst->result.pgr->isiz*pst->result.pgr->jsiz)!=m) {
			sprintf (pout,"Error from %s:  Incompatible grids for %s. \n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
			gafree (pst);
			goto erret;
		}
		
		for (j=0; j<m; j++) {
			if (pst->result.pgr->isiz*pst->result.pgr->jsiz!=1) {
				if(pst->result.pgr->grid[j]!=pst->result.pgr->undef) {
					if(6==i) {for (k=0; k<ns; k++) tg[k*m+j]=pst->result.pgr->grid[j];} // for ns land types
					else if(7==i) {for (k=0; k<ns*nb; k++) eg[k*m+j]=pst->result.pgr->grid[j];}
					else if(8==i) tb[j]=pst->result.pgr->grid[j];
					else if(9==i) wb[j]=pst->result.pgr->grid[j];
					else if(10==i) pm[j]=pst->result.pgr->grid[j];
					else if(11==i) tm[j]=pst->result.pgr->grid[j];
					else if(12==i) wm[j]=pst->result.pgr->grid[j];
				}
			} else {
				if(pst->result.pgr->grid[0]!=pst->result.pgr->undef) {
					if(6==i) {for (k=0; k<ns; k++) tg[k*m+j]=pst->result.pgr->grid[0];} // for n land types
					else if(7==i) {for (k=0; k<ns*nb; k++) eg[k*m+j]=pst->result.pgr->grid[0];} // for nb bands
					else if(8==i) tb[j]=pst->result.pgr->grid[0];
					else if(9==i) wb[j]=pst->result.pgr->grid[0];
					else if(10==i) pm[j]=pst->result.pgr->grid[0];
					else if(11==i) tm[j]=pst->result.pgr->grid[0];
					else if(12==i) wm[j]=pst->result.pgr->grid[0];
				}
			}		
		}
		gafree (pst);
	}
#ifdef lDiag
	sprintf (pout,"finished 3.0: Get surface and mixed layer data\n");gaprnt (0,pout);
#endif 	
//	3.1 Initial level data from top to bottom
	for (j=lvt; j>=1; j--) {
#ifdef lDiag
	sprintf (pout,"3.1: lvt=%d; j=%d\n",lvt,j);gaprnt (0,pout);
#endif 	
		clev = lvconv(lvvals, (float)j);
		pst->dmin[2] = clev;
		pst->dmax[2] = clev;
#ifdef lDiag
	sprintf (pout,"3.1: clev=%g\n",clev);gaprnt (0,pout);
#endif 	
		for (i=0; i<=0; i++) {
			rc = gaexpr(pfc->argpnt[i],pst);
#ifdef lDiag
	sprintf (pout,"3.1: m=%d, i=%d, rc=%d\n",m,i,rc);gaprnt (0,pout);
#endif 	
			if (rc) {
				sprintf (pout,"Error from LW:  read %s error. \n",pfc->argpnt[i]);gaprnt (0,pout);
				goto erret;
			}
			if (pst->type==0) {
				rc = -1;
				sprintf (pout,"Error from %s:  %s must be grid data. \n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
				gafree (pst);
				goto erret;
			}
			/* Check that the two grids are equivalent.  */	
			if ((pst->result.pgr->isiz*pst->result.pgr->jsiz!=1)&&
			(pst->result.pgr->isiz*pst->result.pgr->jsiz!=m)) {
				sprintf (pout,"Error from %s:  Incompatible grids for var%i. \n",lwnam[sel],i+1);gaprnt (0,pout);
				gafree (pst);
				goto erret;
			}
			if(0==i) pgrpl= pst->result.pgr;		
		}
		gafree (pst);
	}              	

#ifdef lDiag
	sprintf (pout,"finished 3.1: Initial level data from top to bottom\n");gaprnt (0,pout);
#endif 	
//	3.2 Get level data from top to bottom */	
	for (j=lvt; j>=1; j--) {
		clev = lvconv(lvvals, (float)j);
		pst->dmin[2] = clev;
		pst->dmax[2] = clev;
		for (i=0; i<=4; i++) {
			rc = gaexpr(pfc->argpnt[i],pst);
			if (rc) {
				sprintf (pout,"Error from %s:  read %s error. \n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
				goto erret;
			}
			if (pst->type==0) {
				rc = -1;
				sprintf (pout,"Error from %s:  %s must be grid data. \n",lwnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
				gafree (pst);
				goto erret;
			}
			/* Check that the two grids are equivalent.  */	
			if ((pst->result.pgr->isiz*pst->result.pgr->jsiz!=1)&&
			(pst->result.pgr->isiz*pst->result.pgr->jsiz!=m)) {
				sprintf (pout,"Error from %s:  Incompatible grids for var%i. \n",lwnam[sel],i+1);gaprnt (0,pout);
				gafree (pst);
				goto erret;
			}
			if(0==i) pgrpl= pst->result.pgr;		
			else if(1==i) pgrta= pst->result.pgr;		
			else if(2==i) pgrwa= pst->result.pgr;		
			else if(3==i) pgrtaucl= pst->result.pgr;		
			else if(4==i) pgrfcld= pst->result.pgr;		
		}
		ptrps = pgr->grid;
		ptrpl = pgrpl->grid;
		ptrta = pgrta->grid;
		ptrwa = pgrwa->grid;
		ptrtaucl = pgrtaucl->grid;
		ptrfcld = pgrfcld->grid;
		for (i=0; i<m; i++) {
			/* Neglect below surface and mixing height*/	
			if (*ptrps==pgr->undef) {
#ifdef lDiag
				sprintf (pout,"Error from %s: grid %i, lvl %i, undef value in ps.\n",lwnam[sel],i,j);gaprnt (0,pout);
#endif					
			} else if (*ptrpl==pgrpl->undef) {
				sprintf (pout,"Error from %s:  undef value in pl.\n",lwnam[sel]);gaprnt (0,pout);
			} else if (1==sel&&pm[i]==pgr->undef) {
#ifdef lDiag
				sprintf (pout,"Error from %s: grid %i, lvl %i, undef value in pm.\n",lwnam[sel],i,j);gaprnt (0,pout);
#endif					
			} else if (1==sel&&*ptrps<=pm[i]) {
				sprintf (pout,"Error from %s: grid %i, lvl %i, ps<=pm.\n",lwnam[sel],i,j);gaprnt (0,pout);
			} else if (1==sel&&pm[i]>*ptrpl&&
					// interpolation for pl< 300 hPa pnly
					((*ptrta!=pgrta->undef&&*ptrwa!=pgrwa->undef&&*ptrfcld!=pgrfcld->undef)||*ptrpl<300)) {
				pl[np[i]*m+i]=*ptrpl;
				// corresponding level of default profile
				k=0;
				while ((*ptrpl-pl0[k])*(*ptrpl-pl0[k+1])>0&&k<74) k++;

				// ta
				if(*ptrta!=pgrta->undef) {
					xx=*ptrta;
					nta[i]++;
				} else {	// default value
					if (k<74) {
						xx=ta0[k]+(ta0[k+1]-ta0[k])/(log(pl0[k+1])-log(pl0[k]))*(log(*ptrpl)-log(pl0[k]));						
					} else {
						xx=ta0[74];
					}
				}
				ta[np[i]*m+i]=0.5*xx;
				if (0!=np[i]&&ta[(np[i]-1)*m+i]!=pgr->undef) ta[(np[i]-1)*m+i]+=0.5*xx;					

				// wa
				if(*ptrwa!=pgrwa->undef) {
					xx=*ptrwa;
					nwa[i]++;
				} else {	// default value
					if (k<74) {
						xx=wa0[k]+(wa0[k+1]-wa0[k])/(log(pl0[k+1])-log(pl0[k]))*(log(*ptrpl)-log(pl0[k]));						
					} else {
						xx=wa0[74];
					}
				}
				wa[np[i]*m+i]=0.5*xx;
				if (0!=np[i]&&wa[(np[i]-1)*m+i]!=pgr->undef) wa[(np[i]-1)*m+i]+=0.5*xx;					

				// cloud should be at particular level, no interpolation allowed
				// taucl
				if(*ptrtaucl!=pgrtaucl->undef) {
					xx=*ptrtaucl;
					ntaucl[i]++;
				} else {
					// default value 4.1881 (isccp D2 tau global mean of 10 yr mean from 9/1983 - 8/1993)
					xx=4.1881;
				}
				for (k=0; k<3; k++) {
					if (0!=np[i]) taucl[(k*lvt1+np[i]-1)*m+i]=xx;
//					taucl[(k*lvt1+np[i])*m+i]=xx;
//					taucl[(k*lvt1+np[i])*m+i]=0.5*xx;
//					if (0!=np[i]&&taucl[(k*lvt1+np[i]-1)*m+i]!=pgr->undef) taucl[(k*lvt1+np[i]-1)*m+i]+=0.5*xx;
				}
				
				// fcld
				if(*ptrfcld!=pgrfcld->undef) {
					xx=*ptrfcld;
					nfcld[i]++;
					if (0!=np[i]) fcld[(np[i]-1)*m+i]=xx;
//					fcld[np[i]*m+i]=xx;
//					fcld[np[i]*m+i]=0.5*xx;
//					if (0!=np[i]&&fcld[(np[i]-1)*m+i]!=pgr->undef) fcld[(np[i]-1)*m+i]+=0.5*xx;
				}
				nm[i]++;	// mixing layer index
				np[i]++;	// surface index	
			} else if (*ptrps>*ptrpl &&
					// interpolation for pl< 300 hPa only
					((*ptrta!=pgrta->undef&&*ptrwa!=pgrwa->undef&&*ptrfcld!=pgrfcld->undef)||*ptrpl<300)) {
				if (1==sel&&lml[i]) {
					// mixed layer
					pl[np[i]*m+i] = pm[i];	// Set np[i] at pm
					if (ta[(np[i]-1)*m+i]!=pgr->undef&&tm[i]!=pgr->undef) ta[(np[i]-1)*m+i]+=0.5*tm[i];
					if (tm[i]!=pgr->undef) ta[np[i]*m+i]=0.5*tm[i];
					if (wa[(np[i]-1)*m+i]!=pgr->undef&&wm[i]!=pgr->undef) wa[(np[i]-1)*m+i]+=0.5*wm[i];
					if (wm[i]!=pgr->undef) wa[np[i]*m+i]=0.5*wm[i];
					// asuume no cloud at MH & surface
					for (k=0; k<3; k++) {
						if (0!=np[i]) taucl[(k*lvt1+np[i]-1)*m+i]=0.;
					}
					if (0!=np[i]) fcld[(np[i]-1)*m+i]=0.;
					lml[i]=0;	// mixed layer logical
					np[i]++;	// surface level index
				}
				pl[np[i]*m+i]=*ptrpl;
				// corresponding level of default profile
				k=0;
				while ((*ptrpl-pl0[k])*(*ptrpl-pl0[k+1])>0&&k<74) k++;
				// ta
				if(*ptrta!=pgrta->undef) {
					xx=*ptrta;
					nta[i]++;
				} else {	// default value
					if (k<74) {
						xx=ta0[k]+(ta0[k+1]-ta0[k])/(log(pl0[k+1])-log(pl0[k]))*(log(*ptrpl)-log(pl0[k]));						
					} else {
						xx=ta0[74];
					}
				}
				ta[np[i]*m+i]=0.5*xx;
				if (0!=np[i]&&ta[(np[i]-1)*m+i]!=pgr->undef) ta[(np[i]-1)*m+i]+=0.5*xx;					
				// wa
				if(*ptrwa!=pgrwa->undef) {
					xx=*ptrwa;
					nwa[i]++;
				} else {	// default value
					if (k<74) {
						xx=wa0[k]+(wa0[k+1]-wa0[k])/(log(pl0[k+1])-log(pl0[k]))*(log(*ptrpl)-log(pl0[k]));						
					} else {
						xx=wa0[74];
					}
				}
				wa[np[i]*m+i]=0.5*xx;
				if (0!=np[i]&&wa[(np[i]-1)*m+i]!=pgr->undef) wa[(np[i]-1)*m+i]+=0.5*xx;					
				// taucl
				if(*ptrtaucl!=pgrtaucl->undef) {
					xx=*ptrtaucl;
					ntaucl[i]++;
				} else {
					// default value 4.1881 (isccp D2 tau global mean of 10 yr mean from 9/1983 - 8/1993)
					xx=4.1881;
				}
				for (k=0; k<3; k++) {
					if (0!=np[i]) taucl[(k*lvt1+np[i]-1)*m+i]=xx;
				}
				
				// fcld
				if(*ptrfcld!=pgrfcld->undef) {
					xx=*ptrfcld;
					nfcld[i]++;
					if (0!=np[i]) fcld[(np[i]-1)*m+i]=xx;
				}
				np[i]++;	// surface index	
			}
			if(1==j) {
				if (1==sel&&lml[i]) {
					// mixed layer
					pl[np[i]*m+i] = pm[i];	// Set np[i] at pm
					if (ta[(np[i]-1)*m+i]!=pgr->undef&&tm[i]!=pgr->undef) ta[(np[i]-1)*m+i]+=0.5*tm[i];
					if (tm[i]!=pgr->undef) ta[np[i]*m+i]=0.5*tm[i];
					if (wa[(np[i]-1)*m+i]!=pgr->undef&&wm[i]!=pgr->undef) wa[(np[i]-1)*m+i]+=0.5*wm[i];
					if (wm[i]!=pgr->undef) wa[np[i]*m+i]=0.5*wm[i];
					// asuume no cloud at MH & surface
					for (k=0; k<3; k++) {
						if (0!=np[i]) taucl[(k*lvt1+np[i]-1)*m+i]=0.;
					}
					if (0!=np[i]) fcld[(np[i]-1)*m+i]=0.;
					lml[i]=0;	// mixed layer logical
					np[i]++;	// surface level index
				}
				if (lsfc[i]) {
					// surface
					pl[np[i]*m+i] = *ptrps;	// Set bottom pressure at ps
					if (ta[(np[i]-1)*m+i]!=pgr->undef&&tb[i]!=pgr->undef) ta[(np[i]-1)*m+i]+=0.5*tb[i];
					if (wa[(np[i]-1)*m+i]!=pgr->undef&&wb[i]!=pgr->undef) wa[(np[i]-1)*m+i]+=0.5*wb[i];
					// asuume no cloud at MH & surface
					for (k=0; k<3; k++) {
						if (0!=np[i]) taucl[(k*lvt1+np[i]-1)*m+i]=0.;
					}
					if (0!=np[i]) fcld[(np[i]-1)*m+i]=0.;
					lsfc[i]=0;
				}
			}
			if (pgr->isiz*pgr->jsiz!=1) ptrps++;
			if (pgrpl->isiz*pgrpl->jsiz!=1) ptrpl++;
			if (pgrta->isiz*pgrta->jsiz!=1) ptrta++;
			if (pgrwa->isiz*pgrwa->jsiz!=1) ptrwa++;
			if (pgrtaucl->isiz*pgrtaucl->jsiz!=1) ptrtaucl++;
			if (pgrfcld->isiz*pgrfcld->jsiz!=1) ptrfcld++;
		}
		gagfre (pgrpl);
		gagfre (pgrta);
		gagfre (pgrwa);
		gagfre (pgrtaucl);
		gagfre (pgrfcld);
	}              	
#ifdef lDiag
	sprintf (pout,"finished 3.2: Get level data from top to bottom\n");gaprnt (0,pout);
#endif 	
//	3.3 Set others	
/* -----input atmospheric profiles */
/*     mid-latitude summer  :  p(mb)  t(k)  h2o(g/g)  o3(g/g) */
/*     np=75 */
/* -----assign co2 amount. units are parts/part by volumn */
	co2 = 3.5e-4f;
	n2o = 2.8e-7f;
	ch4 = 1.75e-6f;
	cfc11 = 3e-10f;
	cfc12 = 5e-10f;
	cfc22 = 2e-10f;
	for (i =0; i < m; ++i) {
		for (j =0; j < np[i]; ++j) {
			reff[i+j*m]=40.0f;
			reff[i+(j+lvt)*m]=10.0f;
		}
	}
/* -----specify aerosol optical, ozone properties */
	for (i = 0; i < m; ++i) {
		for (k = 0; k < np[i]; ++k) {
			xx=exp((log(pl[i + k * m])+log(pl[i + (k+1) * m]))/2.);	// geometric mean
			// specify aerosol optical properties
			for (j =0; j < na; ++j) {
				for (ib = 0; ib < nb; ++ib) {
					if (xx> 850.) {	// > 850 hPa
						taual[i + (k + (ib + j*nb) * lvt1) * m ] = .0050000000000000001f;
						ssaal[i + (k + (ib + j*nb) * lvt1) * m ] = .99f;
						asyal[i + (k + (ib + j*nb) * lvt1) * m ] = .75f;
					} else {
						taual[i + (k + (ib + j * nb) * lvt1) * m ] = 0.f;
						ssaal[i + (k + (ib + j * nb) * lvt1) * m ] = .99f;
						asyal[i + (k + (ib + j * nb) * lvt1) * m ] = .75f;                                  				
					}
				}
			}
			// oa (default value)
			// corresponding level of default profile
			j=0;
			while ((xx-pl0[j])*(xx-pl0[j+1])>0&&j<74) j++;
			if (j<74) {
				oa[k*m+i]=oa0[j]+(oa0[j+1]-oa0[j])/(log(pl0[j+1])-log(pl0[j]))*(log(xx)-log(pl0[j]));						
			} else {
				oa[k*m+i]=oa0[74];
			}
#ifdef lDiag
			if (i==0 && k==5) { sprintf (pout,"pressure=%g, j=%i, o3=%g, np=%i, \n",xx,j,oa[k*m+i],np[i]);gaprnt (0,pout);}
#endif 	
		}
	}

/* -----specify cwc and fcld
	for (i = 0; i < m; ++i) {
		cwc[i + 45*m] = 3.0000000000000001e-5f;
		cwc[i + 46*m] = 3.4999999999999997e-5f;
		cwc[i + 48*m] = 5.0000000000000002e-5f;
		cwc[i + 50*m] = 2e-5f;
		cwc[i + (50+lvt)*m] = 4e-5f;
		cwc[i + (51+lvt)*m] = 5e-5f;
		cwc[i + 66*m] = 2.0000000000000002e-5f;
		cwc[i + (66+lvt)*m] = 4.0000000000000003e-5f;
		cwc[i + (67+lvt)*m] = 5.0000000000000002e-5f;
		cwc[i + (69+lvt)*m] = 5.0000000000000002e-5f;
	}
	*/
#ifdef lDiag
	sprintf (pout,"finished 3.3: Set options and others\n");gaprnt (0,pout);
	sprintf (pout,"m=%d\n",m);gaprnt (0,pout);
#endif 	
//	3.4 Set clouds	
/* -----specify level indices separating high clouds from middle */
/*      clouds (ict)~400 hPa, and middle clouds from low clouds (icb)~700 hPa */
	for (i =0; i < m; ++i) {
		dp400=9.9e33;
		dp700=9.9e33;
		for (j =0; j <= np[i]; ++j) {
			if(abs(pl[i+j*m]-400.)<dp400) {ict[i]=j+1;dp400=abs(pl[i+j*m]-400.);}
			if(abs(pl[i+j*m]-700.)<dp700) {icb[i]=j+1;dp700=abs(pl[i+j*m]-700.);}
		}
#ifdef lDiag
	sprintf (pout,"i=%d, m=%d, ict=%d, icb=%d\n",i,m,ict[i],icb[i]);gaprnt (0,pout);
#endif 	
	}
#ifdef lDiag
	sprintf (pout,"Out: i=%d, m=%d, ict=%d, icb=%d\n",i,m,ict[i],icb[i]);gaprnt (0,pout);
#endif 	
#ifdef lDiag
	sprintf (pout,"finished 3.4 Set clouds\n");gaprnt (0,pout);
#endif 	
//	4.0 Computes effective radius of cloud particles, reff.
/*      See Section 4.3 (Effective radius of cloud paritcles) */
	if (lcwc) {
		cldreff_(&m,&lvt,np,pl,ta,cwc,reff);
	}
#ifdef lDiag
	sprintf (pout,"finished 4.0: Computes effective radius of cloud particles, reff.\n");gaprnt (0,pout);
#endif 	
//	5.0 Compute fluxes due to h2o, co2, o3, trace gases, clouds and aerosols
//	goto print;
	irrad_ (&m,&lvt1,np,nta,nwa,noa,ntaucl,nfcld,&(pgr->undef),
	        pl,ta,wa,oa,tb,&co2,
		&high,&trace,&n2o,&ch4,&cfc11,&cfc12,&cfc22,
		&overcast,&lcwc,cwc,taucl,fcld,ict,icb,reff,
		&ns,fs,tg,eg,tg,eg,rv,
		&aerosol,&na,taual,ssaal,asyal,&roberts,
		flx,flc,dfdts,sfcem,flxu,flxd,flcu,flcd);
			
#ifdef lDiag
	sprintf (pout,"finished 5.0: Compute fluxes due to h2o, co2, o3, trace gases, clouds and aerosols\n");gaprnt (0,pout);
#endif 	
//	6.0 Compute cooling rate profile
//	goto print;
	for (i=0; i< m; ++i) {
		for (k =0; k < np[i]; ++k) {
			if (pl[i+(k+1)*m]>pl[i+k*m] && pl[i+(k+1)*m]!=pgr->undef && pl[i+k*m]!=pgr->undef &&
					flx[i+(k+1)*m]!=pgr->undef && flx[i+k*m]!=pgr->undef){ 
				coolr[i+k*m]=(flx[i+(k+1)*m]-flx[i+k*m])*8.441874f/(pl[i+(k+1)*m]-pl[i+k*m]);
			}
		}
	}
	// set surface (mixed layer) cooling rate
	ptrps = pgr->grid;
	for (i=0; i< m; ++i) {
		if (0==sel) {
			*ptrps=coolr[i+(np[i]-1)*m];
		} else if (1==sel) {
			if (np[i]>0&&nm[i]>0&&(pl[i+np[i]*m]>pl[i+nm[i]*m])&&pl[i+np[i]*m]!=pgr->undef&&
					pl[i+nm[i]*m]!=pgr->undef&&flx[i+np[i]*m]!=pgr->undef && flx[i+nm[i]*m]!=pgr->undef)
			{	
				*ptrps=(flx[i+np[i]*m]-flx[i+nm[i]*m])*8.441874f/(pl[i+np[i]*m]-pl[i+nm[i]*m]);
			} else *ptrps=pgr->undef;
		}
		if (pgr->isiz*pgr->jsiz!=1) ptrps++;
	}
#ifdef lDiag
	sprintf (pout,"finished 6.0: Compute cooling rate profile\n");gaprnt (0,pout);
#endif 	
//	7.0 Print out data
print:
//	7.1 Return surface cooling rate
	rc = gaexpr(pfc->argpnt[5],pst);
	gagfre (pst->result.pgr);
	pst->result.pgr=pgr;
	
//	7.2 Print
	if (!cmpwrd("null",datafilename)) {
		fpdata=fopen(fulldatafilename,"w+b");
		if(!fpdata) {
			sprintf (pout,"Error from %s:  open %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) {
			fpctl=fopen(ctlfilename,"w");
			if(!fpctl) {
				sprintf (pout,"Error from %s:  open %s file error!\n",lwnam[sel],ctlfilename);
				gaprnt (0,pout);			
			}
			// write ctl file
			sprintf(pout,"%s:  write %s file!\n",lwnam[sel],ctlfilename);gaprnt (0,pout);			
			fprintf(fpctl,"%s\n",dsetstr);			
			fprintf(fpctl,"undef %e\n",pgr->undef);
			fprintf(fpctl,"title %s(",lwnam[sel]);		
			for (i=0; i<pfc->argnum; i++) {
				if (i!= pfc->argnum-1) fprintf(fpctl,"%s,",pfc->argpnt[i]);
				else fprintf(fpctl,"%s",pfc->argpnt[i]);
			}
			fprintf(fpctl,")\n");
			fprintf(fpctl,"options zrev template\n",pgr->undef);
			fprintf(fpctl,"xdef %d linear %g %g\n",pst->result.pgr->isiz,
				pst->result.pgr->igrab(pst->result.pgr->ivals,pst->result.pgr->dimmin[pst->result.pgr->idim])
				,*(pst->result.pgr->ivals));
			if(pst->result.pgr->jlinr==1){
				// linear scaling info
				fprintf(fpctl,"ydef %d linear %g %g\n",pst->result.pgr->jsiz,
					pst->result.pgr->igrab(pst->result.pgr->jvals,pst->result.pgr->dimmin[pst->result.pgr->jdim])
					,*(pst->result.pgr->jvals));
			} else {
				// non-linear scaling info		
				fprintf(fpctl,"ydef %d levels  ",pst->result.pgr->jsiz);
				for (j=0; j<pst->result.pgr->jsiz; j++) {
					fprintf(fpctl,"%g ",pst->result.pgr->jvals[j+1]);
					if (0==(j+1)%10) {
						fprintf(fpctl,"\n");
					}
				}
				fprintf(fpctl,"\n");
			}
			fprintf(fpctl,"zdef %d linear 1 1\n",lvt1+1);
			fprintf(fpctl,"tdef %d linear %s %s\n",999,date,dtstr);
			if (0==sel) fprintf(fpctl,"vars %i\n",28);
			else if (1==sel) fprintf(fpctl,"vars %i\n",38);
		}			
		// write bin file
		sprintf (pout,"%s:  write %s file!\n",lwnam[sel],fulldatafilename);gaprnt (0,pout);			
		for (i=0; i< m; ++i) {
			xx=(float)(lvt1+1-np[i]);
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"np 0 99 surface pressure level\n");
		if (0==fwrite(pl,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"pl %d 99 pressure [hPa]\n",(lvt1+1));
#ifdef lDiag
	sprintf (pout,"write pl\n");gaprnt (0,pout);
#endif 	
		if (0==fwrite(ta,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"ta %d 99 temperature [K]\n",lvt1);
#ifdef lDiag
	sprintf (pout,"write ta\n");gaprnt (0,pout);
#endif 	
		if (0==fwrite(wa,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"wa %d 99 specific humidity [kg/kg]\n",lvt1);
#ifdef lDiag
	sprintf (pout,"write wa\n");gaprnt (0,pout);
#endif 	
		if (0==fwrite(oa,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"o3 %d 99 ozone [ppb]\n",lvt1);
#ifdef lDiag
	sprintf (pout,"write oa\n");gaprnt (0,pout);
#endif 	
		if (0==fwrite(taucl,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"taucl %d 99 cloud optical thickness [1]\n",lvt1);
/*		if (0==fwrite(taucl,sizeof(float),3*m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"taucli %d 99 cloud optical thickness (ice)  [1]\n",lvt1);
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"taucll %d 99 cloud optical thickness (liquid drops)[1]\n",lvt1);
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"tauclr %d 99 cloud optical thickness (rain drops) [1]\n",lvt1);
*/
#ifdef lDiag
	sprintf (pout,"write taucl\n");gaprnt (0,pout);
#endif 	
		if (0==fwrite(fcld,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"fcld %d 99 clound amount [fraction]\n",lvt1);
#ifdef lDiag
	sprintf (pout,"write fcld\n");gaprnt (0,pout);
#endif 	
/*
	taual=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(taual+i) = pgr->undef;
	ssaal=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(ssaal+i) = pgr->undef;
	asyal=(float *)malloc(sizeof(float)*m*lvt1*nb*na); for (i=0; i<m*lvt1*nb*na; i++) *(asyal+i) = pgr->undef;
*/		
		
		if (0==fwrite(flx,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flx %d 99 net downward flux, all-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(flxu,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxu %d 99 upward flux, all-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(flxd,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxd %d 99 downward flux, all-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(flc,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flc %d 99 net downward flux, clear-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(flcu,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcu %d 99 upward flux, clear-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(flcd,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcd %d 99 downward flux, clear-sky [w/m**2] (+ downward)\n",(lvt1+1));
		if (0==fwrite(dfdts,sizeof(float),m*(lvt1+1),fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"dfdts %d 99 sensitivity of net downward flux to surface temperature [w/m**2/K]\n",(lvt1+1));
		if (0==fwrite(coolr,sizeof(float),m*lvt1,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"coolr %d 99 cooling rate [K/day]\n",(lvt1));

		// write surface data
#ifdef lDiag
		sprintf (pout,"write surface binary data\n");gaprnt (0,pout);
#endif 	
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=pl[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"ps 0 99 pressure at surface [hPa]\n");
		if (0==fwrite(tb,sizeof(float),m,fpdata)) {
			sprintf (pout,"Error from %s:  write ps to %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"tb 0 99 air temperature at surface [K]\n");
		if (0==fwrite(wb,sizeof(float),m,fpdata)) {
			sprintf (pout,"Error from %s:  write wb to %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"wb 0 99 specific humidity at surface [kg/kg]\n");
/*		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=fcld[i+(np[i]-1)*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write fcldb to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"fcldb 0 99 clound amount at surface [fraction]\n");
		*/
//	fs=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(fs+i) = pgr->undef;
//	tg=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(tg+i) = pgr->undef;
		if (0==fwrite(tg,sizeof(float),m,fpdata)) {	// assume ns=1
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"tg 0 99 land skin temperature [K]\n");
#ifdef lDiag
	sprintf (pout,"write tg\n");gaprnt (0,pout);
#endif 	
//	eg=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(eg+i) = pgr->undef;
		if (0==fwrite(eg,sizeof(float),m,fpdata)) {	// only for the first band 
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"eg 0 99 land surface emissivity [fraction]\n");
#ifdef lDiag
	sprintf (pout,"write eg\n");gaprnt (0,pout);
#endif 	
//	tv=(float *)malloc(sizeof(float)*m*ns); for (i=0; i<m*ns; i++) *(tv+i) = pgr->undef;
//	ev=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(ev+i) = pgr->undef;
//	rv=(float *)malloc(sizeof(float)*m*ns*nb); for (i=0; i<m*ns*nb; i++) *(rv+i) = pgr->undef;
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flx[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flxb to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxb 0 99 net downward flux, all-sky [w/m**2] (+ downward) at surface\n");
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flxu[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flxub to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxub 0 99 upward flux, all-sky [w/m**2] (+ downward) at surface\n");
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flxd[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flxdb to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxdb 0 99 downward flux, all-sky [w/m**2] (+ downward) at surface\n");
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flc[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flcb to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcb 0 99 net downward flux, clear-sky [w/m**2] (+ downward) at surface\n");
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flcu[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flcub to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcub 0 99 upward flux, clear-sky [w/m**2] (+ downward) at surface\n");
		for (i=0; i< m; ++i) {
			if(np[i]>0) xx=flcd[i+np[i]*m];
			else xx=pgr->undef;
			if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
				sprintf (pout,"Error from %s:  write flcdb to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcdb 0 99 downward flux, clear-sky [w/m**2] (+ downward) at surface\n");
		if (0==fwrite(sfcem,sizeof(float),m,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"sfcem 0 99 emission by the surface [w/m**2]\n");
		// surface cooling
		if (0==fwrite(pgr->grid,sizeof(float),m,fpdata)) {
			sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
			gaprnt (0,pout);			
		}
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"coolrsfc 0 99 surface (mixed layer) cooling rate [K/day]\n");


		if (1==sel) {
		// write ml file
#ifdef lDiag
			sprintf (pout,"write ml binary data\n");gaprnt (0,pout);
#endif 	
			for (i=0; i< m; ++i) {
				xx=(float)(lvt1+1-nm[i]);
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"nm 0 99 mixing height level\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=pl[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write pl %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"pm 0 99 pressure at the top of mixed layer [hPa]\n");
			if (0==fwrite(tm,sizeof(float),m,fpdata)) {
				sprintf (pout,"Error from %s:  write tm to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
/*			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=ta[i+(nm[i]-1)*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write tm %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);
				}
			}
			*/
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"tm 0 99 air temperature at the top of mixed layer [K]\n");
			if (0==fwrite(wm,sizeof(float),m,fpdata)) {
				sprintf (pout,"Error from %s:  write wm to %s file error!\n",lwnam[sel],datafilename);
				gaprnt (0,pout);			
			}
			/*
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=wa[i+(nm[i]-1)*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write wm %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);
				}
			}
			*/
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"wm 0 99 specific humidity at the top of mixed layer [kg/kg]\n");
			/*
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=fcld[i+(nm[i]-1)*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write fcldm to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"fcldm 0 99 clound amount at the top of mixed layer [fraction]\n");
			*/
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flx[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flxm to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxm 0 99 net downward flux, all-sky [w/m**2] (+ downward) at the top of mixed layer\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flxu[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flxum to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxum 0 99 upward flux, all-sky [w/m**2] (+ downward) at the top of mixed layer\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flxd[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flxdm to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flxdm 0 99 downward flux, all-sky [w/m**2] (+ downward) at the top of mixed layer\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flc[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flc to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcm 0 99 net downward flux, clear-sky [w/m**2] (+ downward) at the top of mixed layer\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flcu[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flcum to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcum 0 99 upward flux, clear-sky [w/m**2] (+ downward) at the top of mixed layer\n");
			for (i=0; i< m; ++i) {
				if(nm[i]>0) xx=flcd[i+nm[i]*m];
				else xx=pgr->undef;
				if (0==fwrite(&xx,sizeof(float),1,fpdata)) {
					sprintf (pout,"Error from %s:  write flcdm to %s file error!\n",lwnam[sel],datafilename);
					gaprnt (0,pout);			
				}
			}
			if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"flcdm 0 99 downward flux, clear-sky [w/m**2] (+ downward) at the top of mixed layer\n");
		}
		// end of ctl file
		if (!cmpwrd("null",ctlfilename)) fprintf(fpctl,"endvars\n");
		if (!cmpwrd("null",ctlfilename)) fclose(fpctl);
		fclose(fpdata);
	}		
	
#ifdef lDiag
	sprintf (pout,"finished 7.0: Print out data\n");gaprnt (0,pout);
#endif 	
//	8.0 Return
	rc=0;
erret:
	if (NULL!=np) free(nta);
	if (NULL!=np) free(nwa);
	if (NULL!=np) free(noa);
	if (NULL!=np) free(ntaucl);
	if (NULL!=np) free(nfcld);

	if (NULL!=ict) free(ict);
	if (NULL!=icb) free(icb);
	
	if (NULL!=pl) free(pl);
	if (NULL!=ta) free(ta);
	if (NULL!=wa) free(wa);
	if (NULL!=oa) free(oa);
	if (NULL!=taucl) free(taucl);
	if (NULL!=fcld) free(fcld);
	if (NULL!=taual) free(taual);
	if (NULL!=ssaal) free(ssaal);
	if (NULL!=asyal) free(asyal);
	
	if (NULL!=np) free(np);
	if (NULL!=tb) free(tb);
	if (NULL!=wb) free(wb);
	if (NULL!=lsfc) free(lsfc);

	if (NULL!=fs) free(fs);
	if (NULL!=tg) free(tg);
	if (NULL!=eg) free(eg);
//	if (NULL!=tv) free(tv);
//	if (NULL!=ev) free(ev);
	if (NULL!=rv) free(rv);

	if (NULL!=flx) free(flx);
	if (NULL!=flxu) free(flxu);
	if (NULL!=flxd) free(flxd);
	if (NULL!=flc) free(flc);
	if (NULL!=flcu) free(flcu);
	if (NULL!=flcd) free(flcd);
	if (NULL!=dfdts) free(dfdts);
	if (NULL!=coolr) free(coolr);
	if (NULL!=sfcem) free(sfcem);
        if (sel==1) {
		if (NULL!=nm) free(nm);
		if (NULL!=pm) free(pm);
		if (NULL!=tm) free(tm);
		if (NULL!=wm) free(wm);
		if (NULL!=lml) free(lml);
	}
		
	return (rc);
}

int fflw (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = lw (pfc, pst, 0);
	return (rc);
}

int fflw2 (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = lw (pfc, pst, 1);
	return (rc);
}

#endif	

char *interpnam[2] = {"PINTERP","ZINTERP"};

int vinterp (struct gafunc *pfc, struct gastat *pst, int sel) {
// pressure level interpolation
// Ben-Jei Tsuang
// 2005/02/02 
	struct gagrid *pgr[4];
	struct gafile *pfi[2];
	int rc,size[4],i,j,k,lvt[3],n,jj;
	int returnGrid,isGridCompatible;
	int method=0;
	float *field,*pgrid,*plev;
	float *ptr[3];
	float x,*xa,*ya,dy,*y2;
	float *lvvals[2],clev;
	
	float (*lnconv[2]) (float *, float);
	float (*ltconv[2]) (float *, float);
	float (*lvconv[2]) (float *, float);
	float (*conv[2]) (float *, float);
	char methodname[80]="-s";

	/* 0. Working on arguments */
	if (pfc->argnum!=3&&pfc->argnum!=4) {
		sprintf(pout,"Error from %s:  Too many or too few args \n",interpnam[sel]);gaprnt (0,pout);
		gaprnt (0,"                  3 or 4 arguments expected \n");
		gaprnt (0,"usage:\n");
		if (sel==0) {
			sprintf(pout,"d %s(field,pgrid,plev,[-l|-s|-p])\n",interpnam[sel]);gaprnt (0,pout);
			gaprnt (0,"Function returns:  defined grid vinterp holding interpolated values\n");
			gaprnt (0,"GrADS function to interpolate within a 3-D grid to a specified\n");
			gaprnt (0,"pressure level.  Can also be used on non-pressure level data, such\n");
			gaprnt (0,"as sigma or eta-coordinate output where pressure is a function\n");
			gaprnt (0,"of time and grid level.\n\n");
			gaprnt (0,"Arguments:\n");
			gaprnt (0,"   field = name of 3-D grid to interpolate\n");
			gaprnt (0,"   pgrid = name of 3-D grid holding pressure values at each gridpoint\n");
			gaprnt (0,"           If you are using regular pressure-level data, this should be\n");
			gaprnt (0,"           set to the builtin GrADS variable 'lev'.\n");
			gaprnt (0,"   plev  = pressure level at which to interpolate\n");
			gaprnt (0,"   [-l|-s|-p] = an option argument\n");
			gaprnt (0,"           -l for piecewise linear interpolation (fast) (default)\n");
			gaprnt (0,"           -s for spline interpolation (slow & problematic while dx is small)\n");
			gaprnt (0,"           -p for polynomnial interpolation (fast but problematic with extrapolation)\n");
		}
		else if (sel==1) {
			sprintf(pout,"d %s(field,zgrid,zlev,[-l|-s|-p])\n",interpnam[sel]);gaprnt (0,pout);
			gaprnt (0,"Function returns:  defined grid vinterp holding interpolated values\n");
			gaprnt (0,"GrADS function to interpolate within a 3-D grid to a specified\n");
			gaprnt (0,"z level.  Can also be used on non-z level data, such\n");
			gaprnt (0,"as sigma or eta-coordinate output where z is a function\n");
			gaprnt (0,"of time and grid level.\n\n");
			gaprnt (0,"Arguments:\n");
			gaprnt (0,"   field = name of 3-D grid to interpolate\n");
			gaprnt (0,"   zgrid = name of 3-D grid holding z values at each gridpoint\n");
			gaprnt (0,"   zlev  = z level at which to interpolate\n");
			gaprnt (0,"   [-l|-s|-p] = an option argument\n");
			gaprnt (0,"           -l for piecewise linear interpolation (fast) (default)\n");
			gaprnt (0,"           -s for spline interpolation (slow & problematic while dx is small)\n");
			gaprnt (0,"           -p for polynomnial interpolation (fast but problematic with extrapolation)\n");
		}
		return (1);
	}
	/* 0.1 Get plev data (3rg arg) */
	rc = gaexpr(pfc->argpnt[2],pst);
#ifdef lDiag
	sprintf (pout,"0.1\n");gaprnt (0, pout);
#endif	
	if (rc) {
//		for (j=0; j<2; j++) gagfre (pgr[j+1]);
		return (1);
	}
	if (pst->type==0) {
		gafree (pst);
//		for (j=0; j<2; j++) gagfre (pgr[j+1]);
		return (-1);
	}
	pgr[3] = pst->result.pgr;
	size[3] = pgr[3]->isiz * pgr[3]->jsiz;

		
	/* 0.2 Get the field data (1st arg) & pgrid data (2nd arg).  User is responsible
	for valid argument. */
#ifdef lDiag
	sprintf (pout,"0.2\n");gaprnt (0, pout);
#endif	
	for (i=0; i<2; i++) {
   		pfi[i]=getfile(pfc->argpnt[i],pst);
	   	if (pfi[i]==NULL) return (1);
   		size[i+1] = pfi[i]->dnum[0] * pfi[i]->dnum[1];
 		lvvals[i] = pfi[i]->grvals[2];
 		lvconv[i] = pfi[i]->gr2ab[2];
 		conv[i] = pfi[i]->ab2gr[2];
		// 1.1 get return grid & release other pgr data except for pgr[0]
		clev = lvconv[i](lvvals[i], (float)(1));
		pst->dmin[2] = clev;
		pst->dmax[2] = clev;
		rc = gaexpr(pfc->argpnt[i],pst);
#ifdef lDiag
		sprintf (pout,"0.21\n");gaprnt (0, pout);
#endif	
		if (rc) {
			gagfre (pgr[3]);
			return (1);
		}
		if (pst->type==0) {
 			sprintf (pout,"Error from %s:  grid data only.\n",interpnam[sel]);gaprnt (0, pout);
#ifdef lDiag
		sprintf (pout,"0.22\n");gaprnt (0, pout);
#endif	
			gafree (pst);
			gagfre (pgr[3]);
			for (j=0; j<i; j++) gagfre (pgr[j+1]);
			return (-1);
		}
#ifdef lDiag
		sprintf (pout,"0.23\n");gaprnt (0, pout);
#endif	
		pgr[i+1] = pst->result.pgr;
#ifdef lDiag
		sprintf (pout,"0.24\n");gaprnt (0, pout);
#endif	
		size[i+1] = pgr[i+1]->isiz * pgr[i+1]->jsiz;
   		// Get the range of levels
#ifdef lDiag
		sprintf (pout,"0.25\n");gaprnt (0, pout);
		sprintf (pout,"pfi[i]->type=%i\n",pfi[i]->type);gaprnt (0, pout);
//		sprintf (pout,"pfi[i]->dnum[2]=%i\n",pfi[i]->dnum[2]);gaprnt (0, pout);
//		sprintf (pout,"pgr[i+1]->pvar->levels=%i\n",pgr[i+1]->pvar->levels);gaprnt (0, pout);
#endif	
		if (pfi[i]->type==4) {lvt[i+1]=pfi[i]->dnum[2];}
		else {lvt[i+1] = pgr[i+1]->pvar->levels;}
#ifdef lDiag
		sprintf (pout,"0.26\n");gaprnt (0, pout);
#endif	
		if(lvt[i+1]<=0) lvt[i+1]=pfi[i]->dnum[2];
#ifdef lDiag
 		sprintf (pout,"0.27: lvt[%i]=%i\n",i+1,lvt[i+1]);gaprnt (0, pout);
#endif	
 		if (lvt[i+1]<2) {
 			sprintf (pout,"Error from %s:  Too few levels in %s.\n",interpnam[sel],pfc->argpnt[0]);gaprnt (0, pout);
 			gaprnt (0,"  Minimum 2 levels are requied.\n");
			for (j=0; j<i; j++) gagfre (pgr[j+1]);
 			return (1);
 		}
 	}

	/* 0.4 Check for 4th argument: get working directory and filename */	
	if (pfc->argnum == 4) {	
		getwrd(methodname,pfc->argpnt[3],80);
		if (cmpwrd("-l",methodname)) method=0;
		else if (cmpwrd("-s",methodname)) method=1;
		else if (cmpwrd("-p",methodname)) method=2;
		else method =0;	// default
	}

//	1.0 determine return grid, levels and check whether the size is compatible	
#ifdef lDiag
	sprintf (pout,"1.0\n");gaprnt (0, pout);
#endif	
	size[0]=size[1];
	returnGrid=1;
	for (i=2; i<=3; i++) {
		if (size[i]>size[0]) {
			size[0]=size[i];
			returnGrid=i;
		}
	}
	pgr[0]=pgr[returnGrid];
	// 1.2 chk whether grid is compatible	
	isGridCompatible=1;
	for (i=1; i<=3; i++) {
		if (size[0]!= size[i] && 1!= size[i]) {
			isGridCompatible=0;
			sprintf (pout,"grid[%i]'s size = %i\n",i,size[i]);gaprnt (0, pout);
			sprintf (pout,"max grid[%i]'s size = %i\n",returnGrid,size[0]);gaprnt (0, pout);
		}
	}

	if (!isGridCompatible ) {
		sprintf (pout,"Error from %s:  Incompatible grids.\n",interpnam[sel]);gaprnt (0, pout);
		gagfre(pgr[3]);
		return (1);
	}

	// 1.3 choosing the minimum level between field and pgrid	
	lvt[0]=lvt[1];
	for (i=1; i<=2; i++) {
#ifdef lDiag
	sprintf (pout,"1.1: lvt[%i]=%i\n",i,lvt[i]);gaprnt (0, pout);
#endif	
		if (lvt[i]<lvt[0]) {
			lvt[0]=lvt[i];
		}
	}
//	2.0 Allocate memory and data	
#ifdef lDiag
	sprintf (pout,"2.0: Allocating memory size=%i, levels=%i\n",size[0],lvt[0]);gaprnt (0, pout);
#endif	
	field=(float *)malloc(sizeof(float)*size[0]*lvt[0]); for (i=0; i<size[0]*lvt[0]; i++) *(field+i) = pgr[0]->undef;
	pgrid=(float *)malloc(sizeof(float)*size[0]*lvt[0]); for (i=0; i<size[0]*lvt[0]; i++) *(pgrid+i) = pgr[0]->undef;
	plev=(float *)malloc(sizeof(float)*size[0]); for (i=0; i<size[0]; i++) *(plev+i) = pgr[0]->undef;
	xa=(float *)malloc(sizeof(float)*lvt[0]); for (i=0; i<lvt[0]; i++) *(xa+i) = pgr[0]->undef;
	ya=(float *)malloc(sizeof(float)*lvt[0]); for (i=0; i<lvt[0]; i++) *(ya+i) = pgr[0]->undef;
	y2=(float *)malloc(sizeof(float)*lvt[0]); for (i=0; i<lvt[0]; i++) *(y2+i) = pgr[0]->undef;
	if (!field||!pgrid||!plev||!xa||!ya||!y2) {
		sprintf(pout,"%d: Out of memory\n",2.0);gaprnt (1,pout);
		rc=1; goto erret;
	}

	// 2.1 Set surface (pleb) data	
#ifdef lDiag
	sprintf (pout,"2.1: Get surface data\n");gaprnt (0,pout);
#endif 	
	for (j=0; j<size[0]; j++) {
		if (size[3]!=1) {
			if (pgr[3]->grid[j]!=pgr[3]->undef) plev[j]=pgr[3]->grid[j];
		} else {
			if (pgr[3]->grid[0]!=pgr[3]->undef) plev[j]=pgr[3]->grid[0];
		}
	}
	// 2.2 release unneed grid except for return grid	
	for (j=1; j<=3; j++) if(j!=returnGrid) gagfre (pgr[j]);
//	2.3 Get level data (field, pgrid) from bottom to top */	
#ifdef lDiag
	sprintf (pout,"2.3: Get level data\n");gaprnt (0,pout);
#endif 	
	for (j=0; j<lvt[0]; j++) {
#ifdef lDiag
	sprintf (pout,"2.3.1: Get level data %i\n",j);gaprnt (0,pout);
#endif 	
		for (i=0; i<2; i++) {
			clev = lvconv[i](lvvals[i], (float)(j+1));
			pst->dmin[2] = clev;
			pst->dmax[2] = clev;
			rc = gaexpr(pfc->argpnt[i],pst);
			if (rc) {
				sprintf (pout,"Error from %s:  read %s error. \n",interpnam[sel],pfc->argpnt[i]);gaprnt (0,pout);
				sprintf (pout,"1) pgr[1]=%d;pgr[2]=%d. \n",pgr[1],pgr[2]);gaprnt (0,pout);
				for (k=0; k<i; k++) gagfre(pgr[k]); 
				goto erret;
			}
			if(0==i) pgr[1]= pst->result.pgr;		
			else if(1==i) pgr[2]= pst->result.pgr;		
		}
		ptr[1] = pgr[1]->grid;
		ptr[2] = pgr[2]->grid;
		for (i=0; i<size[0]; i++) {
#ifdef lDiag
//	sprintf (pout,"2.3.2: j=%i, i=%i\n",j,i);gaprnt (0,pout);
#endif 	
			if(*ptr[1]!=pgr[1]->undef) field[j*size[0]+i]= *ptr[1];		
			if(*ptr[2]!=pgr[2]->undef) pgrid[j*size[0]+i]= *ptr[2];
			if (size[1]!=1) ptr[1]++;
			if (size[2]!=1) ptr[2]++;
		}
		gagfre (pgr[1]);
		gagfre (pgr[2]);
#ifdef lDiag
	sprintf (pout,"3) pgr[1]=%d;pgr[2]=%d. \n",pgr[1],pgr[2]);gaprnt (0,pout);
	sprintf (pout,"2.3.3\n");gaprnt (0,pout);
#endif 	
	}              	
//	3.0 Start to work	
#ifdef lDiag
	sprintf (pout,"3.0: Start to work\n");gaprnt (0,pout);
#endif
	ptr[0] = pgr[0]->grid;
	for (i=0; i<size[0]; i++) {
#ifdef lDiag
	sprintf (pout,"3.0.1: i=%i\n",i);gaprnt (0,pout);
#endif
		if(plev[i]!=pgr[0]->undef) {
			// 3.1 preparing the working vector
			n=0;
			for (j=0; j<lvt[0]; j++) {
#ifdef lDiag
//	sprintf (pout,"3.1.1: i=%i, j=%i\n",i,j);gaprnt (0,pout);
#endif
				if((field[j*size[0]+i]!=pgr[0]->undef)&&(pgrid[j*size[0]+i]!=pgr[0]->undef)) {
					if (sel==0) {
					// log scale interpolation
						xa[n]=log(pgrid[j*size[0]+i]);
						x=log(plev[i]);
					} else if (sel==1) {
					// linear scale interpolation		
						xa[n]=pgrid[j*size[0]+i];
						x=plev[i];
					}
					ya[n]=field[j*size[0]+i];
					n++;
				}
			}
			// 3.2 call interpolation
			if (n>0) {
				if (n>2 && 1==method) {
				// 3.2.1 spine interpolation
#ifdef lDiag
	sprintf (pout,"3.2.1.1 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
					rc=sort2b(n,xa,ya);
#ifdef lDiag
	sprintf (pout,"3.2.1.2 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
					if (!rc) {
						spline(xa,ya,n,9.99e33,9.99e33,y2);	// natural spline
#ifdef lDiag
	sprintf (pout,"3.2.1.3 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
	gaprnt(0,"y2=");for (j=0;j<n;j++) {sprintf (pout,"%f ",y2[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
						rc=splintb(xa,ya,y2,n,x,ptr[0]);
						if (rc) {
							sprintf (pout,"3.2.1.4 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
							gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
							gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
							gaprnt(0,"y2=");for (j=0;j<n;j++) {sprintf (pout,"%f ",y2[j]);gaprnt (0,pout);};gaprnt(0,"\n");
						}
					}
					if (rc) *ptr[0]=pgr[0]->undef;
				}
				else if (2==method) {
				// 3.2.2 polynominal interpolation
					rc=polintb(xa,ya,n,x,ptr[0],&dy);
					if (rc) { sprintf(pout,"Error from %s. \n",interpnam[sel]);gaprnt (1,pout);}
				}
				else {
				// 3.2.3 piecewise linear interpolation
#ifdef lDiag
	sprintf (pout,"3.2.3.1 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
					rc=sort2b(n,xa,ya);
#ifdef lDiag
	sprintf (pout,"3.2.3.2 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
					if (rc) *ptr[0]=pgr[0]->undef;
					else {
						locate(xa,n,x,&jj);
						if(-1==jj) {
							// left of leftmost point	
							if(abs(xa[1]-xa[0])>0) *ptr[0]=ya[0]+(ya[1]-ya[0])/(xa[1]-xa[0])*(x-xa[0]);
							else *ptr[0]=ya[0];
						}
						else if (n-1==jj) {
							// right of the rightmost point
							if(abs(xa[n-2]-xa[n-1])>0)*ptr[0]=ya[n-1]+(ya[n-2]-ya[n-1])/(xa[n-2]-xa[n-1])*(x-xa[n-1]);
							else *ptr[0]=ya[n-1];
						}
						else {
							// mid-point
							if(abs(xa[jj+1]-xa[jj])>0) *ptr[0]=ya[jj]+(ya[jj+1]-ya[jj])/(xa[jj+1]-xa[jj])*(x-xa[jj]);
							else *ptr[0]=(ya[jj+1]+ya[jj])/2.;
						}
					}
				}
			} else {
				*ptr[0]=pgr[0]->undef;
			}
#ifdef lDiag
	sprintf (pout,"3.2 i=%i,n=%i,vinterp=%f",i,n,*ptr[0]);gaprnt (0,pout);
	gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
	gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
#endif
		} else {
			*ptr[0]=pgr[0]->undef;
		}
#ifdef lDiag
		if (*ptr[0]!=pgr[0]->undef&& abs(*ptr[0])>10000.) {
			sprintf (pout,"3.3 i=%i,n=%i,x=%f,y==%f",i,n,x,*ptr[0]);gaprnt (0,pout);
			gaprnt(0,"x=");for (j=0;j<n;j++) {sprintf (pout,"%f ",xa[j]);gaprnt (0,pout);};gaprnt(0,"\t");
			gaprnt(0,"y=");for (j=0;j<n;j++) {sprintf (pout,"%f ",ya[j]);gaprnt (0,pout);};gaprnt(0,"\n");
		}
#endif
		ptr[0]++;
	}
//	4.0 Finished	
#ifdef lDiag
	sprintf (pout,"4.0\n");gaprnt (0, pout);
#endif	
 	/* Release storage and return */
	if (NULL!=field) free(field);
	if (NULL!=pgrid) free(pgrid);
	if (NULL!=plev) free(plev);
	if (NULL!=xa) free(xa);
	if (NULL!=ya) free(ya);
	if (NULL!=y2) free(y2);
	pst->result.pgr = pgr[0];
	return (0);
	
erret:
/* Error return */	
	if (NULL!=field) free(field);
	if (NULL!=pgrid) free(pgrid);
	if (NULL!=plev) free(plev);
	if (NULL!=xa) free(xa);
	if (NULL!=ya) free(ya);
	if (NULL!=y2) free(y2);
	return (rc);	
}

int ffpinterp (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = vinterp (pfc, pst, 0);
	return (rc);
}

int ffzinterp (struct gafunc *pfc, struct gastat *pst) {
	int rc;
	rc = vinterp (pfc, pst, 1);
	return (rc);
}

float calcLinearY (float x,float x0,float y0,float x1,float y1) {
	return (y0+(y1-y0)/(x1-x0)*(x-x0));
}
int calcLinearJ(int i,float x0,float y0,float x1,float y1,float lnmin,float lnscl, float ltmin, float ltscl, float *x, float *y) {
	int j;
	*x=lnmin+(i-0.5)*lnscl; //left boundary
	if((x1-x0)!=0) {
		*y=calcLinearY(*x,x0,y0,x1,y1);
		j=(int)((*y-ltmin)/ltscl+0.5);
	} else {
		*y=-9.99e33;
		j=-999;
	}
	return (j);
}
float calcLinearX (float y,float x0,float y0,float x1,float y1) {
	return (x0+(x1-x0)/(y1-y0)*(y-y0));
}
int calcLinearI(int j,float x0,float y0,float x1,float y1,float lnmin,float lnscl, float ltmin, float ltscl, float *x, float *y) {
	int i;
	*y=ltmin+(j-0.5)*ltscl; // lower boundary
	if((y1-y0)!=0) {
		*x=calcLinearX(*y,x0,y0,x1,y1);
		i=(int)((*x-lnmin)/lnscl+0.5);
	} else {
		*x=-9.99e33;
		i=-999;
	}
	
	return (i);
}
int ffline (struct gafunc *pfc, struct gastat *pst) {
	
	struct gagrid *pgr;
	float (*iconv) (float *, float);
	float (*jconv) (float *, float);
	float *ivars, *jvars;
	float lon,lat,lnmin,lnmax,lnscl,ltmin,ltmax,ltscl;
	int rc,i,j,siz,icnt,cntflg;
	int i0,j0,i1,j1,ii,jj,iiold,jjold,iistop,jjstop;
	char *ch;
	float x0,y0,x1,y1,u,v,xx0,yy0,xx1,yy1,x,y,length,totlen;
	float *ptr;
	struct gafile *pfi;
        int iLonLat=1;                                 /*ams to avoid UTM stuff */
	
	if (pfc->argnum<5||pfc->argnum>6) {
		gaprnt (0,"Error from LINE:  5-6 arguments expected \n");
		gaprnt (0,"line(exp,x0,y0,x1,y1[,-l|-m|-r]) \n");
		gaprnt (0,"usage:\n");
		gaprnt (0,"Function returns: draw line from (x0,y0) to (x1,y1)\n");
		gaprnt (0,"  Set the line at the value val (default 1)\n");
		gaprnt (0,"  using the template of exp\n");
		gaprnt (0,"Arguments:\n");
		gaprnt (0,"   (x0,y0),(x1,y1): coordindates in (lon, lat)\n");
		gaprnt (0,"   -l: return the length [km] in each grid\n");
		gaprnt (0,"   -m: return the logics (1:pass, 0: non-passing)\n");		
		gaprnt (0,"   -r: return the length ratio of each segment (fraction)\n");		
		return (1);
	}
	
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (rc);
	if (pst->type==0) {
		gaprnt (0,"Error from LINE: 1st argument must be a grid\n");
		gafree (pst);
		return (1);
	}
	pfi = pst->pfid;	
	pgr = pst->result.pgr;
	if (pgr->idim!=0 || pgr->jdim!=1) {
		gaprnt (0,"Error from LINE: Grid must vary in X, Y\n");
		gafree (pst);
		return (1);
	}
	if (!pgr->ilinr || !pgr->jlinr) {
		gaprnt (0,"Error from LINE: Grid must have linear scaling\n");
		gafree (pst);
		return (1);
	}
	siz = pgr->isiz * pgr->jsiz;
	ptr = pgr->grid;
	for (i=0; i<siz; i++) {
		*ptr = 0.;	// initial: set zero
		ptr++;
	}
	/* Set up scaling for converting lon-lat to grid units */
	
	iconv = pgr->igrab;
	ivars = pgr->ivals;
	jconv = pgr->jgrab;
	jvars = pgr->jvals;
	lnmin = iconv(ivars,(float)pgr->dimmin[0]);
	lnmax = iconv(ivars,(float)pgr->dimmax[0]);
	ltmin = iconv(jvars,(float)pgr->dimmin[1]);
	ltmax = iconv(jvars,(float)pgr->dimmax[1]);
	lnscl = (lnmax-lnmin)/((float)pgr->isiz-1);
	ltscl = (ltmax-ltmin)/((float)pgr->jsiz-1);
	 
	// printf("qqq lnmin: %f lnmax %f isiz %d ltmin %f ltmax %f jsiz %d\n",lnmin,lnmax,pgr->isiz,ltmin,ltmax,pgr->jsiz);
	
	if (valprs(pfc->argpnt[1],&x0)==NULL) {
		sprintf (pout,"Error from %s: Invalid Argument\n","x0");
		gaprnt (0,pout);
		sprintf (pout,"  Expecting arg%d: %s to be a value\n",2,pfc->argpnt[1]);
		gaprnt (0,pout);
		return (1);
	}
	if (valprs(pfc->argpnt[2],&y0)==NULL) {
		sprintf (pout,"Error from %s: Invalid Argument\n","y0");
		gaprnt (0,pout);
		sprintf (pout,"  Expecting arg%d: %s to be a value\n",3,pfc->argpnt[2]);
		gaprnt (0,pout);
		return (1);
	}
	if (valprs(pfc->argpnt[3],&x1)==NULL) {
		sprintf (pout,"Error from %s: Invalid Argument\n","x1");
		gaprnt (0,pout);
		sprintf (pout,"  Expecting arg%d: %s to be a value\n",4,pfc->argpnt[3]);
		gaprnt (0,pout);
		return (1);
	}
	if (valprs(pfc->argpnt[4],&y1)==NULL) {
		sprintf (pout,"Error from %s: Invalid Argument\n","y1");
		gaprnt (0,pout);
		sprintf (pout,"  Expecting arg%d: %s to be a value\n",5,pfc->argpnt[4]);
		gaprnt (0,pout);
		return (1);
	}
	if (pfc->argnum==6) {
		ch = pfc->argpnt[5];
		if (*ch == '-' && *(ch+1) == 'l') cntflg = 1;
		else if (*ch == '-' && *(ch+1) == 'm') cntflg = 2;
		else if (*ch == '-' && *(ch+1) == 'r') cntflg = 3;
		else gaprnt (1,"Warning from LINE: Invalid flag.  Ignored.\n");
	} else {cntflg = 2;}

	/* unit vector */
        u=(x1-x0)/sqrt(pow(x1-x0,2)+pow(y1-y0,2));
        v=(y1-y0)/sqrt(pow(x1-x0,2)+pow(y1-y0,2));

	// 1.0 Is (x0,y0),(x1,y1) in domain?
	// if (x0<lnmin) x0+=360.0;
	// else if (x0>lnmax) x0-=360.0;
	
	i0 = floor((x0-lnmin)/lnscl+0.5);
	j0 = floor((y0-ltmin)/ltscl+0.5);
	i1 = floor((x1-lnmin)/lnscl+0.5);
	j1 = floor((y1-ltmin)/ltscl+0.5);
	xx0=x0;
	yy0=y0;
	xx1=x1;
	yy1=y1;
	// (x0,y0)
	// printf("qqq i0: %d j0 %d i1 %d j1 %d\n",i0,j0,i1,j1);
	if (i0<0) {
		if (i1>=0 ) {
			i0=0;
			j0=calcLinearJ(i0,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx0,&yy0);
		} else { goto warn;}		
	}
	if (i0>(pgr->isiz-1)) {
		if (i1<=(pgr->isiz-1) ) {
			i0=pgr->isiz-1;
			j0=calcLinearJ(i0,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx0,&yy0);
		} else { goto warn;}		
	}
	if (j0<0) {
		if (j1>=0 ) {
			j0=0;
			i0=calcLinearI(j0,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx0,&yy0);
		} else { goto warn;}		
	}
	if (j0>(pgr->jsiz-1)) {
		if (j1<=(pgr->jsiz-1) ) {
			j0=pgr->jsiz-1;
			i0=calcLinearI(j0,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx0,&yy0);
		} else { goto warn;}		
	}
	// (x1,y1)
	if (i1<0) {
		if (i0>=0 ) {
			i1=0;
			j1=calcLinearJ(i1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx1,&yy1);
		}
		else { goto warn;}		
	}
	if (i1>(pgr->isiz-1)) {
		if (i0<=(pgr->isiz-1) ) {
			i1=pgr->isiz-1;
			j1=calcLinearJ(i1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx1,&yy1);
		}
		else { goto warn;}		
	}
	if (j1<0) {
		if (j0>=0 ) {
			j1=0;
			i1=calcLinearI(j1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx1,&yy1);
		}
		else { goto warn;}		
	}
	if (j1>(pgr->jsiz-1)) {
		if (j0<=(pgr->jsiz-1) ) {
			j1=pgr->jsiz-1;
			i1=calcLinearI(j1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&xx1,&yy1);
		}
		else { goto warn;}		
	}
// 	printf("qqq xx0: %f yy0 %f xx1 %f yy1 %f\n",xx0,yy0,xx1,yy1);
//	printf("qqq i0: %d j0 %d i1 %d j1 %d\n",i0,j0,i1,j1);
	// total length
#ifdef USETUTM
        iLonLat = pfi->iLonLat;
#endif
	if (2==iLonLat) {	// utm coord
		totlen=sqrt(pow(xx1-xx0,2)+pow(yy1-yy0,2));
	} else { // 1==pfi->iLonLat (LonLat Coord.) 
		totlen=6.37E3*3.1416/180.0*sqrt(pow((xx1-xx0)*cos((yy1+yy0)/2.*3.1416/180.0),2)+pow(yy1-yy0,2));
		// convert from lon/lat to km
	}
	// 2.0 start to work (fill columnwise)
	if(u>=0) {
		iistop=i1+1;
	} else {
		iistop=i1-1;
	}
	ii=i0;
	jj=j0;
	do {
		iiold=ii;
		jjold=jj;
		if(u>0.) {
			ii++;
			jj=calcLinearJ(ii,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);		
		} else if(u==0.) {
		       	ii++;  // to match iistop criterion
			jj=j1;
		} else {
			ii--;
			jj=calcLinearJ(ii,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);		
		}
		if(v>=0) {
			jj=min(jj,j1);
			jjstop=jj+1;
		} else {
			jj=max(jj,j1);
			jjstop=jj-1;
		}
		j=jjold;
		do {
			icnt=j*pgr->isiz+iiold;
			if(cntflg==1|cntflg==3) {
				if(j!=jj) {
					if(v>0.) {
						calcLinearI(j+1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);
					} else if (v<0.) {
						calcLinearI(j,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);
					}	
				} else { // last row
					if(iiold!=i1) {
						if(u>0) {
							calcLinearJ(iiold+1,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);
						} else if (u<0.) {
							calcLinearJ(iiold,x0,y0,x1,y1,lnmin,lnscl,ltmin,ltscl,&x,&y);
						}	
					} else {  // last column
						x=xx1;
						y=yy1;
					}
				}
#ifdef USEUTM
                                iLonLat = pfi->iLonLat;
#endif
				if (2==iLonLat) {	// utm coord
					length=sqrt(pow((x-xx0),2)+pow(y-yy0,2));
				} else { // 1==pfi->iLonLat (LonLat Coord.) 
					length=6.37E3*3.1416/180.0*sqrt(pow((x-xx0)*cos((y+yy0)/2.*3.1416/180.0),2)+pow(y-yy0,2));
					// convert from lon/lat to km
				}
				if(cntflg==1) {
					pgr->grid[icnt]=length;
				} else if(cntflg==3) {
					if(totlen==0) { // one point only
					       	pgr->grid[icnt]=1;
					} else {
						pgr->grid[icnt]=length/totlen;
					}
				}
				xx0=x;
				yy0=y;
			} else if(cntflg==2) {
				pgr->grid[icnt]=1;
			} else {
				pgr->grid[icnt]=1;
			}
			if(v>=0) {
				j++;
			} else {
				j--;
			}	
		} while (j!=jjstop);
	} while (ii!=iistop);
retrn:
	return(0);
	
warn:
	gaprnt (0,"Warning in LINE: Outside the domain\n");
	return (0);
}

int ffvint2 (struct gafunc *pfc, struct gastat *pst) {
// modify by Ben-Jei Tsuang for constant and expresssion for all the arguments 1, 2 &3
// 2005/01/08 
	struct gagrid *pgrb, *pgr, *pgrv, *pgrt;
	struct gafile *pfi;
	int rc, size, i, j, lvt;
	float *ps, *var, *res, *top;
	float *lvvals;
	float (*lvconv) (float *, float);
	float clev,ulev,blev,ulevi,blevi,kgm;
	
	if (pfc->argnum!=3) {
		gaprnt (0,"Error from VINT2:  3 arguments expected \n");
		gaprnt (0,"vint(psexpr,expr,top)\n");
		gaprnt (0,"usage:\n");
		gaprnt (0,"This function performs a mass-weighted vertical integral in mb \n");
		gaprnt (0,"pressure coordinates. The three arguments to vint are:\n");
		gaprnt (0,"Arguments:\n");
		gaprnt (0,"   psexpr=a GrADS expression for the surface pressure, in mb,\n");
		gaprnt (0,"         which bounds the integral on the bottom.\n");
		gaprnt (0,"   expr= a GrADS expression representing the quantity to be\n");
		gaprnt (0,"         integrated.\n");
		gaprnt (0,"   top= the bounding top pressure, in mb. This value must\n");
		gaprnt (0,"         be a constant and cannot be provided as an expression. \n");
		gaprnt (0,"The calculation is a sum of the mass-weighted layers:\n"); 
		gaprnt (0,"f/g * sum(expr * Delta(level))\n"); 
		gaprnt (0,"The bounds of the integration are the surface pressure and the\n");
		gaprnt (0,"indicated top value. The scale factors are f=100 and g=9.8. The\n");
		gaprnt (0,"summation is done for each layer present that is between the bounds.\n");
		gaprnt (0,"The layers are determined by the Z levels of the default file. Each\n");
		gaprnt (0,"layer is considered to be from the midpoints between the levels actually\n");
		gaprnt (0,"present, and is assumed to have the same value throughout the layer, namely\n");
		gaprnt (0,"the value of the gridpoint at the middle of the layer. \n");
		gaprnt (0,"Usage Notes\n"); 
		gaprnt (0,"The summation is done using the Z levels from the default file, so it is important\n");
		gaprnt (0,"that the default file have the same Z dimension coordinates as expr. Data levels\n");
		gaprnt (0,"below and above the bounds of the summation are ignored. The Z dimension in\n");
		gaprnt (0,"world-coordinate units is assumed to be pressure values given in millibars (mb).\n");
		gaprnt (0,"The units of g are such that when the expression integrated is specific humidity (q)\n");
		gaprnt (0,"in units of g/g, the result is kg of water per square meter, or precipitable water in mm.\n"); 
		gaprnt (0,"It is usually a good idea to make the top pressure value to be at the top of a layer,\n");
		gaprnt (0,"which is midway between grid points. For example, if the default file (and the data) have\n");
		gaprnt (0,"pressure levels of ...,500,400,300,250,... then a good value for top might be 275, the value\n");
		gaprnt (0,"at the top of the layer that extends from 350 to 275 mb. The vint function operates only\n");
		gaprnt (0,"in an X-Y varying dimension environment.\n"); 
		gaprnt (0,"Examples\n");  
		gaprnt (0,"A typical use of vint might be:\n");  
		gaprnt (0,"vint(ps,q,275)\n"); 
		gaprnt (0,"This expression will integrate specific humidity to obtain precipitable water, in mm.\n");  
		return (1);
	}
		
	/* Get the surface pressure field (1st arg).  User is responsible
	for valid argument.  Then get the lowest level of the
	field to integrate.  */
	
	rc = gaexpr(pfc->argpnt[0],pst);
	if (rc) return (1);
	if (pst->type==0) {
		gafree (pst);
		return (-1);
	}
	pgrb = pst->result.pgr;
	
	/* Get top pressure level (3rd arg) */
	rc = gaexpr(pfc->argpnt[2],pst);
	if (rc) return (1);
	if (pst->type==0) {
		gagfre (pgrb);
		gafree (pst);
		return (-1);
	}
	pgrt = pst->result.pgr;

	/* Get the range of levels from the default file.  Set the
	level in the status block to the first level.  */
	
	pfi = pst->pfid;

	/* Then get the lowest level of the
	field to integrate.  */

	lvconv = pfi->gr2ab[2];
	lvvals = pfi->grvals[2];
	clev = lvconv(lvvals, 1.0);
	ulev = lvconv(lvvals, 2.0);
	ulevi = clev + ((ulev-clev)/2.0);
	pst->dmin[2] = clev;
	pst->dmax[2] = clev;
	if (pst->idim==2) {
		pst->idim = pst->jdim;
		pst->jdim = -1;
	}
	if (pst->jdim==2) pst->jdim = -1;

	rc = gaexpr(pfc->argpnt[1],pst);
	if (rc) {
		gagfre (pgrb);
		gagfre (pgrt);
		return (1);
	}
	if (pst->type==0) {
		gafree (pst);
		gagfre (pgrb);
		gagfre (pgrt);
		return (-1);
	}
	pgr = pst->result.pgr;
	
	lvt = pgr->pvar->levels;
//	lvt = pfi->dnum[2];
	if (lvt<3) {
		gaprnt (0,"Error from VINT:  Too few levels in default file \n");
		return (1);
	}
	/* Check that the two grids are equivalent.  */
	
	if ((pgrb->isiz!=pgr->isiz&&pgrb->isiz!=1)||(pgrb->jsiz!=pgr->jsiz&&pgrb->jsiz!=1) ||
		(pgrt->isiz!=pgr->isiz&&pgrt->isiz!=1)||(pgrt->jsiz!=pgr->jsiz&&pgrt->jsiz!=1)) {
		gaprnt (0,"Error from VINT:  Incompatible grids. \n");
		goto erret;
	}
	
	/* Apply appropriate mass weight (kg/m**2) to first level.
	It is assumed the vertical coordinate system is mb.    */
	
	size = pgr->isiz * pgr->jsiz;
	kgm = 100.0/9.8;
	ps = pgrb->grid;
	top = pgrt->grid;
	res = pgr->grid;
	for (i=0; i<size; i++) {
		if (*ps==pgrb->undef || *top==pgrt->undef ||*res==pgr->undef ||*ps<=*top ) *res = pgr->undef;
		else if (ulevi<=*ps) *res *= kgm*(*ps-max(ulevi,*top));
		else *res = pgr->undef;
		if(pgrb->isiz*pgrb->jsiz!=1) ps++;
		res++;
		if(pgrt->isiz*pgrt->jsiz!=1) top++;
	}
	
	/* Go through the intermediate levels and apply mass weight. */
	
	for (i=2; i<lvt; i++) {
		clev = lvconv(lvvals, (float)i);
		ulev = lvconv(lvvals, (float)(i+1));
		ulevi = clev + ((ulev-clev)/2.0);
		blev = lvconv(lvvals, (float)(i-1));
		blevi = clev + ((blev-clev)/2.0);
		pst->dmin[2] = clev;
		pst->dmax[2] = clev;
		rc = gaexpr(pfc->argpnt[1],pst);
		if (rc) goto erret;
		if (pst->type==0) {
			rc = -1;
			gafree (pst);
			goto erret;
		}
		pgrv = pst->result.pgr;
		ps = pgrb->grid;
		top = pgrt->grid;
		res = pgr->grid;
		var = pgrv->grid;
		for (j=0; j<size; j++) {
			if ((*ps!=pgrb->undef) && (*top!=pgrt->undef) &&(*var!=pgrv->undef) && (*ps>*top) ) {
				if (*res==pgr->undef) {
					if (ulevi<=*ps) *res = *var*kgm*(*ps-max(ulevi,*top));
				} else {
					if ((blevi>=*top)&&(ulevi<=*ps)) // all belong the layer
//					*res += (*var*kgm*(min(blevi,*ps)-max(ulevi,*top)));
					*res += (*var*kgm*(blevi-max(ulevi,*top)));
				}
			}
			if(pgrb->isiz*pgrb->jsiz!=1) ps++;
			res++; var++;
			if(pgrt->isiz*pgrt->jsiz!=1) top++;
		}
		gafree (pst);
	}
	
	/* Do top, and last, level */
	
	clev = lvconv(lvvals, (float)lvt);
	blev = lvconv(lvvals, (float)(lvt-1));
	blevi = clev + ((blev-clev)/2.0);
	pst->dmin[2] = clev;
	pst->dmax[2] = clev;
	rc = gaexpr(pfc->argpnt[1],pst);
	if (rc) goto erret;
	if (pst->type==0) {
		rc = -1;
		gafree (pst);
		goto erret;
	}
	pgrv = pst->result.pgr;
	ps = pgrb->grid;
	top = pgrt->grid;
	res = pgr->grid;
	var = pgrv->grid;
	for (i=0; i<size; i++) {
		if ((*ps!=pgrb->undef) && (*top!=pgrt->undef) && (*var!=pgrv->undef) && (*ps>*top)) {
			if (*res==pgr->undef) *res = *var * kgm * (*ps-*top);
			else if (blevi>=*top) {
//				*res += (*var*kgm*(min(blevi,*ps)-*top));
				*res += (*var*kgm*(blevi-*top));
			}
		}
		if(pgrb->isiz*pgrb->jsiz!=1) ps++;
		res++; var++;
		if(pgrt->isiz*pgrt->jsiz!=1) top++;
	}
	gafree (pst);
	
	/* Release storage and return */
	
	rc = 0;
	pst->type = 1;
	pst->result.pgr = pgr;
	gagfre (pgrt);
	gagfre (pgrb);
	return (0);
	
	/* Error return */
	
erret:
	
	gagfre (pgrt);
	gagfre (pgrb);
	gagfre (pgr);
	return (rc);
}


void gaprnt_ ( int level, char *msg ) {

  if ( level <= verbose ) gaprnt(level,msg);

}

/* .................................................................. */


int f_bjt (struct gafunc *pfc, struct gastat *pst) {

  int rc;
  char *name = pfc->argpnt[pfc->argnum];

          if ( strcmp ( "lt" , name) == 0 ) 
               rc = fflt ( pfc, pst );
     else if ( strcmp ( "jd" , name) == 0 ) 
               rc = ffjd ( pfc, pst );
     else if ( strcmp ( "cosz" , name) == 0 ) 
               rc = ffcosz ( pfc, pst );
     else if ( strcmp ( "dayratio" , name) == 0 ) 
               rc = ffdayratio ( pfc, pst );
     else if ( strcmp ( "if" , name) == 0 ) 
               rc = ffif ( pfc, pst );
     else if ( strcmp ( "maxv" , name) == 0 ) 
               rc = ffmaxv ( pfc, pst );
     else if ( strcmp ( "minv" , name) == 0 ) 
               rc = ffminv ( pfc, pst );
     else if ( strcmp ( "which" , name) == 0 ) 
               rc = ffwhich ( pfc, pst );
     else if ( strcmp ( "ftest" , name) == 0 ) 
               rc = ffftest ( pfc, pst );
     else if ( strcmp ( "ttest" , name) == 0 ) 
               rc = ffttest ( pfc, pst );
     else if ( strcmp ( "tfit" , name) == 0 ) 
               rc = fftfit ( pfc, pst );
     else if ( strcmp ( "fit" , name) == 0 ) 
               rc = fffit ( pfc, pst );
     else if ( strcmp ( "tcorr2" , name) == 0 ) 
               rc = fftcorr2 ( pfc, pst );
     else if ( strcmp ( "tregr2" , name) == 0 ) 
               rc = fftregr2 ( pfc, pst );
     else if ( strcmp ( "tmave2" , name) == 0 ) 
               rc = fftmave2 ( pfc, pst );
     else if ( strcmp ( "madvu" , name) == 0 ) 
               rc = ffmadvu ( pfc, pst );
     else if ( strcmp ( "madvv" , name) == 0 ) 
               rc = ffmadvv ( pfc, pst );
     else if ( strcmp ( "madvw" , name) == 0 ) 
               rc = ffmadvw ( pfc, pst );
     else if ( strcmp ( "muadv" , name) == 0 ) 
               rc = ffmuadv ( pfc, pst );
     else if ( strcmp ( "mvadv" , name) == 0 ) 
               rc = ffmvadv ( pfc, pst );
     else if ( strcmp ( "mwadv" , name) == 0 ) 
               rc = ffmwadv ( pfc, pst );
     else if ( strcmp ( "satvap" , name) == 0 ) 
               rc = ffsatvap ( pfc, pst );
     else if ( strcmp ( "dew" , name) == 0 ) 
               rc = ffdew ( pfc, pst );
#ifdef LW
     else if ( strcmp ( "lw" , name) == 0 ) 
               rc = fflw ( pfc, pst );
     else if ( strcmp ( "lw2" , name) == 0 ) 
               rc = fflw2 ( pfc, pst );
#endif
     else if ( strcmp ( "pinterp" , name) == 0 ) 
               rc = ffpinterp ( pfc, pst );
     else if ( strcmp ( "zinterp" , name) == 0 ) 
               rc = ffzinterp ( pfc, pst );
     else if ( strcmp ( "line" , name) == 0 ) 
               rc = ffline ( pfc, pst );
     else if ( strcmp ( "vint2" , name) == 0 ) 
               rc = ffvint2 ( pfc, pst );
     else {
       sprintf(pout,"Function '%s' not found in libbjt!\n" , name);
       gaprnt(0,pout);
       return 1;
     }

    if (rc) return rc;

    /* Make sure undef mask is properly set */
    rc = gex_setUndef (pst);
    return rc; 

}

/*

                         -----------------
                         POD Documentation
                         -----------------
*/


#else

=pod

=head1 NAME

libbjt.gex - Ben-Jei Tsuang`s Collection of GrADS v1.9 Extensions 

=head1 SYNOPSIS

=head2 Time Related Functions

=over 4

=item

display B<lt>(I<EXPR>) -  Local time

=item

display B<jd>(I<EXPR>) -  Julian day


=item

display B<cosz>(I<EXPR>, I<H|D|M>) -  Cosine of solar zenith angle

=item

display B<dayratio>(I<EXPR>) -  Daylight ratio

=back

=head2 Statistical Functions

=over 4

=item

display B<ftest>(I<RATIO>,I<DF1>,I<DF2>) - F-Test  

=item

display B<ttest>(I<DIFF>,I<DF>) - T-Test  

=item

display B<tfit>(I<Y1,X1,X2,X3,..,XN,time=TBEG,time=TEND[,DT,FILENAME]>) - Local multiple linear regression

=item

display B<fit>(I<Y1,X1,X2,X3,..,XN,time=TBEG,time=TEND[,DT,FILENAME]>) -  Global multiple linear regression

=item

display B<tcorr2>(I<X1,X2,time=TBEG,time=TEND [,DT]>) -  Time correlation (r) 

=item

display B<tregr2>(I<X,Y,time=TBEG,time=TEND [,DT]>)  -   Gives the expected value of y departure given a 1 unit departure in x. (Same as tregr with less contrained)

=item

display B<tmave2>(I<MASKEXPR,EXPR,time=TBEG,time=TEND [,DT]>)  -  Time averaging with masking

=back


=head2 Advection Functions

=over 4

=item

display B<muadv>(I<U>,I<EXPR>) -  Changing rate [EXPR/s] due to zonal advection 

=item

display B<mvadv>(I<V>,I<EXPR>) -  Changing rate [EXPR/s] due to meridional advection 

=item

display B<mwadv>(I<W>,I<EXPR>) -  Changing rate [EXPR/s] due to vertical advection

=item

display B<madvu>(I<U,EXPR>) -  Zonal flux gradient: -d(U*EXPR)/dx 

=item

display B<madvv>(I<V,EXPR>) - Meridional flux gradient: -d(V*EXPR)/dy 

=item

display B<madvw>(I<W,EXPR>) - Vertical flux gradient: -d(W*EXPR)/dz 

=back

=head2 Thermodynamic Functions

=over 4

=item

display B<satvap>(I<T>) -  Saturated vapor pressure 

=item

display B<dew>(I<PVAP>) -  Dew-point Temperature

=back


=head2 Radiative Transfer Functions

=over 4

=item

display B<lw>(I<PL,TA,WA,TAUCL,FCLD,PS,TG,EG,TB,WB,TAUCLB,FCLDB [,BINFILE,CTLFILE,-r|-c|-h|-l]>) -  Longwave radiative coolimg rate (K/d)

=item

display B<lw2>(I<PL,TA,WA,TAUCL,FCLD,PS,TG,EG,TB,WB,PM,TM,WM,TAUCLM,FCLDM [,BINFILE,CTLFILE,-r|-c|-h|-l]>) -  Longwave radiative coolimg rate (K/d) (same as lw but requires data at the top of mixed layer)

=back

=head2 Vertical Interpolation/Integration Functions 

=over 4

=item

display B<pinterp>(I<EXPR,PGRID,PLEV,[-l|-s|-p]>) -  Vertical interpolation within a 3-D grid to a specified p level. 

=item

display B<zinterp>(I<field,zgrid,zlev,[-l|-s|-p]>) -  Vertical interpolation within a 3-D grid to a specified z level. 

=item 

display B<vint2>(I<PSEXPR,EXPR,TOP>) - Modified vertical integration

=back


=head2 Miscelaneous Functions

=over 4

=item

display B<if>(I<EXPR1,OP,EXPR2,TRUE_EXP,FALSE_EXP>) - Compare expressions

=item

display B<which>(I<EXPR1,COND1,EXP1,COND2,EXPR2,...,ELSE_EXP>) -  Label gridpoints according to expressions

=item

display B<maxv>(I<EXPR1>,I<EXPR2>)  -  Maximum value

=item

display B<minv>(I<EXPR1>,I<EXPR2>)  -  Minimum value

=item

display B<line>(I<EXPR,LON1,LAT1,LON2,LAT2[,-l|-m|-r]>)  - Draw a Line  

=back


=head1 DESCRIPTION 

This library implements Ben-Jei Tsuang`s collection GrADS extensions,
including a variety of statistical, mathematical and otherwise utility
functions.

These functions were initially built in an experimental version of
GrADS v1.8 contributed by Ben-Jei Tsuang ca. 2003. With the
introduction of dynamic linked extensions by the OpenGrADS project in
2006 these functions were among the first set of user defined functions
to be implemented as such.

=head1 COMMAND LINE HELP

Usage notice for most of these functions can be obtained at the GrADS
command line. For example,

   ga-> d lw

produces

   lw
   Error from LW:  10-13 arguments expected
   lw(pl,ta,wa,taucl,fcld,ps,tg,eg,tb,wb [,'binfile','ctlfile',-r-c-h-l])
   usage:
   Function returns: thermal infrared fluxes
   Following the NASA Technical Memorandum (NASA/TM-2001-104606,
   Vol. 19) of Chou, Suarez, Liang, and Yan (2001). This NASA TM
   has been revised a few timessince. It computes thermal infrared
   fluxes due to emission by water vapor, ozone, co2, o2, minor
   trace gases, clouds, and aerosols and due to scattering by
   clouds and aerosols.
    
   Arguments:
      pl = level pressure (hPa)
      ta = layer temperature (K)
      wa = layer specific humidity (kg/kg)
      taucl= cloud optical thickness (dimensionless)
    ...

=head1 TIME RELATED FUNCTIONS

=head2  B<lt>(I<EXPR>) 

=over 4

This function calculates local time (in hours); specify any valid
GrADS expression on input.

=back

=head2  B<jd>(I<EXPR>) 

=over 4

This function computes the Julian day since January 0001; specify any valid
GrADS expression on input.

=back

=head2  B<cosz>(I<EXPR>, I<h|d|m>) 

=over 4

This function calculates the cosine of solar zenith angle on various
time scale. On input,

=over 8

=item I<EXPR>

Any GrADS vaild expression, e.g., C<lat>

=back

Specify one of the following for the time scale:

=over 8

=item I<h>

Hourly value of the cosine of solar zenith angle.

=item I<d>

Daily mean of the cosine of solar zenith angle.

=item I<m>

Monthly mean of the cosine of solar zenith angle.

=back

=back



=head2  B<dayratio>(I<EXPR>)

=over 4

This function calculates daylight ratio of EXPR during the time interval of the variable, where 1 argument is required..

=over 8

=item I<EXPR>

Any GrADS valid expression, e.g., C<lat>.

=back

=back


=head1 STATISTICAL FUNCTIONS

=head2  B<ftest>(I<RATIO>,I<DF1>,I<DF2>)

=over 4

This function conducts the F test. It calculates the probability for
the I<ratio> of two variances, where 3 arguments are expected.

=over 8

=item I<RATIO>

The ration of variances: (variance 1)/(variance 2)

=item I<DF1>

Degrees of freedom for variance 1

=item I<DF2>

Degrees of freedom for variance 2

=back

=back


=head2  B<ttest>(I<DIFF>,I<DF>)

=over 4

This function conducts the T test. It calculates the probability two
means to be different, where 2 arguments are expected.

=over 8

=item I<DIFF>

The normalized difference between 2 means:

   DIFF = (m1-m2)/stdv

where C<m1> and C<m2> are the 2 means, and C<stdv> is the standard deviation.

=item I<DF>

Degrees of freedom.

=back

=back



=head2  B<tfit>(I<Y1,X1,X2,X3,..,XN,time=TBEG,time=TEND[,DT,FILENAME]>)

=over 4

This function conducts point multiple linear regression for

   Y1 = F(X1,X2,X2,..,XN) 

for each grid point seperately during the period between I<TBEG> and
I<TEND>, with time interval I<DT> to skip.  The correlation
coefficients (r) at each grid are returned. If the option FILENAME
is provided, many statistical summaries are written to the file.

=over 8

=item I<Y1,X1,X2,X3,..,XN>

Must be expressions.

=item I<time=TBEG>

Begining of the time, which can be any valid time variable with
formats like C<time=jan1990> or C<t=100>.

=item I<time=TEND>

End of the time period, which can be any valid time variable with
formats like C<time=dec2000>, C<t=500> or C<t+100>.

=item I<DT> (optional)

Time interval to skip, which can be any valid time variable
(e.g. C<1mon, 1day>).  

=item I<FILENAME> (optional)

Filename to which many statistical summaries are written to.

=back

=back


=head2  B<fit>(I<Y1,X1,X2,X3,..,XN,time=TBEG,time=TEND[,DT,FILENAME]>)

=over 4


This function conducts global multiple linear regression for

   Y1 = F(X1,X2,X2,..,XN) 

for all the grid points simultaneously during the period between
I<TBEG> and I<TEND>, with time interval I<DT> to skip.  The results
are written to screen. If the option I<FILENAME> is provided, many
statictical summaries are written to the file.

=over 8

=item I<Y1,X1,X2,X3,..,XN>

Must be expressions.

=item I<time=TBEG>

Begining of the time, which can be any valid time variable with
formats like C<time=jan1990> or C<t=100>.

=item I<time=TEND>

End of the time period, which can be any valid time variable with
formats like C<time=dec2000>, C<t=500> or C<t+100>.

=item I<DT> (optional)

Time interval to skip, which can be any valid time variable
(e.g. C<1mon, 1day>).  

=item I<FILENAME> (oprional)

Filename to which many statistical summaries are written to.

=back

=back


=head2  B<tregr2>(I<X,Y,time=TBEG,time=TEND[,DT]>)

=over 4

This function calculates C<m> coefficient for the linear regression 

   Y = m * X + C 

at each grid during the period between I<TBEG> and I<TEND>, with
timeinterval I<DT> to skip. (Same as B<tregr>, but with I<DT> option).

=over 8

=item I<Y, X> 

must be expressions.

=item I<time=TBEG>

Begining of the time, which can be any valid time variable with 
formats like C<time=jan1990>. or C<t=100>.

=item I<time=TEND>

End of the time, which can be any valid time variable with formats 
like C<time=dec2000>, C<t=500> or C<t+100>.

=item I<DT>

Time interval to skip, which can be any valid time variable 
(e.g. C<1mon>, C<1day>.).

=back 

=back


=head2  B<tmave2>(I<MASKEXPR,EXPR,time=TBEG,time=TEND[,DT]>)

=over 4

This function does time averaging while applying a mask. 
(Same as B<tmave>, but with I<DT> option.)

=over 8

=item I<MASKEXPR>

The mask expression; when evaluated at a fixed time, it must give a
single value.

=item I<EXPR>

The expression to be averaged

=item I<time=TBEG> 

Begining of the time, which can be any valid time variable with
formats like C<time=jan1990> or C<t=100>.

=item I<time=TEND>

End of the time, which can be any valid time variable with formats
like C<time=dec2000>, C<t=500> or C<t+100>.

=item I<DT>

Time interval to skip, which can be any valid time variable 
(e.g. C<1mon>, C<1day>).

=item I<Usage Notes>

This function works similarly to the ave function, except for the
masking. Using B<tmave2> is much more efficient than using B<maskout> with
ave).

The function loops through the specified time steps, and evaluates
I<MASKEXPR> at each fixed time. I<MASKEXPR> must yeild a single
value. If this value is the undefined/missing data value, then expr
for that time is not included in the average.  If I<MASKEXPR> is not
the undefined data value, it is used as the weight for I<EXPR> in the
average. So if you define I<MASKEXPR> accordingly, you can use the
B<tmave2> function to do weighted time averaging.

The tricky aspect of using B<tmave2> is setting up I<MASKEXPR>. If I<EXPR> is a
grid with X and/or Y and/or Z varying, then I<MASKEXPR> B<must> refer to
either a defined variable or a file with only time varying. In
general, you have to set up I<MASKEXPR> in advance.

=item I<Examples>

Say you want to average C<slp> over some time range but only when C<sst>
over some region is above some value. You can do this by:

   ga-> set x 1
   ga-> set y 1
   ga-> set t 1 last
   ga-> define sstmask = aave(sst,lon=-180,lon=-90,lat=-20,lat=20)
   ga-> define sstmask = const(maskout(sstmask,sstmask-25.0),1)

Now I<SSTMASK> is a time series where the value is 1 when the C<sst> areal
average is above 25 and undefined when the value is below 25. B<maskout>
set the values below 25 to missing; const set the non-missing values
to 1. We can now do our tmave2:

   ga-> set lon -180 -90
   ga-> set lat -20 20
   ga-> set t 1
   ga-> d tmave2(sstmask,slp,t=1,t=last)

The mask could also be written to a file with all dimensions
nonvarying except for time. Here is what some of the records in the
data descriptor file might look like:

   DSET maskfilename
   XDEF 1 linear 1 1
   YDEF 1 linear 1 1
   ZDEF 1 linear 1 1
   TDEF 100 linear 

=back 

=back


=head1 ADVECTION FUNCTIONS

=head2  B<madvu>(I<U>,I<EXPR>)

=over 4

This function calculates 

   -d(u*EXPR)/dx

=over 8

=item I<U>

Zonal wind speed (u component), m/s. 

=item I<EXPR>

An expression

=back

=back



=head2  B<madvv>(I<V>,I<EXPR>)

=over 4

This function calculates 

   -d(V*EXPR)/dy

=over 8

=item I<V>

Meridional wind speed (v component), m/s. 

=item I<EXPR>

An expression

=back

=back



=head2  B<madvw>(I<W>,I<EXPR>)

=over 4

This function calculates 

   -d(W*EXPR)/dp 

using an upwind scheme.

=over 8

=item I<W>

Vertical wind speed (w component) (unit of vertical coordinate/s, such as hPa/s for the vertical coordinate in hPa)

=item I<EXPR>

An expression.

=item I<Remarks>

Note that no-slip boundary conditions are assumed. i.e.,

      w(lev=0)=0
      C(lev=0)=C(lev=1)
  
      concp, wp
           |---cn
      conc, w
           |---cnm
      concm, wm


=back

=back



=head2  B<muadv>(I<U>,I<EXPR>)

=over 4

This function computes the time rate of change I<EXPR> at each grid point due
to zonal advecion (plus increase) according to Bott`s 4-level scheme.

=over 8

=item I<U>

Zonal wind speed (u component),  m/s. 

=item I<EXPR>

An expression

=item I<Reference>

Bott, A., 1989a: A positive definite advection scheme obtained by
nonlinear renormalization of the advection flux. I<Mon. Wea.
Rev.>, B<117>, 1006-1015.  

Bott, A., 1989b: Notes and correspondance. I<Mon. Wea. Rev.>,
B<117>, 2633-2636.


=back

=back



=head2  B<mvadv>(I<V>,I<EXPR>)

=over 4

This function computes the time rate of change I<EXPR> at each grid point due
to meridional advecion (plus increase) according to Bott`s 4-level scheme.

=over 8

=item I<V>

Meridional wind speed (v component),  m/s. 

=item I<EXPR>

An expression

=item I<Reference>

Bott, A., 1989a: A positive definite advection scheme obtained by
nonlinear renormalization of the advection flux. I<Mon. Wea.
Rev.>, B<117>, 1006-1015.  

Bott, A., 1989b: Notes and correspondance. I<Mon. Wea. Rev.>,
B<117>, 2633-2636.


=back

=back


=head2  B<mwadv>(I<W>,I<EXPR>) 

=over 4

This function computes the time rate of change I<EXPR> at each grid point due
to vertical advecion (plus increase) using an upwind scheme.

=over 8

=item I<W>

Vertical wind speed (w component) (unit of vertical coordinate/s, such
as hPa/s for the vertical coordinate in hPa)

=item I<EXPR>

An expression

=back

=back

=head1 THERMODYNAMICAL FUNCTIONS

=head2  B<satvap>(I<T>)

=over 4

This function calculates the saturated vapor pressure (Pa)

=over 8

=item I<T> 

Temperature (K)

=back

=back


=head2  B<dew>(I<PVAP>) 

=over 4

This function calcuates the dew point temperature (K).

=over 8

=item I<PVAP>

Vapor pressure (Pa)

=back

=back

=head1 RADIATIVE TRANSFER FUNCTIONS

=head2  B<lw>(I<PL,TA,WA,TAUCL,FCLD,PS,TG,EG,TB,WB,TAUCLB,FCLDB[,BINFILE,CTLFILE,-r|-c|-h|-l]>)

=over 4

This function computes the cooling rate [K/day] and thermal infrared
fluxes and returns it in a file.

=over 8

=item I<PL>

level pressure (hPa)

=item I<TA>

layer temperature (K)

=item I<WA >

layer specific humidity (kg/kg)

=item I<TAUCL>

cloud optical thickness (dimensionless)

=item I<FCLD>

cloud fraction

=item I<PS>

surface pressure (hPa)

=item I<TG>

land or ocean surface temperature (K)

=item I<EG>

land or ocean surface emissivity (fraction)

=item I<TB>

surface air temperature (K)

=item I<WB>

surface air specific humidity (kg/kg)

=item I<TAUCLB>

surface cloud optical thickness (dimensionless)

=item I<FCLDB>

surface cloud amount (fraction)

=item I<BINFILE>

binary file name ('null' or blank for no output)

=item I<CTLFILE>

ctl file name ('null' or blank for no output)

=item I<-r>

using Roberts et al. (1976) water vaopr continuum

=item I<-c>

using Clough et al. (1989) water vaopr continuum (default)

=item I<-h >

using look-up table (high acc but slow) (default)

=item I<-l>

using k-dist method (low acc but fast)

=item I<Reference>

Following the NASA Technical Memorandum (NASA/TM-2001-104606,
Vol. 19) of Chou, Suarez, Liang, and Yan (2001). This NASA TM
has been revised a few times since. It computes thermal infrared
fluxes due to emission by water vapor, ozone, CO2, O2, minor
trace gases, clouds, and aerosols and due to scattering by
clouds and aerosols.

=back

=back


=head2  B<lw2>(I<PL,TA,WA,TAUCL,FCLD,PS,TG,EG,TB,WB,PM,TM,WM,TAUCLM,FCLDM [,BINFILE,CTLFILE,-r|-c|-h|-l]>)

=over 4

This function computes the cooling rate [K/day] and thermal infrared
fluxes and returns it in a file. (Same as lw, but required data at the
top of mixed layer)

=over 8


=item I<PL >

level pressure (hPa)

=item I<TA >

layer temperature (K)

=item I<WA >

layer specific humidity (kg/kg)

=item I<TAUCL>

cloud optical thickness (dimensionless)

=item I<FCLD>

cloud amount (fraction)

=item I<PS >

surface pressure (hPa)

=item I<TG >

land or ocean surface temperature (K)

=item I<EG >

land or ocean surface emissivity (fraction)

=item I<TB >

surface air temperature (K)

=item I<WB >

surface air specific humidity (kg/kg)

=item I<TAUCLB>

surface cloud optical thickness (dimensionless)

=item I<FCLDB>

surface cloud amount (fraction)

=item I<PM >

pressure at the top of mixed layer (hPa)

=item I<TM >

temperature at the top of mixed layer (K)

=item I<WM >

specific humidity at the top of mixed layer (kg/kg)

=item I<TAUCLM >

cloud optical thickness at the top of mixed layer (dimensionless)

=item I<FCLDM >

cloud amount at the top of mixed layer (fraction)

=item I<BINFILE >

binary file name ('null' or blank for no output)

=item I<CTLFILE >

ctl file name ('null' or blank for no output)

=item I<-r > (optional)

using Roberts et al. (1976) water vaopr continuum

=item I<-c > (optional)

using Clough et al. (1989) water vaopr continuum (default)

=item I<-h > (optional)

using look-up table (high acc but slow) (default)

=item I<-l > (optional)

using k-dist method (low acc but fast)

=item I<Reference>

Following the NASA Technical Memorandum (NASA/TM-2001-104606, Vol. 19)
of Chou, Suarez, Liang, and Yan (2001). This NASA TM has been revised
a few timessince. It computes thermal infrared fluxes due to emission
by water vapor, ozone, co2, o2, minor trace gases, clouds, and
aerosols and due to scattering by clouds and aerosols.

=back

=back


=head1 VERTICAL INTERPOLATION/INTEGRATION FUNCTIONS

=head2  B<pinterp>(I<FIELD,PGRID,PLEV,[-l|-s|-p]>)

=over 4

This function interpolates a 3-D field to a specified pressure level.
Can also be used on non-pressure level data, such as sigma or
eta-coordinate output where pressure is a function of time and grid
level.

=over 8

=item I<Function returns> 

Defined grid C<vinterp> holding interpolated values

=item I<FIELD>

Name of 3-D grid to interpolate

=item I<PGRID>

Name of 3-D field holding pressure values at each gridpoint.  If you
are using regular pressure-level data, this should be set to the
builtin GrADS variable C<lev>.

=item I<PLEV>

Pressure level at which to interpolate (can be 3-D, such as reserved
variable C<lev>)


=item I<-l> (optional)

for piecewise linear interpolation (fast) (default)

=item I<-s> (optional)

for spline interpolation (slow & problematic while dx is small)

=item I<-p> (optional)

for polynomnial interpolation (fast but problematic with extrapolation)
           

=back

=back



=head2  B<zinterp>(I<FIELD,ZGRID,ZLEV,[-l|-s|-p]>)

=over 4

This function interpolates a 3-D field to a specified z level.  Can
also be used on non-z level data, such as sigma or eta-coordinate
output where C<z> is a function of time and grid level.

=over 8

=item I<Function returns> 

Defined grid C<vinterp> holding interpolated values

=item I<FIELD>

Name of 3-D grid to interpolate

=item I<ZGRID>

Name of 3-D grid holding z values at each gridpoint

=item I<ZLEV>

z level at which to interpolate

=item -l (optional)

for piecewise linear interpolation (fast) (default)

=item -s (optional)

for spline interpolation (slow & problematic while dx is small)

=item -p (optional)

for polynomnial interpolation (fast but problematic with extrapolation)
           

=back

=back


=head2  B<vint2>(I<PSEXPR,EXPR,TOP>)

=over 4

This function performs a mass-weighted vertical integral in mb
pressure coordinates.

The calculation is a sum of the mass-weighted layers:

        f/g * sum(expr * Delta(level))

The bounds of the integration are the surface pressure and the
indicated top value. The scale factors are C<f=100> and C<g=9.8>. The
summation is done for each layer present that is between the bounds.
The layers are determined by the Z levels of the default file. Each
layer is considered to be from the midpoints between the levels actually
present, and is assumed to have the same value throughout the layer, namely
the value of the gridpoint at the middle of the layer.

=over 8

=item I<PSEXPR>

A GrADS expression for the surface pressure, in mb, which bounds the
integral on the bottom.

=item I<EXPR>

A GrADS expression representing the quantity to be integrated.

=item I<TOP> 

The bounding top pressure, in mb. This value must be a constant and
cannot be provided as an expression.

=item I<Usage Notes>

The summation is done using the Z levels from the default file, so it
is important that the default file have the same Z dimension
coordinates as I<EXPR>. Data levels below and above the bounds of the
summation are ignored. The Z dimension in world-coordinate units is
assumed to be pressure values given in millibars (mb).  The units of C<g>
are such that when the expression integrated is specific humidity (C<q>)
in units of g/g, the result is kg of water per square meter, or
precipitable water in mm.  It is usually a good idea to make the top
pressure value to be at the top of a layer, which is midway between
grid points. For example, if the default file (and the data) have
pressure levels of ...,500,400,300,250,... then a good value for top
might be 275, the value at the top of the layer that extends from 350
to 275 mb. The B<vint2> function operates only in an X-Y varying dimension
environment.

=item I<Example>

A typical use of vint might be:

   ga-> d vint(ps,q,275)

This expression will integrate specific humidity to obtain
precipitable water, in mm.

=back

=back


=head1 MISCELANEOUS FUNCTIONS


=head2  B<if>(I<EXPR1,OP,EXPR2,TRUE_EXPR,FALSE_EXPR>)

=over 4

This function compares I<EXPR1> and I<EXPR2>, where 5 arguments are
expected.  if their relation is true, I<TRUE_EXP> is returned, otherwise
expression I<FALSE_EXPR> is returned.

=over 8

=item I<OP>

One of the following: ==, =, >, <, >=, <=, !=

=item I<EXPR2, TRUE_EXPR, FALSE_EXPR>

These can be a expression, constant or -u.

=back

=back


=head2  B<which>(I<EXPR,COND1,EXPR1,COND2,EXPR2,...,ELSE_EXPR>)

=over 4

This function labels gridpoints according to specified conditions,
where an even number of arguments is expected. The number of arguments
is limited by the maximum characters (about 128 charcters) being
handled by the argument string. If I<EXPR> equals to I<COND1>, then I<EXPR1> is
returned.  Otherwise, if I<EXPR> equals to I<COND2>, then I<EXPR2> is returned. If
none of the conditions are met, I<ELSE_EXPR> is returned.

=over 8

=item I<EXPR>

A valid GrADS expression

=item I<EXPR2, TRUE_EXPR, FALSE_EXPR>

These can be a expression, constant or -u.

=back

=back



=head2  B<maxv>(I<EXPR1>,I<EXPR2>) 

=over 4

This function displays the maximum value between I<EXPR1> and I<EXPR2>,
where 2 arguments are expected.

=over 8

=item I<EXPR1>

Must be an expression

=item I<EXPR2> 

Can be an expression, constant or -u.

=back

=back



=head2  B<minv>(I<EXPR1>,I<EXPR2>) 

=over 4

This function displays the minimum value between I<EXPR1> and I<EXPR2>,
where 2 arguments are expected.

=over 8

=item I<EXPR1>

Must be an expression

=item I<EXPR2> 

Can be an expression, constant or -u.

=back

=back


=head2  B<line>(I<EXPR,LON1,LAT1,LON2,LAT2[,-l|-m|-r]>)

=over 4

This function draws a line from I<(LON1,LAT1)> to I<(LON2,LAT2)>.  Set
the line at the value I<VAL> (default 1) using the template of I<EXPR>.

=over 8

=item I<LON1,LAT1>

Starting longitude/latitude

=item I<LON2,LAT2>

Ending longitude/latitude

=item I<-l> (optional)

 return the length [km] in each grid

=item I<-m> (optional)

 return the logics (1:pass, 0: non-passing)

=item I<-r> (optional)

return the length ratio of each segment (fraction)


=back

=back

=head1 EXAMPLES

=head2 C<Minv>: displaying the minimum value between I<EXP1> and I<EXP2>

  set clevs -12 -9 -6 -3 0 3 6 9 12
  d minv(u,0)
  run cbarn.gs
  draw title d minv(u,0)

=head2 C<Maxv>: displaying the maximum value between I<EXP1> and I<EXP2>

  set clevs -12 -9 -6 -3 0 3 6 9 12
  d maxv(u,0)
  run cbarn.gs
  draw title d maxv(u,0)

=head2 C<If>: selectively displaying values according to the relationship between two expressions.

  d if(u,>,0,t,-u)
  run cbarn.gs
  draw title if(u,>,0,t,-u)

=head2 C<Which>: label gridpoints according to specific conditions

  set clevs 0 1 2  
  d which(minv(maxv(u,0),10),0,0,10,2,1)
  run cbarn.gs
  draw title u<=0: 0; 0<u<10: 1; u>=10: 2

=head2 C<tfit>: performing multiple linear regression on individual gridpoints

  set t 1
  d tfit('var.0','var.3','var.4',t=1,t=100)
  d tfit('var.0','var.3',t=1,t=100,1mo,"/silver/data/nchu/fit/dmdtE")
  d tfit('var.0','var.3','var.4',t='mon',t+100,1mo,"/silver/data/nchu/fit/dmdtEP")

  d tfit('var.0','var.3','var.4',time=jan1990,time=dec2002)
  d tfit('var.0','var.3',time=jan1990,time=dec2002,1mo,"/silver/data/nchu/fit/dmdtE")
  d tfit('var.0','var.3','var.4',time=jan1990,time=dec2002,1mo,"/silver/data/nchu/fit/dmdtEP")

=head2 C<fit>: performing global multiple linear regression
  
  set t 1
  d fit('var.0','var.3','var.4',t=1,t=100)
  d fit('var.0','var.3',t=1,t=100,1mo,"/silver/data/nchu/fit/dmdtE")
  d fit('var.0','var.3','var.4',t='mon',t+100,1mo,"/silver/data/nchu/fit/dmdtEP")

  d fit('var.0','var.3','var.4',time=jan1990,time=dec2002)
  d fit('var.0','var.3',time=jan1990,time=dec2002,1mo,"/silver/data/nchu/fit/dmdtE")
  d fit('var.0','var.3','var.4',time=jan1990,time=dec2002,1mo,"/silver/data/nchu/fit/dmdtEP")

=head2 C<muadv>: computing zonal advecion

  d muadv(u,t)
  run cbarn.gs
  draw title -u*dT/dx (K s`a-1`n)

=head2 C<mvadv>: computing meridional advecion

  d mvadv(v,t)
  run cbarn.gs
  draw title -v*dT/dy (K s`a-1`n)

=head2 C<mwadv>: computing vertical advecion using an upwind scheme

  d mwadv(w,t)/100  
  run cbarn.gs
  draw title -w*dT/dz (K s`a-1`n)

Notice that the 100 factor above converts the unit of C<w> from C<Pa/s> to
C<hPa/s> if the vertical coordinate is in C<hPa>.

=head2 C<cosz>: computing the cosine of the solar zenith angle

  d cosz(lat,h)
  run cbarn.gs
  draw title hourly cosine zenith angle

  d cosz(lat,d)
  run cbarn.gs
  draw title daily mean cosine zenith angle

  d cosz(lat,m)
  run cbarn.gs
  draw title monthly mean cosine zenith angle

=head2 C<lt>: computing the local time

  d lt(lat)
  run cbarn.gs
  draw title local time (h)

=head2 C<dayratio>: computing the daytime ratio

  d dayratio(lat)
  run cbarn.gs
  draw title daytime ratio (lat)

=head2 C<jd>: computing the Julian day sice January 0001

  d jd(lat)
  run cbarn.gs
  draw title julian date from 1 January 0001 (jd=1)\d jd(lat)

=head2 C<dew>: computing the dew point temperature

  d dew(q/0.622*lev*100)
  run cbarn.gs
  draw title dew point (K)

=head2 C<satvap>: computing the saturated temperature

  d satvap(t)
  run cbarn.gs
  draw title saturated temperature (Pa)

=head2 C<line>: drawing lines

  d line(u,30,60,200,-40,-r)
  run cbarn.gs
  draw title draw line\d line(u,lon0,lat0,lon1,lat1,-r)

=head2 C<vint2>: modified vertical integration

  d vint2(1000,dtdt,300)
  run cbarn.gs
  draw title Integrate[dTdt,{dp,1000,300}] (K/s*kg/m2)

  d vint2(pressfc/100,dTdt,presML/100)
  run cbarn.gs
  draw title (K/s*kg/m2)
  
  d vint(ps,q,275)
  draw title Integrate q from surface to 275 hPa\ Total Precipitable Water (mm)

=head2 C<zinterp>: Vertical interpolation to a specified p level. 

First, conduct 3-d vertical interpolation from file 3 p level to file 1 level.
  
  define cbtzw=pinterp(cbtzw.3,lev.3,lev)   

Then conduct vertical interpolation to 1000 hPa:

  define sxdflmc=pinterp(sxdfl1,pl1,1000)   
  define flxmc=pinterp(flx.'id',pl.'id',levmc.2)
  define tml=pinterp(tprs.2*pow(1000/lev.2,0.286),lev.2,levmh.2(z=1))
  define tsfc=pinterp(tprs.2*pow(1000/lev.2,0.286),lev.2,pressfc.3(z=1)/100)

=head2 C<zinterp>: Vertical interpolation to a specified z level. 

  define tmp1=zinterp(tprs.2,zprs.2/9.8,h1)
  define tmp2=zinterp(tprs.2,zprs.2/9.8,h2)
  define pressfc=zinterp(lev.4,zprs.4/9.8,elv.1(t=1))
  define levmh=zinterp(lev.4,zprs.4/9.8,elvmh)

=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=back

=head1 AUTHORS

Ben-Jei Tsuang <btsuang@yahoo.com>. Arlindo da Silva
<dasilva@opengrads.org> implemented these functions as dynamic User
Defined Extensions.

=head1 COPYRIGHT

Copyright (c) 2003-2008 by Ben-Jei Tsuang <btsuang@yahoo.com>.
All Rights Reserved.

This is free software released under the GNU General Public License;
see the source for copying conditions. There is NO warranty; not even
for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=cut

#endif

