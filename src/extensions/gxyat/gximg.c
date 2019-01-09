/*

    Copyright (C) 1997-2007 by Arlindo da Silva <dasilva@opengrads.org>
    All Rights Reserved.

    Portions Copyright (C) 1999 Matthias Muennich 

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

/***********************************************************
 *
 * GXIMG: a grads metafile converter based on Image Magick
 *        Magick Wand API. This utility has been adapted from
 *        Matthias Muennich's GXPNG utility. Thanks, Matt.
 *
 ************************************************************
 *
 *   Gximg dependends on the Image Magick 6.x and all the

 *   other libraries used to build Image Magick (too many
 *   to list).
 *
 ***********************************************************/

#define VERSION 0.1.0

/* default file extensions */
#define IN_EXT "gx"

/* default size of the graph */
#define SX 800
#define SY 600

/* PNMAX: maximal points in polygons */
#define PNMAX 4096

/* -------------------------- headers ---------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wand/MagickWand.h>

/* ------------------------ Local Types-------------------------------- */

  /* Image */
  typedef struct
  {
    DrawingWand       *dw;
    MagickWand        *mw;
  }
  myImage, *myImagePtr;

  /* Point */
  typedef struct
  {
    int x, y;
  }
  myPoint, *myPointPtr;

  extern int myCompareInt(const void *a, const void *b);

/* -------------------------- Wand Exception Handlers ------------------- */

#define ThrowWandException(wand) \
  { \
    char \
      *description; \
   \
    ExceptionType \
      severity; \
   \
    description=MagickGetException(wand,&severity); \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
    exit(-1); \
  }
#define ThrowDrawException(wand) \
  { \
    char \
      *description; \
   \
    ExceptionType \
      severity; \
   \
    description=DrawGetException(wand,&severity); \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetDrawModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
    exit(-1); \
  }
#define ThrowPixelException(wand) \
  { \
    char \
      *description; \
   \
    ExceptionType \
      severity; \
   \
    description=PixelGetException(wand,&severity); \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
    exit(-1); \
  }

/* ------------------------ global vars -------------------------------- */

/* The Image */
myImage *im, *w[12];
MagickBooleanType status;

PointInfo coords[PNMAX];                        /* Image Magick point  */

/* command line options (flags) */
struct options {int reverse, gif, verbose, sx, sy;}; 
static char out_ext[] = "png"; /* by default write PNG */

FILE *infile;

/* Basic colors, for convenience */
PixelWand *black, *white, *red, *green, *blue;

/* --------------------------- Color Functions ----------------------------- */

#define MXCOL 100         /* Max number of colors */

/* myColor */
typedef struct
{
  int def;
  int r, g, b, a;
  char str[32];
  PixelWand *pw;
}
myColor, *myColorsPtr;

void myColorSet(myColor *c, int r, int g, int b, int a) {
  int status;
  char str[32];
  c->r = r;
  c->g = g;
  c->b = b;
  c->a = a;
  /* TO DO: handle alpha channel */
  if (!c->def) c->pw = NewPixelWand();
  sprintf(str,"rgb(%d,%d,%d)",r,g,b);
  status=PixelSetColor(c->pw,str);
  if (status == MagickFalse) 
    ThrowPixelException(c->pw);
  c->def = 1;
  strncpy(c->str,str,31);
}

void initColors(myColor Colors[], int ncol) {

/* RGB values for 16 Rainbow colors */
/* these exactly match what's in gxX.c so that the graphical display 
   and the printim output look the same */
static int 
 r[16]={0,255,250,  0, 30,  0,240,230,240,160,160,  0,230,  0,130,170},
 g[16]={0,255, 60,220, 60,200,  0,220,130,  0,230,160,175,210,  0,170},
 b[16]={0,255, 60,  0,255,200,130, 50, 40,200, 50,255, 45,140,220,170};

 int i;

 /* Set all colors to undefined */
 for ( i=0; i<ncol; i++ ) {
       Colors[i].def = 0;
       strcpy(Colors[i].str,"<undefined>");
 }

 /* Define rainbow colors */ 
 for ( i=0; i<16; i++ ) {
       myColorSet ( &Colors[i], r[i], g[i], b[i], 0 );
 }

}

