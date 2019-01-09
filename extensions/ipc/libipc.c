/*

    Copyright (C) 2007 by Arlindo da Silva <dasilva@opengrads.org>
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

% perldoc libipc.c 

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < libipc.c > libipc.pod

and place libipc.pod in a place perldoc can find it.

*/


#ifdef POD

=pod

=head1 NAME

libipc.gex - Support Functions for GrADS Inter-process Communication

=head1 SYNOPSIS

=head3 GrADS Commands:

=over 4

B<ipc_open> I<FILENAME> I<MODE>

B<ipc_save> I<EXPR> [I<FILENAME>]

B<ipc_close> [I<MODE>]

B<ipc_verb>  [I<ON|OFF>]

B<ipc_error> 

=back

=head3 GrADS Functions:

=over 4

define C<var> = B<ipc_save> ( I<EXPR> [, I<FILENAME>] )

define C<var> = B<ipc_load> ( [I<FILENAME>] )

=back 

=head1 DESCRIPTION 

This library of GrADS extensions implements functions to enable GrADS
inter-proceess communication.  The primary use of these functions is
to facilitate the interface of GrADS to other applications by means of
bi-directional pipes (e.g., Perl, Python, IDL, Octave, etc.). The current
implementation provides functions and commands for exporting and
importing gridded fields from and into GrADS. These functions can also
be used as a convenient alternative to C<LATS> or C<fwrite> for saving
and retrieving gridded variables to and from disk files.

Data is exchanged by means of a stream interface: either external
files or through STDIN and STDOUT, the prefered method when
interfacing with bi-directional pipes. The actual file format borrows
from the GrADS classic I<User Defined Functions> (UDFs), with extensions
to allow exporting/importing of timeseries (more generally, a
collection of 2D slices). See TRANSFER FILE FORMAT below for a
description of this format.

=cut

(see end of file for the rest of the documentation)

..........................................................................

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "grads.h"

#define IPC_READ  1
#define IPC_WRITE 2

static FILE *ifile = NULL;  /* input transfer file */
static FILE *ofile = NULL;  /* output transfer file */

static char pout[256];      /* Build error msgs here */
static verbose = 0;         /* quiet by default */

static char hand_shake[] = "\n<RC> 0 </RC>\n</IPC>\n";

static int  error_rc = 0;
static char error_msg[512];

/* GrADS v1.x compatibility functions */
int gex_expr1(char *expr, struct gastat *pst);
int gex_setUndef (struct gastat *pst);
#define gaexpr gex_expr1


/* fwrite wrapper with conversion to float first */
static size_t flt_fwrite(double *d_ptr, size_t size, size_t nitems, FILE *stream) {
  float *f_ptr;
  int i;
  f_ptr = (float *) malloc(sizeof(float) * nitems);
  if ( f_ptr ) {
    for(i=0; i<nitems; i++) f_ptr[i] = (float) d_ptr[i];
    nitems = fwrite(f_ptr,sizeof(float),nitems,stream);
    free(f_ptr);
    return nitems;
  } else {
    return 0;
  }
}

/* fwrite wrapper with conversion to float first */
static size_t flt_fread(double *d_ptr, size_t size, size_t nitems, FILE *stream) {
  float *f_ptr;
  int i;
  f_ptr = (float *) malloc(sizeof(float) * nitems);
  if ( f_ptr ) {
    nitems = fread(f_ptr,sizeof(float),nitems,stream);
    for(i=0; i<nitems; i++) d_ptr[i] = (double) f_ptr[i];
    free(f_ptr);
    return nitems;
  } else {
    return 0;
  }
}

#define fwrite flt_fwrite
#define fread  flt_fread

/*                       -------                         */

/* Reports last error condition - useful when using pipes */
int c_Error ( int argc, char **argv, struct gacmn *pcm) {
  if ( error_rc ) gaprnt (0,error_msg);
  return (error_rc);
}

/* no error */
static void ipc_resetError() {
  error_rc = 0;
  error_msg[0] = '\0';
}

/* set error code and message */
static void ipc_setError(int rc, char *msg) {
  error_rc = rc;
  strncpy(error_msg,msg,511);
  error_msg[511] = '\0';
}

