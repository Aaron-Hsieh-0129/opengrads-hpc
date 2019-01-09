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

*/

#include <stdio.h>
#include "grads.h"

static counter = 0; /* Illustrates how to keep a state */

/* Sample Hello, World! user defined command */

int c_hello ( int argc, char **argv, struct gacmn *pcm) {
    int i;

    counter++;
    printf("[%d] Hello, GrADS v2.0 World! \n", counter);
    for (i=0; i<argc; i++)
    printf("    argv[%d] = <%s>\n", i, argv[i]);
    return 0;

}

int f_hello (struct gafunc *pfc, struct gastat *pst) {

  int rc;

  counter++;
  printf("[%d] Hello, GrADS v2.0 World!\n", counter);
  rc = gaexpr("0",pst);                    /* return a zero grid  */
  return (rc);

}


