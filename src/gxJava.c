/*

    Copyright (C) 1997-2008 by Arlindo da Silva <dasilva@opengrads.org>
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

/*  This file implements an interface to Java under NextedVM. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XStandardColormap  void
#define XColor             void

#include "gatypes.h"
#include "gx.h"

// ........................................................................

#define GXRDRW  0;
#define GXDBGN  1;
#define GXDCOL  2;
#define GXDACL  3;
#define GXDWID  4;
#define GXDFRM  5;
#define GXDREC  6;
#define GXDFIL  7;
#define GXDDRW  8;
#define GXDMOV  9;
#define GXDEND 10;
#define GXDBAT 11;
#define GXDSGL 12;
#define GXDDBL 13;
#define GXDSWP 14;
#define GXDXSZ 15;
#define GXRDRW 16;

// ........................................................................

/* These are NextedVM functions for interfacing to the Java subsystem */
extern void _pause();
extern int _call_java(gaint a, gaint b, gaint c, gaint d);

static gaint batch = 0;       // whether we are in batch mode or not
static gaint unused=-99;
static gaint flag=0;          // whether repaintng is needed

// ........................................................................

/* Device interface level.  Following routines need to be
   interfaced to the hardware:

   gxdbgn - Initialize graphics output.  Set up any hardware
            scaling needed, clear the graphics display area,
            etc.
   gxdcol - Set hardware color.  The colors should be set up
            as follows:
            0 - black;   1 - white    2 - red     3 - green
            4 - blue     5 - cyan     6 - magenta 7 - yellow
            8 - orange   9 - purple  10 - yell-grn  11 - lt.blue
           12 - ora.yell 3 - blu-grn 14 - blu-purp  15 - grey
            If colors are not available then grey scales can
            be used, or the call can be a no op.
   gxdwid - Set hardware line weight.
   gxdfrm - New frame.  If in single buffer mode, clear the active
            display.  If in double buffer mode, clear the background
            buffer.
   gxdrec - Draw a color filled rectangle.
   gxddrw - Draw a line using current attributes
   gxdmov - Move to a new point
   gxdend - Terminate graphics output.

*/

// Simple routine to set/retrieve status flags

gaint gxflag(int mode) __attribute__((section(".text")));
gaint gxflag(int flag_) {
  int rc = flag;
  flag = flag_;  // reset flag value
  return rc;     // return previous value of flag
}

// Attempting to set environment variables
void setenv_(char *var,char *val) __attribute__((section(".text")));
void setenv_(char *var,char *val) { 
  // printf("Setting %s=<%s>\n",var,val);
  setenv(var,val,1); 
}


/*
                       -----------------------------------
                       Level 1: Basic graphics, no widgets 
                       -----------------------------------
*/

/*
   gxdbgn - Initialize graphics output.  Set up any hardware
            scaling needed, clear the graphics display area,
            etc.
*/
void gxdbgn(gadouble xsz, gadouble ysz) { 
  gaint func = GXDBGN;
  gafloat args[2];
  if ( !batch ) {
    args[0] = (gafloat) xsz;
    args[1] = (gafloat) ysz;
    _call_java(func,(gaint) &args,unused,unused);
  }
}

void gxdxsz(gaint w, gaint h) {
  gaint func = GXDXSZ;
  if ( !batch ) {
    _call_java(func,w,h,unused);
  }
}

void gxrdrw(gaint flag) {   
  gaint func = GXDCOL;
  if ( !batch ) _call_java(func,flag,unused,unused);
}

void gxdcol(gaint clr) {
   gaint func = GXDCOL;
   if ( !batch ) _call_java(func,clr,unused,unused);
} 

gaint gxdacl(gaint clr, gaint red, gaint green, gaint blue) {
  gaint func = GXDACL;
  gaint args[4] = {clr, red, green, blue};
  if ( !batch ) return _call_java(func,(gaint) &args,unused,unused);
  else return 0;
} 

void gxdwid(gaint wid) {
  gaint func = GXDWID;
  if ( !batch ) _call_java(func,wid,unused,unused);
}

void gxdmov(gadouble x, gadouble y) {
  gaint func = GXDMOV;
  gafloat args[2];
  if ( !batch ) {
    args[0] = (gafloat) x;
    args[1] = (gafloat) y;
    _call_java(func,(gaint) &args,unused,unused);
  }
} 

void gxddrw(gadouble x, gadouble y) {
  gaint func = GXDDRW;
  gafloat args[2];
  if ( !batch ) {
    args[0] = (gafloat) x;
    args[1] = (gafloat) y;
    _call_java(func,(gaint) &args,unused,unused);
  }
} 