/* Set verbose mode */
int c_Verb ( int argc, char **argv, struct gacmn *pcm) {

  ipc_resetError();
  if ( argc > 1 ) 
    if ( strcmp(argv[1],"on")==0 || strcmp(argv[1],"ON")==0 )
      verbose = 1;
    else
      verbose = 0;
  else  
      verbose = 1 - verbose;

  if ( verbose )  gaprnt (0,"IPC verbose turned ON\n");
  else            gaprnt (0,"IPC verbose turned OFF\n");

  return 0;

}

/* ....................................................................... */

/* Open transfer file read/write */
int c_Open ( int argc, char **argv, struct gacmn *pcm) {

    int mode;

    ipc_resetError();
    if ( argc < 3 ) {
	sprintf (pout,"Error from %s: not enough arguments\n",argv[0]);
        ipc_setError(1,pout);
	gaprnt (0,pout);
	sprintf (pout,"Usage:\n   %s filename mode\n",argv[0]);
	gaprnt (0,pout);
	gaprnt (0,"where *mode* is either 'r' for read or 'w' for write.\n");
        return (1);
    }

    /* Valid mode? */
    if ( (strlen(argv[2])==1) && (*argv[2] == 'r') ) {
      mode = IPC_READ;
    }  
    else if ( (strlen(argv[2])==1) && (*argv[2] == 'w') ) {
      mode = IPC_WRITE;
    }  
    else {
      sprintf (pout,"Error from %s: invalid mode '%s'; enter either 'r' or 'w'",
               argv[0], argv[2] );
      gaprnt (0,pout);
      ipc_setError(1,pout);
      return (1);
    }

    /* Use stdin or stdout */
    if ( (strlen(argv[1])==1) && (*argv[1]=='-') ) {

	if ( mode == IPC_READ ) {
	    ifile = stdin;
	    if ( verbose ) 
		gaprnt (0,"Using <STDIN> as input transfer stream\n");
        } else {
	    ofile = stdout;
	    if ( verbose ) {
		gaprnt (0,"Using <STDOUT> as output transfer stream.\n");
		gaprnt (0,"From now on verbose will be tuned OFF.\n");
                verbose = 0;
	    }
	}


    /* Open a disk file */
    } else {

      if ( verbose ) {
	sprintf (pout,"Opening transfer file %s\n",argv[1]);
	gaprnt (0,pout);
      }

      if ( mode == IPC_READ ) {
	  ifile = fopen(argv[1],"rb");
	  if (ifile==NULL) goto ferr;
      } else {
	  ofile = fopen(argv[1],"wb");
	  if (ofile==NULL) goto ferr;
      }

    }  /* stdio or disk file */

    return (0);  /* all done */

ferr:
	sprintf (pout,"Error from %s: Error opening transfer file '%s'\n",
		 argv[0], argv[1]);
	gaprnt (0,pout);
        ipc_setError(1,pout);
	return (1);

 } 

/* ....................................................................... */

int c_Close ( int argc, char **argv, struct gacmn *pcm) {
    
    int r_mode, w_mode;

    r_mode = w_mode = 0;

    if ( argc > 1 ) {

	/* Valid mode? */
	     if ( (strlen(argv[2])==1) && (*argv[2] == 'r') ) {
		r_mode = 1;
	}    
        else if ( (strlen(argv[2])==1) && (*argv[2] == 'w') ) {
  	        w_mode = 1;
	}  
	else {
	    sprintf (pout,
                     "Error from %s: invalid mode '%s'; enter either 'r' or 'w'",
		     argv[0], argv[2] );
	    gaprnt (0,pout);
            ipc_setError(1,pout);
	    return (1);
	} 

    /* If no mode specified, assumed closing of both input/output */
    } else {
	r_mode = 1;
        w_mode = 1;
    }

    /* OK, let us do the closing */
    if ( r_mode && ( ifile && (ifile != stdin ) ) ) {
	if ( verbose ) gaprnt (0,"Closing input transfer file\n");
	fclose(ifile);
        ifile = (FILE *) NULL;
    }

    if ( w_mode && ( ofile && (ofile != stdout) ) ) {
	if ( verbose ) gaprnt (0,"Closing output transfer file\n");
	fclose(ofile);
        ofile = (FILE *) NULL;
    }

    return (0);

}

/* ....................................................................... */

