/*
    gex: API for Enabling Extensions in GrADS v2

    Copyright (C) 2009 by Arlindo da Silva <dasilva@opengrads.org>
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
 
   24Jan2009  da Silva  Initial design.
   24Apr2009  da Silva  Revised.

 */

#include "grads.h"
#include "gex.h"
#include "gex_private.h"

static int pst2var(gastat *pst, var_t *var) {
}

static int var2pst(gastat *pst, var_t *var) {
}



/*
                 -------------------------- 
                  User Defined Extensions
                   Constructor/Destructors
                 -------------------------- 
*/

gexMeta_t * gexMetaCreate  (void *gex, int isiz, int jsiz);
      int   gexMetaDestroy (void *gex, gexMeta_t *meta);

gexVar_t *  gexVarCreate  (void *gex, gexMeta_t meta);
     int    gexVarDestroy (void *gex, gexVar_t *var);


/*
                 -------------------------- 
                  User Defined Extensions
                   User Callable Funtions
                 -------------------------- 
*/

char *gagsdo (char *cmd, gaint *rc);

/*---
  Execute a generic GrADS command */
int gexCmd  (gex_t *gex, char *cmd) {
  int rc;
  gex->result = gagsdo (cmd,&rc); /* just like scripts do */
  return rc;
} 

/*---
  Return ith line of result, based on gsflin() */
char * gexLine (gex_t *gex, int lnum, int *rc) {
  char *ch, *res;
  integer i, len, ret;

  /* Find the desired line in the string */
  ch = gex->result;
  i = 1;
  while (*ch && i<lnum) {
    if (*ch=='\n') i++;
    ch++;
  }

  /* Get length of returned line. */
  len = 0;
  while (*(ch+len)!='\0' && *(ch+len)!='\n') len++;

  /* Allocate storage for the result */
  res = (char *)malloc(len+1);
  if (res==NULL) {
    printf ("Error:  Storage allocation error\n");
    ret = 1;
    goto retrn;
  }

  /* Deliver the result and return */
  for (i=0; i<len; i++) *(res+i) = *(ch+i);
  *(res+len) = '\0';

  ret = 0;

 retrn:
  *rc = ret;
  return res;
}

/*---
  Return jth word of ith line of result, based on gsfwrd() 
*/
char * gexWord (gex_t *gex, int lnum, int j, int *rc) {
  char *ch, *res, *line;
  integer i, len, ret;

  line = gexLine(gex,lnum,&ret);
  if (ret) {
    res = (char *) NULL;
    goto retrn;
  }

  /* Find word */
  i = 0;
  while (*ch) {
    if (*ch==' '||*ch=='\n'||*ch=='\t'||i==0) {
      while (*ch==' '||*ch=='\n'||*ch=='\t') ch++;
      if (*ch) i++;
      if (i==wnum) break;
    } else ch++;
  }

  /* Get length of returned word. */
  len = 0;
  while (*(ch+len)!='\0' && *(ch+len)!=' '
         && *(ch+len)!='\t' && *(ch+len)!='\n') len++;

  /* Allocate storage for the result */
  res = (char *)malloc(len+1);
  if (res==NULL) {
    printf ("Error:  Storage allocation error\n");
    ret = 1;
    goto retrn;
  }

  /* Deliver the result and return */
  for (i=0; i<len; i++) *(res+i) = *(ch+i);
  *(res+len) = '\0';

  ret = 0;

  /* Release storage and return */
retrn:
  if (line) free(line);
  *rc = ret;
  return (res);
}

/*---
  Evaluate GrADS expression, returning variable 
*/
int gexEval (gex_t *gex, char *expr, var_t *var) {
  int rc;
  rc = gaexpr(expr, gex->pst);
  var = (var_t *) NULL;
  return 0;  
}

/*---
  Define a variable 
*/
int gexDef (gex_t *gex, char *name, var_t *var) {
  return 0;
}
