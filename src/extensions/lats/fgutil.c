
/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include "fgrib.h"

static unsigned int mask[] = {0,1,3,7,15,31,63,127,255};

void set_int3(unsigned char *string, unsigned int n) {
    string[0] = (n >> 16) & 255;
    string[1] = (n >>  8) & 255;
    string[2] =  n        & 255;
}

void set_int2(unsigned char *string, unsigned int n) {
    string[0] = (n >>  8) & 255;
    string[1] =  n        & 255;
}


/* (c) 1996 Wesley Ebisuzaki
 *
 * grib: convert linear list of ints to a bitstream
 */

void list2bitstream(unsigned int *list, unsigned char *bitstream, int ndata, int nbits) 
{
    int cbits, jbits;
    unsigned int j, c;

    if (nbits == 0) {
	return;
    }
    if (nbits < 0) {
	latsError("nbits < 0!  nbits = %d\n", nbits);
	exit(0);
    }

    cbits = 8;
    c = 0;
    while (ndata-- > 0) {
        j = *list++;
	jbits = nbits;
	while (cbits <= jbits) {
	    if (cbits == 8) {
	        *bitstream++ = (j >> (jbits - cbits)) & 255;
	        jbits -= 8;
	    }
	    else {
	        c = (c << cbits) + ((j >> (jbits - cbits)) & mask[cbits]);
	        jbits -= cbits;
    	        *bitstream++ = c;
		cbits = 8;
	        c = 0;
	    }
	}
	/* now jbits < cbits */
	if (jbits) {
	    c = (c << jbits) + (j & mask[jbits]);
	    cbits -= jbits;
	}
    }
    if (cbits != 8) *bitstream++ = c << cbits;
}