static int ipc_Save ( char *expr, FILE *ofile, struct gastat *pst) {

  struct gagrid *pgr;
  struct dt dtim;
  double (*conv) (double *, double);
  double rvals[20],*v;
  int siz,i;
  int rc, save_verbose;

  ipc_resetError();

  if (ofile==(FILE *)NULL) {
    sprintf (pout,"Error from ipc_Save: Error opening transfer file\n");
    gaprnt (0,pout);
    ipc_setError(1,pout);
    return (1);
  }

  if ( ofile==stdout ) {
#if 0
    rc = fwrite(hand_shake,sizeof(char),strlen(hand_shake),stdout);
    if ( rc < strlen(hand_shake) ) {
      gaprnt(0,"Error from ipc_Save: cannot write hand shake to stdout");
      ipc_setError(1,"Error from ipc_Save: cannot write hand shake to stdout");
      return (1);
    }
#endif
    printf("\n<EXP>\n"); /* marker for stream synchronization */
    save_verbose = verbose;
    verbose = 0; /* to be sure */
  }

#ifdef DOING_HEADER

  /* Write hearder record to transfer file */
  rvals[0] = 1.0; /* just 1 argument */
  rc = fwrite (rvals,sizeof(double),20,ofile);
  if (rc<20) goto werr;
  if (verbose)
  printf("[w] hdr with %d elements\n",rc);

#endif

 /* Write expression to transfer file */
  rc = gaexpr(expr,pst);         /* Evaluate      */
  if (rc) {
    ipc_setError(rc,"cannot evaluate expression\n");
    return (rc);
  }

  if (pst->type!=1) {
    rc = -1;
    ipc_setError(rc,"non-gridded data not yet supported\n");
    return(rc);
  }

  pgr = pst->result.pgr;                   /* Fill in header */
  rvals[0] = pgr->undef;
  rvals[1] = pgr->idim;
  rvals[2] = pgr->jdim;
  rvals[3] = pgr->isiz;
  rvals[4] = pgr->jsiz;
  rvals[5] = pgr->ilinr;
  rvals[6] = pgr->jlinr;
  // for(i=0; i<7; i++ ) printf("[w] i=%d rvals=%f\n",i,rvals[i]);
  if (pgr->idim>-1 && pgr->ilinr==1) {     /* Linear scaling info */
    if (pgr->idim==3) {
      gr2t (pgr->ivals,pgr->dimmin[3],&dtim);
      rvals[11] = dtim.yr;
      rvals[12] = dtim.mo;
      rvals[13] = dtim.dy;
      rvals[14] = dtim.hr;
      rvals[15] = dtim.mn;
      rvals[16] = *(pgr->ivals+6);  
      rvals[17] = *(pgr->ivals+5);
      rvals[9] = -999.0;
      rvals[10] = -999.0;    
    } else {
      conv = pgr->igrab;
      rvals[7] = conv(pgr->ivals,pgr->dimmin[pgr->idim]);
      rvals[8] = *(pgr->ivals);
    }
  }
  if (pgr->jdim>-1 && pgr->jlinr==1) {
    if (pgr->jdim==3) {
      gr2t (pgr->jvals,pgr->dimmin[3],&dtim); 
      rvals[11] = dtim.yr;
      rvals[12] = dtim.mo;
      rvals[13] = dtim.dy;
      rvals[14] = dtim.hr;
      rvals[15] = dtim.mn;
      rvals[16] = *(pgr->jvals+6);
      rvals[17] = *(pgr->jvals+5);
      rvals[9] = -999.0;
      rvals[10] = -999.0;
    } else {
      conv = pgr->jgrab;
      rvals[9] = conv(pgr->jvals,pgr->dimmin[pgr->jdim]);
      rvals[10] = *(pgr->jvals);
    }
  }
  siz = pgr->isiz*pgr->jsiz;                 /* Write header */
  rc = fwrite(rvals,sizeof(double),20,ofile);
  if (rc<20) {
    gafree(pst);
    goto werr;
  }
  if (verbose)
  printf("[w] %d+",rc);
                                          /* Write grid   */
  rc = fwrite(pgr->grid,sizeof(double),siz,ofile);
  if (rc<siz) {
    gafree(pst);
    goto werr;
  }
  if (verbose)
  printf("%d+",rc);
  if (pgr->idim>-1) {                  /* write i dim scaling */
    v = pgr->grid;
    if (pgr->idim<3) {
      conv = pgr->igrab;
      for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
	*v = conv(pgr->ivals,(double)i);
	v++;
      }
    } else {
      for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) {
	*v = (double)i;
	v++;
      }
    }
    rc = fwrite(pgr->grid,sizeof(double),pgr->isiz,ofile);
    if (rc<pgr->isiz) {
      gafree(pst);
      goto werr;
    }
    if (verbose)
    printf("%d+",rc);
  }
  if (pgr->jdim>-1) {                /* write j dim scaling */
    v = pgr->grid;
    if (pgr->jdim<3) {
      conv = pgr->jgrab;
      for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
	*v = conv(pgr->jvals,(double)i);
	v++;
      }
    } else {
      for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) {
	*v = (double)i;
	v++;
      }
    }
    rc = fwrite(pgr->grid,sizeof(double),pgr->jsiz,ofile);
    if (rc<pgr->jsiz) {
      gafree(pst);
      goto werr;
    }
    if (verbose)
    printf("%d elements\n",rc);
  }

  if ( ofile==stdout ) {
       printf("\n</EXP>\n<IPC> ipc_save %s -\n", expr);
       verbose = save_verbose;
  }

  return (0);

