/*

    gxX Stubs for Secure GrADS.

---
    Copyright (C) 2011 by Arlindo da Silva <dasilva@opengrads.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XStandardColormap  void
#define XColor             void

#include <gatypes.h>
#include "gx.h"
#include "bitmaps.h"

static gaint reds[16] =   {  0,255,250,  0, 30,  0,240,230,240,160,160,  0,230,  0,130,170};
static gaint greens[16] = {  0,255, 60,220, 60,200,  0,220,130,  0,230,160,175,210,  0,170};
static gaint blues[16]  = {  0,255, 60,  0,255,200,130, 50, 40,200, 50,255, 45,140,220,170};

static batch = 1;

/* Stubs for gxX.c */
void gxwdln(void) {}
void gxdbat(void) { printf("WARNING: X11 has been disabled for Secure GrADS.\n"); }
void gxdgeo(char *arg) {}
void gxdbgn(gadouble xsz, gadouble ysz) { 
  batch = 1;
  printf("WARNING: X11 has been disabled for Secure GrADS.\n");
 }
void gxgrey(gaint flag) {}
void gxdend(void) {}
void gxdfrm(gaint iact) {}
void gxdeve(gaint flag) {}
void gxdbtn(gaint flag, gadouble *xpos, gadouble *ypos, gaint *mbtn, gaint *type, gaint *info, gadouble *rinfo) {}
void gxdcol(gaint clr) {}
gaint gxdacl(gaint clr, gaint red, gaint green, gaint blue) {}
gaint gxbcol(XStandardColormap *best, XColor *cell) {}
void gxdwid(gaint wid) {}
void gxdmov(gadouble x, gadouble y) {}
void gxddrw(gadouble x, gadouble y) {}
void gxdrec(gadouble x1, gadouble x2, gadouble y1, gadouble y2) {}
void gxdsgl(void) {}
void gxddbl(void) {}
void dump_back_buffer(char *filename) {}
void dump_front_buffer(char *filename) {}
void gxdswp(void) {}
gaint gxqfil(void) {}
void gxdfil(gadouble *xy, gaint n) {}
void gxdxsz(gaint xx, gaint yy) {}
// void gxdbck(gaint flg) {}
// gaint gxdbkq(void) {}
void gxdpbn(gaint bnum, struct gbtn *pbn, gaint redraw, gaint btnrel, gaint nstat) {}
void gxdrmu(gaint mnum, struct gdmu *pmu, gaint redraw, gaint nstat) {}
void gxdsfn(void) {}
void gxdrdw(void) {}
void gxrdrw(gaint flag) {}
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
void gxdXflush(void){}
gaint gxdacol(gaint clr, gaint red, gaint green, gaint blue, gaint alpha){return 0;}
gadouble gxdch(char ch, gaint fn, gadouble x, gadouble y, gadouble w, gadouble h, gadouble rot)
   {return (gadouble) 0.0;}
void gxdclip(gadouble xlo, gadouble xhi, gadouble ylo, gadouble yhi){}
void gxdopt(gaint opt) {}
gadouble gxdqchl(char ch, gaint fn, gadouble w)
   {return (gadouble) 0.0;}
void gxsetpatt(gaint pnum) {}
void gxdsignal(gaint sig) {}



