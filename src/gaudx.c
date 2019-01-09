/*
    gaudx: OpenGrADS User Defined Extensions for GrADS v2

    Copyright (C) 1997-2009 by Arlindo da Silva <dasilva@opengrads.org>
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

---

   REVISION HISTORY:
   23Aug2008  da Silva  Derived from v1.9.0-rc1 implementation.


 */

#ifdef __nestedvm__

/* Not yet supported under Java */
#include <stdio.h>
int gaudi (void *pcm) { return 0; }
int   gaudc (char *com ) {return -1; }              /* Execute a UDC */
void* gaudf (char *name) { return (void *) NULL; }  /* Return pointer to UDF handler */

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>  /* For dlopen, etc */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "grads.h"
#include "gaudx.h"

/* External Utilities Data */
char * nxtwrd(char * ch);

/* Private Data */

static char pout[256];   /* Build error msgs here */
static char Com[1024];
static char Cmd[1024];

static      gaudx_t *Udx =      (gaudx_t *) NULL; 
static struct gacmn *Pcm = (struct gacmn *) NULL;

/* Private Function prototypes */

static gaudx_t * gaudx_create  (char *filename);
static void      gaudx_destroy (gaudx_t *self);

static gaudx_t * gaudx_find    (gaudx_t *self, char *name, int which);
static void      gaudx_summary (gaudx_t *udx, int which, int fmt);
static void      gaudx_query   (gaudx_t *self, int fmt);
static void      gaudx_append  (gaudx_t *self, gaudx_t *another);
static int       gaudx_isfun   (gaudx_t *self, char *name);
static int       gaudx_iscmd   (gaudx_t *self, char *name);

static int       gaudx_cmd     (gaudx_t *self, char *com, struct gacmn  *pcm);
static int       gaudx_fun     (gaudx_t *self, char *name, 
                                struct gafunc *pfc, struct gastat *pst);

static int       gaudx_frun    (gaudx_t *self, 
                                struct gafunc *pfc, struct gastat *pst);
static void    * gaudx_fptr    (gaudx_t *self, char *name );

static int getargv(char *str, char ** const argv, int max_argc); /* parser */

#define STRCPY(dst,src) \
  { \
	dst = (char *) malloc ( strlen( src ) + 1 );\
        if (! dst ) goto memerr;\
	strcpy( dst , src ); \
  }

/* ----------------------------------------------------------------
                             ----------------
                             Public Functions
                             ----------------

 Note: The actual implementation of this package is done in a thread save
       manner (see OO'ish functions below). However, to simplify the
       interface to the main GrADS code, the public interface relies on
       a single instance of the *udx* object kept as a static variable.

 To do:
       - Implement API level 1

*/

/* -- Initialize User Defined Extensions -- */
int gaudi (struct gacmn *pcm) {
  Pcm = pcm; /* save this do that we can run commands */
  if ( !Udx ) Udx = gaudx_create(NULL);
}