werr:
  gaprnt (0,"Error from ipc_Save: Error writing to transfer file\n");
  ipc_setError(1,"Error from ipc_Save: Error writing to transfer file\n");
  return (1);

}


/* ....................................................................... */

int c_Save ( int argc, char **argv, struct gacmn *pcm) {

  struct gastat *pst;
  char *fname;
  int rc;

  ipc_resetError();
  
  if ( argc == 1 ) {
   sprintf (pout,"\nError: not enough arguments\nUsage:  %s expr [fname]\n\n",
                  argv[0]);
    gaprnt (0,pout);
    ipc_setError(1,pout);
    return (1);
  }

  /* Open the transfer file */
  if ( argc > 2 ) {
      fname = argv[2];
      if ( (strlen(fname)==1) && (*fname='-') ) {
	  if ( verbose ) {
	      gaprnt (0,"Using STDOUT for output stream");
	  }
	  ofile = stdout;
      } else {
	  if ( verbose ) {
	      sprintf (pout,"Opening OUTPUT transfer file %s\n",fname);
	      gaprnt (0,pout);
	  }
	  ofile = fopen(fname,"wb"); /* ipc_Save will check ofile */
      }
  }


  /* Initialize gastat block */
  pst = getpst(pcm);
  if ( !pst ) {
    sprintf (pout,"Error from %s: Error initializing gastat block\n",argv [0]);
    gaprnt (0,pout);
    fclose(ofile);
    ipc_setError(1,pout);
    return (1);
  }

  /* save expression to file */
  rc = ipc_Save ( argv[1], ofile, pst );

  gafree(pst);        /* Done with expr */
  // pst = getpst(pcm); 
  // gaexpr("0",pst);    /* just return zero */

  /* All the args are written.  Close the transfer file */
  if ( (argc > 2) ) {
    if ( ofile!=stdout ) {
      if ( verbose ) {
	sprintf (pout,"Closing output transfer file %s\n",argv[2]);
	gaprnt (0,pout);
      }
      fclose (ofile);
    }
    ofile = NULL;
  }


  return (rc);

}

/* ....................................................................... */

int f_Save (struct gafunc *pfc, struct gastat *pst) {

  char *fname;
  int rc;
 
  int argc = pfc->argnum + 1; /* for consistency with c_Save */
  char *name = pfc->argpnt[pfc->argnum];

  ipc_resetError();
 
  if ( argc == 1 ) {
   sprintf (pout,"\nError: not enough arguments\nUsage:  %s expr [fname]\n\n",
                  name);
    gaprnt (0,pout);
    ipc_setError(1,pout);
    return (1);
  }

  /* Open the transfer file */
  if ( argc > 2 ) {
      fname = pfc->argpnt[1];
      if ( (strlen(fname)==1) && (*fname='-') ) {
	  if ( verbose ) {
	      gaprnt (0,"Using STDOUT for output stream");
	  }
	  ofile = stdout;
      } else {
	  if ( verbose ) {
	      sprintf (pout,"Opening OUTPUT transfer file %s\n",fname);
	      gaprnt (0,pout);
	  }
	  ofile = fopen(fname,"wb"); /* ipc_Save will check ofile */
      }
  }


  /* save wxpression to file */
  rc = ipc_Save ( pfc->argpnt[0], ofile, pst );

  gafree(pst);
  gaexpr("0",pst);

  /* All the args are written.  Close the transfer file */
  if ( (argc > 2) ) {
    if ( ofile!=stdout ) {
      if ( verbose ) {
	sprintf (pout,"Closing output transfer file %s\n",pfc->argpnt[1]);
	gaprnt (0,pout);
      }
      fclose (ofile);
    }
    ofile = NULL;
  }

  return (rc);

}