void gxdrec(gadouble x1, gadouble x2, gadouble y1, gadouble y2) {
  gaint func = GXDREC;
  gafloat args[4];
  if ( !batch ) {
    args[0] = (gafloat) x1;
    args[1] = (gafloat) x2;
    args[2] = (gafloat) y1;
    args[3] = (gafloat) y2;
    _call_java(func,(gaint) &args,unused,unused);
  }
}

void gxdfil(gadouble *xy, gaint n) {
  gaint func = GXDFIL;
  gafloat XY[1024]; // should be large enough for most cases
  gafloat *xy_f;
  gaint i;
  if ( !batch ) {
    if ( 2*n > 1024 ) {
      xy_f = (gafloat *) malloc(sizeof(gafloat) * 2 * n);
    } else {
      xy_f = XY;
    }
    for ( i=0; i<2*n; i++ ) {
      xy_f[i] = (gafloat) xy[i];
    }
    _call_java(func,(gaint) xy_f,n,unused);
    if ( 2*n > 1024 ) free(xy_f);
  }    
}

void gxdfrm(gaint iact) {
  gaint func = GXDFRM;
  _call_java(func,iact,unused,unused);
}

/*
   gxdsgl - Initiate single buffer mode.  Normal mode of operation.
   gxddbl - Initiate double buffer mode, if available.  Both the
            foreground and background displays should be cleared.
   gxdswp - Swap buffers when in double buffer mode.  Should take
            no action if in single buffer mode.
*/

void gxdsgl(void) {  
   gaint func = GXDSGL;
  _call_java(func,unused,unused,unused);
}
void gxddbl(void) {
   gaint func = GXDDBL;
  _call_java(func,unused,unused,unused);
}
void gxdswp(void) {
   gaint func = GXDSWP;
  _call_java(func,unused,unused,unused);
}

void gxdbat(void) { 
   gaint func = GXDBAT;
   batch = 1;
  _call_java(func,unused,unused,unused);
}  // indicates that we are running in batch mode

// .........................................................................................

/*
                       -------------------------
                       Level 2: widgets and such  
                       -------------------------
*/


void gxwdln(void) {}
void gxdgeo(char *arg) {}
void gxdbtn(gaint flag, gadouble *xpos, gadouble *ypos, gaint *mbtn, gaint *type, 
            gaint *info, gadouble *rinfo) {}
void gxgrey(gaint flag) {}
void gxdend(void) {}
void gxdeve(gaint flag) {}
gaint gxbcol(XStandardColormap *best, XColor *cell) {}
void dump_back_buffer(char *filename) {}
void dump_front_buffer(char *filename) {}
gaint gxqfil(void) {}
void gxdbck(gaint flg) {}
gaint gxdbkq(void) {}
void gxdpbn(gaint bnum, struct gbtn *pbn, gaint redraw, gaint btnrel, gaint nstat) {}
void gxdrmu(gaint mnum, struct gdmu *pmu, gaint redraw, gaint nstat) {}
void gxdsfn(void) {}
void gxdrdw(void) {}
void gxrswd(gaint flag) {}
void gxcpwd(void) {}
void gxrs1wd(gaint wdtyp, gaint wdnum) {}
void gxevbn(struct gevent *geve, gaint iobj) {}
void gxevrb(struct gevent *geve, gaint iobj, gaint i, gaint j) {}
void gxdrbb(gaint num, gaint type, gadouble xlo, gadouble ylo, gadouble xhi, gadouble yhi, gaint mbc) {}
gaint gxevdm(struct gevent *geve, gaint iobj, gaint ipos, gaint jpos) {}
gaint gxpopdm(struct gdmu *gmu, gaint iobj, gaint porig, gaint iorig, gaint jorig) {}
char *gxdlg(struct gdlg *qry) {}
void gxdssv(gaint frame) {}
void gxdssh(gaint cnt) {}
void gxdsfr(gaint frame) {}
void gxdptn(gaint typ, gaint den, gaint ang) {}
gaint win_data(struct xinfo *xinf) {}


static gaint reds[16] =   {  0,255,250,  0, 30,  0,240,230,240,160,160,  0,230,  0,130,170};
static gaint greens[16] = {  0,255, 60,220, 60,200,  0,220,130,  0,230,160,175,210,  0,170};
static gaint blues[16]  = {  0,255, 60,  0,255,200,130, 50, 40,200, 50,255, 45,140,220,170};


void gxdgcoord (gadouble x, gadouble y, gaint *i, gaint *j) {
  if (batch) {
    *i = -999;
    *j = -999;
    return;
  }
}
void gxdimg(gaint *im, gaint imin, gaint jmin, gaint isiz, gaint jsiz) {}
void gxqdrgb (gaint clr, gaint *r, gaint *g, gaint *b) {
  if (clr>=0 && clr<16) {
    *r = reds[clr];
    *g = greens[clr];
    *b = blues[clr];
  } 
  return;
}