/* -- Execute a User Defined Command -- */
int gaudc (char *com_) {
  gaudx_t *udx_new;
  char *cmd, *com;
  int i, n;

  /* Just return when we get a blank line */
  if ( !com_ ) return -1;

  /* Check for all spaces */
  getstr(com=Com,com_,1024); /* handles blank line */
  if ( strlen(com)==0 ) return -1;

  /* case insensitive version of input string */
  strncpy(cmd=Cmd,com_,1023); cmd[1023]='\0';
  lowcas(cmd);

  /* Handle UDX related commands */
  if (cmpwrd("load",cmd)) {  /* Load UDX table on CLI */
    cmd= nxtwrd(cmd);
    if ( !cmd ) return -1;
    if (cmpwrd("udxt",cmd) ){
        com = nxtwrd(com);
        com = nxtwrd(com); /* preserve case for file name */
        if ( com ) {
          for(i=strlen(com)-1; i>0; i++) {
            if (com[i]!=' ') break;
            else             com[i]='\0';
          }
          udx_new = gaudx_create(com); /* load table */
          if ( udx_new ) { 
            if ( !Udx ) Udx = udx_new;             /* new table */
            else        gaudx_append(Udx,udx_new); /* append it */
            return (0);
          } else {
            sprintf(pout,"ERROR: could not load UDX table <%s>\n",com);
            gaprnt(0,pout);
            return (1);  /* failed to load file */
          }
        } else {
          gaprnt(0,"ERROR: missing UDX table file name\n");
          return (2);      /* file name missing */
        } 
    }
  }

  /* Stop here if no UDX table has been loaded */
  if ( !Udx ) return (-1);


  if (cmpwrd("q",cmd)||cmpwrd("query",cmd)) {
    cmd = nxtwrd(cmd);
    if ( !cmd ) {
      gaprnt (2,"\nFor information on User Defined Extensions:\n");
      gaprnt (2,"  q udc      Returns list of User Defined Commands\n");
      gaprnt (2,"  q udf      Returns list of User Defined Functions\n");
      gaprnt (2,"  q udx      Returns list of User Defined Commands & Functions\n\n");
      return (-1);
    }
    else if (cmpwrd("udx",cmd) ){
      gaudx_query(Udx,2);
      gaprnt (2,"\n");
      return (0);
    }
    else if (cmpwrd("udxt",cmd) ){
      gaudx_query(Udx,1);
      gaprnt (2,"\n");
      return (0);
    }
    else if (cmpwrd("udc",cmd) ){
      gaudx_summary(Udx,UDX_CMD,2);
      gaprnt (2,"\n");
      return (0);
    }
    else if (cmpwrd("udf",cmd) ){
      gaudx_summary(Udx,UDX_FUN,2);
      gaprnt (2,"\n");
      return (0);
    }
  }

  /* Now, handle user defined extensions */
  return gaudx_cmd(Udx,com,Pcm);            

}

/* -- This is the UDF handler; it will perform any necessary setup before
      calling user's code. Notice that this function should *not* be called
      directly but rather through a function pointer returned by gaudf(). -- */

int gaudf_ (struct gafunc *pfc, struct gastat *pst) {
  /* Check if initialized */
  if ( !Udx ) {
    gaprnt(0,"gaudf_: called but UDX package has not been initialized");
    return -99;
  }
  return (int) gaudx_frun(Udx,pfc,pst);
}

/* -- Return pointer to handler of UDF 'name' -- */
void* gaudf (char *name) {                
  gaudx_t *udx_;
  int (*fpntr)(struct gafunc *, struct gastat *);  
  /* Check if initialized */
  if ( !Udx ) return (void *) NULL; 
  if ( udx_=gaudx_find(Udx,name,UDX_FUN) ) {
    Udx = udx_;
    fpntr = gaudf_;
  }
  else                              
    fpntr = (void *) NULL; 
  return (void *) fpntr;
}

/* -------------------------------------------------------------------------

                   -----------------------
                       Private Functions
                        OO'ish Interface
                   ------------------------

*/