/* ------------------------------------------------------------------------- */

void printOptions(char *argv[]){
       fprintf(stderr,"%s%s%s","Usage: ",argv[0],
	   " [-hrv -x <pixels> -y <pixels> -i <in_file>[."  IN_EXT
           "] -o <out_file>] [<in_file>[."IN_EXT"]].\n");
       fprintf(stderr,"Options:\n");
       fprintf(stderr,"     -i   <in_file>[."IN_EXT"].\n");;
       fprintf(stderr,"     -h   Fill polygons horizontally.\n");
       fprintf(stderr,"     -o   <out_file> (default: basename(in_file).%s, '-' = stdout).\n",out_ext);
       fprintf(stderr,"     -r   Black background.\n");
       fprintf(stderr,"     -v   Verbose.\n");
       fprintf(stderr,"     -x <pixels>  # pixels horizontally.\n");
       fprintf(stderr,"     -y <pixels>  # pixels vertically.\n");
     exit(8);
     }

/* ------------------------------------------------------------------- */

void parseArg(int argc,char *argv[],struct options *o,char **fin, char **fout){
     register int i,j;
     if(argc==1) printOptions(argv);
     for (i=1;i<argc;i++) {
       if (*(argv[i])=='-') {  /* parse options */
         j = 0;
         while (*(argv[i]+(++j))) {
           if (*(argv[i]+j)=='i') {*fin = argv[++i];break;}
           else if (*(argv[i]+j)=='o') {*fout = argv[++i];break;}
           else if (*(argv[i]+j)=='r') o->reverse = 1;
           else if (*(argv[i]+j)=='x') {
	     sscanf(argv[++i],"%d",&(o->sx));break;
	   }
           else if (*(argv[i]+j)=='y') {
	     sscanf(argv[++i],"%d",&(o->sy));break;
	   }
           else if (*(argv[i]+j)=='v') {
	       o->verbose = 1;
	       printf("This is gximg Version %s\n", "VERSION" );
	   }
           else { 
	     fprintf(stderr,"Unknown option: %s\n\n",argv[i]);
	     printOptions(argv);
	     exit(1);
	   }
         }
       }
      else  /* No command line "-" */
        *fin=argv[i];
    }
    if(o->gif) strcpy(out_ext,"gif");
    return;
}

/* ------------------------------------------------------------------------ */

void openFiles(char **fin, char **fout,short verbose){ /* Open files */
  int i;

  if (*fin==NULL) {
    *fin = (char *) malloc(sizeof(char)*150);
     fgets(*fin,150,stdin);
     printf("read infile = %s\n",*fin);
  }
  infile = fopen(*fin ,"rb");
  if (infile == NULL) {
    *fin=strcat(*fin,"."IN_EXT);
    infile = fopen(*fin,"rb");
    if (infile == NULL) {
      (*fin)[strlen(*fin)-3]='\0';
      printf ("Input file %s[."IN_EXT"] not found.\n",*fin);
      exit(1);
    }
  }
  if (*fout==NULL) {
    *fout = (char *) malloc(sizeof(char)*150);
    strcpy(*fout,*fin);
    for (i=strlen(*fout)-1;i>=0;i--) {
      if((*fout)[i]=='.') {strcpy((*fout)+i+1,out_ext);
	break;
      }
      if(i==0){strcpy((*fout)+strlen(*fout),".");
         strcpy((*fout)+strlen(*fout),out_ext);
      }
    }
  }
  if(verbose) {
    printf("GrADS metafile: %s\n",*fin);
    if(strcmp(*fout,"-")==0) printf("output to stdout\n");
    else printf("PNG-file: %s\n",*fout);
  }
  return;
}

/* ------------------------------------------------------------------------ */

