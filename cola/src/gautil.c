/* Copyright (C) 1988-2018 by George Mason University. See file COPYRIGHT for more information. */
/* Modified in 2026 for the terminal display and calculation progress; see COPYING. */

/* Originally authored by B. Doty */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H

#include "config.h"

/* If autoconfed, only include malloc.h when it's present */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#else /* undef HAVE_CONFIG_H */

#include <malloc.h>

#endif /* HAVE_CONFIG_H */

#include "grads.h"
#include "gx.h"

struct gamfcmn mfcmn;

static char pout[1256];   /* Build Error msgs here */

char *gatxtl(char *str, gaint color);
char *gatxtlp(char *str);

/* Set while GrADS waits for a command line, so that Ctrl-C starts a fresh
   line instead of interrupting anything: 1 with readline, 2 without. */
volatile sig_atomic_t ga_at_prompt = 0;
static char ga_prompt_text[300];

/* Retrieves the next command from the user.  Leading blanks
   are stripped.  The number of characters entered before the
   CR is returned.                                                    */

gaint nxtcmd (char *cmd, char *prompt) {
gaint past,cnt;

#ifndef STNDALN
  gaprogreset();
  gxidle();
#endif
  snprintf(ga_prompt_text,sizeof(ga_prompt_text),"%s ",gatxtl(prompt,-1));
  printf ("%s",ga_prompt_text);
  fflush(stdout);
  past = 0;
  cnt = 0;
  ga_at_prompt = 2;
  while (1) {
    *cmd = getchar();
    if (*cmd == EOF) { ga_at_prompt = 0; return (-1); }
    if (*cmd == '\n') {
      cmd++;
      *cmd = '\0';
      ga_at_prompt = 0;
      return (cnt);
    }
    if (past || *cmd != ' ') {
      cmd++; cnt++; past = 1;
    }
  }
}


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

/* Add an offset to a time.  Output to dto.                          */