/* -- Constructor --  */
static gaudx_t * gaudx_create(char *filename) {

  gaudx_t *udx      = (gaudx_t *) NULL;
  gaudx_t *udx_prev = (gaudx_t *) NULL;

  char *cname;
  FILE *cfile;
  char rec[260], *str;
  int argc;
  char *argv[7]; /* no more then 7 tokens per line on UDX Table*/
  char *unknown = "<Unknown>";

  /* If file name is not given, look for GAUDXT env variable */
  if ( filename ) cname = filename;
  else            cname = getenv("GA2UDXT");

  /* Got a file name to work with ... */
  if (cname) {

    /* Open the file */
    sprintf (pout,"Loading User Defined Extensions table <%s> ... ",cname);
    gaprnt (2,pout);
    cfile = fopen(cname,"r");
    if (cfile) {
      gaprnt (2,"ok.\n");
    } else {
      gaprnt(0,"failed.\n");
      return (gaudx_t *) NULL;
    }

    /* Read the file. */
    while (1) {

      /* Read record: <type> <api> <cmd_name> <func_name>  <dll_file_name> */

      str = fgets(rec,256,cfile);
      if (str==NULL) break;
      while ( *str=='#' ) {
              str = fgets(rec,256,cfile);
              if (str==NULL) break;
      }
      rec[strlen(rec)-1] = '\0';
      if (str==NULL) break;

      argc = getargv(str, argv, 7);
      if ( argc == 0 ) continue; /* blank line */
      if ( argc < 5 ) goto fmterr;

      lowcas(argv[0]); /* case insensitive */

      /* Handle user defined commands/functions only */      
      if ( cmpwrd(argv[0],"udc") || cmpwrd(argv[0],"udc*") ||
           cmpwrd(argv[0],"udf") || cmpwrd(argv[0],"udf*") ) {

	udx = (gaudx_t *) malloc(sizeof(gaudx_t));
	if (udx==NULL) goto memerr;

        /* Command or function? */
        if ( cmpwrd(argv[0],"udc") || cmpwrd(argv[0],"udc*") ) 
	     udx->type = UDX_CMD;
	else if ( cmpwrd(argv[0],"udf") || cmpwrd(argv[0],"udf*") ) 
	     udx->type = UDX_FUN;
        else
             udx->type = UDX_UNKNOWN;

        if ( cmpwrd(argv[1],"0") ) udx->api = UDX_LOWLEVEL;
        else                       udx->api = UDX_UNKNOWN;

	STRCPY(udx->name ,argv[2]);
	STRCPY(udx->func ,argv[3]);
	STRCPY(udx->flib ,argv[4]);

        /* Handle ^/$ in DLL file name */
        if ( argv[4][0]=='^' || argv[4][0]=='$' ) {
          udx->fname = (char *) malloc (strlen(argv[4])+strlen(cname)+1);
          if ( !udx->fname ) goto memerr;
          fnmexp(udx->fname,argv[4],cname);
        } else
          STRCPY(udx->fname,argv[4]);

	if ( argc > 5) {
             STRCPY(udx->descr,argv[5]);
        } else      udx->descr = unknown;

	udx->handle = (void *) NULL;  /* load on first usage */
	udx->cpntr  = (void *) NULL;  /* load on first usage */
	udx->fpntr  = (void *) NULL;  /* load on first usage */
	
        /* Whether DLL goes in and out of scope each time */
        if ( argv[0][3] == '*'  )   udx->mode = UDX_VOLATILE; 
        else                        udx->mode = UDX_PERSIST; 
	
	lowcas(udx->name);  /* grads is case insensitive */
      	
	/* Update linked list */
	udx->next = (gaudx_t *) NULL;
        if ( !udx_prev ) {  /* first time around */
	  udx->root = udx;
        } else {
	  udx->root = udx_prev->root;
	  udx_prev->next = udx;
        }
	udx_prev = udx;
      }

    }
    fclose (cfile);
  
  }

  return udx;

memerr:
  gaprnt(0,"Memory allocation error: user defined extensions\n");
  return (gaudx_t *) NULL;

fmterr:
  gaprnt(0,"Format error in user defined command table:\n");
  sprintf (pout,"  Processing function name: <%s>, argc = %d \n",str,argc);
  gaprnt (0,pout);
  free (udx);
  goto wname;

wname:
  sprintf (pout,"  File name is: %s\n",cname);
  gaprnt (0,pout);
  return (gaudx_t *) NULL;

}

/* ......................................................................... */

/* -- Appends udx_b to the end of udx_a -- */
static void gaudx_append(gaudx_t *udx_a, gaudx_t *udx_b ) {
  gaudx_t *root, *tail;

  if ( !udx_b ) return;  /* nothing to append */
  if ( !udx_a ) {        /* udx_a is empty, replace it with udx_b */
    udx_a = udx_b;
    return;
  }

  /* Find end of destination linked-list "udx_a" */
  root = (gaudx_t *) udx_a->root; 
  udx_a = root;
  while (udx_a) {
    if ( !udx_a->next ) break;
    udx_a = (gaudx_t *) udx_a->next;
  }

  /* Chain linked list udx_b to the end of udx_a */
  udx_a->next = (gaudx_t *) udx_b->root;

  /* correct the root of udx_b nodes */
  
  udx_b = (gaudx_t *) udx_b->root; 
  while(udx_b) { 
    udx_b->root = root;
    udx_b = (gaudx_t *) udx_b->next;
  }

}