void myImageGenesis() 
{
   MagickWandGenesis();
  /*
      Basic pixels
  */
  black = NewPixelWand();
  white = NewPixelWand();
    red = NewPixelWand();
  green = NewPixelWand();
   blue = NewPixelWand();
  PixelSetColor(white,"white");
  // PixelSetOpacity(white,(double) 1.0);
  PixelSetColor(black,"black");
  // PixelSetOpacity(black,(double) 0.0);
  PixelSetColor(red,"red");
  PixelSetColor(blue,"blue");
  PixelSetColor(green,"green");
}

void myImageTerminus() {
    MagickWandTerminus();
}

myImagePtr myImageCreate (int sx, int sy, myColor bkg) 
{

  myImage *im;

  unsigned long x1=1, y1=1, x2, y2;

   im = ( myImage * ) malloc(sizeof(myImage));

  im->mw = NewMagickWand();  /* wand where the image will be rendered later */
  im->dw = NewDrawingWand(); /* drawing wand */
  
  /* Create image in wand */
  status = MagickNewImage ( im->mw, sx, sy, bkg.pw );
  if (status == MagickFalse) 
      ThrowWandException(im->mw);

  status = MagickSetImageType ( im->mw, TrueColorType );
  if (status == MagickFalse) 
      ThrowWandException(im->mw);
  
  status = MagickSetAntialias(im->mw, MagickTrue);
  if (status == MagickFalse) 
      ThrowWandException(im->mw);
  
  status = MagickSetBackgroundColor(im->mw, bkg.pw);
  if (status == MagickFalse) 
      ThrowWandException(im->mw);

  /* View box for rendering the image later */
  x2 = (unsigned long) sx;   
  y2 = (unsigned long) sy;
  DrawSetViewbox(im->dw, x1, y1, x2, y2 );

  /* Foreground color */
  // DrawSetFillColor(im->dw,black);
  DrawSetStrokeColor(im->dw,black);
  // DrawSetStrokeWidth(im->dw,(double) 2);
  // DrawSetStrokeLineJoin(im->dw,BevelJoin);

  return im;

}

void myImageLine (myImagePtr im, int x1, int y1, int x2, int y2, myColor c)
{
  DrawSetStrokeColor(im->dw,c.pw);
  DrawLine(im->dw, (double) x1, (double) y1,  (double) x2, (double) y2);
}

/* ------------------------------------------------------------------------ */

void myImageFilledRectangle(myImagePtr im, double x1, double y1, 
                            double x2, double y2, myColor c)
{
   // printf("rectangle: %g %g %g %g with %s\n", x1, y1, x2, y2, c.str);
    DrawSetStrokeColor(im->dw,c.pw);
    DrawSetStrokeOpacity(im->dw, (double) 0.0);
    // DrawSetStrokeAntialias(im->dw,MagickFalse);
    DrawSetFillColor(im->dw,c.pw);
    DrawSetFillOpacity(im->dw, (double) 1.0);
    DrawRectangle(im->dw,x1,y1,x2,y2);
    // DrawSetStrokeAntialias(im->dw,MagickTrue);
}

void myImageDrawPolyline(PointInfo coords[], short *ncoord, int wd, myColor c)
{
    unsigned long n;

    n = (unsigned long) *ncoord + 1;
    if ( n<1 ) return;
    DrawSetStrokeColor(im->dw, c.pw);     /* set the pen color */
    DrawSetFillOpacity(im->dw, (double) 0.0);
    if ( n>2 ) {
	DrawPolyline(im->dw, n, (PointInfo *) coords);
    } else {
        DrawLine(im->dw,coords[0].x,coords[0].y,
		        coords[1].x,coords[1].y);
    }
   *ncoord=0;

   return;
}