void timadd (struct dt *dtim, struct dt *dto) {
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

/* Subtract an offset from a time.  Subtract minutes/hours/days
   first so that we will exactly reverse the operation of timadd     */

void timsub (struct dt *dtim, struct dt *dto) {
gaint s1,s2;

  /* Subtract minutes, hour, and days directly.  Then normalize
     to days, then normalize deficient days from months/years.       */

  dto->mn = dtim->mn - dto->mn;
  dto->hr = dtim->hr - dto->hr;
  dto->dy = dtim->dy - dto->dy;
  s1 = dto->mo; s2 = dto->yr;
  dto->mo = dtim->mo;
  dto->yr = dtim->yr;

  while (dto->mn < 0) {dto->mn+=60; dto->hr--;}
  while (dto->hr < 0) {dto->hr+=24; dto->dy--;}

  while (dto->dy < 1) {
    dto->mo--;
    if (dto->mo < 1) {dto->mo=12; dto->yr--;}
    if (dto->mo==2 && qleap(dto->yr)) dto->dy += 29;
    else dto->dy += mosiz[dto->mo];
  }

  /* Now subtract months and years.  Normalize as needed.            */

  dto->mo = dto->mo - s1;
  dto->yr = dto->yr - s2;

  while (dto->mo < 1) {dto->mo+=12; dto->yr--;}

  /* Adjust for leaps */

  if (dto->mo==2 && dto->dy==29 && !qleap(dto->yr)) {
    dto->mo=3; dto->dy=1;
  }
}

/* Convert from Absolute time (year/month/day/etc.) to grid
   coordinate.                                                       */

gadouble t2gr (gadouble *vals, struct dt *dtim) {
struct dt stim;
gaint eyear,mins,mons;
gadouble val,*moincr,*mnincr,rdiff;

  /* Get constants associated with this conversion                   */

  stim.yr = (gaint)(*vals+0.1);
  stim.mo = (gaint)(*(vals+1)+0.1);
  stim.dy = (gaint)(*(vals+2)+0.1);
  stim.hr = (gaint)(*(vals+3)+0.1);
  stim.mn = (gaint)(*(vals+4)+0.1);

  moincr = vals+5;
  mnincr = vals+6;

  /* If the increment for this conversion is days, hours, or minutes,
     then we do our calculations in minutes.  If the increment is
     months or years, we do our calculations in months.              */


  if (*mnincr>0.1) {
    mins = timdif(&stim,dtim,0);
    rdiff = (gadouble)mins;
    val = rdiff/(*mnincr);
    val += 1.0;
    return (val);
  } else {
    mons = timdif(&stim,dtim,1);
    eyear = stim.yr;
    if (stim.yr > dtim->yr) eyear = dtim->yr;
    rdiff = (((dtim->yr - eyear)*12) + dtim->mo) -
            (((stim.yr - eyear)*12) + stim.mo);
    stim.yr = dtim->yr;
    stim.mo = dtim->mo;
    mins = timdif(&stim,dtim,0);
    if (mins>0) {
      if (dtim->mo==2 && qleap(dtim->yr) ) {
        rdiff = rdiff + (((gadouble)mins)/41760.0);
      } else {
        rdiff = rdiff + (((gadouble)mins)/((gadouble)momn[dtim->mo]));
      }
    }
    val = rdiff/(*moincr);
    val += 1.0;
    return (val);
  }
}

/* Convert from a t grid coordinate to an absolute time.           */

void gr2t (gadouble *vals, gadouble gr, struct dt *dtim) {
struct dt stim;
gadouble *moincr,*mnincr;
gadouble v;

  /* Get constants associated with this conversion                   */
  stim.yr = (gaint)(*vals+0.1);
  stim.mo = (gaint)(*(vals+1)+0.1);
  stim.dy = (gaint)(*(vals+2)+0.1);
  stim.hr = (gaint)(*(vals+3)+0.1);
  stim.mn = (gaint)(*(vals+4)+0.1);
  moincr = vals+5;
  mnincr = vals+6;

  /* Initialize output time                                          */
  dtim->yr = 0;
  dtim->mo = 0;
  dtim->dy = 0;
  dtim->hr = 0;
  dtim->mn = 0;

  /* Do conversion if increment is in minutes.                       */
  if (*mnincr>0.1) {
    v = *mnincr * (gr-1.0);
    if (v>0.0) v = v + 0.5;   /* round */
    else v = v - 0.5;
    dtim->mn = (gaint)v;
    if (dtim->mn<0) {
      dtim->mn = -1 * dtim->mn;
      timsub (&stim,dtim);
    } else {
      timadd (&stim,dtim);
    }
    return;

  /* Do conversion if increment is in months.  Same as for minutes,
     except special handling is required for partial months.   
     JMA There is a bug here, and some precision decisions that need attention */

  } else {
    v = *moincr * (gr-1.0);
    if (v<0.0) dtim->mo = (gaint)(v-0.9999); /* round (sort of)       */
    else dtim->mo = (gaint)(v+0.0001);
    v = v - (gadouble)dtim->mo;                /* Get fractional month  */
    if (dtim->mo<0) {
      dtim->mo = -1 * dtim->mo;
      timsub (&stim,dtim);
    } else timadd (&stim,dtim);
    if (v<0.0001) return;         /* if fraction small, return       */

    if (dtim->mo==2 && qleap(dtim->yr) ) {
      v = v * 41760.0;
    } else {
      v = v * (gadouble)momn[dtim->mo];
    }
    stim = *dtim;
    dtim->yr = 0;
    dtim->mo = 0;
    dtim->dy = 0;
    dtim->hr = 0;
    dtim->mn = (gaint)(v+0.5);
    timadd (&stim,dtim);
    return;
  }
}

/* Calculate the difference between two times and return the
   difference in minutes (if flag==0) or months (if flag==1). 
   The calculation is time2 - time1, so
   if time2 is earlier than time1, the result is negative.           */

gaint timdif (struct dt *dtim1, struct dt *dtim2, gaint flag) {
gaint min1,min2,mon1,mon2,yr;
struct dt *temp;
gaint swap,mo1,mo2;

  swap = 0;
  if (dtim1->yr > dtim2->yr) {
    temp = dtim1;
    dtim1 = dtim2;
    dtim2 = temp;
    swap = 1;
  }

  /* add up minutes/months for each year between time2 and time1 */
  min1 = 0;
  min2 = 0;
  mon2 = 0;
  yr = dtim1->yr;
  while (yr < dtim2->yr) {
    if (qleap(yr)) min2 += 527040L;
    else min2 += 525600L;
    mon2 += 12;
    yr++;
  }

  if (flag) {
    /* return months */  
    mon1 = dtim1->mo;
    mon2 += dtim2->mo;
    if (swap) return (mon1-mon2);
    else return (mon2-mon1);
  } 
  else {
    /* return minutes */
    mo1 = dtim1->mo;
    mo2 = dtim2->mo;
    if (qleap(dtim1->yr)) {
      min1 = min1+mnacul[mo1]+(dtim1->dy*1440L)+(dtim1->hr*60L)+dtim1->mn;
    } else {
      min1 = min1+mnacum[mo1]+(dtim1->dy*1440L)+(dtim1->hr*60L)+dtim1->mn;
    }
    if (qleap(dtim2->yr)) {
      min2 = min2+mnacul[mo2]+(dtim2->dy*1440L)+(dtim2->hr*60L)+dtim2->mn;
    } else {
      min2 = min2+mnacum[mo2]+(dtim2->dy*1440L)+(dtim2->hr*60L)+dtim2->mn;
    }
    if (swap) return (min1-min2);
    else return (min2-min1);
  }
}

/* Test for leap year.  Rules are:

      Divisible by 4, it is a leap year, unless....
      Divisible by 100, it is not a leap year, unless...
      Divisible by 400, it is a leap year.                           */

gaint qleap (gaint year)  {
gaint i,y;

/*mf - disable if 365 day calendar mf*/

 if(mfcmn.cal365 == 1) return(0);

  y = year;

  i = y / 4;
  i = (i*4) - y;
  if (i!=0) return (0);

  i = y / 100;
  i = (i*100) - y;
  if (i!=0) return (1);

  i = y / 400;
  i = (i*400) - y;
  if (i!=0) return (0);

  return (1);


}

static char *mons[12] = {"jan","feb","mar","apr","may","jun",
			 "jul","aug","sep","oct","nov","dec"};

/* Parse an absolute date/time value.  Format is:

   12:00z 1jan 1989 (jan,feb,mar,apr,may,jun,jul,aug,sep,oct,nov,dec)

   Must have Z or Month abbrev, or value is invalid.  'def' contains
   higher order missing values (usually from tmin in pst).  Lower order
   values are defaulted to be: dy = 1, hr = 0, mn = 0.              */

char *adtprs (char *ch, struct dt *def, struct dt *dtim) {
gaint val,flag,i;
char *pos;
char monam[5];

  pos = ch;

  dtim->mn = 0;
  dtim->hr = 0;
  dtim->dy = 1;

  if (*ch>='0' && *ch<='9') {
  flag = 0;
    ch = intprs (ch,&val);
    if (*ch == ':' || tolower(*ch) == 'z') {
      if (val>23) {
        gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
        snprintf(pout,1255,"  Hour = %i -- greater than 23\n",val);
        gaprnt (0,pout);
        return (NULL);
      }
      dtim->hr = val;
      if (*ch == ':') {
        ch++;
        if (*ch>='0' && *ch<='9') {
          ch = intprs (ch,&val);
          if (val>59) {
            gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
            snprintf(pout,1255,"  Minute = %i -- greater than 59\n",val);
            gaprnt (0,pout);
            return (NULL);
          }
          if (tolower(*ch)!='z') {
            gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
            gaprnt (0,"  'z' delimiter is missing \n");
            return (NULL);
          }
          dtim->mn = val;
          ch++;
          if (*ch>='0' && *ch<='9') ch = intprs (ch,&val);
          else val = def->dy;
        } else {
          gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
          gaprnt (0,"  Missing minute value \n");
          return (NULL);
        }
      } else {
        ch++;
        if (*ch>='0' && *ch<='9') ch = intprs (ch,&val);
        else val = def->dy;
      }
    } else flag = 2;
    dtim->dy = val;
  } else flag = 1;

  monam[0] = tolower(*ch);
  monam[1] = tolower(*(ch+1));
  monam[2] = tolower(*(ch+2));
  monam[3] = '\0';

  i = 0;
  while (i<12 && !cmpwrd(monam,mons[i]) ) i++;
  i++;

  if (i==13) {
    if (flag==1) {
      gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
      gaprnt (0,"  Expected month abbreviation, none found\n");
      return (NULL);
    }
    if (flag==2) {
      gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
      gaprnt (0,"  Missing month abbreviation or 'z' delimiter\n");
      return (NULL);
    }
    dtim->mo = def->mo;
    dtim->yr = def->yr;
  } else {
    dtim->mo = i;
    ch+=3;
    /* parse year */
    if (*ch>='0' && *ch<='9') {
      /* use fullyear only if year 1 = 0001*/
      if(*(ch+2)>='0' && *(ch+2)<='9') {
	mfcmn.fullyear=1;   /* 4-digit year */
      } else {
	mfcmn.fullyear=0;   /* 2-digit year */
      }
      ch = intprs (ch,&val);
    } else {
      val = def->yr;
    }

    /* turn off setting of < 100 years to 1900 or 2000 */
    if(mfcmn.fullyear == 0) {
      if (val<50) val+=2000;
      else if (val<100) val+=1900;
    }
    dtim->yr = val;
  }

  i = mosiz[dtim->mo];
  if (dtim->mo==2 && qleap(dtim->yr)) i = 29;
  if (dtim->dy > i) {
    gaprnt (0,"Syntax Error:  Invalid Date/Time value.\n");
    snprintf(pout,1255,"  Day = %i -- greater than %i \n",dtim->dy,i);
    gaprnt (0,pout);
    return (NULL);
  }
  return (ch);
}

/* Parse a relative date/time (offset).  Format is:

   nn (yr/mo/dy/hr/mn)

   Examples:  5mo
              1dy12hr
              etc.

   Missing values are filled in with 0s.                             */

char *rdtprs (char *ch, struct dt *dtim) {
gaint flag,val;
char *pos;
char id[3];

  pos = ch;

  dtim->yr = 0;
  dtim->mo = 0;
  dtim->dy = 0;
  dtim->hr = 0;
  dtim->mn = 0;

  flag = 1;

  while (*ch>='0' && *ch<='9') {
    flag = 0;
    ch = intprs(ch,&val);
    id[0] = *ch; id[1] = *(ch+1); id[2] = '\0';
    if (cmpwrd("yr",id)) dtim->yr = val;
    else if (cmpwrd("mo",id)) dtim->mo = val;
    else if (cmpwrd("dy",id)) dtim->dy = val;
    else if (cmpwrd("hr",id)) dtim->hr = val;
    else if (cmpwrd("mn",id)) dtim->mn = val;
    else {
      gaprnt (0,"Syntax Error:  Invalid Date/Time offset.\n");
      snprintf(pout,1255,"  Expecting yr/mo/dy/hr/mn, found %s\n",id);
      gaprnt (0,pout);
      return (NULL);
    }
    ch+=2;
  }
  if (flag) {
    gaprnt (0,"Syntax Error:  Invalid Date/Time offset.\n");
    gaprnt (0,"  No offset value given\n");
    return (NULL);
  }
  return (ch);
}


/* Compares two strings.  A match occurs if the leading
   blank-delimited words in the two strings match.  CR and NULL also
   serve as delimiters.                                               */

gaint cmpwrd (char *ch1, char *ch2) {

  while (*ch1==' '||*ch1=='\t') ch1++;  /* Advance past leading blanks.     */
  while (*ch2==' '||*ch2=='\t') ch2++;

  while (*ch1 == *ch2) {
    if (*ch1==' '||*ch1=='\t'||*ch1=='\0'||*ch1=='\n'||*ch1=='\r' ) return (1);
    ch1++; ch2++;
  }

  if ( (*ch1==' '||*ch1=='\t'||*ch1=='\0'||*ch1=='\n'||*ch1=='\r') &&
       (*ch2==' '||*ch2=='\t'||*ch2=='\0'||*ch2=='\n'||*ch2=='\r') ) return (1);
  return (0);
}
/* case insensitive version of cmpwrd  */

gaint cmpwrdl (char *ch1, char *ch2) {
  if(ch1 == NULL || ch2 == NULL) return(0);

  while (*ch1==' '||*ch1=='\t') ch1++;  /* Advance past leading blanks.     */
  while (*ch2==' '||*ch2=='\t') ch2++;

  while (tolower(*ch1) == tolower(*ch2)) {
    if (*ch1==' '||*ch1=='\t'||*ch1=='\0'||*ch1=='\n'||*ch1=='\r' ) return (1);
    ch1++; ch2++;
  }

  if ( (*ch1==' '||*ch1=='\t'||*ch1=='\0'||*ch1=='\n'||*ch1=='\r' ) &&
       (*ch2==' '||*ch2=='\t'||*ch2=='\0'||*ch2=='\n'||*ch2=='\r' ) ) return (1);
  return (0);
}

/* Moves a pointer to the start of the next blank-delimited word
   in a string.  If not found, NULL is returned.                     */

char * nxtwrd (char *ch) {

  while (*ch!=' '&&*ch!='\t') {                     /* Skip 1st word  */
    if (*ch == '\0' || *ch == '\n' || *ch == '\r') return (NULL);
    ch++;
  }
  while (*ch==' '||*ch=='\t') ch++;                 /* Find next word */
  if (*ch == '\0' || *ch == '\n' || *ch == '\r') return (NULL);
  return (ch);
}


/* Linear conversion routine for dimension conversions.               */

gadouble liconv (gadouble *vals, gadouble v) {
  return ( (*vals * v) + *(vals+1) );
}

/* Non-linear scaling routine for discrete levels.  Linear interp
   between levels.  Scaling beyond upper and lower bounds is
   linear based on the last and first grid spacing, respectively.
   In each case a pointer to a list of values is provided.  The
   list contains in its first element the number of values
   in the list.    */

/* Convert a grid value to a world coordinate value.
   This operation needs to be efficient, since it gets done
   very often.  */

gadouble gr2lev (gadouble *vals, gadouble gr) {
gaint i;
  if (gr<1.0) return ( *(vals+1) + (1.0-gr)*(*(vals+1)-*(vals+2)) );
  if (gr>*vals) {
    i = (gaint)(*vals+0.1);
    return ( *(vals+i) + (gr-*vals)*(*(vals+i)-*(vals+i-1)) );
  }
  i = (gaint)gr;
  return (*(vals+i)+((gr-(gadouble)i)*(*(vals+i+1)-*(vals+i))));
}

/* Convert from world coordinate value to grid value.  This operation
   is not set up to be efficient, under the assumption that it won't
   get done all that often.  */

gadouble lev2gr (gadouble *vals, gadouble lev) {
gaint i,num;
gadouble gr;
  num = (gaint)(*vals+0.1);
  for (i=1; i<num; i++) {
    if ( (lev >= *(vals+i) && lev <= *(vals+i+1)) ||
         (lev <= *(vals+i) && lev >= *(vals+i+1)) ) {
      gr = (gadouble)i + (lev - *(vals+i))/(*(vals+i+1) - *(vals+i));
      return (gr);
    }
  }
  if (*(vals+1)<*(vals+num)) {
    if (lev<*(vals+1)) {
      gr = 1.0 + ((lev-*(vals+1))/(*(vals+2)-*(vals+1)));
      return (gr);
    }
    gr = (gadouble)i + ((lev-*(vals+i))/(*(vals+i)-*(vals+i-1)));
    return (gr);
  } else {
    if (lev>*(vals+1)) {
      gr = 1.0 + ((lev-*(vals+1))/(*(vals+2)-*(vals+1)));
      return (gr);
    }
    gr = (gadouble)i + ((lev-*(vals+i))/(*(vals+i)-*(vals+i-1)));
    return (gr);
  }
}

/* Convert from ensemble number to ensemble name */
char *e2ens (struct gafile *pfi, gadouble e) {

  char *name;
  if ((gaint)(e-0.99) < pfi->dnum[4]) {
    name = pfi->ens1[(gaint)(e-0.99)].name;
    return name;
  }
  else return NULL;
}



/* Parses a number in a character string.
   This routine will detect numbers of the form:
       nnnn
       -nnnn

   Args:    ch     - pointer to the number, in character form.
            val    - integer value returned
            return value  - address of 1st character past the
                            number parsed.  NULL if no number found
                            at pointer ch or if the number is an
                            invalid format.
             */

char *intprs (char *ch, gaint *val) {

gaint nflag,flag;

  nflag = 0;
  if (*ch=='-') { nflag = 1; ch++; }
  else if (*ch=='+') ch++;

  *val = 0;
  flag = 1;

  while (*ch>='0' && *ch<='9') {
    *val = *val*10 + (gaint)(*ch-'0');
    flag = 0;
    ch++;
  }

  if (flag) return (NULL);

  if (nflag) *val = -1 * *val;
  return (ch);
}

char *longprs (char *ch, long *val) {

gaint nflag,flag;

  nflag = 0;
  if (*ch=='-') { nflag = 1; ch++; }
  else if (*ch=='+') ch++;

  *val = 0;
  flag = 1;

  while (*ch>='0' && *ch<='9') {
    *val = *val*10 + (gaint)(*ch-'0');
    flag = 0;
    ch++;
  }

  if (flag) return (NULL);

  if (nflag) *val = -1 * *val;
  return (ch);
}


/* Converts strings to double */
char * getdbl(char *ch, gadouble *val) {
  char * pos;
  gadouble res;

  res = strtod(ch, &pos);
  if (pos==ch) {
    return NULL;
  } else {
    *val = res;
    return pos;
  }
}

/* Converts strings to double */
char * getflt(char *ch, gafloat *val) {
char * pos;
  *val = (gafloat)strtod(ch, &pos);
  if (pos==ch) {
    return NULL;
  } else {
    return pos;
  }
}



/* dimprs parses a dimension 'expression', ie, where the user
   specifies an absolute or relative dimension value.
   The format is:

   dim op val

   where:  dim = x,y,z,t,e,lat,lon,lev,time,ens,offt
           op  = +, -, or =
           val = dimension value

   Examples are:
           t=1
           lev=500
           time=00:00z01jan1989

   The coordinates are evaluated with respect to the coordinate
   transformations of the file descriptor passed.  Grid
   coordinates are returned.  Relative offsets are evaluated
   from the values in the status block.

   In addition, r=radius is also supported.  The dimension value
   returned is the radius, and the dimension number returned
   is 10. This is only valid for stn type files.

   wflag is set to 
   0 if the dimension expression was grid coordinates; 
   1 if it was world coordinates; 
   2 if forecast time offsets are used
                                                                */

char *dimprs (char *ch, struct gastat *pst, struct gafile *pfi,
              gaint *dim, gadouble *d, gaint type, gaint *wflag) {
struct dt dtim;
struct gaens *ens;
gadouble (*conv) (gadouble *, gadouble);
gadouble *cvals,v;
/* gadouble g1,g2; */
gaint i,op,len,enum1;
char *pos;
char name[15],ename[20];
  
  /* parse the dimension name */
  i = 0;
  while (*ch>='a' && *ch<='z' && i<6) {
    name[i] = *ch;
    ch++; i++;
  }
  name[i] = '\0';
  if (i>4) {
    gaprnt (0,"Syntax Error:  Invalid dimension expression \n");
    snprintf(pout,1255,"  Expecting x/y/z/t/offt/e/lon/lat/lev/time/ens, found %s\n",name);
    gaprnt (0,pout);
    return (NULL);
  }

  /* parse the operator */
  if      (*ch == '=') op = 0;
  else if (*ch == '+') op = 1;
  else if (*ch == '-') op = 2;
  else {
    gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
    snprintf(pout,1255,"  Expecting +/-/= operator, found %c\n",*ch);
    gaprnt (0,pout);
    return (NULL);
  }

  /* dimension is TIME */
  ch++;
  if (cmpwrd("time",name)) {
    if (op==0) {
      if ((pos=adtprs(ch,&(pst->tmin),&dtim)) == NULL) {
        gaprnt (0,"  Invalid absolute time in dimension expression\n");
        return (NULL);
      }
    } else {
      if ((pos=rdtprs(ch,&dtim)) == NULL) {
        gaprnt (0,"  Invalid relative time in dimension expression\n");
	return (NULL);
      }
    }
  } 
  /* dimension is ENS */
  else if (cmpwrd("ens",name)) {
    /* parse the ensemble name */
    pos = ch;
    len=0;
    while (len<16 && *pos!=')' ) {
      ename[len] = *pos;
      len++; 
      pos++;
    }
    ename[len] = '\0';
  }
  /* all other dimensions */
  else {
    if ((pos=getdbl(ch,&v)) == NULL) {
      gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
      gaprnt (0,"  Dimension value missing or invalid\n");
      return (NULL);
    }
  }
  ch = pos;

  /* We now have all the info we need about this dimension expression to evaluate it.  */
  if      (cmpwrd("x",name))    *dim = 0;
  else if (cmpwrd("y",name))    *dim = 1;
  else if (cmpwrd("z",name))    *dim = 2;
  else if (cmpwrd("t",name))    *dim = 3;
  else if (cmpwrd("offt",name)) *dim = 3;
  else if (cmpwrd("e",name))    *dim = 4;
  else if (cmpwrd("lon",name))     *dim = 5;
  else if (cmpwrd("lat",name))     *dim = 6;
  else if (cmpwrd("lev",name))     *dim = 7;
  else if (cmpwrd("time",name))    *dim = 8;
  else if (cmpwrd("ens",name))     *dim = 9;
  else if (type==0 && cmpwrd("r",name)) *dim = 10;
  else {
    gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
    snprintf(pout,1255,"  Expecting x/y/z/t/offt/e/lat/lon/lev/time/ens, found %s\n",name);
    gaprnt (0,pout);
    return (NULL);
  }

  /* for station expressions */
  if (*dim==10) {
    *d = v;
    return (ch);
  }

  /* dimension expression is given in grid coordinates: x, y, z, t, offt, or e */
  *wflag = 0;          
  if (*dim < 5) {  
    if (cmpwrd("offt",name)) *wflag=2;     /* trip the time offset flag */
    if (op==0) {
      *d = v + pfi->dimoff[*dim];          /* straight override of fixed dimension value */
      return (ch);
    } else {
      /* make sure the dimension is not varying */
      if (*dim==pst->idim || *dim==pst->jdim) {
	gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
	gaprnt (0,"  Cannot use an offset value with a varying dimension\n");
	snprintf(pout,1255,"  Varying dimension = %i \n",*dim);
	gaprnt (0,pout);
	return (NULL);
      }
      /* get current dimension value in grid coordinates from gastat structure */
      if (*dim == 3) {
        *d = t2gr(pfi->abvals[3],&(pst->tmin));  
      } 
      else {
        if (pfi->type==1 || pfi->type==4) { 
          conv = pfi->ab2gr[*dim];
          cvals = pfi->abvals[*dim];
          *d = conv(cvals,pst->dmin[*dim]);
        } else {
          *d = pst->dmin[*dim];
        }
      }
      /* combine offset with current dimension value */
      if (op==1) *d = *d + v;
      if (op==2) *d = *d - v;
      return (ch);
    }
  } 
  /* dimension expression is given in world coordinates: lon, lat, lev, time, or ens */
  else {                        
    *dim = *dim - 5;
    *wflag = 1;          
/*     if (cmpwrd("offtime",name)) { */
/*       /\* determine the size of the time offset in grid units *\/ */
/*       g1 = t2gr(pfi->abvals[3],&(pst->tmin)); */
/*       timadd (&(pst->tmin),&dtim); */
/*       g2 = t2gr(pfi->abvals[3],&dtim); */
/*       v = g2 - g1; */
/*       *wflag=2;      /\* trip the time offset flag *\/ */
/*     } */
    if (op>0) {
      /* check to make sure dimension isn't varying */
      if (*dim==pst->idim || *dim==pst->jdim) {
        gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
        gaprnt (0,"  Cannot use an offset value with a varying dimension\n");
        snprintf(pout,1255,"  Varying dimension = %i \n",*dim);
        gaprnt (0,pout);
        return (NULL);
      }
      /* check to make sure dimension isn't E */
      if (*dim==4) {
	gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
	gaprnt (0,"  Cannot use an offset value with an ensemble name\n");
	return (NULL);
      } 
      /* combine offset with current dimension value from gastat structure */
      if (*dim==3) {
        if (op==1) timadd (&(pst->tmin),&dtim);
        if (op==2) timsub (&(pst->tmin),&dtim);
      } 
      else {
        if (op==1) v = pst->dmin[*dim] + v;
        if (op==2) v = pst->dmin[*dim] - v;
      }
    }

    if (*dim == 4) {
      /* loop over ensembles, looking for matching name */
      ens = pfi->ens1;
      i=0;
      enum1=-1;
      while (i<pfi->dnum[*dim]) {
	if (strcmp(ename,ens->name) == 0) enum1=i;  /* grid coordinate of matching name */
	i++; ens++;
      }
      if (enum1<0) {
	gaprnt (0,"Syntax Error:  Invalid dimension expression\n");
	snprintf(pout,1255,"  Ensemble name \"%s\" not found\n",ename);
	gaprnt (0,pout);
	return (NULL);
      }
      /* straight override of ensemble grid coordinate */
      *d = enum1 + 1 + pfi->dimoff[*dim];
      return (ch);
    } 
    /* get the grid coordinate for the new (combined) dimension value */
    else if (*dim == 3) {
      *d = t2gr(pfi->abvals[3],&dtim);
    } else {
      if (pfi->type==1 || pfi->type==4) {  /* grids  */
        conv = pfi->ab2gr[*dim];
        cvals = pfi->abvals[*dim];
        *d = conv(cvals,v);
      } else {
        *d = v;                            /* station data */
      }
    }
    return (ch);
  }
}

/*mf version
  convert all upper case alphabetic characters to lower case.
  The GrADS system is case insensitive, and assumes lower case
  internally in most cases. Does not turn to lower case if in "'s
*/
void lowcas (char *ch) {
gaint i;
gaint qflag=0;

  while (*ch!='\0' && *ch!='\n') {
    i = *ch;
    if(*ch == '\"' && qflag == 0 ) {
      qflag=1;
      } else if(*ch == '\"' && qflag == 1 ) {
	qflag=0;
      }
    if (i>64 && i<91 && qflag==0) {
      i+=32;
      *ch = i;
    } else if(i == 95) {
      *ch=i;
    }
    ch++;
  }
}

/* convert to upper case */
void uppcas (char *ch) {
gaint i;

  while (*ch!='\0' && *ch!='\n') {
    i = *ch;
    if (i>96 && i<123) {
      i -= 32;
      *ch = i;
    }
    ch++;
  }
}

/* Copies a string of a specified length, or when \0 or \n is hit.
   Trailing blanks are removed, and the output string is terminated
   with '\0'.                                                         */

void getstr (char *ch1, char *ch2, gaint len) {
char *ch;

  ch = ch1;
  while (len>0 && *ch2!='\n' && *ch2!='\0') {
    *ch1 = *ch2;
    len--;
    ch1++;  ch2++;
  }
  ch1--;
  while (ch1>=ch && *ch1==' ') ch1--;
  ch1++;
  *ch1 = '\0';
}

/* Copies a word of a specified length, or when \0 or \n or \r or ' ' is
   encountered.  The word is terminated with '\0'. ch2 is src, ch1 is dest */

void getwrd (char *ch1, char *ch2, gaint len) {
char *ch;

  ch = ch1;
  while (len>0 && *ch2!='\n' && *ch2!='\0' && *ch2!='\r' && *ch2!=' ' ) {
    *ch1 = *ch2;
    len--;
    ch1++;  ch2++;
  }
  *ch1 = '\0';
}

/* Determines word length up to next delimiter */

gaint wrdlen (char *ch2) {
gaint len;
  len = 0;
  while (*ch2!='\n' && *ch2!='\0' && *ch2!=' ' && *ch2!='\t') {
    len++;
    ch2++;
  }
  return(len);
}

/* Get minimum and maximum grid value.  Set rmin and rmax in the
   grid descriptor.                                                  */

void gamnmx (struct gagrid *pgr) {
gadouble *r;
gaint i,size,cnt;
char *rmask;

  size = pgr->isiz * pgr->jsiz;
  if (size==1) return;
  pgr->rmin=  9.99E35;
  pgr->rmax= -9.99E35;
  r     = pgr->grid;
  rmask = pgr->umask; 
  cnt=0;
  for (i=0;i<size;i++) {
    if (*rmask == 1) {
      cnt++;
      if (pgr->rmin>*r) {
	pgr->rmin = *r;
      }
      if (pgr->rmax<*r) pgr->rmax = *r;
    }
    r++; rmask++;
  }
  if (cnt==0 || pgr->rmin==9.99e35 || pgr->rmax==-9.99e35) {
    pgr->rmin = pgr->undef;
    pgr->rmax = pgr->undef;
    pgr->umin = pgr->umax = 0;
  }
  else {
    pgr->umin = pgr->umax = 1;
  }
}

/*  Determine min and max values of station data */

void gasmnmx (struct gastn *stn) {
struct garpt *rpt;

  stn->smin = stn->undef;
  stn->smax = stn->undef;
  rpt = stn->rpt;
  while (rpt!=NULL) {
    if (rpt->umask == 1) {
      if (stn->smin == stn->undef) {
	stn->smin = rpt->val;
      }
      else {
	if (stn->smin > rpt->val) {
	  stn->smin = rpt->val;
	}
      }
      if (stn->smax == stn->undef) {
	stn->smax = rpt->val;
      }
      else {
	if (stn->smax < rpt->val) {
	  stn->smax = rpt->val;
	}
      }
    }
    rpt = rpt->rpt;
  }
}

/* Remove blanks from a string */
gaint garemb (char *ch) {
char *cc;
gaint cnt;
gaint qflag=0;

  cc = ch;
  cnt = 0;

  while (*ch!='\n' && *ch!='\0') {
    /* do not remove blanks if string in quotes */
    if (*ch == '\"' && qflag == 0) {
      qflag=1;
    } else if (*ch == '\"' && qflag == 1) {
      qflag=0;
    }

    if (((*ch!=' ') || qflag ) && (*ch!='\"')) {
      *cc = *ch;
      cc++; cnt++;
    }
    ch++;
  }
  *cc = '\0';
  return (cnt);
}

static gadouble glts15[40] = {
       -86.60,-82.19,-77.76,-73.32,-68.88,-64.43,-59.99,
       -55.55,-51.11,-46.66,-42.22,-37.77,-33.33,-28.89,
       -24.44,-20.00,-15.55,-11.11, -6.67, -2.22,  2.22,
         6.67, 11.11, 15.55, 20.00, 24.44, 28.89, 33.33,
        37.77, 42.22, 46.66, 51.11, 55.55, 59.99, 64.43,
        68.88, 73.32, 77.76, 82.19, 86.60};
static gadouble glts20[52] = {
       -87.38,-83.98,-80.56,-77.13,-73.71,-70.28,-66.85,
       -63.42,-59.99,-56.57,-53.14,-49.71,-46.28,-42.85,
       -39.43,-36.00,-32.57,-29.14,-25.71,-22.28,-18.86,
       -15.43,-12.00, -8.57, -5.14, -1.71,  1.71,  5.14,
         8.57, 12.00, 15.43, 18.86, 22.28, 25.71, 29.14,
        32.57, 36.00, 39.43, 42.85, 46.28, 49.71, 53.14,
        56.57, 59.99, 63.42, 66.85, 70.28, 73.71, 77.13,
        80.56, 83.98, 87.38};
static gadouble glts30[80] = {
       -88.29, -86.07, -83.84, -81.61, -79.37, -77.14, -74.90,
       -72.67, -70.43, -68.20, -65.96, -63.72, -61.49, -59.25,
       -57.02, -54.78, -52.55, -50.31, -48.07, -45.84, -43.60,
       -41.37, -39.13, -36.89, -34.66, -32.42, -30.19, -27.95,
       -25.71, -23.48, -21.24, -19.01, -16.77, -14.53, -12.30,
       -10.06,  -7.83,  -5.59,  -3.35,  -1.12,   1.12,   3.35,
         5.59,   7.83,  10.06,  12.30,  14.53,  16.77,  19.01,
        21.24,  23.48,  25.71,  27.95,  30.19,  32.42,  34.66,
        36.89,  39.13,  41.37,  43.60,  45.84,  48.07,  50.31,
        52.55,  54.78,  57.02,  59.25,  61.49,  63.72,  65.96,
        68.20,  70.43,  72.67,  74.90,  77.14,  79.37,  81.61,
        83.84,  86.07,  88.29};

static gadouble glats[102] = {
 -88.66,-86.91,-85.16,-83.41,-81.65,-79.90,-78.14,-76.39,-74.63,
 -72.88,-71.12,-69.36,-67.61,-65.85,-64.10,-62.34,-60.58,-58.83,
 -57.07,-55.32,-53.56,-51.80,-50.05,-48.29,-46.54,-44.78,-43.02,
 -41.27,-39.51,-37.76,-36.00,-34.24,-32.49,-30.73,-28.98,-27.22,
 -25.46,-23.71,-21.95,-20.19,-18.44,-16.68,-14.93,-13.17,-11.41,
  -9.66, -7.90, -6.15, -4.39, -2.63, -0.88,  0.88,  2.63,  4.39,
   6.15,  7.90,  9.66, 11.41, 13.17, 14.93, 16.68, 18.44, 20.19,
  21.95, 23.71, 25.46, 27.22, 28.98, 30.73, 32.49, 34.24, 36.00,
  37.76, 39.51, 41.27, 43.02, 44.78, 46.54, 48.29, 50.05, 51.80,
  53.56, 55.32, 57.07, 58.83, 60.58, 62.34, 64.10, 65.85, 67.61,
  69.36, 71.12, 72.88, 74.63, 76.39, 78.14, 79.90, 81.65, 83.41,
  85.16, 86.91, 88.66 };

static gadouble m32lts[32] = {-20.453, -18.01, -15.763, -13.738,
  -11.95, -10.405, -9.097, -8.010, -7.120, -6.392, -5.253, -4.25,
  -3.25, -2.25, -1.25, -0.25, 0.25, 1.25, 2.25, 3.25, 4.25, 5.253,
  6.392, 7.12, 8.01, 9.097, 10.405, 11.95, 13.738, 15.763, 18.01,
  20.453};

/* From Mike Timlin */
static gadouble gltst62[94] = {
        -88.542, -86.6531, -84.7532, -82.8508, -80.9473, -79.0435, 
        -77.1394, -75.2351, -73.3307, -71.4262, -69.5217, -67.6171, 
        -65.7125, -63.8079, -61.9033, -59.9986, -58.0939, -56.1893, 
        -54.2846, -52.3799, -50.4752, -48.5705, -46.6658, -44.7611, 
        -42.8564, -40.9517, -39.047, -37.1422, -35.2375, -33.3328, 
        -31.4281, -29.5234, -27.6186, -25.7139, -23.8092, -21.9044, 
        -19.9997, -18.095, -16.1902, -14.2855, -12.3808, -10.47604, 
        -8.57131, -6.66657, -4.76184, -2.8571, -0.952368, 0.952368, 
        2.8571, 4.76184, 6.66657, 8.57131, 10.47604, 12.3808, 14.2855, 
        16.1902, 18.095, 19.9997, 21.9044, 23.8092, 25.7139, 27.6186, 
        29.5234, 31.4281, 33.3328, 35.2375, 37.1422, 39.047, 40.9517, 
        42.8564, 44.7611, 46.6658, 48.5705, 50.4752, 52.3799, 54.2846, 
        56.1893, 58.0939, 59.9986, 61.9033, 63.8079, 65.7125, 67.6171, 
        69.5217, 71.4262, 73.3307, 75.2351, 77.1394, 79.0435, 80.9473, 
        82.8508, 84.7532, 86.6531, 88.542 };

/* Given the starting point and the length, return the MOM32 lats */

gadouble *gamo32 (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 32) {
    gaprnt (0,"Open Error: Invalid MOM32 scaling.\n");
    gaprnt (0,"  Maximum 32 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gamo32");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: MOM32 Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = m32lts[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}


/* Given the starting point and the length, return the gaussian lats */

gadouble *gagaus (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 102) {
    gaprnt (0,"Open Error: Invalid GAUSR40 scaling.\n");
    gaprnt (0,"  Maximum 102 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gagaus");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: Gaussian Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = glats[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}

/* Given the starting point and the length, return the gaussian lats
  for R30 grids */

gadouble *gags30 (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 80) {
    gaprnt (0,"Open Error: Invalid GAUSR30 scaling.\n");
    gaprnt (0,"  Maximum 80 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gags30");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: Gaussian Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = glts30[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}

/* Given the starting point and the length, return the gaussian lats
  for R20 grids */

gadouble *gags20 (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 52) {
    gaprnt (0,"Open Error: Invalid GAUSR20 scaling.\n");
    gaprnt (0,"  Maximum 52 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gags20");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: Gaussian Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = glts20[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}

/* Given the starting point and the length, return the gaussian lats
  for R15 grids */

gadouble *gags15 (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 40) {
    gaprnt (0,"Open Error: Invalid GAUSR15 scaling.\n");
    gaprnt (0,"  Maximum 40 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gags15");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: Gaussian Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = glts15[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}

/* Given the starting point and the length, return the gaussian lats
  for T62 grids */ 
/* From Mike Timlin */

gadouble *gagst62 (gaint istrt, gaint num) {
gaint size;
gadouble *vals;
size_t sz;

  istrt--;
  if (istrt+num > 94) {
    gaprnt (0,"Open Error: Invalid GAUST62 scaling.\n");
    gaprnt (0,"  Maximum 94 latitudes exceeded \n");
    return (NULL);
  }
  sz = (num+3) * sizeof(gadouble);
  vals = (gadouble *)galloc(sz,"gagst62");
  if (vals==NULL) {
    gaprnt (0,"Memory Allocation Error: Gaussian Grid Scaling\n");
    return (NULL);
  }
  *vals = (gadouble)num;
  for (size=0; size<num; size++) *(vals+size+1) = gltst62[size+istrt];
  *(vals+num+1) = -999.9;
  return (vals);
}

char *monc[12] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG",
                  "SEP","OCT","NOV","DEC"};

gaint gat2ch (struct dt *dtim, gaint tinc, char *ch, gaint chlen) {
gaint mn1,mn2,hr1,hr2,dy1,dy2,len,mnth;

  mnth = dtim->mo - 1L;
  mn1 = dtim->mn/10L;
  mn2 = dtim->mn - (mn1*10);
  hr1 = dtim->hr/10L;
  hr2 = dtim->hr - (hr1*10);
  dy1 = dtim->dy/10L;
  dy2 = dtim->dy - (dy1*10);
  if (tinc==1) {
    snprintf(ch,chlen-1,"%04i",dtim->yr);
  }
  else if (tinc==2) {
    if (dtim->yr==9999L) {
      snprintf(ch,chlen-1,"%s",monc[mnth]);
    } else {
      snprintf(ch,chlen-1,"%s%04i",monc[mnth],dtim->yr);
    }
  }
  else if (tinc==3) {
    snprintf(ch,chlen-1,"%i%i%s%04i",dy1,dy2,monc[mnth],dtim->yr);
  }
  else if (tinc==4) {
    snprintf(ch,chlen-1,"%i%iZ%i%i%s%04i",hr1,hr2,dy1,dy2,monc[mnth],dtim->yr);
  }
  else if (tinc==5) {
    snprintf(ch,chlen-1,"%i%i:%i%iZ%i%i%s%04i",hr1,hr2,mn1,mn2,dy1,dy2,monc[mnth],dtim->yr);
  }
  else snprintf(ch,chlen-1,"???");
  len=0;
  while (ch[len]) len++;
  return (len);
}

/* Compare two strings given the length.  */
/* Return 0 if the string match, otherwise 1.  */

gaint cmpch (char *str1, char *str2, gaint len) {

  while (len>0) {
    len--;
    if (*(str1+len) != *(str2+len)) return (1);
  }
  return (0);
}

/* Free anything hung off a gastat block */

void gafree (struct gastat *pst) {

  if (pst->type == 1) {
    gagfre (pst->result.pgr);
    pst->result.pgr=NULL;
  } else {
    gasfre (pst->result.stn);
    pst->result.stn=NULL;
  }
}

void gagfre (struct gagrid *pgr) {

  if (pgr==NULL) return;
  if (pgr->alocf) {
    if (pgr->ivals != NULL) gree(pgr->ivals,"f88");
    if (pgr->jvals != NULL) gree(pgr->jvals,"f89");
  }
  if (pgr->idim>-1) {
    gree(pgr->grid,"f90");
    gree(pgr->umask,"f91");
  }
  gree(pgr,"f92");
}

void gasfre (struct gastn *stn) {
gaint i;
  if (stn==NULL) return;
  if (stn->tvals) {
    gree(stn->tvals,"f237");
  }
  if (stn->rpt) {
    for (i=0; i<BLKNUM; i++) {
      if (stn->blks[i] != NULL) gree(stn->blks[i],"f238");
    }
  }
  gree(stn,"f239");
}


/* Expand file names prefixed with '^' from data descriptor
   files */

void fnmexp (char *out, char *in1, char *in2) {
char *pos, *ch, envv[20], *envr, CR=13;
gaint i,j;

  if (*in1=='$') {
    in1++;
    i = 0;
    while (*in1!='/' && *in1!='\0' && i<16) {
      envv[i] = *in1;
      i++; in1++;
    }
    envv[i] = '\0';
    envr = gxgsym(envv);
    if (envr) {
      i = 0; j = 0;
      while (*(envr+j)) {
        *(out+i) = *(envr+j);
        i++; j++;
      }
      /* handle CR for descriptor files created under MS Windows */
      while (*in1!='\0' && *in1!=' ' && *in1!='\n' && *in1!=CR) {
        *(out+i) = *in1;
        i++; in1++;
      }
      *(out+i) = '\0';
    }
    return;
  }
  ch = in2;
  pos=NULL;
  while (*ch!='\0' && *ch!=' ' && *ch!='\n') {
    if (*ch=='/') pos=ch;
    ch++;
  }
  if (pos) pos++;
  while (pos!=NULL && in2<pos) {
    *out = *in2;
    out++; in2++;
  }
  in1++;
  while (*in1!='\0' && *in1!=' ' && *in1!='\n' && *in1!=CR) {
    *out = *in1;
    out++; in1++;
  }
  *out = '\0';
}

/* Given a file name template and a dt structure, fill in to get the file name */

char *gafndt (char *fn, struct dt *dtim, struct dt *dtimi, gadouble *vals, 
	      struct gachsub *pch1st, struct gaens *ens1st, gaint t, gaint e, gaint *flag) {
struct gachsub *pchsub;
struct gaens *ens;
struct dt stim;
gaint len,olen,iv,tdif,i,tused,eused,mo,doy,dys,hrs,mns;
char *fnout, *in, *out, *work, *in2, *out2, *newfn;
size_t sz;

  tused = eused = 0;
  olen = 0;
  while (*(fn+olen)) olen++;              /* get length of original DSET entry */
  olen+=5;
  sz = olen;
  newfn = (char *)galloc(sz+1,"newfn");  /* allocate memory for new DSET with chsubs */
  if (newfn==NULL) return (NULL);

  in = fn;         /* original dset entry */
  out = newfn;     /* output string */

  /* Do the string substitution first */
  while (*in) {
    pchsub = pch1st;
    if (*in=='%' && *(in+1)=='c' && *(in+2)=='h') {
      /* we have a string to substitute */
      tused=1;
      while (pchsub) {
        if (t>=pchsub->t1 && (pchsub->t2 == -99 || t<=pchsub->t2) ) {
          len = wrdlen(pchsub->ch);           /* get length of substitution string */
          olen += len;
	  sz = olen;
          work = (char *)galloc(sz+1,"work");  /* allocate memory for new chsubbed string */
          if (work==NULL) { gree(newfn,"f240"); return (NULL); }

	  out2 = work;                        
          in2 = newfn;                       
          while (in2!=out) {
            *out2 = *in2;  /* copy chars from new DSET that occur before this %ch into new work string */
            in2++; out2++;
          }
          gree(newfn,"f241");      
          newfn = work;               /* replace newfn */
          out = out2;                 /* replace output string */
          getwrd(out,pchsub->ch,len); /* copy substitution string to output string */
          out += len;
          break;
        }
        pchsub = pchsub->forw;        
      }
      in+=3;
    }
    else {
      /* just copy this character from original DSET entry to output string */
      *out = *in;    
      in++; out++;
    }
  }
  *out = '\0';

  /* Now begin again with new chsubbed dset entry and do all the time template substitutions */
  in = newfn;
  olen = 0;
  while (*(newfn+olen)) olen++;
  olen+=5;
  sz = olen;
  fnout = (char *)galloc(sz+1,"fnout");
  if (fnout==NULL) { gree(newfn,"f240a"); return (NULL); }
  out = fnout;  /* output string */

  while (*in) {
    pchsub = pch1st;
    ens = ens1st;
    /* handle template strings for initial time */
    if (*in=='%' && *(in+1)=='i') {
      tused=1;
      if (*(in+2)=='x' && *(in+3)=='1') { 
        snprintf(out,sz,"%i",dtimi->yr/10);
        while (*out) out++;
        in+=4;
      } else if (*(in+2)=='x' && *(in+3)=='3') {
        snprintf(out,sz,"%03i",dtimi->yr/10);
        out+=3; in+=4;
      } else if (*(in+2)=='y' && *(in+3)=='2') {
        iv = dtimi->yr/100;
        iv = dtimi->yr - iv*100;
        snprintf(out,sz,"%02i",iv);
        out+=2;  in+=4;
      } else if (*(in+2)=='y' && *(in+3)=='4') {
        snprintf(out,sz,"%04i",dtimi->yr);
        out+=4;  in+=4;
      } else if (*(in+2)=='m' && *(in+3)=='1') {
          snprintf(out,sz,"%i",dtimi->mo);
        while (*out) out++;
        in+=4;
      } else if (*(in+2)=='m' && *(in+3)=='2') {
        snprintf(out,sz,"%02i",dtimi->mo);
        out+=2;  in+=4;
      } else if (*(in+2)=='m' && *(in+3)=='h') {
        if (dtimi->dy < 16) *out='a';
        else *out = 'b';
        out+=1;  in+=4;
      } else if (*(in+2)=='m' && *(in+3)=='H') {
        if (dtimi->dy < 16) *out='A';
        else *out = 'B';
        out+=1;  in+=4;
      } else if (*(in+2)=='m' && *(in+3)=='c') {
        *out = *(mons[dtimi->mo-1]);
        *(out+1) = *(mons[dtimi->mo-1]+1);
        *(out+2) = *(mons[dtimi->mo-1]+2);

        out+=3;  in+=4;
      } else if (*(in+2)=='d' && *(in+3)=='1') {
        snprintf(out,sz,"%i",dtimi->dy);
        while (*out) out++;
        in+=4;
      } else if (*(in+2)=='d' && *(in+3)=='2') {
        snprintf(out,sz,"%02i",dtimi->dy);
        out+=2;  in+=4;
      } else if (*(in+2)=='h' && *(in+3)=='1') {
        snprintf(out,sz,"%i",dtimi->hr);
        while (*out) out++;
        in+=4;
      } else if (*(in+2)=='h' && *(in+3)=='2') {
        snprintf(out,sz,"%02i",dtimi->hr);
        out+=2;  in+=4;
      } else if (*(in+2)=='h' && *(in+3)=='3') {
        snprintf(out,sz,"%03i",dtimi->hr);
        out+=3;  in+=4;
      } else if (*(in+2)=='n' && *(in+3)=='2') {
        snprintf(out,sz,"%02i",dtimi->mn);
        out+=2;  in+=4;
      } else if (*(in+2)=='j' && *(in+3)=='3') {
	doy = dtimi->dy;
	mo = dtimi->mo-1;
	while (mo>0) {
	  doy += mosiz[mo];
	  if (mo==2 && qleap(dtimi->yr)) doy+=1;
	  mo--;
	}
	snprintf(out,sz,"%03i",doy);
	out+=3;  in+=4;
      } else {
        *out = *in;
        in++; out++;
      }
    } 
    /* handle template strings for any time */
    else if (*in=='%' && *(in+1)=='x' && *(in+2)=='1') {   /* decade */
      tused=1;
      snprintf(out,sz,"%i",dtim->yr/10);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='x' && *(in+2)=='3') { 
      tused=1;
      snprintf(out,sz,"%03i",dtim->yr/10);
      out+=3; in+=3;
    } else if (*in=='%' && *(in+1)=='y' && *(in+2)=='2') {   /* year */
      tused=1;
      iv = dtim->yr/100;
      iv = dtim->yr - iv*100;
      snprintf(out,sz,"%02i",iv);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='y' && *(in+2)=='4') {
      tused=1;
      snprintf(out,sz,"%04i",dtim->yr);
      out+=4;  in+=3;
    } else if (*in=='%' && *(in+1)=='m' && *(in+2)=='1') {   /* month */
      tused=1;
      snprintf(out,sz,"%i",dtim->mo);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='m' && *(in+2)=='2') {
      tused=1;
      snprintf(out,sz,"%02i",dtim->mo);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='m' && *(in+2)=='h') {
      tused=1;
      if (dtim->dy < 16) *out='a';
      else *out = 'b';
      out+=1;  in+=3;
    } else if (*in=='%' && *(in+1)=='m' && *(in+2)=='H') {
      tused=1;
      if (dtim->dy < 16) *out='A';
      else *out = 'B';
      out+=1;  in+=3;
    } else if (*in=='%' && *(in+1)=='m' && *(in+2)=='c') {
      tused=1;
      *out = *(mons[dtim->mo-1]);
      *(out+1) = *(mons[dtim->mo-1]+1);
      *(out+2) = *(mons[dtim->mo-1]+2);
      out+=3;  in+=3;
    } else if (*in=='%' && *(in+1)=='d' && *(in+2)=='1') {   /* day */
      tused=1;
      snprintf(out,sz,"%i",dtim->dy);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='d' && *(in+2)=='2') {
      tused=1;
      snprintf(out,sz,"%02i",dtim->dy);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='h' && *(in+2)=='1') {   /* hour */
      tused=1;
      snprintf(out,sz,"%i",dtim->hr);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='h' && *(in+2)=='2') {
      tused=1;
      snprintf(out,sz,"%02i",dtim->hr);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='h' && *(in+2)=='3') {
      tused=1;
      snprintf(out,sz,"%03i",dtim->hr);
      out+=3;  in+=3;
    } else if (*in=='%' && *(in+1)=='n' && *(in+2)=='2') {   /* minute */
      tused=1;
      snprintf(out,sz,"%02i",dtim->mn);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='j' && *(in+2)=='3') {   /* julian day */
      tused=1;
      doy = dtim->dy;
      mo = dtim->mo-1;
      while (mo>0) {
	doy += mosiz[mo];
	if (mo==2 && qleap(dtim->yr)) doy+=1;
	mo--;
      }
      snprintf(out,sz,"%03i",doy);
      out+=3;  in+=3;
    } 
    /* time index t, starting with 1 */
    else if (*in=='%' && *(in+1)=='t' && *(in+2)=='1') {   
      tused=1;
      snprintf(out,sz,"%i",t);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='2') {
      tused=1;
      snprintf(out,sz,"%02i",t);
      out+=2;  in+=3;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='3') {
      tused=1;
      snprintf(out,sz,"%03i",t);
      out+=3;  in+=3;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='4') {
      tused=1;
      snprintf(out,sz,"%04i",t);
      out+=4;  in+=3;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='5') {
      tused=1;
      snprintf(out,sz,"%05i",t);
      out+=5;  in+=3;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='6') {
      tused=1;
      snprintf(out,sz,"%06i",t);
      out+=6;  in+=3;
    } 
    /* time index t, starting with 0 */
    else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='1') {   
      tused=1;
      snprintf(out,sz,"%i",t-1);
      while (*out) out++;
      in+=4;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='2') {
      tused=1;
      snprintf(out,sz,"%02i",t-1);
      out+=2;  in+=4;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='3') {
      tused=1;
      snprintf(out,sz,"%03i",t-1);
      out+=3;  in+=4;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='4') {
      tused=1;
      snprintf(out,sz,"%04i",t-1);
      out+=4;  in+=4;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='5') {
      tused=1;
      snprintf(out,sz,"%05i",t-1);
      out+=5;  in+=4;
    } else if (*in=='%' && *(in+1)=='t' && *(in+2)=='m' && *(in+3)=='6') {
      tused=1;
      snprintf(out,sz,"%06i",t-1);
      out+=6;  in+=4;
    }
    /* forecast hour */
    else if (*in=='%' && *(in+1)=='f' && *(in+2)=='2') {
      tused=1;
      stim.yr = (gaint)(*vals+0.1);
      stim.mo = (gaint)(*(vals+1)+0.1);
      stim.dy = (gaint)(*(vals+2)+0.1);
      stim.hr = (gaint)(*(vals+3)+0.1);
      stim.mn = (gaint)(*(vals+4)+0.1);
      tdif = timdif(dtimi,dtim,0);
      /* tdif = (tdif+30)/60; */
      tdif = tdif/60;   /* forecast hour not rounded up anymore */
      if (tdif<99) snprintf(out,sz,"%02i",tdif);
      else snprintf(out,sz,"%i",tdif);
      while (*out) out++;
      in+=3;
    } else if (*in=='%' && *(in+1)=='f' && *(in+2)=='3') {
      tused=1;
      stim.yr = (gaint)(*vals+0.1);
      stim.mo = (gaint)(*(vals+1)+0.1);
      stim.dy = (gaint)(*(vals+2)+0.1);
      stim.hr = (gaint)(*(vals+3)+0.1);
      stim.mn = (gaint)(*(vals+4)+0.1);
      tdif = timdif(dtimi,dtim,0); 
      /* tdif = (tdif+30)/60; */
      tdif = tdif/60;   /* forecast hour not rounded up anymore */
      if (tdif<999) snprintf(out,sz,"%03i",tdif);
      else snprintf(out,sz,"%i",tdif);
      while (*out) out++;
      in+=3;
    } 
    /* forecast minute */
    else if (*in=='%' && *(in+1)=='f' && *(in+2)=='n' && *(in+3)=='2') {
      tused=1;
      stim.yr = (gaint)(*vals+0.1);
      stim.mo = (gaint)(*(vals+1)+0.1);
      stim.dy = (gaint)(*(vals+2)+0.1);
      stim.hr = (gaint)(*(vals+3)+0.1);
      stim.mn = (gaint)(*(vals+4)+0.1);
      tdif = timdif(dtimi,dtim,0); 
      if (tdif<99) snprintf(out,sz,"%02i",tdif);
      else snprintf(out,sz,"%i",tdif);
      while (*out) out++;
      in+=4;
    } 
    /* forecast time in hours/minutes (hhnn) */ 
    else if (*in=='%' && *(in+1)=='f' && *(in+2)=='h' && *(in+3)=='n') {
      tused=1;
      stim.yr = (gaint)(*vals+0.1);
      stim.mo = (gaint)(*(vals+1)+0.1);
      stim.dy = (gaint)(*(vals+2)+0.1);
      stim.hr = (gaint)(*(vals+3)+0.1);
      stim.mn = (gaint)(*(vals+4)+0.1);
      tdif = timdif(dtimi,dtim,0); 
      hrs = tdif/60;
      mns = tdif - (hrs*60);
      if (hrs<99) snprintf(out,sz,"%02i%02i",hrs,mns);
      else snprintf(out,sz,"%i%02i",hrs,mns);
      while (*out) out++;
      in+=4;
    } 
    /* forecast time in days/hours/minutes (ddhhnn) */ 
    else if (*in=='%' && *(in+1)=='f' && *(in+2)=='d' && *(in+3)=='h' && *(in+4)=='n') {
      tused=1;
      stim.yr = (gaint)(*vals+0.1);
      stim.mo = (gaint)(*(vals+1)+0.1);
      stim.dy = (gaint)(*(vals+2)+0.1);
      stim.hr = (gaint)(*(vals+3)+0.1);
      stim.mn = (gaint)(*(vals+4)+0.1);
      tdif = timdif(dtimi,dtim,0); 
      dys = tdif/1440;
      hrs = (tdif - (dys*1440))/60;
      mns = tdif - (dys*1440) - (hrs*60);
      if (dys<99) snprintf(out,sz,"%02i%02i%02i",dys,hrs,mns);
      else snprintf(out,sz,"%i%02i%02i",dys,hrs,mns);
      while (*out) out++;
      in+=5;
    } 
    /* ensemble name substitution */
    else if  (*in=='%' && *(in+1)=='e') {
      eused=1;
      if (ens == NULL) {
	gree(fnout,"f242");
        gree(newfn,"f240b");
	return (NULL);
      } else {
	/* advance through array of ensemble structures, till we reach ensemble 'e' */
	i=1;
	while (i!=e) { i++; ens++; }
	len = strlen(ens->name);
	if (len < 1) {
	  gree(fnout,"f243");
          gree(newfn,"f240c");
	  return (NULL);
	}
	olen += len;
	sz = olen;
	work = (char *)galloc(sz+1,"work2");     /* Reallocate output string */
	if (work==NULL) {
	  gree(fnout,"f244");
          gree(newfn,"f240d");
	  return (NULL);
	}
	in2 = fnout;            /* copy the string we've got so far */
	out2 = work;
	while (in2!=out) {
	  *out2 = *in2;
	  in2++; out2++;
	}
	gree(fnout,"f245");
	fnout = work;
	out = out2;
	getwrd(out,ens->name,len);
	out += len;
      }
      in+=2;
    }
    else {
      *out = *in;
      in++; out++;
    }
  }
  *out = '\0';
  if (eused==1 && tused==1) {
    *flag = 3;                       /* templating on E and T */
  } 
  else if (eused==1 && tused==0) {
    *flag = 2;                       /* templating only on E */
  }
  else if (eused==0 && tused==1) { 
    *flag = 1;                       /* templating only on T */
  }
  else {
    *flag = 0;                       /* no templating */
  }
  gree(newfn,"f240e");
  return (fnout);
}

/* Byte swap requested number of 2 byte elements */

void gabswp2 (void *r, gaint cnt) {
gaint i;
char *ch1,*ch2,cc1,cc2;

  ch1 = (char *)r;
  ch2 = ch1+1;
  for (i=0; i<cnt; i++) {
    cc1 = *ch1;
    cc2 = *ch2;
    *ch1 = cc2;
    *ch2 = cc1;
    ch1+=2; ch2+=2;
  }
}

/* Byte swap requested number of 4 byte elements */

void gabswp (void *r, gaint cnt) {
gaint i;
char *ch1,*ch2,*ch3,*ch4,cc1,cc2;

  ch1 = (char *)r;
  ch2 = ch1+1;
  ch3 = ch2+1;
  ch4 = ch3+1;
  for (i=0; i<cnt; i++) {
    cc1 = *ch1;
    cc2 = *ch2;
    *ch1 = *ch4;
    *ch2 = *ch3;
    *ch3 = cc2;
    *ch4 = cc1;
    ch1+=4; ch2+=4; ch3+=4; ch4+=4;
  }
}

/* Byte swap requested number of 8 byte elements */

void gabswp8 (void *r, gaint cnt) {
gaint i;
char *ch;

  ch = (char *)r;
  for (i=0; i<cnt; i++) {
    ganbswp (ch+i*8, 8);
  }
}

/* joew 071902 Byte swap a single element with the width given */
void ganbswp(char *buf, gaint cnt) {
  gaint i, j;
  char tmp;
  for (i=0, j=cnt-1; i<cnt/2; i++,j--) {
    tmp = buf[i];
    buf[i] = buf[j];
    buf[j] = tmp;
  }
}

/* Byte swap a report header from a station data file */

void gahswp (struct rpthdr *hdr) {
  gabswp((gafloat *)(&(hdr->lat)),5);
}

/* Return day of week for date/time  0=sunday, 6=saturday */

gaint dayweek (struct dt *dtime) {
struct dt anch;
gaint i,j;
  if (dtime->yr<1950 || dtime->yr>2020) return(7);
  anch.yr = 1950;
  anch.mo = 1;
  anch.dy = 1;
  anch.hr = 0;
  anch.mn = 0;
  i = timdif(&anch,dtime,0);
  i = i/1440;
  j = i/7;
  i = i - j*7;
  return(i);
}

/*
 * convert an IBM float to single precision number v1.0
 *
 *                      Wesley Ebisuzaki
 */

gafloat ibm2flt(unsigned char *ibm) {
gaint positive, power;
gauint abspower;
galint mant;
gadouble value, exp;

 positive = (ibm[0] & 0x80) == 0;
 mant = (ibm[1] << 16) + (ibm[2] << 8) + ibm[3];
 power = (gaint) (ibm[0] & 0x7f) - 64;
 abspower = power > 0 ? power : -power;
 
 exp = 16.0;
 value = 1.0;
 while (abspower) {
   if (abspower & 1) {
     value *= exp;
   }
   exp = exp * exp;
   abspower >>= 1;
 }
 
 if (power < 0) value = 1.0 / value;
 value = value * mant / 16777216.0;
 if (positive == 0) value = -value;
 return (gafloat)value;
}

/*
 * convert a float to an IBM single precision number v1.0
 *
 *                      Wesley Ebisuzaki
 *
 * doesn't handle subnormal numbers
 */

gaint flt2ibm(gafloat x, unsigned char *ibm) {
gaint sign, exp, i;
gadouble mant;

 if (x == 0.0) {
   ibm[0] = ibm[1] = ibm[2] = ibm[3] = 0;
   return 0;
 }
 
 /* sign bit */
 if (x < 0.0) {
   sign = 128;
   x = -x;
 }
 else sign = 0;
 
 mant = frexp((gadouble) x, &exp);
 
 if (mant >= 1.0) {
   mant = 0.5;
   exp++;
 }
 while (exp & 3) {
   mant *= 0.5;
   exp++;
 }
 
 exp = exp/4 + 64;
 
 if (exp < 0) {
   fprintf(stderr,"underflow in flt2ibm\n");
   ibm[0] = ibm[1] = ibm[2] = ibm[3] = 0;
   return 0;
 }
 if (exp > 127) {
   fprintf(stderr,"overflow in flt2ibm\n");
   ibm[0] = sign | 127;
   ibm[1] = ibm[2] = ibm[3] = 255;
   return -1;
 }
 
 /* normal number */
 
 ibm[0] = sign | exp;
 
 mant = mant * 256.0;
 i = floor(mant);
 mant = mant - i;
 ibm[1] = i;
 
 mant = mant * 256.0;
 i = floor(mant);
 mant = mant - i;
 ibm[2] = i;
 
 ibm[3] = floor(mant*256.0);
 
 return 0;
}

/* wesley ebisuzaki v0.1
 *
 * takes 4 byte character string (single precision ieee big-endian)
 * and returns a float
 *
 * doesn't handle NAN, infinity and any other funny stuff in ieee
 *
 * ansi C
 */

gafloat ieee2flt(unsigned char *ieee) {
gadouble fmant;
gaint exp;

 if (ieee[0] == 0 && ieee[1] == 0 && ieee[2] == 0 && ieee[3] == 0)
   return (gafloat) 0.0;
 
 exp = ((ieee[0] & 127) << 1) + (ieee[1] >> 7);
 fmant = (gadouble) ((gaint) ieee[3] + (gaint) (ieee[2] << 8) +
		     (gaint) ((ieee[1] | 128) << 16));
 if (ieee[0] & 128) fmant = -fmant;
 return (gafloat) (ldexp(fmant, (gaint) (exp - 128 - 22)));
}

gadouble ieee2dbl(unsigned char *ieee) {
gadouble fmant;
gaint exp;

 if (ieee[0] == 0 && ieee[1] == 0 && ieee[2] == 0 && ieee[3] == 0)
   return (gadouble) 0.0;
 
 exp = ((ieee[0] & 127) << 1) + (ieee[1] >> 7);
 fmant = (gadouble) ((gaint) ieee[3] + (gaint) (ieee[2] << 8) +
		     (gaint) ((ieee[1] | 128) << 16));
 if (ieee[0] & 128) fmant = -fmant;
 return (gadouble) (ldexp(fmant, (gaint) (exp - 128 - 22)));
}


/*
 * convert a float to an ieee single precision number v1.1
 * (big endian)
 *                      Wesley Ebisuzaki
 *
 * bugs: doesn't handle subnormal numbers
 * bugs: assumes length of integer >= 25 bits
 */

gaint flt2ieee(gafloat x, unsigned char *ieee) {
gaint sign, exp;
gauint umant;
gadouble mant;

 if (x == 0.0) {
   ieee[0] = ieee[1] = ieee[2] = ieee[3] = 0;
   return 0;
 }
 /* sign bit */
 if (x < 0.0) {
   sign = 128;
   x = -x;
 }
 else sign = 0;
 mant = frexp((gadouble) x, &exp);
 umant = mant * 16777216 + 0.5;
 if (umant >= 16777216) {
   umant = umant / 2;
   exp++;
 }
 /* bit 24 should be a 1 .. not used in ieee format */
 exp = exp - 1 + 127;
 if (exp < 0) {
   /* signed zero */
   ieee[0] = sign;
   ieee[1] = ieee[2] = ieee[3] = 0;
   return 0;
 }
 if (exp > 255) {
   /* signed infinity */
   ieee[0] = sign + 127;
   ieee[1] = 128;
   ieee[2] = ieee[3] = 0;
   return 0;
 }
 /* normal number */
 ieee[0] = sign + (exp >> 1);
 ieee[3] = umant & 255;
 ieee[2] = (umant >> 8) & 255;
 ieee[1] = ((exp & 1) << 7) + ((umant >> 16) & 127);
 return 0;
}

/* Copies indicated scaling info into newly allocated 
   gadouble array.  args:
 
   vals -- input scaling array
   lin  -- input is linear or levels
   dir  -- direction of scaling info:
              0 for gr to ab 
              1 for ab to gr
   dim  -- dimension the scaling info is for

   lin, dir, and dim are provided solely to figure out how
   many values are to be copied.  This assumes knowledge
   of how the various scaling items are set up.  */
   

gadouble *cpscal (gadouble *vals, gaint lin, gaint dir, gaint dim) {
gaint i,num;
gadouble *vvv;
size_t sz;

 if (dim<0) {
   gaprnt (0,"cpscal error:  dim is not >= 0 \n");
   return (NULL);
 }
 if (dim==3) {
   num = 8;
 }
 else {
   if (lin==1) num = 3;
   else num = (gaint)(*vals+0.5) + 5;
 }
 sz = sizeof(gadouble)*num;
 vvv = (gadouble *)galloc(sz,"cpscal");
 if (vvv==NULL) {
   snprintf(pout,1255,"cpscal memory allocation error; dim=%d lin=%d num=%d\n",dim,lin,num);
   gaprnt(0,pout);
   return (NULL);
 }
 for (i=0; i<num; i++) {
   *(vvv+i) = *(vals+i);
 }
 return (vvv);
}



/*  handle var name of the form longnm=>abbrv
    or just the abbrv with no long name */

gaint getvnm (struct gavar *pvar, char *mrec) {
gaint ib,i,j,k,len,flag;

  ib = 0;
  while (*(mrec+ib)==' ') ib++;

  if (*(mrec+ib)=='\0' || *(mrec+ib)=='\n') return(1);

  /* Scan for the '=>' string */
  len = 0;
  i = ib;
  flag = 0;

  while (1) {
    if (*(mrec+i)==' ' || *(mrec+i)=='\0' || *(mrec+i)=='\n') break;
    if (*(mrec+i)=='=' && *(mrec+i+1)=='>') {
      flag = 1;
      break;
    }
    len++ ; i++; 
  }

  if (flag) {
    for (j=ib; j<i; j++) {
      k = j-ib;
      pvar->longnm[k] = *(mrec+j); 
      /* substitute ~ for spaces in longname */
      if (pvar->longnm[k]=='~') pvar->longnm[k]=' '; 
    }
    pvar->longnm[len] = '\0';
    i+=2;
  } else {
    i = 0;
    pvar->longnm[0] = '\0';
  } 

  if (*(mrec+i)=='\n' || *(mrec+i)=='\0') return (1);

  getwrd (pvar->abbrv, mrec+i, 15);
  lowcas(pvar->abbrv);

  /* Check if 1st character is lower-case alphabetic */
  if (islower(*(pvar->abbrv))) return(0);
  else return (1);
}


/*  Parse ensemble names in EDEF record */
gaint getenm (struct gaens *ens, char *mrec) {
gaint i;

  i = 0;
  if (*(mrec+i)=='\n' || *(mrec+i)=='\0') return (1);
  getwrd (ens->name, mrec+i, 15);
  return(0);

}

/* Test if two doubles are equal; returns 1 if args are not equal */
gaint dequal(gadouble op1, gadouble op2, gadouble tolerance) {
  if (fabs(op1 - op2) <= tolerance) return(0) ;
  else return(1) ;
}


/* Following two routines used in GRIB2 handling */

/* applies the scale factor to scaled grib2 code values */
gadouble scaled2dbl(gaint scale_factor, gaint scale_value) {
   if (scale_factor == 0) return (gadouble) scale_value;
   return scale_value * Int_Power(0.1, scale_factor);
}
/*  returns x**y  */
gadouble Int_Power(gadouble x, gaint y) {
  gadouble value;
  if (y < 0) {
    y = -y;
    x = 1.0 / x;
  }
  value = 1.0;
  while (y) {
    if (y & 1) {
      value *= x;
    }
    x = x * x;
    y >>= 1;
  }
  return value;
}


#ifndef HAVE_FSEEKO

gaint fseeko(FILE *stream, off_t offset, gaint whence) {
  fseek(stream, (long)offset, whence);
}

off_t ftello(FILE *stream) {
  return (off_t)ftell(stream);
}
#endif


#ifndef STNDALN

/* Progress of a long calculation (ave, sum, tloop, define and the like),
   shown on the terminal so it is plain how much is left. A loop says how
   many steps it takes (gaprogbeg), how many it has done (gaprogstep), and
   when it is through (gaprogend). The line also gives the number of
   calculation threads (set threads). Loops inside loops narrow it down: a
   define over 12 times of an ave over 1000 is 1/12000 of the way per step
   of the ave. Nothing shows for the first second, or when standard output
   is not a terminal, and the line is taken away before anything else is
   printed and when the command ends, so output and logs read as before.
   GA_PROGRESS=off turns it off; a number sets the delay in seconds. iTerm2
   also shows it in its own progress bar (outside tmux). */

#include <time.h>
#include <sys/ioctl.h>
#include "gaomp.h"

#define PROG_LEVELS 8

static struct {
  char what[16];
  gaint total, done;
} prog[PROG_LEVELS];
static gaint progdepth=0;          /* loops running */
static gaint progn=0;              /* of them recorded */
static gadouble progf=0.0;         /* fraction done, never going back */
static gaint progshown=0;          /* the line is on the terminal */
static gaint progpct=-1;           /* last percentage given iTerm2 */
static gaint progon=-1;            /* -1 not decided yet */
static gaint progiterm=0;
static gaint progutf8=0;
static gadouble progdelay=1.0;
static gadouble progt0,proglast;

static gadouble prognow (void) {
struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return (t.tv_sec + t.tv_nsec*1e-9);
}

static void progsetup (void) {
char *v,*lc;
  progon = isatty(1);
  v = getenv("GA_PROGRESS");
  if (v && *v) {
    if (!strcmp(v,"off") || !strcmp(v,"no") || !strcmp(v,"0")) progon = 0;
    else if (atof(v)>0.0) progdelay = atof(v);
  }
  v = getenv("LC_TERMINAL");
  lc = getenv("TERM_PROGRAM");
  progiterm = ((v && !strcmp(v,"iTerm2")) || (lc && !strcmp(lc,"iTerm.app"))) &&
              (getenv("TMUX")==NULL || *getenv("TMUX")=='\0');
  lc = getenv("LC_ALL");
  if (lc==NULL || *lc=='\0') lc = getenv("LC_CTYPE");
  if (lc==NULL || *lc=='\0') lc = getenv("LANG");
  progutf8 = lc && (strstr(lc,"UTF-8") || strstr(lc,"utf-8") || strstr(lc,"UTF8") ||
                    strstr(lc,"utf8"));
}

static void progtime (char *buf, size_t n, gadouble sec) {
long t;
  t = (long)(sec+0.5);
  if (t<60) snprintf(buf,n,"%ld s",t);
  else if (t<3600) snprintf(buf,n,"%ld:%02ld",t/60,t%60);
  else snprintf(buf,n,"%ld:%02ld:%02ld",t/3600,(t/60)%60,t%60);
}

/* Write it out now; the caller has flushed stdout */

static void progput (const char *s) {
size_t n;
ssize_t r;
  n = strlen(s);
  while (n) {
    r = write(1,s,n);
    if (r<=0) return;
    s += r;
    n -= r;
  }
}

/* Take the line away, before other output or when the work is done */

void gaprogpause (void) {
  if (!progshown) return;
  fflush(stdout);
  progput("\r\033[K");
  progshown = 0;
}

static void progdraw (void) {
struct winsize ws;
char line[256],bar[512],el[32],left[32],label[64],count[80];
gadouble f,t,elapsed;
gaint i,cols,width,fill,pct,used;

  /* how far along, the steps inside counting as part of a step */
  f = 0.0;
  for (i=progn-1; i>=0; i--) {
    if (prog[i].total<=0) continue;
    f = (prog[i].done + f) / prog[i].total;
    if (f>1.0) f = 1.0;
  }
  /* a step may hold more than one loop: keep to the furthest so far */
  if (f<progf) f = progf;
  progf = f;
  t = prognow();
  elapsed = t - progt0;
  if (elapsed < progdelay) return;
  if (progshown && t-proglast < 0.2) return;
  proglast = t;

  cols = 80;
  if (ioctl(1,TIOCGWINSZ,&ws)==0 && ws.ws_col>0) cols = ws.ws_col;
  if (cols>200) cols = 200;

  label[0] = '\0';
  for (i=0; i<progn; i++) {               /* define > ave */
    if (i) strncat(label," > ",sizeof(label)-strlen(label)-1);
    strncat(label,prog[i].what,sizeof(label)-strlen(label)-1);
  }
  i = ga_omp_get_threads();                /* the calculation threads at work */
  snprintf(count,sizeof(count),"%d/%d  %d thread%s",prog[0].done,prog[0].total,
           i,i==1 ? "" : "s");
  progtime(el,sizeof(el),elapsed);
  if (f>0.0 && elapsed>=1.0) {
    progtime(left,sizeof(left),elapsed*(1.0-f)/f);
    snprintf(line,sizeof(line)," %3d%%  %s  %s, about %s left",
             (gaint)(f*100.0),count,el,left);
  } else
    snprintf(line,sizeof(line)," %3d%%  %s  %s",(gaint)(f*100.0),count,el);
  used = (gaint)strlen(label) + 4 + (gaint)strlen(line);
  width = cols - 1 - used;
  if (width>40) width = 40;
  if (width<10) {                          /* narrow: no bar */
    width = 0;
    snprintf(bar,sizeof(bar),"%s%s",label,line);
  } else {
    fill = (gaint)(f*width+0.5);
    bar[0] = '\0';
    strncat(bar,label,sizeof(bar)-1);
    strncat(bar," [",sizeof(bar)-strlen(bar)-1);
    for (i=0; i<width; i++)
      strncat(bar,i<fill ? (progutf8 ? "\xe2\x96\x88" : "#") : (progutf8 ? "\xe2\x96\x91" : "."),
              sizeof(bar)-strlen(bar)-1);
    strncat(bar,"]",sizeof(bar)-strlen(bar)-1);
    strncat(bar,line,sizeof(bar)-strlen(bar)-1);
  }
  fflush(stdout);
  progput("\r");
  progput(bar);
  progput("\033[K");
  progshown = 1;
  pct = (gaint)(f*100.0);
  if (progiterm && pct!=progpct) {
    snprintf(line,sizeof(line),"\033]9;4;1;%d\a",pct);
    progput(line);
    progpct = pct;
  }
}

/* A loop of total steps starts. Returns its level, for gaprogend. */

gaint gaprogbeg (const char *what, gaint total) {
gaint lv;
  if (progon<0) progsetup();
  if (progdepth==0) {
    progt0 = prognow();
    proglast = 0.0;
    progf = 0.0;
  }
  lv = progdepth++;
  if (lv<PROG_LEVELS) {
    snprintf(prog[lv].what,sizeof(prog[lv].what),"%s",what);
    prog[lv].total = total;
    prog[lv].done = 0;
    progn = lv+1;
  }
  return (lv);
}

/* The innermost loop now knows how many steps it takes */

void gaprogtotal (gaint total) {
  if (progdepth==0 || progdepth>PROG_LEVELS) return;
  prog[progdepth-1].total = total;
}

/* The innermost loop has done this many of its steps */

void gaprogstep (gaint done) {
  if (progdepth==0 || progdepth>PROG_LEVELS) return;   /* too deep to follow */
  prog[progdepth-1].done = done;
  if (progon>0) progdraw();
}

/* The loop at this level is through, or gave up, with any inside it */

void gaprogend (gaint level) {
  if (level<0 || level>=progdepth) return;
  progdepth = level;
  progn = level<PROG_LEVELS ? level : PROG_LEVELS;
  if (progdepth==0) gaprogreset();
}

/* The command is over: whatever was left goes */

void gaprogreset (void) {
  progdepth = 0;
  progn = 0;
  gaprogpause();
  if (progpct>=0) {
    fflush(stdout);
    progput("\033]9;4;0\a");
    progpct = -1;
  }
}

#endif  /* STNDALN */


#if READLINE==1
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>

/* Retrieves the next command from the user.  Leading blanks
   are stripped.  The number of characters entered before the
   CR is returned.                                                    */

gaint nxrdln (char *cmd, char *prompt) {
char *ch, *ch2;

#ifndef STNDALN
  gaprogreset();
  gxidle();
#endif
  ga_at_prompt = 1;
  ch=readline(gatxtlp(prompt));
  ga_at_prompt = 0;
  if ( ch== NULL) {
    return(-1);
  } else {
    ch2 = ch;
    while (*ch == ' ') ch++;   /* Skip leading blanks */
    strcpy(cmd, ch);
    if (*ch) add_history(ch);   /* Skip blank lines */
  }
  return(strlen(cmd)+1);
}

/* Ctrl-C while readline waits for a command: readline has already echoed
   ^C and put the terminal back, and calls the GrADS handler, which calls
   this. Drop what was typed and start a new line, as a shell does. */

void ga_prompt_sigint (void) {
  if (ga_at_prompt==1) {
    rl_crlf();
    rl_on_new_line();
    rl_replace_line("",0);
    rl_redisplay();
    return;
  }
  if (ga_at_prompt==2) {
    /* the terminal driver has already thrown the typed line away */
    if (write(1,"\n",1)<0) return;
    if (write(1,ga_prompt_text,strlen(ga_prompt_text))<0) return;
  }
}

#else

void ga_prompt_sigint (void) {
  if (ga_at_prompt==2) {
    if (write(1,"\n",1)<0) return;
    if (write(1,ga_prompt_text,strlen(ga_prompt_text))<0) return;
  }
}

#endif  /* matches #if READLINE==1 */
