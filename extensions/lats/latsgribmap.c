
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DRIVER_GAGMAP
#include "grads.h"
#include "gagmap.h"

static char pout[256];   /* Build error msgs here */

/*mf 961205 --- expose Mike Fiorino's global struct to these routines for 365 day calandars mf*/
extern struct gamfcmn mfcmn;
/*mf 961205 --- expose Mike Fiorino's global struct to these routines for 365 day calandars mf*/

void latsgribmap (char *name, int notaucheck) {

  int didmatch;
  ifile=name;
  if(notaucheck) notau = 1;

  skip=0;
  verb=0;         /* verbose defautl is YES */
  g1ver=2;        /* default version */ 
  scaneof=0;      /* option to ignore failure to find data at end of file */
  scanEOF=0;      /* option to ignore failure to find data at end of file */
  scanlim=1000;   /* the default # of max bytes between records */
  tauflg=0;       /* search for a fixed tau in filling the 4-D volume */
  tauoff=0;       /* the fixed tau in h */
  tau0=0;         /* set the base dtg for tau search */
  update=0;       /* set the base dtg for tau search */
  write_map=1;    /* write out the map (testing only) */
  diag=0;         /* full diagnostics */
  mpiflg=0;
  mfcmn.fullyear=1; /* initialize the GrADS calendar so it is set to the file calendar in gaddes */

  // Temporarily uses an external gribmap
  // didmatch=gribmap();
  sprintf(pout,"gribmap -i %s",name);  
  didmatch = system(pout) - 255;

/*mf --- error conditions ---*/

  if(didmatch==0) {
    printf("\n");
    printf("LATS_GRIB: latsgribmap WARNING: no GRIB records matched!!!!!\n");
    printf("LATS_GRIB: the .ctl will return undefined data only\n");
    printf("LATS_GRIB: likely cause -- improper LATS setup\n");
    printf("\n");

  } else if ( didmatch >= 100) {
    printf("\n");
    printf("LATS_GRIB: latsgribmap ERROR: problem with the GRIB data...\n");
    printf("LATS_GRIB: rc = %d\n",didmatch);
    printf("LATS_GRIB: contact fiorino@llnl.gov\n");
    printf("\n");

  } else if ( didmatch > 1 && didmatch < 100) {
    printf("\n");
    printf("LATS_GRIB: latsgribmap ERROR: GRIB data improperly coded by fgbds.c\n");
    printf("LATS_GRIB: rc = %d\n",didmatch);
    printf("LATS_GRIB: contact fiorino@llnl.gov\n");
    printf("\n");

  } else if (! quiet){
    printf("LATS_GRIB: SUCCESS -- gribmap for GrADS/VCS seems to have worked...\n\n");
  }


}