void myImageFilledPolygon(PointInfo coords[], short *ncoord, myColor c)
{
    unsigned long n;
    n = (unsigned long) *ncoord + 1;
    if ( n<1 ) return;
    DrawSetStrokeColor(im->dw,c.pw);
    DrawSetStrokeOpacity(im->dw, (double) 0.0);
    DrawSetStrokeAntialias(im->dw,MagickFalse);
    DrawSetFillColor(im->dw,c.pw);
    DrawSetFillOpacity(im->dw, (double) 1.0);
    DrawPolygon(im->dw, n, (PointInfo *) coords);
    DrawSetStrokeAntialias(im->dw,MagickTrue);
   *ncoord=0;

   return;
}


void myImageWrite (myImagePtr im, char *fout)
{

  /* render the image on wand */
  status = MagickDrawImage (im->mw, im->dw);
  if (status == MagickFalse)
    ThrowWandException(im->mw);

  /* write the image */
  status=MagickWriteImage(im->mw,fout);
  if (status == MagickFalse)
    ThrowWandException(im->mw);

}

void myImageDestroy (myImagePtr im)
{
    im->dw = DestroyDrawingWand(im->dw);
    im->mw = DestroyMagickWand(im->mw);
    free(im);
}

/* ---------------------------------------------------------------------- 

                                   M A I N
 
  ----------------------------------------------------------------------- */
