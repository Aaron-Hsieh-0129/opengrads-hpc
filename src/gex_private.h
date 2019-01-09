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

/* Level 1 API */
typedef struct {
  struct gacmn  *pcm;
  struct gafunc *pfc;
  struct gastat *pst;
  char *result;        /* holds result string */
} gex_t;    