/* ......................................................................... */

/* -- Query method --  */
static void gaudx_query (gaudx_t *udx, int fmt) {

  gaudx_summary(udx, (int)UDX_CMD,  fmt);
  gaudx_summary(udx, (int) UDX_FUN, fmt);

}

static void gaudx_header(char *descr, int which,int fmt) {
  /* Header */
  int i, n;
  char title[512];
  if (fmt==2) {
    n = (40 - strlen(descr) ) / 2;
    if ( n<0 ) n = 0;
    title[0] = '\0';
    for (i=0; i<n; i++) title[i] = ' ';
    title[i] = '\0';
    strcat(title,descr);
    if ( which == UDX_CMD ) {
      sprintf(pout,"\n   Command   %-40s     Function@Library\n",title);
      gaprnt (2,pout);
    }
    else if ( which == UDX_FUN ) {
      sprintf(pout,"\n  Function   %-40s     Function@Library\n",title);
      gaprnt (2,pout);
    }
    gaprnt (2,  " ----------- ---------------------------------------  --------------------------\n");
  }

}


static void gaudx_summary (gaudx_t *udx, int which, int fmt) {

  udx = (gaudx_t *) udx->root;
  while (udx) {
    if ( udx->type == which ) {
      if ( fmt==1 ) {
        sprintf (pout," %12s -> %-12s from <%s>\n",
                 udx->name, udx->func, udx->fname);
        gaprnt (2,pout);
      } else {
        if ( udx->name[0]==' ' ) {
          gaudx_header(udx->descr,which,fmt);
        } else {
          sprintf (pout," %-11s %-40s %s@%s\n",
                   udx->name, udx->descr, udx->func, udx->flib);
          gaprnt (2,pout);
        }
      } 
    }
    udx = (gaudx_t *) udx->next;
  }

}

/* ........................................................................ */

/* Check whether an extention exists; returns UDX_CMD, UDX_FUN or -1 (error) */

static gaudx_t * gaudx_find(gaudx_t *udx, char *name, int which) {
  udx = (gaudx_t *) udx->root;
  while (udx) { 
    if ( cmpwrd(udx->name,name) && udx->type==which ) return udx;
    udx = udx->next;
  }
  return (gaudx_t *) NULL;
}

static int gaudx_isit(gaudx_t *udx, char *name, int which) {
  udx = (gaudx_t *) udx->root;
  while (udx) { 
    if ( cmpwrd(udx->name,name) && udx->type==which ) return 1;
    udx = udx->next;
  }
  return 0;
}

static int gaudx_isfun(gaudx_t *udx, char *name) {
  return gaudx_isit(udx,name,UDX_FUN);
}

static int gaudx_iscmd(gaudx_t *udx, char *name) {
  return gaudx_isit(udx,name,UDX_CMD);
}

/* ........................................................................ */

/* -- Run method: execute the user defined command, if available --*/

static int gaudx_cmd (gaudx_t *udx, char *com, struct gacmn *pcm ) {

    int rc;
    char *error, *cmd;
    int argc;
    char *argv[255];

    /*  Parse command line arguments */
    cmd = strdup(com);
    argc = getargv(cmd,argv,255); /* parse command line */

    /* Look for the UDC */
    udx = gaudx_find(udx,argv[0],UDX_CMD);
    if ( !udx ) goto err;
    if ( udx->type != UDX_CMD ) goto err;

    /* Load the DLL if it is not already loaded */
    if ( !udx->handle ) {
      udx->handle = dlopen (udx->fname, RTLD_LAZY);
      if (!udx->handle) {
	sprintf (pout, "%s\n", dlerror());
	gaprnt (0,pout);
	goto err;
      }
    }

    /* Get function pointer, if nedded */
    if ( !udx->cpntr ) {
      dlerror();    /* Clear any existing error */
      *(void **) (&udx->cpntr) = dlsym(udx->handle, udx->func);
      if ((error = dlerror()) != NULL)  {
	sprintf (pout, "%s\n", error);
	gaprnt (0,pout);
	goto err;
      }
    }

    /* run the command */
    rc = (udx->cpntr)(argc,argv,pcm);

    /* When the function is volatile, release the DLL */
    if ( udx->mode == UDX_VOLATILE ) {
      dlclose(udx->handle);
      udx->handle = (void *) NULL;
      udx->cpntr  = (void *) NULL;
    }

    /* All done */
    free(cmd);
    return rc;

 err:
  return -1;

}