/* ....................................................................... */

int f_Load (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  struct dt dtim;
  double (*conv) (double *, double);
  double rvals[20],*v;
  int rc,iarg,siz,i;
  char *ch,rec[80], *fname;
  char *name = pfc->argpnt[pfc->argnum];

  ipc_resetError();

  if ( (pfc->argnum>0) ) {
      fname = pfc->argpnt[0];
      if ( (strlen(fname)==1) && (*fname='-') ) {
	  if ( verbose ) {
	      gaprnt (0,"Using STDIN for input stream");
	  }
	  ifile = stdin;
      } else {
	  if ( verbose ) {
	      sprintf (pout,"Opening INPUT transfer file %s\n",fname);
	      gaprnt (0,pout);
	  }
	  ifile = fopen(fname,"rb");
      }
  }

  if (ifile==(FILE *)NULL) {
    sprintf (pout,"Error from %s: Error opening transfer file\n",name);
    gaprnt (0,pout);
    ipc_setError(1,pout);
    return (1);
  }

#ifdef DOING_CLASSIC_HEADER
    
  /* Read the header record, which contains the return code */
  rc = fread(rvals,sizeof(double),20,ifile);
  if (rc<20) goto rerr;
  if (verbose)
  printf("[r] hdr with %d elements\n",rc);

#endif

  /* If all is ok, read the grid header */
  rc = fread(rvals,sizeof(double),20,ifile);
  if (rc<20) goto rerr;
  if (verbose)
  printf("[r] %d+",rc);

  /* Start building the gagrid block */

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
  for (i=0; i<4; i++) pgr->dimmin[i] = 1;
  // for(i=0; i<7; i++ ) printf("[r] i=%d rvals=%f\n",i,rvals[i]);
  if ( pgr->idim<-1 || pgr->idim>3 ) goto ferr; 
  if ( pgr->jdim<-1 || pgr->jdim>3 ) goto ferr; 
  if ( pgr->ilinr<0 || pgr->ilinr>1) goto ferr; 
  if ( pgr->jlinr<0 || pgr->jlinr>1) goto ferr; 
  if ( pgr->jdim>-1 && pgr->idim>pgr->jdim) goto derr; 
  if ( pgr->idim==-1 && pgr->isiz!=1) goto ferr; 
  if ( pgr->jdim==-1 && pgr->jsiz!=1) goto ferr; 
  if ( pgr->isiz<1) goto ferr; 
  if ( pgr->jsiz<1) goto ferr; 
  for (i=0; i<4; i++) pgr->dimmin[i] = 1;
  if (pgr->idim>-1) pgr->dimmax[pgr->idim] = pgr->isiz;
  if (pgr->jdim>-1) pgr->dimmax[pgr->jdim] = pgr->jsiz;

  if (pgr->idim>-1 && pgr->idim!=pst->idim && pgr->idim!=pst->jdim) goto derr;
  if (pgr->jdim>-1 && pgr->jdim!=pst->idim && pgr->jdim!=pst->jdim) goto derr;

  /* Set up linear scaling info */

  if (pgr->idim>-1 && pgr->ilinr==1) {     /* Linear scaling info */
    if (pgr->idim==3) {
      v = (double *)malloc(sizeof(double)*8);
      if (v==NULL) goto merr;
      *v = rvals[11];
      *(v+1) = rvals[12];
      *(v+2) = rvals[13];
      *(v+3) = rvals[14];
      *(v+4) = rvals[15];
      *(v+6) = rvals[16];
      *(v+5) = rvals[17];
      *(v+7) = -999.9;
      pgr->ivals = v;
      pgr->iavals = v;
    } else {
      v = (double *)malloc(sizeof(double)*6);
      if (v==NULL) goto merr;
      *v = rvals[8];
      *(v+1) = rvals[7]-rvals[8];
      *(v+2) = -999.9;
      pgr->ivals = v;
      *(v+3) = 1.0 / rvals[8];
      *(v+4) = -1.0 * (rvals[7]-rvals[8]) / rvals[8];
      *(v+5) = -999.9;
      pgr->iavals = v+3;
      pgr->iabgr = liconv;
      pgr->igrab = liconv;
    }
  }
  if (pgr->jdim>-1 && pgr->jlinr==1) {     /* Linear scaling info */
    if (pgr->jdim==3) {
      v = (double *)malloc(sizeof(double)*8);
      if (v==NULL) goto merr;
      *v = rvals[11];
      *(v+1) = rvals[12];
      *(v+2) = rvals[13];
      *(v+3) = rvals[14];
      *(v+4) = rvals[15];
      *(v+6) = rvals[16];
      *(v+5) = rvals[17];
      *(v+7) = -999.9;
      pgr->jvals = v;
      pgr->javals = v;
    } else {
      v = (double *)malloc(sizeof(double)*6);
      if (v==NULL) goto merr;
      *v = rvals[10];
      *(v+1) = rvals[9]-rvals[10];
      *(v+2) = -999.9;
      pgr->jvals = v;
      *(v+3) = 1.0 / rvals[10];
      *(v+4) = -1.0 * (rvals[9]-rvals[10]) / rvals[10];
      *(v+5) = -999.9;
      pgr->javals = v+3;
      pgr->jabgr = liconv;
      pgr->jgrab = liconv;
    }
  }

  /* Read in the data */

  siz = pgr->isiz * pgr->jsiz;
  v = (double *)malloc(sizeof(double)*siz);
  if (v==NULL) {
    free(pgr);
    goto merr;
  }
  rc = fread(v,sizeof(double),siz,ifile);
  if (rc<siz) goto rerr;
  pgr->grid = v;
  if (verbose)
  printf("%d+",rc);

  /* Read in non-linear scaling info, if any */

  /* i dim scaling */
  if (pgr->idim>-1) { /* read coord var if jdim is varying */
    v = (double *)malloc(sizeof(double)*(pgr->isiz+2));
    if (v==NULL) {
      free(pgr->grid);
      free(pgr);
      goto merr;
    }
    *v = pgr->isiz;
    rc = fread(v+1,sizeof(double),pgr->isiz,ifile);
    if (verbose)
    printf("%d+",rc);
    if (rc<pgr->isiz) goto rerr;
    *(v+pgr->isiz+1) = -999.9;
    if (pgr->ilinr==0) { /* set pgr only if non-linear scaling */
      pgr->ivals = v;
      pgr->iavals = v;
      pgr->iabgr = lev2gr;
      pgr->igrab = gr2lev;
    } else {
      free(v); /* not needed in this case */
    }
  }

  /* j dim scaling */
  if (pgr->jdim>-1 ) { /* read coord var if jdim is varying */
    v = (double *)malloc(sizeof(double)*(pgr->jsiz+2));
    if (v==NULL) {
      free(pgr->grid);
      free(pgr);
      goto merr;
    }
    *v = pgr->jsiz;
    rc = fread(v+1,sizeof(double),pgr->jsiz,ifile);
    if (verbose)
    printf("%d elements\n",rc);
    if (rc<pgr->jsiz) goto rerr;
    *(v+pgr->jsiz+1) = -999.9;
    if (pgr->jlinr==0) { /* set pgr only if non-linear scaling */
      pgr->jvals = v;
      pgr->javals = v;
      pgr->jabgr = lev2gr;
      pgr->jgrab = gr2lev;
    } else {
      free(v);
    } 
  }

  /* Close file if we opened it */
  if ( (pfc->argnum>0) ) {
      if ( ifile != stdin )  {
	  if ( verbose ) {
	      sprintf (pout,"Closing input transfer file %s\n",pfc->argpnt[0]);
	      gaprnt (0,pout);
	  }
	  fclose (ifile);
      }
      ifile = NULL;
  }

  /* We are done.  Return.  */
  pgr->umask = (char *) NULL;  /* Will be allocated in setUndef */
  pst->result.pgr = pgr;
  pst->type = 1;

  return gex_setUndef (pst);

werr:
  sprintf (pout,"Error from %s: Error writing to transfer file\n",name);
  gaprnt (0,pout);
  fclose(ofile);
  ipc_setError(1,pout);
  return (1);

rerr:
  sprintf (pout,"Error from %s: Error reading from transfer file\n",name);
  gaprnt (0,pout);
  fclose(ifile);
  ipc_setError(1,pout);
  return (1);

merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n",name);
  gaprnt (0,pout);
  fclose (ifile);
  ipc_setError(1,pout);
  return (1);

ferr:
  sprintf (pout,"Error from %s: Invalid transfer file format\n",name);
  gaprnt (0,pout);
  fclose (ifile);
  ipc_setError(1,pout);
  return (1);

derr:
  sprintf (pout,"Error from %s: Invalid dimension environment ",name);
  gaprnt (0,pout);
  gaprnt (0,"in result grid\n");
  fclose (ifile);
  ipc_setError(1,pout);
  return (1);
}

