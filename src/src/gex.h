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

#define GEX_GRID     1
#define GEX_STATION  2
#define GEX_GAMAJOR  2

/*
                 -------------------------- 
                     Public Data Types
                 -------------------------- 
*/

#if GEX_GAMAJOR == 2
typedef double gaData_t;  /* type of variable data */
#else
typedef float  gaData_t;  /* type of variable data */
#endif

/* Variable index-space metadata */
   typedef struct {
     int idim, jdim; /* dimensions associated with each axis */
     int isiz, jsiz; /* axis sizes */
     int iflg, jflg; /* linear scaling flag or each axis */
     int x[2], y[2], z[2], t[2], e[2];    /* index ranges */
     gaData_t *lat, *lon, *lev, *ens;     /* world coordinates   */
     char     *time[6];                   /* [cc,yy,mm,dd,hh,nn] */
   } gexMeta_t;

/* Variable data and metadata */
   typedef struct {
     int       type;   /* Variable type: grid of station */
     int       size;   /* size of data/mask */
     gaData_t  *data;  /* data array */
     char      *mask;  /* undef mask */
     gexMeta_t *meta;  /* variable metadata data */
   } gexVar_t;


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

/* Execute a generic GrADS command */
int gexCmd  (void *gex, char *cmd); 
int gexLine (void *gex, int i, char *line); 
int gexWord (void *gex, int i, int j, char *word); 

/* Evaluate GrADS expression, returning variable */
//int gexEval (void *gex, char *expr, var_t *var);

/* Define a variable */
//int gexDef (void *gex, char *name, var_t *var);
