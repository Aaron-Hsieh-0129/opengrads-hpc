/*
    gaudx: OpenGrADS User Defined Extensions for GrADS v2

    Copyright (C) 1997-2007 by Arlindo da Silva <dasilva@opengrads.org>
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
   24Dec2009  da Silva  Further improvements: added API level, simplified
                        interface to maon GrADS code.
 */

#define UDX_UNKNOWN  -1
#define UDX_CMD       1  /* Commands */
#define UDX_FUN       2  /* Functions */

#define UDX_VOLATILE 1  /* the DLL is released after function execution */
#define UDX_PERSIST  2  /* the DLL is not closed after execution */

#define UDX_LOWLEVEL 0  /* UDF uses Low level API */

/* User Defined Extensions */
   typedef struct 
   {
     void *root;      /* Root node of linked list */
     void *next;      /* Next node in linked list */

     int   api;       /* UDF API Level: 0=expert */
     char *name;      /* Extension name */
     int   type;      /* Extension type: UDX_CMD or UDX_FUN */

     char *func;      /* Function name in library, usually same as name */
     char *fname;     /* file name for DLL implementing the user function */
     char *flib;      /* Short file name for DLL */

     void *handle;    /* DLL handle */
     int   mode;      /* UDX_PERSIST or UDX_VOLATILE */

     char *descr;     /* Short description */

                      /* function pointer - notice standard prototypes */

     /* Low-level API: 0 (legacy, for developers only) */
     int (*cpntr)(int argc, char **argv, struct gacmn *pcm);  /* UDCs */
     int (*fpntr)(struct gafunc *, struct gastat *);          /* UDFs */

     /* User-level API: 1 */
     int (*cpntr1)(void *gex, int argc, char **argv);  /* UDCs */
     int (*fpntr1)(void *gex, int argc, char **argv);  /* UDFs */

   } gaudx_t;

/* Public Function prototypes */
int   gaudc (char *com ); /* Execute a UDC */
void* gaudf (char *name); /* Return pointer to UDF handler */

/* Level 1 API */