/* -- Executes an UDF; must first call gaudx_find() to position pointer -- */

static int gaudx_frun(gaudx_t *udx, struct gafunc *pfc, struct gastat *pst) {

    int rc;
    char *error;

    /* Make sure udx is in a proper state */
    if ( !udx )                 return -2;
    if ( udx->type != UDX_FUN ) return -3;

    /* Load the DLL if it is not already loaded */
    if ( !udx->handle ) {
      udx->handle = dlopen (udx->fname, RTLD_LAZY);
      if (!udx->handle) {
	sprintf (pout, "%s\n", dlerror());
	gaprnt (0,pout);
	return -4;
      }
    }

    /* Get function pointer, if nedded */
    if ( !udx->fpntr ) {
      dlerror();    /* Clear any existing error */
      *(void **) (&udx->fpntr) = dlsym(udx->handle, udx->func);
      if ((error = dlerror()) != NULL)  {
	sprintf (pout, "%s\n", error);
	gaprnt (0,pout);
	return -5;
      }
    }

    /* Execute the function */
    if ( udx->api == UDX_LOWLEVEL ) 
      if ( pfc->argnum<20 ) { /* This 20 is hardwired in grads.h */
        pfc->argpnt[pfc->argnum] = udx->name; /* add function name */
        rc = (udx->fpntr)(pfc,pst);
      }
      else rc = -6; /* too many args */

    else
      rc = -7; /* Only low-level API for now */

    /* When the function is volatile, release the DLL */
    if ( udx->mode == UDX_VOLATILE ) {
      dlclose(udx->handle);
      udx->handle = (void *) NULL;
      udx->fpntr  = (void *) NULL;
    }

    /* All done */
    return rc;
}

/* Obsolete - scheduled for removal */
#if 0
/* Load and run the UDF */
static int gaudx_fun (gaudx_t *udx, char *name,
                      struct gafunc *pfc, struct gastat *pst) {

    int rc;
    char *error;

    /* Look for the UDF if a name is given */
    if ( name ) udx = gaudx_find(udx,name,UDX_FUN);
    if ( !udx ) goto err;
    if ( udx->type != UDX_FUN ) goto err;

    /* Load the DLL if it is not already loaded */
    if ( !udx->handle ) {
      udx->handle = dlopen (udx->fname, RTLD_LAZY);
      if (!udx->handle) {
	sprintf (pout, "%s\n", dlerror());
	gaprnt (0,pout);
	goto err;
      }
    }

    /* Get function pointer, if nedded */
    if ( !udx->fpntr ) {
      dlerror();    /* Clear any existing error */
      *(void **) (&udx->fpntr) = dlsym(udx->handle, udx->func);
      if ((error = dlerror()) != NULL)  {
	sprintf (pout, "%s\n", error);
	gaprnt (0,pout);
	goto err;
      }
    }

    /* run the command */
    rc = (udx->fpntr)(pfc,pst);

    /* When the function is volatile, release the DLL */
    if ( udx->mode == UDX_VOLATILE ) {
      dlclose(udx->handle);
      udx->handle = (void *) NULL;
      udx->fpntr  = (void *) NULL;
    }

    /* All done */
    return rc;

 err:
    return -2;
}
#endif

/* ......................................................................... */