/*................................................................. */

/* Replacement for define until bug is fixed */

int c_Define ( int argc, char **argv, struct gacmn *pcm) {

  int i, rc;
  char cmd[512];

  ipc_resetError();

    if ( argc < 2 ) {
	sprintf (pout,"Error from %s: not enough arguments.\n",argv[0]);
        ipc_setError(1,pout);
	gaprnt (0,pout);
	sprintf (pout,"Usage:\n   %s variable = expr",argv[0]);
	gaprnt (0,pout);
        return (1);
    }

    sprintf(cmd,"define\0");
    for ( i=1; i<argc; i++ ) sprintf(cmd,"%s %s\0", cmd,argv[i]);

    rc = gadef ( cmd, pcm, 0 );
    if ( rc ) ipc_setError(1,pout);

    return (rc);

}

/* end of code, not POD */

#endif

/* ....................................................................... */

#ifdef POD

=head1 COMMANDS PROVIDED

=head2 B<ipc_open> I<FILENAME> I<MODE>

=over 4

This command opens a file for exporting or importing gridded fields
from or into GrADS. On iput, 

=over 4

I<FILENAME> is the name of the file to open; specify "-" for selecting
standard input/output instead of a disk file.

I<MODE> - it can be either I<w> for exporting gridded data from GrADS
or I<r> for importing gridded data into GrADS. All data is exchanged
using the native binary format. 

