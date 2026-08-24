/*
    Copyright (C) 2009 by Arlindo da Silva <dasilva@opengrads.org>
    Modified in 2026 to mark prompt escape sequences non-printing; see COPYING.
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

/* Simple functions to produce color text using ANSI color sequences,
   see:

        http://en.wikipedia.org/wiki/ANSI_escape_code

   The main function used from grads is gatxtl(string,level) which
   colorizes a *string* given a "level" as in gaprnt(), except that
   level=-1 means the prompt.

   The actual colors used dependend on the color scheme specified
   during initialization; see gatxtl() below for the specific colors.
   Usually,

   color scheme          works well with a
   ------------          -----------------
       0                 black background
       1                 white background
       2                 green background

*/


#include <stdio.h>
#include "gatypes.h"

static gaint color_on = 0; /* off by default */
static gaint scheme   = 0;   /* color scheme */

/* Normal colors */
static char *black   = "[30m";
static char *red     = "[31m";
static char *green   = "[32m";
static char *yellow  = "[33m";
static char *blue    = "[34m";
static char *magenta = "[35m";
static char *cyan    = "[36m";
static char *white   = "[37m";
static char *reset   = "[39m";

static char *normal  = "[0m";
static char *bold    = "[1m";

/* Normal colors */
/* static char *Black   = "[90m"; */
/* static char *Red     = "[91m"; */
/* static char *Green   = "[92m"; */
/* static char *Yellow  = "[93m"; */
/* static char *Blue    = "[94m"; */
/* static char *Magenta = "[95m"; */
/* static char *Cyan    = "[96m"; */
/* static char *White   = "[97m"; */

void gatxti(gaint on, gaint cs) {  /* Turn this feature ON/OFF */
  color_on = on;
  if ( cs < 0 ) cs = 0;
  scheme = cs;
}

/* Print ANSI sequence associated with a color name.
   Available options for *nomal* intensite colors are:

          black  
          red    
          green  
          yellow 
          blue   
          magenta
          cyan   
          white  

    Bright colors are specified by capitalizing the first leter,
    e.g., "Red". Specify color=NULL for a reset.

*/

void gatxt(char *color) {
  if ( !color_on ) return;
  if ( color==NULL ) {
    printf("%s",reset);
    return;
  }

  /* Normal */
       if ( color[0]=='b' && 
            color[2]=='a' ) printf("%s",black);
  else if ( color[0]=='r' ) printf("%s",red);
  else if ( color[0]=='g' ) printf("%s",green);
  else if ( color[0]=='y' ) printf("%s",yellow);
  else if ( color[0]=='b' ) printf("%s",blue);
  else if ( color[0]=='m' ) printf("%s",magenta);
  else if ( color[0]=='c' ) printf("%s",cyan);
  else if ( color[0]=='w' ) printf("%s",white);

  else if ( color[0]=='o' ) printf("%s",normal);
  else if ( color[0]=='*' ) printf("%s",bold);

  /* Bright colors */
  else if ( color[0]=='B' && 
            color[2]=='a' ) printf("%s",black);
  else if ( color[0]=='R' ) printf("%s",red);
  else if ( color[0]=='G' ) printf("%s",green);
  else if ( color[0]=='Y' ) printf("%s",yellow);
  else if ( color[0]=='B' ) printf("%s",blue);
  else if ( color[0]=='M' ) printf("%s",magenta);
  else if ( color[0]=='C' ) printf("%s",cyan);
  else if ( color[0]=='W' ) printf("%s",white);

}

static char buffer[256];

/* Map a color name to its ANSI sequence. Bright names share the normal
   sequences because the bright table above is commented out. */
static char *ga_color_sequence(char *color) {

       if ( color[0]=='b' && 
            color[2]=='a' ) return black;
  else if ( color[0]=='r' ) return red;
  else if ( color[0]=='g' ) return green;
  else if ( color[0]=='y' ) return yellow;
  else if ( color[0]=='b' ) return blue;
  else if ( color[0]=='m' ) return magenta;
  else if ( color[0]=='c' ) return cyan;
  else if ( color[0]=='w' ) return white;

  else if ( color[0]=='B' && 
            color[2]=='a' ) return black;
  else if ( color[0]=='R' ) return red;
  else if ( color[0]=='G' ) return green;
  else if ( color[0]=='Y' ) return yellow;
  else if ( color[0]=='B' ) return blue;
  else if ( color[0]=='M' ) return magenta;
  else if ( color[0]=='C' ) return cyan;
  else if ( color[0]=='W' ) return white;

  return NULL;
}

/* Colorize a line editor prompt. \001 and \002 are readline's
   RL_PROMPT_START_IGNORE and RL_PROMPT_END_IGNORE: they tell it the enclosed
   bytes occupy no screen columns, and readline strips them before printing.
   Without them readline counts the escape sequences as visible width and
   miscalculates every redraw, leaving fragments of the previous line behind
   on history recall and cursor movement.

   Only for prompts handed to readline. Anything printed with printf must use
   gatxts, or the \001 and \002 bytes reach the terminal. */
char *gatxtp(char *str, char *color) {
  char *seq;

  if ( !color_on ) return str;
  seq = ga_color_sequence(color);
  if ( seq==NULL ) return str;

  snprintf(buffer,255,"\001%s\002%s\001%s\002",seq,str,reset);
  buffer[255] = '\0';
  return (char *) buffer;
}

char *gatxts(char *str, char *color) { /* colorize the string */
  char *seq;

  if ( !color_on ) return str;
  seq = ga_color_sequence(color);
  if ( seq==NULL ) return str;

  snprintf(buffer,255,"%s%s%s",seq,str,reset);
  buffer[255] = '\0';
  return (char *) buffer;
}

char *gatxtl(char *str, gaint level) { /* colorize according to level */

  if ( scheme==0 ) {
    if (level==-1) return gatxts(str,"Green"); /* prompt */
    if (level==0 ) return gatxts(str,"Red");
    if (level==1 ) return gatxts(str,"magenta");
    if (level==2 ) return gatxts(str,"yellow");
  }
  else if ( scheme==1 ) {
    if (level==-1) return gatxts(str,"Green"); /* prompt */
    if (level==0) return gatxts(str,"Red");
    if (level==1) return gatxts(str,"magenta");
    if (level==2) return gatxts(str,"blue");
  }
  else if ( scheme==2 ) {
   if (level==-1) return gatxts(str,"Blue"); /* prompt */
    if (level==0) return gatxts(str,"black");
    if (level==1) return gatxts(str,"magenta");
    if (level==2) return gatxts(str,"white");
  } 
  return (str);
}

/* Prompt colorizer for the line editor. Same scheme choice as gatxtl(str,-1),
   but with the escape sequences marked non-printing. */
char *gatxtlp(char *str) {

  if ( scheme==0 || scheme==1 ) return gatxtp(str,"Green");
  else if ( scheme==2 )         return gatxtp(str,"Blue");
  return (str);
}