int main (int argc, char *argv[])  {

   myColor Colors[MXCOL];

   short cmd, opts[7];
   int col=0, wd=0;
   int sx=SX,sy=SY,sh;
   double lly,ury;
   float blowx,blowy;
   struct options o;                       /* command line options */
   register int  i;
   short ncoord=0,ncoordSave,fcnt;
   char *fin=NULL,*fout=NULL;              /* file names */
   fpos_t infile_pos;
   short width[12] = {                      /* width (units: .001 inch) */
               2 ,7 ,10 ,14 ,17 ,20 ,
	       24 ,27 ,31 ,34 ,38 ,41 };

  o.reverse=o.verbose=o.gif=o.sx=o.sy=0;


/*                           Initialization                         
                             --------------
*/

  myImageGenesis();                     /* Image package initialization */
  initColors(Colors, MXCOL);            /* Initialize colors */

  parseArg(argc,argv,&o,&fin,&fout);    /* Parse command line arguments */
  openFiles(&fin,&fout,o.verbose);      /* open files */

  /*
                            Translate Metafile
                            ------------------
  */

  fcnt = 1;
  while (1) {

    fread (&cmd, sizeof(short), 1, infile);

    if (cmd==-11){				/* Draw to */
      fread (opts, sizeof(short), 2, infile);
      coords[++ncoord].x=(double)opts[0]*blowx;
      coords[ncoord].y  =(double)(sy-opts[1]*blowy);
    }

    else if (cmd==-10){				/* Move to */
      if (ncoord) myImageDrawPolyline(coords,&ncoord,wd,Colors[col]);
      fread ((char *) opts, sizeof(short), 2, infile);
      coords[ncoord].x=(double)opts[0]*blowx;
      coords[ncoord].y=(double)(sy-opts[1]*blowy);
    }

    else if (cmd==-4) {				/* Set line width */
      if (ncoord) myImageDrawPolyline(coords,&ncoord,wd,Colors[col]);
      fread ((char *)opts, sizeof(short), 2, infile);
      i = opts[0];
      if (i>12) i=12;
      else if (i<1) i=1;
      wd=width[i-1]*blowx+1;
    }

    else if (cmd==-3) {			       /* Set color */
      if (ncoord) {
	ncoordSave=ncoord;
	myImageDrawPolyline(coords,&ncoord,wd,Colors[col]);
	coords[ncoord].x=coords[ncoordSave].x; /* move to last pnt of polygon */
	coords[ncoord].y=coords[ncoordSave].y;
      }
      fread ((char *)opts, sizeof(short), 1, infile);
      col = opts[0];
      if (col<0) col=0;
      if (col>MXCOL-1) col=MXCOL-1;
      if (!(Colors[col].def)) col=15;
    }

    else if (cmd==-7){				/* Start fill */
      fread ((char *)opts, sizeof(short), 1, infile);
    }

    else if (cmd==-8){				/* End fill */
      myImageFilledPolygon(coords, &ncoord, Colors[col]);
    }

    else if (cmd==-6){				/* Rectangle fill */
      if (ncoord) myImageDrawPolyline(coords,&ncoord,wd,Colors[col]);
      fread ((char *)opts, sizeof(short), 4, infile);
      lly=(double) (sy-opts[2]*blowy);
      ury=(double) (sy-opts[3]*blowy);
      myImageFilledRectangle(im, (double) opts[0]*blowx,ury,
                                 (double) opts[1]*blowx,lly, Colors[col]);
    }

    else if (cmd==-9) {				/* End of plotting */
      if(o.verbose) printf ("Number of pages = %i\n",fcnt);

      myImageWrite(im, fout);
      myImageDestroy(im);
      myImageTerminus();                   /* Image package finalization */
      return(0);
    }

    else if (cmd==-1) {				/* Start of plotting */
      fread ((char *)opts, sizeof(short), 2, infile);
      if(opts[0]<opts[1]) {sh=sx; sx=sy; sy=sh;}
      if(o.sx) {
          sx=o.sx;
         if(o.sy==0) sy=sx *(opts[1]/100)/(opts[0]/100);
      }
      if(o.sy) {
          sy=o.sy;
          if(o.sx==0) sx=sy*(opts[0]/100)/(opts[1]/100);
      }
      /* picture size in metafile: opts[0] x opts[1] */ 
      blowx=(float)sx/(float)opts[0];  
      blowy=(float)sy/(float)opts[1];
      if(o.verbose) printf("Image size: %d x %d pixels\n",sx,sy);

      if(o.reverse) {
	myColorSet(&Colors[0], 255, 255, 255, 0); /* white background */
	myColorSet(&Colors[1],   0,   0,   0, 0); /* black foreground */
      }

      for (i=0;i<=width[11]*blowx;i++) w[i] = myImageCreate(i+1,i+1,Colors[0]);
      im = myImageCreate(sx,sy,Colors[0]);
      col=1;
      ncoord = 0;
    }

    else if (cmd==-2) {				/* New Page */
      if (ncoord) myImageDrawPolyline(coords,&ncoord,wd,Colors[col]);
      myImageWrite(im, fout);
      myImageDestroy(im);
      for (i=0;i<=width[11]*blowx;i++)
	  myImageDestroy(w[i]);
      fgetpos(infile,&infile_pos);
      fread (&cmd , sizeof(short), 1, infile); 
      fsetpos(infile,&infile_pos);
      if (cmd != -9) {  /* if next command is not end_of_plotting */
	printf("Frame 1 plotted. Multiple frames not supported.\n");
        myImageTerminus();                   /* Image package finalization */
        return(0); 
      }
      else {
        myImageTerminus();                    /* Image package finalization */
        return(0);
      }
    }

    else if (cmd==-5){				/* Define new color */
      fread ((char *)opts, sizeof(short), 4, infile);
      i = opts[0];
      if (i>15 && i<MXCOL) myColorSet(&Colors[i],opts[1],opts[2],opts[3],0);
    }

    else if (cmd==-15) {		/* Begin string drawing -- ignore*/
    }

    else if (cmd==-16) {		/* End string drawing -- skip */
      fread ((char *)opts, sizeof(short), 7, infile); /* get skip length */
      fseek(infile,
            (long)(opts[0]/sizeof(short)*sizeof(short)+2*sizeof(short)),
             SEEK_CUR); 
    }

    else if (cmd==-20) {		/* Draw button -- ignore */
      fread ((char *)opts, sizeof(short), 1, infile);
    }

    else {
       printf ("Fatal error: Invalid command \"%i\" found in metafile\"%s\".\n",cmd,fin);
       printf ("Is \"%s\" really a GrADS (v1.5 or higher) metafile?\n",fin);
       myImageTerminus();                   /* Image package finalization */
      return(1);
    }

  }

  myImageTerminus();                   /* Image package finalization */

}