B<IMPORTANT>. When standard output is selected, the verbose mode is 
automatically turned off to avoid corrupting the output stream.

=back 

=back 

=head2 B<ipc_save> I<EXPR> [I<FILENAME>]

=over 4

This command evaluates the GrADS expression I<EXPR> and saves the
resulting gridded field to a file or to standard output. On input,

=over 4

I<EXPR> is a gridded GrADS expression

I<FILENAME> is an optional parameter specifying a file name to export
the data to, or "-" for exporting the data to standard output. When
I<FILENAME> is specified as a disk file, the file is opened, written
to and closed upon completion. Therefore, I<FILENAME> should not be
specified in the presence of a looping dimension (animation sequence),
or else the file will be overwritten for each instance of the looping
dimension. In such cases, explicity B<open> and B<close> the file
before and after exporting to it.

=back

=back

=head2 B<ipc_close> [I<MODE>]

=over 4

This command closes the streams used for exporting/importing gridded
data. It has no effect when the streams are standard input/output. On input,

=over 4

=item I<MODE> is set to B<w> the file used for exporting data is
closed; when I<MODE> is set to B<r> the file used for importing data
is closed. If omitted, both streams are closed.

=back

=back

=head2 B<ipc_verb> [I<ON|OFF>]

=over 4

This command toggles verbose ON/OFF; do not use it when using
STDIN/STDOUT as transfer streams. If not argument is provided 
it will toggle the verbose mode ON or OFF.

=back

=head2 B<ipc_error>

=over 4

This command reprints the last IPC error message, returning its error code. 
This is useful when exchanging data using bi-directional pipes.

=back

=head1 FUNCTIONS PROVIDED

=head2 define I<void> = B<ipc_save> ( I<EXPR> [, I<FILENAME>] )

=over 4

This function is similar to the B<ipc_save> command above. It is
provided as a function for symmetry with the B<imp> function below,
and to allow exporting a timeseries to a single file using the GrADS
B<define> command. For saving a timeseries, setup the dimension
environment in GrADS as usual for an animation sequence and issue a
C<define> command such as

        define void = ipc_save(sqrt(ua*ua+va*va))

Remember not to specify a disk I<FILENAMNE> as an argument to
B<ipc_save()> when intending to save a timeseries to a single file, or
else your file will contain only the last 2D field in the sequence.

=back

=head2 define C<var> = B<ipc_load> ( [I<FILENAME>] )

=over 4

This function imports gridded data from the currently open stream or
from file I<FILENAME>, if specified. Do not specify I<FILENAMNE> when
intending to import a timeseries from a single file. In such cases,
explicity B<open> and B<close> the file before and after exporting to
it.

=back

=head1 EXAMPLES

