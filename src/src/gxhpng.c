/* 
 * include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/* Rasterize current metafile buffer via gd library.  Loosly based
   on the gxpng utility:

   gxpng Copyright 1999 Matthias Muennich 

   Has been modified to behave more like the X interface, so the
   output image will look more like the screen image (B. Doty)

   Modified for GD version 2 (B. Doty, 11/06) */

#include "gd.h"

gaint gxhpng (char *, gaint, gaint, gaint, gaint, char *, char *, gaint) ;
void gxgdpoly (gaint, gaint, gaint);
void gaprnt (gaint, char *);

/* default size of the graph */

#define SX 800
#define SY 600

static char pout[256];   /* Build error msgs here */
static gdImagePtr im;
static gdPoint xxyy[256];

/* rgb values for 16 defined colors */
/* these exactly match what's in gxX.c so that the graphical display 
   and the printim output look the same */
static gaint 
 rr[16]={0,255,250,  0, 30,  0,240,230,240,160,160,  0,230,  0,130,170},
 gg[16]={0,255, 60,220, 60,200,  0,220,130,  0,230,160,175,210,  0,170},
 bb[16]={0,255, 60,  0,255,200,130, 50, 40,200, 50,255, 45,140,220,170};

gaint gxhpng (char *fnout, gaint xin, gaint yin, gaint bwin, gaint gifflg, 
	    char *bgImage, char *fgImage, gaint tcolor) {
FILE *ofile, *bgfile, *fgfile;
gdPoint *xybuf=NULL;
gdImagePtr imfg, imbg=0;
float xlo,xhi,ylo,yhi;
gaint xpos,ypos,xs,ys,xrs,yrs;
gaint cmd,i,len,cnt,flag,ii,siz,xp,yp,xp2,yp2,thck,backbw;
gaint ccol,fflag,xyc=0,red,grn,blu,xcur,ycur,xsav,ysav,retcod;
gaint cdef[256],cnum[256],rc[256],gc[256],bc[256]; 
gaint xycnt=0,xyflag,gdthck;
short *poi,*pend;

  im = NULL;

  if (bwin<-900) backbw = gxdbkq();
  else backbw = bwin;

  for (i=0; i<=255; i++) {
    cdef[i]=0;
    rc[i] = 125; gc[i] = 125; bc[i] = 125;  /* initialize colors */
  }
  for (i=0; i<16; i++) { 
    rc[i] = rr[i]; gc[i] = gg[i]; bc[i] = bb[i];
  }

  /*  Set up pointers into current meta buffer list */

  if (dbmode && pntf==0) {
    lens2[pnt2-1] = hpnt-hbuff;
    cnt = pnt2; flag = 1;
  } else {
    lens[pnt-1] = hpnt-hbuff;
    cnt = pnt; flag = 0;
  }

  /*  Allocate the gd image and set up the scaling for it */

  if (xin<0 || yin<0) {  
    /* user has not specified image size */
    if (xrsize > yrsize) {
      xs = SX; 
      ys = (gaint)((yrsize/xrsize)*SX);
    }
    else {
      xs = (gaint)((xrsize/yrsize)*SX); 
      ys = SX;
    }
  } 
  else {
    xs = xin; 
    ys = yin;
  }
  xrs = (gaint)(xrsize*1000.0+0.5);
  yrs = (gaint)(yrsize*1000.0+0.5);

  /* handle background PNG picture */
  if (*bgImage) {
    /* Make sure bgImage is a .png -- otherwise return error */
    len = 0;
    while (*(bgImage+len)) len++;
    len = len-4;
    if (len>0) {
      if (*(bgImage+len+1)!='p' || 
	  *(bgImage+len+2)!='n' || 
	  *(bgImage+len+3)!='g' ) {
	if (*(bgImage+len+1)!='P' || 
	    *(bgImage+len+2)!='N' || 
	    *(bgImage+len+3)!='G' ) {
	  return(5);
	} 
      }
    }

    if ((bgfile=fopen(bgImage,"rb"))) {
      if ((im=gdImageCreateFromPng(bgfile)) != NULL) {
	if (im->sx < xs || im->sy < ys) {
	  gdImageDestroy(im);
	  im=NULL;
	}
      } else {
	fclose(bgfile);
	return(7);
      }
      fclose(bgfile);
    } else {
      return(3);
    }
  }

  if (!im) {
    /* im = gdImageCreateTrueColor(xs,ys);  For anti-aliasing */
    im = gdImageCreate(xs,ys);
  }

  /*  Set up background and foreground colors */
  if (backbw) {
    cnum[0] = gdImageColorAllocate(im, 255, 255, 255);
    cnum[1] = gdImageColorAllocate(im, 0, 0, 0);
  } else {
    cnum[0] = gdImageColorAllocate(im, 0, 0, 0);
    cnum[1] = gdImageColorAllocate(im, 255, 255, 255);
  }
  cdef[0] = 1; cdef[1] = 1;
  ccol = 1;

  /* Loop thru allocated meta buffers and handle the graphics commands found there */
  fflag = 0;
  xyflag = 0;
  gdthck = 1;
  for (ii=0; ii<cnt; ii++) {
    if (flag) {
      poi = bufs2[ii];
      pend = poi + lens2[ii];
    } else { 
      poi = bufs[ii];
      pend = poi + lens[ii];
    } 

    while (poi<pend) {
      /* Get message type */
      cmd = *poi; 

      /* Handle various message types */
      /* -9 is end of file.  Should not happen. */
      if (cmd==-9) {
        gaprnt(0,"Logic Error 4 in gxhpng.  Notify Developer\n");
        return(99);
      }

      /*  -1 indicates start of file.  Should not ocurr. */
      else if (cmd==-1) {
        gaprnt(0,"Logic Error 8 in gxhpng.  Notify Developer\n");
        return(99);
      }

      /* -2 indicates new frame.  Also should not ocurr */
      else if (cmd==-2) {
        gaprnt(0,"Logic Error 12 in gxhpng.  Notify Developer\n");
        return(99);
      }

      /* -3 indicates new color.  One arg; color number.  */
      /*  Allocate in gd if not done already */
      else if (cmd==-3) {
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt=0; }
        ccol = *(poi+1);
        if (ccol<0) ccol=0;
        if (cdef[ccol]==0) {
          cnum[ccol] = gdImageColorAllocate(im, rc[ccol], gc[ccol], bc[ccol]);
          cdef[ccol] = 1;
        }
        poi += 2;
      }

      /* -4 indicates new line thickness.  It has two arguments */
      else if (cmd==-4) {
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        thck = *(poi+2);
        gdthck = 1;
        if (thck>5) gdthck = 2;
        if (thck>11) gdthck = 3;
        poi += 3;
      }

      /*  -5 defines a new color, in rgb.  It has four int args */
      /*  If this changes the existing definition for this color, 
          then indicate this has not yet been allocated to gd */
      else if (cmd==-5){
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        i = *(poi+1);
        red = *(poi+2);
        grn = *(poi+3);
        blu = *(poi+4);
        if (rc[i]!=red || gc[i]!=grn || bc[i]!=blu) {
          rc[i] = red; gc[i] = grn; bc[i] = blu;
          cdef[i] = 0;
        }
        poi += 5;
      }

      /* -6 is for a filled rectangle.  It has four float args. */ 
      else if (cmd==-6){
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        xlo = *(poi+1); ylo = *(poi+3);
        xhi = *(poi+2); yhi = *(poi+4);
        xp = (xlo*xs)/xrs;
        yp = ys-(ylo*ys)/yrs;
        xp2 = (xhi*xs)/xrs;
        yp2 = ys-(yhi*ys)/yrs;
        if (xp>xp2) { 
           i = xp;
           xp = xp2;
           xp2 = i;
        }
        if (yp>yp2) { 
           i = yp;
           yp = yp2;
           yp2 = i;
        }
        gdImageFilledRectangle(im, xp, yp, xp2, yp2, cnum[ccol]);
        poi += 5;
      }

      /* -7 indicates the start of a polygon fill.  It has one arg. */
      else if (cmd==-7){
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        siz = *(poi+1);
        xybuf = (gdPoint *)malloc(sizeof(gdPoint)*(siz+1));
        if (xybuf==NULL) {
          gaprnt(0,"Memory allocation error: gxhpng\n");
          return(99);
        }
        fflag = 1;
        xyc = 0;
        poi += 2;
      }

      /* -8 is to terminate polygon fill.  It has no args */
      else if (cmd==-8) {
        if (xybuf->x != (xybuf+xyc-1)->x ||
            xybuf->y != (xybuf+xyc-1)->y) {
          (xybuf+xyc)->x = xybuf->x;
          (xybuf+xyc)->y = xybuf->y;
          xyc++;
        }
        gdImageFilledPolygon(im, xybuf, xyc, cnum[ccol]); 
        free (xybuf);
        fflag = 0;
        poi += 1;
      }

      /* -10 is a move to instruction.  It has two float args */ 
      else if (cmd==-10){
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        xpos = *(poi+1); ypos = *(poi+2);
        xsav = (xpos*xs)/xrs;
        ysav = ys-(ypos*ys)/yrs;
        if (fflag) {
          (xybuf+xyc)->x = xsav;
          (xybuf+xyc)->y = ysav;
          xyc++;
        } else {   
          xxyy[0].x = xsav;
          xxyy[0].y = ysav;
          xycnt = 1;
          xyflag = 0;
        }
        poi += 3;
      }

      /*  -11 is draw to.  It has two float args. */  
      else if (cmd==-11){
        xpos = *(poi+1); ypos = *(poi+2);
        xcur = (xpos*xs)/xrs;
        ycur = ys-(ypos*ys)/yrs;
        if (fflag) {    /* Assume first poly point is moveto */
          if ((xybuf+xyc-1)->x != xcur || (xybuf+xyc-1)->y != ycur) { 
            (xybuf+xyc)->x = xcur;
            (xybuf+xyc)->y = ycur;
            xyc++;
          }
        } else {
          xxyy[xycnt].x = xcur;
          xxyy[xycnt].y = ycur;
          if (xycnt<255) xycnt++;
          else {
            gxgdpoly(xycnt+1,cnum[ccol],gdthck);
            xxyy[0].x = xcur;
            xxyy[0].y = ycur;
            xycnt = 1; 
          }
          xyflag = 1;
        }
        xsav = xcur; ysav = ycur;
        poi += 3;
      }

      /* -12 indicates new fill pattern.  We ignore it here */
      else if (cmd==-12) {
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        poi += 4;
      }

      /* -20 is a draw widget.  We ignore it here. */
      else if (cmd==-20) {
        if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }
        poi += 2;
      }

      /* Any other command would be invalid */
      else {
        gaprnt(0,"Logic Error 20 in gxhpng.  Notify Developer\n");
        return(99);
      }
    }
  }
  if (xyflag) { gxgdpoly(xycnt,cnum[ccol],gdthck); xyflag=0; xycnt = 0; }

  /* handle foreground PNG picture */
  if (*fgImage) {
    /* Make sure fgImage is a .png -- otherwise return error */
    len = 0;
    while (*(fgImage+len)) len++;
    len = len-4;
    if (len>0) {
      if (*(fgImage+len+1)!='p' || 
	  *(fgImage+len+2)!='n' || 
	  *(fgImage+len+3)!='g' ) {
	if (*(fgImage+len+1)!='P' || 
	    *(fgImage+len+2)!='N' || 
	    *(fgImage+len+3)!='G' ) {
	  return(6);
	} 
      }
    }

    if ((fgfile=fopen(fgImage,"rb"))) {
      if ((imfg=gdImageCreateFromPng(fgfile)) !=NULL) {
	gdImageCopy(im,imfg,0,0,0,0,imfg->sx,imfg->sy);
      }
      else {
	fclose(fgfile);
	return(8);
      }
    } else {
      return(4);
    }
    fclose(fgfile);
    gdImageDestroy(imfg);
  }


  retcod = 0;
  /* optionally convert a color to transparent */
  if (tcolor != -1 ) {
    if (cdef[tcolor]){
      gdImageColorTransparent(im,cnum[tcolor]);
      snprintf(pout,255,"Transparent color: #%d\n",tcolor);
      gaprnt(2,pout);
    }
  }
  if (*bgImage) {
    if ((bgfile=fopen(bgImage,"rb"))) {
      if ((imbg=gdImageCreateFromPng(bgfile)) !=NULL) {
	gdImageCopy(imbg,im,0,0,0,0,im->sx,im->sy);
      }
    }
    fclose(bgfile);
    gdImageDestroy(im);
    im=imbg;
  }

  ofile = fopen(fnout, "wb");
  if (ofile==NULL) { 
    snprintf(pout,255,"Open error on %s\n",fnout);
    gaprnt(0,pout);
    retcod = 1; 
  } else {
    if (gifflg==1) {                    /* image output in gif format */
      gdImageGif (im, ofile);
    }
    else if (gifflg==3) {     	  /* image output in jpg format */
      gdImageJpeg(im, ofile, -1);
    }
    else {                       	  /* image output in png format */
      gdImagePng(im, ofile);
    }
    fclose(ofile);
  }

  gdImageDestroy(im);
  return (retcod);
}

/* Turns out that the anti-aliasing doesn't work with
   the line thickness, in gd-v2.  Decided to not use
   the anti-aliasing this version.  But kept the 
   function calls here, commented out, for possible
   future use.  */

void gxgdpoly (gaint xycnt, gaint col, gaint thck) {
  if (xycnt==2) {
/*     gdImageSetAntiAliased(im,col); */
     gdImageSetThickness(im,thck);
/*     gdImageLine(im, xxyy[0].x, xxyy[0].y, xxyy[1].x, xxyy[1].y, gdAntiAliased); */
     gdImageLine(im, xxyy[0].x, xxyy[0].y, xxyy[1].x, xxyy[1].y, col);
  }
  if (xycnt>2) {
/*    gdImageSetAntiAliased(im,col); */
    gdImageSetThickness(im,thck);
/*    gdImageOpenPolygon(im, xxyy, xycnt, gdAntiAliased); */
    gdImageOpenPolygon(im, xxyy, xycnt, col);
  }
}