/* -- Destructor -- */
static void gaudx_destroy(gaudx_t *udc) {
  gaudx_t *next;
  next = (gaudx_t *) udc->root;
  while (next) {
    udc  = next;
    next = (gaudx_t *) udc->next;
    if ( udc->handle ) dlclose(udc->handle);
    free(udc);
  }
}


/*
                   ---------------------
                     Private Utilities
                   ---------------------

 NOTE: These could go in gautil.c

*/


/* check for a whitespace character (\\t, \\n, \\v, \\f, \\r) */
#define IsSpace(ch) ((unsigned int)(ch - '\t') < 5u || ch == ' ')

/*ams

    Given a command line string, generate argc, **argv; allowing for 
   quoted arguments.  

   Example:
 
      str: print -o '|ggv -'                     
    gives: argc = 3
           argv[0] = print
           argv[1] = -o
           argv[2] = |ggv -

    Note: *str is modified with NULL added for delimiters; make a
          a copy first if you want to preserve it.

ams*/

static int getargv(char *str, char ** const argv, int max_argc)
{
	int i, argc = 0;
        char del;
	for (i = 0; i < max_argc; i++) argv[i] = NULL;
	if (!str) return 0;
	while (*str) {
		while (IsSpace(*str)) *(str++) = '\0';
		if (*str && argc < max_argc - 1) {
			if ( *str=='"' || *str=='\'' ) {
                               del=*str;
                               str++;
                        } else del=' ';

                        if ( *str ) {
			  argv[argc] = str;
			  argc++;
			}
                        if ( del==' ' )
			  while (*str && !IsSpace(*str)) str++;
                        else { 
			  while (*str && *str!=del) str++;
			  if ( *str ) *str = ' ';
                        }
		}
		else
			break;
	}
	return argc;
}


/* ----------------------------------------------------------------- 

              GrADS v1.x Compatibility Functions
               
*/

/* Wrapper around gaexpr() to return data with undef values based 
   on mask (just like in GrADS v1.x) */
int gex_expr1(char *expr, struct gastat *pst) {
  int rc, i;
  struct gagrid *pgr;
  struct gastn  *stn;
  struct garpt  *rpt;

  /* evaluate the expression */
  rc = gaexpr(expr,pst);
  if(rc) return(rc);

  /* Set gridded data undef */
  if(pst->type==1) {
    pgr = pst->result.pgr;
    for (i=0; i<(pgr->isiz*pgr->jsiz); i++ ) 
       if ( ! pgr->umask[i] ) pgr->grid[i] = pgr->undef;

  /* Set station data undef */
  } else {
    stn = pst->result.stn;
    rpt = stn->rpt;
    while(rpt) {
      if(!rpt->umask) rpt->val = stn->undef;
      rpt=rpt->rpt;
    }
  }
  return 0;
}


/* Make sure output result has mask properly set */

int gex_setUndef (struct gastat *pst) {
  int rc, i;
  struct gagrid *pgr;
  struct gastn  *stn;
  struct garpt  *rpt;

  /* Set mask for gridded data */
  if(pst->type==1) {
    pgr = pst->result.pgr;
    if ( !pgr->umask ) {    /* mak sure this is allocated */
      pgr->umask = (char *) malloc(sizeof(char) * pgr->isiz*pgr->jsiz); 
      if (!pgr->umask ) return 1;
    }
    for (i=0; i<(pgr->isiz*pgr->jsiz); i++ ) 
      if ( pgr->grid[i] == pgr->undef ) pgr->umask[i] = 0;
      else                              pgr->umask[i] = 1;
  }

  /* Set mask for station data */
  else {
    stn = pst->result.stn;
    rpt = stn->rpt;
    while(rpt) {
        if ( rpt->val == stn->undef ) rpt->umask = 0;
        else                          rpt->umask = 1;
      rpt=rpt->rpt;
    }
  }
  return 0;
}

/* ----------------------------------------------------------------- */

void gex_setPDF (struct gastat *pst) {
    pst->pdf1 = Pcm->pdf1;  /* so that newly defined var can be found */
}

/* ----------------------------------------------------------------- */

#endif