Saving/loading simple variables with only 2 varying dimensions (say,
fixed time), specifying a transfer file:

        ga-> ipc_save ps ps.bin
        ga-> define saved = ipc_load("ps.bin") 
        ga-> display ps-saved

The same example, using B<open> and B<close>:

        ga-> ipc_open ps.bin w
        ga-> ipc_save ps
        ga-> ipc_close

        ga-> ipc_open ps.bin r
        ga-> display ps-ipc_load()
        ga-> ipc_close

Saving a timeseries:
 
        ga-> set lon 0 360
        ga-> set lat -90 90
        ga-> set lev 300
        ga-> set t 1 5
        ga-> ipc_open zg.bin w
        ga-> define void = ipc_save(zg)
        ga-> ipc_close 

Retrieving the same time series:

        ga-> set t 1 5
        ga-> ipc_open zg.bin r
        ga-> define saved = ipc_load()
        ga-> display saved 
        ga-> ipc_close 


=head1 TRANSFER FILE FORMAT 

The format of the transfer file borrows from GrADS traditional UDF
transfer files, but it has been simplified a bit since only gridded
fields need to be exchanged. It has also been slightly extended to
allow the saving/loading of a collection of 2D fields to a single
file, as is the case when dealing with timeseries data. In particular,
the first header record with 20 float point numbers used in
traditional UDF files has been eliminated.

Since the argument is always an expression, GrADS will evaluate the
expression and write the result to the transfer file.  Currently only
gridded data is supported, but support of station data is planned for
future releases.  Several records will be written to the file for each
value of the looping dimension (usually time).

=head2 1st record:  The dimension environment record.  

This record contains 20 values, all floating point.  Note that some of
the values are essentially integer, but for convenience they are
written as a floating point array.  Appropriate care should be taken
in converting these values back to integer. The description of each
one of these 20 float point numbers follow:

                 1:  Undefined value for the grid
                 2:  i dimension (idim).  Dimensions are:
                     -1 - None
                      0 - X dimension (lon)
                      1 - Y dimension (lat)
                      2 - Z dimension (lev)
                      3 - T dimension (time)
                 3:  j dimension (jdim).  Note:  if idim and
                     jdim are -1, the grid is a single value.
                     If jdim is -1, the grid is a 1-D grid.
                 4:  number of elements in the i direction (isiz)
                 5:  number of elements in the j direction (jsiz)
                     Array is dimensioned (isiz,jsiz).
                 6:  i direction linear flag.  If 0, the
                     i dimension has non-linear scaling.
                 7:  j dimension linear flag.
                 8:  istrt.  This is the world coordinate value
                     of the first i dimension, ONLY if the i dimension
                     has linear scaling and the i dimension is not
                     time.
                 9:  iincr.  Increment in the i dimension of the
                     world coordinate.  ONLY if the i dimension has
                     linear scaling.
                 10: jstrt.  World coordinate of the first j
                     dimension, only if the j dimension has linear
                     scaling, and the j dimension is not time.
                 11: jincr.  World coordinate increment for j
                     dimension.
                 12: If one of the dimensions is time, values
                     12 to 16 are defined as the start time
                     12 is the start year.
                 13: start month
                 14: start day
                 15: start hour
                 16: start minute
                 17: Values 17 and 18 contain the time increment
                     for the time dimension.  17 contains the
                     increment in minutes.
                 18: increment in months.  (GrADS handles all
                     increments in terms of minutes and months).
                 19,20: reserved

=head2 2nd record:  This contains the gridded data.

It has isiz*jsiz floating point numbers.

=head2 3rd record.  The i-dimension coordinate variable

Whether or not the i or j dimension scaling is non-linear, the world
coordinate values at each integral i(j) dimension value is written.
Thus, isiz float point numbers will be written. Notice that this is a
departure from the classic UDF format.

=head2 4th record.  The j-dimension coordinate variable

This record has the j dimension world coordinate values; it contains
jsiz floating point numbers.

Unlike the classic UDF file format, the 3rd or 4th records are always
written.  Note that the time dimension is ALWAYS linear as currently
implemented in GrADS.

=head1 AUTHOR 

Arlindo da Silva (dasilva@opengrads.org), based on code fragments from GrADS. 

=head1 COPYRIGHT

Copyright (C) 2007 Arlindo da Silva; portions derived from GrADS source 
code Copyright (C) 1988-2007 by Brian Doty and the Institute of Global 
Environment and Society (IGES).  

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif
