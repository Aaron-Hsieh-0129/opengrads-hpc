
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#define _POSIX_SOURCE 1
#include <math.h>
#include <float.h>
#include <string.h>
#include "fgrib.h"
#include "grads.h"
/*
  
  routine to grib 1 pack a floating point array
  written by Mike Fiorino, NMC on 940105

  input:   *fpin -- pointer to a float array of nij elements
           undef -- float of an undefined value
           nbits -- # bits /grid point

  output:  *chout -- pointer a char stream where the packed field
                     will put 

  return:  bds->len -- number of bytes in the field

*/

int bds_set(float *fpin, grib_pds *pds,
	    grib_bds *bds, grib_bms *bms,
	    float undef, int nij, int nbits,
	    int hasmissing, float missingdelta) {

  /* max and min of the input field and packing factor */
   
  float amax,amin,pakfac;
  
  /* conversion factor for log2 calc = 1/log10(2)) */
  
  static float log2fact=3.321928;
  
  /* temp vars */

  float a1,a2,a3,a4,a5;
  int i,i1,i2,i3,cnt,cntm,cntv,rc;

  unsigned char ibmfloat[4];

  unsigned int *intout;
  unsigned int *mask;
  unsigned int iu1;

  int len,bnum,flg;
  float ref;
  float dfac,bfac;

  /*---
    calculate # of bytes the grib field 
  ---*/
  
  
  mask = (unsigned int *)malloc(sizeof(int)*nij);
  if(mask == NULL){
    latsError("\nMalloc error for int mask to make BMS");
    return 88;
  }
  
  intout = (unsigned int *)malloc(sizeof(unsigned int)*nij);
  if(intout == NULL){
    latsError("\nMalloc error for int out for BDS");
    return 88;
  }
  

  /*---
    find the max and min of the field 
  ---*/
  
  amax=-FLT_MAX;
  amin=FLT_MAX;
  cntm=0;
  cntv=0;
  if(VERB) printf("uuuuuuuuuuuuuiiiiiiiii %e\n",undef);

  if(hasmissing){
	  for( i=0 ; i<nij ; i++) {
		  
		  *(mask+i)=1;
/*		  if(*(fpin+i) == undef ) {
 */
		  if(fabs(*(fpin+i) - undef) <= missingdelta) {
			  *(mask+i)=0;
			  cntm++;
		  } else {
			  cntv++;
		  }
		  
		  if(*(fpin+i) > amax && *(mask+i) ) amax=*(fpin+i);
		  if(*(fpin+i) < amin && *(mask+i) ) amin=*(fpin+i);
		  
	  }
  }
  else{
	  cntv = nij;
	  for( i=0 ; i<nij ; i++) {
		  *(mask+i)=1;
		  if(*(fpin+i) > amax ) amax=*(fpin+i);
		  if(*(fpin+i) < amin ) amin=*(fpin+i);
	  }
  }
  if(VERB) {
    printf("qqq cntm = %d %d %d %d\n",cntm,cntv,(cntm+cntv),nij);
    printf("amax = %f amin = %f \n",amax,amin);
  }

  /*---
    FIXED #bit/grid point
    compute scale factor and packing factor for cas 
  ---*/

  if(VERB) printf("bbbbbbbbbbbbb %i\n",pds->dsf);

  if(pds->dsf != -999){

  /*---
    DECIMAL SCALE FACTOR -
    compute #bits/grid point
    scale factor and packing factor for cas 
  ---*/

    dfac=pow(10.0,(double)pds->dsf);
    bfac=1.0;
    ref=amin*dfac;
    a1=(amax-amin)*dfac;

    i1=(int)(a1+0.5);

    if(VERB) printf("DSF a1 = %g %d %d %f %f %f\n",a1,i1,pds->dsf,amax,amin,pow(10.0,(double)pds->dsf));

    i2=0;
    i3=i1;
    while(i3 != 0) {
      i3=i3/2;
      i2++;
    }

    nbits=i2;

    bds->bsf=0.0;
    if(nbits > MAXNBITS){
	    nbits=MAXNBITS;     /* check if nbits is too big and set to max */
	    pds->dsf = -999;		     /* Force to use binary scaling */
    }
  }

  if(pds->dsf == -999) {
    a1=(amax-amin)/(pow(2.0,(double)(nbits+1))-3);

    if(a1 != 0.0) {			     /* test if a constant field when nbits is set */
      bds->bsf=(int)floor(log10(a1)*log2fact)+2;  
    } else {
      bds->bsf=1;
      nbits=0;
    }   
/*
   special case of single bit packing -- output is 0 or 1
*/

    if(nbits == 1)  bds->bsf=0 ;

    if(VERB) printf("a1 = %g bds->bsf = %d %f \n",a1,bds->bsf,pow(2.0,(double)bds->bsf));
  
    bfac=pow(2.0,(double)(-bds->bsf));
    dfac=1.0;
    ref=amin;

    if(VERB) printf("pack factor = %f\n",bfac);

/* ---- set the decimal scale factor in the PDS ----*/

    pds->dsf=0;
    set_int2(&pds->pds[26],(unsigned int)abs(pds->dsf));

  }

/* if nbits = 0 then we have a constant fields set to 1 */

  if(nbits==0) nbits=1;

  
  bds->len=(cntv*nbits)/8;             /* # data bytes */ 
  if((cntv*nbits) % 8) bds->len++;
  bds->len += 11 ;                    /* header size */
  if(bds->len % 2) bds->len++ ;           /* must be even # bytes */
  bds->nub = bds->len*8 - 11*8 - cntv*nbits ;  /* # of unused bits at end */
  
  if(VERB) printf("bds->len = %d bds->nub = %d nbits %d\n",
		  bds->len,bds->nub,nbits);
  
  bds->bds = (unsigned char *)malloc(sizeof(char)*bds->len);
  if(bds->bds == NULL){
    latsError("\nMalloc error or BDS char");
    return 88;
  }

  set_int3(&bds->bds[0],bds->len);
  
  bds->bds[3]=bds->nub;

  if(bds->dt) SETBIT(bds->bds[3],7) ;
  if(bds->pt) SETBIT(bds->bds[3],6) ;
  if(bds->od) SETBIT(bds->bds[3],5) ;
  if(bds->af) SETBIT(bds->bds[3],4) ;

  set_int2(&bds->bds[4],abs(bds->bsf));
  if(bds->bsf<0) SETBIT(bds->bds[4],7);
  
  rc=flt2ibm(ref, ibmfloat);
  ref=ibm2flt(ibmfloat); 
  if(VERB) printf("amin = %e ref = %e rc=%d \n",amin,ref,rc);

  strncpy((char *) &bds->bds[6],(char *) ibmfloat,4);

  bds->nb=nbits;
  bds->bds[10]=bds->nb;

  /*---------------
    simple packing and load
  ----------------*/

  cnt=0;
  if(VERB) printf("ddddddddddddddddddddddd %f %f %f \n",dfac,ref,bfac);
  for ( i=0 ; i<nij ; i++ ) {
    if(*(mask+i)) {

/*mf 970826 - feature of  OSF! mriim1.mri-jma.go.jp V3.0 358.78 alpha

  *(intout+cnt)=(unsigned int)(( (*(fpin+i)*dfac) - ref )*bfac+0.5);
  fails,  does the assignment BEFORE the calc

mf*/
      iu1=(unsigned int)(( (*(fpin+i)*dfac) - ref )*bfac+0.5);
      *(intout+cnt)=iu1;
/*
printf("QQQ %d %d %d %d \n",i,cnt,*(intout+cnt),(unsigned int)((*(fpin+i)*dfac-ref)*bfac+0.5));
*/
      cnt++;
    }
  }
  list2bitstream(intout,&bds->bds[11], cntv, nbits) ;

/*------------
   bms
--------------*/

  bms->len = 0;				     /* Don't write bms if no missing data */
  pds->bflg = 0;			     /* By default, no missing data */
  if(cntm) {

    bms->len=nij/8;             /* # data bytes */ 
    if(nij % 8) bms->len++;
    bms->len += 6 ;                    /* header size */
    if(bms->len % 2) bms->len++ ;           /* must be even # bytes */
    bms->nub = bms->len*8 - 6*8 - nij;  /* # of unused bits at end */
    if(VERB) printf("bbbb %d %d %d\n",nij,bms->len,bms->nub);

    bms->bms = (unsigned char *)malloc(sizeof(char)*bms->len);
    if(bms->bms == NULL){
      latsError("\nMalloc error for char mask to make BMS");
      return 88;
    }

/* ---- set the pds bms flag ----*/

    pds->bflg=1;
    if(pds->bflg) SETBIT(pds->pds[7],6);

    set_int3(&bms->bms[0],bms->len);
    bms->bms[3]=bms->nub;
    set_int2(&bms->bms[4],bms->nu);

    list2bitstream(mask,&bms->bms[6], nij, 1) ;

  }

  free(mask);
  free(intout);
  return(0);
  
}
