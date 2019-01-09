/*

    Copyright (C) 2007 by Arlindo da Silva <dasilva@opengrads.org>
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
 * GXYAT: a grads metafile converter based on the Cairo Vector
 *        Graphics Library (http://cairographics.org/), creating 
 *        output in all formats supported by cairo: png, postscript,
 *        pdf, svg and screen output on X and Windows. See manText()
 *        below for a built-in manual page.
 *
 ************************************************************
 *
 *   Gxyat depends on the Cairo Vector Graphics Library which
 *   is widely available on Linux, Windows and Mac OS/X.
 *
 *   This utility has been adapted from 'gximg' which in turn 
 *   derives from Matthias Muennich's 'gxpng' utility. 
 *   Thanks, Matt, for all the great work on GrADS metafile
 *   translation! 
 *
 * Building Instructions
 * ---------------------
 * 
 * 1) Requirements: Cairo Vector Graphics Library available from
 *    http://cairographics.org/. I have only tried to build it
 *    with cairo v1.4.6 and v1.4.2, but it might work with previous 
      versions as well.
 * 2) Just compile this file and link it with cairo:
 *    gcc -O -o gxyat -I/usr/include/cairo gxyat.c -lcairo
 * 3) Type 'gxyat' without arguments for a Unix style manual page
 *    in ASCII. This is all the documentation there is - besides
 *    the source code, of course. 
 *
 ***********************************************************/

/* default file extensions */
#define IN_EXT "gm"

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
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>

#ifdef ___GAUDX___
#include <setjmp.h>
#include "grads.h"
#include "gacols.h"
static jmp_buf Env;

#define exit(rc)  longjmp(Env,rc)
/* Replace fread() when implementing an UDC: read from memory buffer  */
#define fread udx_mread

#else

#define MAXCLRS 256

#endif

// global,static variables to hold user-defined alpha & mask values
// these are set by c_rgba & read by gxyat routines.
// is there a better way to initialize the rgb_alpha array
// so that array values dont need to be hard-set here?
// P.Romero

static int    rgb_amask[MAXCLRS]={0};
static double rgb_alpha[MAXCLRS]={-1.0};

/* ------------------------ Local Types-------------------------------- */

  /* Image */
  typedef struct
  {
    cairo_t          *cr;       /* graphics context */
    cairo_surface_t  *surface;  /* graphics sfc */
    cairo_surface_type_t surface_type;
  }
  myImage, *myImagePtr;

  /* Output Closure for PNG files */
  typedef struct
  {
    char     *fname;    /* output filename or pipe */
    FILE     *stream;   /* file or pipe */
    int      is_open;   /* whether the file stream is open */
    int      is_pipe;   /* whether the stream is a pipe */
  }
  fpwrite_t;

        /* Max number of colors */

/* myColor */
typedef struct
{
  int def;
  double r, g, b, a;
}
myColor, *myColorsPtr;

/* ------------------------ global vars -------------------------------- */

/* command line options (flags) */
struct options {int reverse, antialias, verbose, sx, sy, ws, pltfill;}; 
static char out_ext[] = "png"; /* by default write PNG */

static FILE *infile;

static char  MYNAME[] = "gxyat";
static char VERSION[] = "1.3.1";

/* ------------------------------------------------------------------------- */

#define manText() "\
\n\
NAME\n\
     GxYat - Yet Another GrADS Metafile Translator\n\
\n\
SYNOPSIS\n\
     gxyat [OPTIONS]  output_image_file          (built-in GrADS command)\n\
     gxyat [OPTIONS]  input_metafile_name[.gm]   (stand alone utility)\n\
\n\
DESCRIPTION\n\
     GxYat is a GrADS metafile translator based on the Cairo Vector\n\
     Graphics library. It can be implemented as a stand alone command\n\
     line utility or as a built-in GrADS command through User Defined\n\
     eXtensions (UDXs). \n\
\n\
     GxYat is capable of producing PNG, Postscript, PDF and Scalable\n\
     Vector Graphics (SVG) output. When producing PNG images, the translator\n\
     uses antialiasing for rendering line drawing but not for polygon fills,\n\
     resulting in nicer fonts, but no artificial horizontal lines in the \n\
     shaded contours. \n\
\n\
     When used as built-in GrADS command, one specifies the output image\n\
     file name on the command line, pretty much like printim. However,\n\
     image size and other option syntax depart slightly from printim; see\n\
     OPTIONS below for details.\n\
\n\
     When used as stand alone utility, the input file extension is \n\
     assumed to be '.gm', if omitted. One can use '-' to indicate \n\
     that the metafile is coming from standard input. \n\
\n\
     The input metafile may contain more than one frame; multipage PS and\n\
     PDF output will result in such cases. However, PNG and SVG output\n\
     require one image per file. By using %%d or other printf variant\n\
     format such as %%02d in the output file name it is possible to have\n\
     the resulting files numbered in sequence. See EXAMPLES below.\n\
\n\
     The output format is determined from the output file name extension; \n\
     the default is PNG. Because the PNG produced by Cairo is 32 bits, the\n\
     default output PNG files tend to be somewhat larger than image files\n\
     produced with the printim command. A work around is to use an utility \n\
     such as *pngquant* available from\n\
\n\
                http://www.libpng.org/pub/png/apps/pngquant.html \n\
\n\
     to produce 8-bit PNG images. By using the pipe option (see example 7)\n\
     you can perform this task in one step.\n\
\n\
     The GxYat output can be displayed by a variety of viewers. On Linux, \n\
     I particularly like evince, a document viewer that is capable of \n\
     displaying all the formats produced by GxYat: be sure to check out\n\
     its presentation mode.  Firefox 2 has native support for PNG and SVG;\n\
     Acrobad Reader, as well as Ghostscript and frontends can read the PS/\n\
     PDF output. The ImageMagick utilities can handle all GxYat output\n\
     formats except for SVG which it does not render properly. On Linux, \n\
     the Inkscape SVG Vector Illustrator is a nice SVG editor for annotating\n\
     GrADS plots (since it is SVG, the output is scalable). Adobe \n\
     Illustrator also has native suppoprt for SVG.\n\
\n\
OPTIONS\n\
     +a           turns anti-aliasing OFF (default is ON)\n\
     -f           Fill image output with plot area ONLY. Margins are removed\n\
                  Aspect ratio is NOT maintained. Plot frame should be turned off\n\
                  to avoid defects around edges of output (using 'set gafram off').\n\
     -h           prints this manual page\n\
     -i fname     input file name; '-' means standard input;   \n\
     -o fname     output file name/pipe command; the file name extension \n\
                  determines the desired format:\n\
                     png     Portable Network Graphics        \n\
                     ps      Postscript\n\
                     pdf     Portable Document Format (PDF)\n\
                     svg     Scalable Vector Graphics\n\
                  Both PS and PDF allow multiple pages per file; PNG and\n\
                  SVG require one image per file; in such cases, specify %%d\n\
                  in the file name for the image number in the sequence \n\
                  - see EXAMPLES below.\n\
                  Default:  same base name as input file, 'png' extension\n\
     -r           Black background (default is white)\n\
     -v           Verbose.\n\
     -x m         For images, the number of horizontal pixels (default=800);\n\
                  For PS/PDF output, the horizontal size of the images \n\
                  in points (1 point = 1/72 inches); default is 792 points\n\
                  (11 inches)\n\
     -y n         For images, the number of vertical pixels (default=600);\n\
                  For PS/PDF output, the horizontal size of the images \n\
                  in points (1 point = 1/72 inches); default is 612 points\n\
                  (8.5 inches)\n\
     -w s         Specify line width scaling; default is 1.0. Use this\n\
                  if you think the lines are too thick or too faint\n\
\n\
EXAMPLES\n\
     1) Producing a PNG image file with the default 800x600 size:\n\
        $ gxyat model.gx\n\
     2) Producing a PDF file with the default 792x612 size (11x8.5 inches):\n\
        $ gxyat -o model.pdf model.gx\n\
     3) Producing a PNG image file of size 1024x758 with a black\n\
        background:\n\
        $ gxyat -x 1024 -y 758 -r model.gx \n\
     4) Producing a SVG file to be rendered with size 1600x1200:\n\
        $ gxyat -o model.svg -x 1600 -y 1200 model.gx\n\
     5) Producing multiple image files from a metafile with multiple frames:\n\
        $ gxyat -o frame-%%d.png frames.gx\n\
        $ gxyat -o frame-%%02d.svg frames.gx\n\
     6) The following DOES NOT work:\n\
        $ gxyat -o frame-%%d.ps  frames.gx\n\
        $ gxyat -o frame-%%d.pdf frames.gx\n\
     7) Producing a PNG image file with the default 800x600 size, but \n\
        piping the output through another program to reduce the file size:\n\
        $ gxyat -o '|pngquant 256 > test.png' model.gx\n\
        Notice that in this case the .png in the end is still used for \n\
        determining the format of the output file.\n\
\n\
BUGS\n\
     Only PNG images do not have artificial horizontal lines in shaded\n\
     contours; the other formats still do, at least when rendered to the\n\
     screen with antialising on. This is a feature of the Cairo Vector\n\
     Graphics Library and/or the particular viewers. Since the SVG standard\n\
     makes provision for selective antialiasing during rendering, it is \n\
     possible that Cairo will support this feature at some point.\n\
     The PS output still needs some tuning; the bounding box seem to be\n\
     correct, though.\n\
     Currently, the pipe option on output only works with PNG output.\n\
\n\
TO DO\n\
     Allow for compressed SVG and PS output through zlib.\n\
     Allow for output to STDOUT and/or to pipes.\n\
     Allow for images to be rendered directly on the screen.\n\
\n\
VERSION\n\
     GxYat Version %s built with Cairo Version %s\n\
\n\
AUTHOR\n\
     Arlindo da Silva <dasilva@opengrads.org> based on gxpng\n\
     by Matthias Muennich. \n\
\n\
COPYRIGHT\n\
     Copyright (c) 2008 Arlindo da Silva\n\
     This is free software released under the GNU General Public\n\
     License; see the source for copying conditions.\n\
     There is NO  warranty;  not even for MERCHANTABILITY or FITNESS \n\
     FOR A PARTICULAR PURPOSE.\n\
\n\
SEE ALSO\n\
     Other GrADS metafile translators: gxeps, gxps, gxtran, gxpng, gximg\n\
     GrADS Home Page: http://grads.iges.org/grads \n\
     Cairo Vector Graphics Library: http://cairographics.org/ \n\n"

int printHelp(int rc){
  int i;
  if ( rc ) 
    fprintf(stderr,manText(),VERSION,cairo_version_string());
  else
    fprintf(stderr,"Enter '%s -h' for additional information.\n", MYNAME);
  exit(1);
}

/* ------------------------------------------------------------------------ */
cairo_surface_type_t myImageGetSurfType ( char *fout ) {
  int n;
  n = strlen(fout)-4;
  if ( strcmp(&fout[n],".svg") == 0 || strcmp(&fout[n],".SVG") == 0 ) 
    return CAIRO_SURFACE_TYPE_SVG;
  else if ( strcmp(&fout[n],".pdf") == 0 || strcmp(&fout[n],".PDF") == 0 ) 
    return CAIRO_SURFACE_TYPE_PDF;
  else if ( strcmp(&fout[n+1],".ps") == 0 || strcmp(&fout[n],".PS") == 0 ) 
    return CAIRO_SURFACE_TYPE_PS;
  else 
    return CAIRO_SURFACE_TYPE_IMAGE;

} 

/* ------------------------------------------------------------------------ */

void myImageInit (myImage *im, double sx, double sy, myColor bkg ) {          
	// cairo_set_fill_rule (im->cr,CAIRO_FILL_RULE_WINDING);
	// cairo_set_fill_rule (im->cr,CAIRO_FILL_RULE_EVEN_ODD);
	// cairo_set_tolerance(im->cr,1.0);
	//cairo_rectangle ( im->cr, 1, 1,  sx,  sy);
	//cairo_fill(im->cr);
	//P.Romero
	//used 'paint' instead of 'fill rectangle' to set background color
	//set the operator to source
	cairo_set_line_width(im->cr,1);
	cairo_set_line_cap(im->cr,CAIRO_LINE_CAP_ROUND);
	cairo_set_source_rgba(im->cr,bkg.r,bkg.g,bkg.b,bkg.a);
	cairo_set_operator (im->cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (im->cr);
}
/* ------------------------------------------------------------------------ */

void myImageFilename (char *fname, char *fout, char *ext, int n) {
  int i;

  /* add image number to file name if necessary */
  if ( strstr(fout,"%") ) 
    sprintf(fname,fout,n);
  else if ( n > 1 ) {  /* user forgot the %d, add it anyway */
    i = strlen(fout)-strlen(ext)-1;
    strncpy(fname,fout,i); /* base name */
    fname[i]='\0';
    sprintf(fname,"%s-%d.%s",fname,n,ext);
  }
  else 
    strncpy(fname,fout,1023);

}

/* ------------------------------------------------------------------------ */

myImagePtr myImageCreate (double sx, double sy, char *fout, myColor bkg ) {
                        
  myImage *im;
  char fname[1024];
 
  im = (myImage *) malloc(sizeof(myImage));
  im->surface_type = myImageGetSurfType(fout);

  /* Create the surface */
  if ( im->surface_type == CAIRO_SURFACE_TYPE_SVG ) {
    myImageFilename(fname,fout,"svg",1);
    im->surface = cairo_svg_surface_create (fname,sx,sy);
  }
  else if ( im->surface_type == CAIRO_SURFACE_TYPE_PDF )
    im->surface = cairo_pdf_surface_create (fout,sx,sy);
  else if ( im->surface_type == CAIRO_SURFACE_TYPE_PS ) {
    im->surface = cairo_ps_surface_create (fout,sx,sy);
    cairo_ps_surface_dsc_comment (im->surface, "%%Title: produced by gxyat");
    cairo_ps_surface_dsc_comment (im->surface, "%%PapeOrder: Ascend");
    cairo_ps_surface_dsc_comment (im->surface, "%%PaperSize: Letter");
    if ( sx > sy ) 
      cairo_ps_surface_dsc_comment (im->surface, "%%PageOrientation: Landscape");
    else
      cairo_ps_surface_dsc_comment (im->surface, "%%PageOrientation: Portrait");
  }
  else if ( im->surface_type == CAIRO_SURFACE_TYPE_IMAGE ) {
    im->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                              (int)sx,(int)sy);
  } else {
    fprintf(stderr,"%s: unknown surface type; plase check extension of input file name.\n%s: ",MYNAME);
    printHelp(0);

  }

  /* Creace cairo object on given surface */
  im->cr = cairo_create(im->surface);
  cairo_translate(im->cr,-1.0,-1.0);  /* only once on creation */
  myImageInit (im, sx, sy, bkg );     /* make sure we have a clean slate */

  return im;

}

/* ------------------------------------------------------------------------ */

void myImageDestroy (myImagePtr im)
{
  cairo_surface_destroy(im->surface);
  cairo_destroy(im->cr);
  free(im);
}

/* ------------------------------------------------------------------------ */
//P.Romero
//since we're now passing alpha as a double, changed argument type
//we also dont need to change its value (divide by 127)
void myColorDef(myColor *c, int r, int g, int b, double a) {
	c->r = ( (double) r) / 255.0;
	c->g = ( (double) g) / 255.0;
	c->b = ( (double) b) / 255.0;
	c->a = a;
	c->def = 1;
}

/* ------------------------------------------------------------------------ */

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
 }

 /* Define rainbow colors */ 
 for ( i=0; i<16; i++ ) {
       myColorDef ( &Colors[i], r[i], g[i], b[i], 1.0 );
 }

 /* If needed, Initialize alpha channel parameters */
 if ( rgb_alpha[0]<0 )
   for ( i=0; i<MAXCLRS; i++ ) rgb_alpha[i] = 1.0;
       
}

/* ------------------------------------------------------------------------ */
//P.Romero
//added 'mask' argument
//added test for mask value, & set cairo operator accordingly
void myImageFilledRectangle(myImagePtr im, double x1, double y1, 
                            double x2, double y2, myColor c, int mask)
{
	cairo_save(im->cr);
	if (mask==1){
		cairo_set_operator (im->cr,CAIRO_OPERATOR_SOURCE);
	} else{
		cairo_set_operator (im->cr,CAIRO_OPERATOR_OVER);
	}
	cairo_set_source_rgba(im->cr,c.r,c.g,c.b,c.a);
	//printf("1: %f\n2: %f\n3: %f\n4: %f\n",x1,y1,(x2-x1),(y2-y1));
	//cairo_rectangle (im->cr, x1, y1, (x2-x1), (y2-y1));
	//cairo_rectangle (im->cr, 0, 0, 801, 601);
	cairo_move_to(im->cr, x1, y1);
	cairo_line_to(im->cr, x2, y1);
	cairo_line_to(im->cr, x2, y2);
	cairo_line_to(im->cr, x1, y2);
	cairo_fill ( im->cr );
	cairo_restore(im->cr);
}

/* ------------------------------------------------------------------------ */
/* Call back to allow wriitng PNG to a pipe or regular file */

static 
cairo_status_t fpwrite (fpwrite_t *closure, unsigned char *data, 
                        unsigned int length) {
  size_t n;
  /* open file or pipe */
  if ( !closure->is_open || !closure->stream) {
    if ( closure->fname[0] == '|' ) {
      closure->is_pipe=1;
      if ( ! (closure->stream = (FILE *) popen(closure->fname+1,"w")) ) 
           return CAIRO_STATUS_WRITE_ERROR;
    } else {
      closure->is_pipe = 0;
      if ( ! (closure->stream = (FILE *) fopen(closure->fname,"wb")) ) 
           return CAIRO_STATUS_WRITE_ERROR;
    }
    closure->is_open = 1;
  }
    /* Write the data */
    n = fwrite(data, sizeof(unsigned char), (size_t) length, closure->stream); 
    if ( n == length ) return CAIRO_STATUS_SUCCESS;
    else               return CAIRO_STATUS_WRITE_ERROR;

}

void myImageWrite (myImagePtr im, char *fout, int n, int verb)
{
  cairo_status_t status;
  char fname[1024];
  fpwrite_t closure;

  myImageFilename(fname,fout,"png",n);

  /* write out image */
  // status = cairo_surface_write_to_png(im->surface,fname);
  closure.fname = fname;
  closure.is_open = 0;
  status = cairo_surface_write_to_png_stream(im->surface,
          (cairo_write_func_t) fpwrite, &closure );
  if ( status != CAIRO_STATUS_SUCCESS ) {
    fprintf(stderr,"%s: could not write file %s. ",MYNAME,fname);
    printHelp(0);

  } else if ( verb ) 
    printf("     - Writing: PNG file %s\n",fname);

  if ( closure.is_open ) {
    if ( closure.is_pipe ) pclose(closure.stream);
    else                   fclose(closure.stream);
    closure.is_open = 0;
  }

}

/* ------------------------------------------------------------------------ */

myImagePtr myImageNewPage ( myImage *im, char *fout, myColor bkg,
                            double sx, double sy, int fcnt, 
                            cairo_surface_type_t surface_type,
                            cairo_antialias_t    antialias ) {

   char fname[1024];

   if ( surface_type == CAIRO_SURFACE_TYPE_IMAGE ) {
     im = myImageCreate(sx,sy,fout,bkg);
   } else if ( surface_type == CAIRO_SURFACE_TYPE_SVG ) {
   myImageFilename(fname,fout,"svg",fcnt); 
     im = myImageCreate(sx,sy,fname,bkg);
   } else { 
     myImageInit (im, sx, sy, bkg );  
   }
   cairo_set_antialias ( im->cr, antialias ); /* just in case */

   return (myImage *) im;

}

/* ------------------------------------------------------------------- */

void parseArg(int argc,char *argv[],struct options *o,char **fin, char **fout){
     register int i,j;
     if(argc==1) printHelp(1);
     for (i=1;i<argc;i++) {
       if (*(argv[i])=='-' && i<argc-1 ) {  /* parse options */
         j = 0;
         while (*(argv[i]+(++j))) {
           if (*(argv[i]+j)=='i') {*fin = argv[++i];break;}
           else if (*(argv[i]+j)=='o') {*fout = argv[++i];break;}
           else if (*(argv[i]+j)=='r') o->reverse = 1;
           else if (*(argv[i]+j)=='h') printHelp(1);
           else if (*(argv[i]+j)=='x') {
	     sscanf(argv[++i],"%d",&(o->sx));break;
	   }
           else if (*(argv[i]+j)=='y') {
	     sscanf(argv[++i],"%d",&(o->sy));break;
	   }
           else if (*(argv[i]+j)=='w') {
	     sscanf(argv[++i],"%d",&(o->ws));break;
	   }
           else if (*(argv[i]+j)=='v') {
	       o->verbose = 1;
	       printf("\nThis is GxYat v%s ", VERSION );
               printf("built with Cairo v%s\n\n",
                      cairo_version_string());
	   }
           else if (*(argv[i]+j)=='f') o->pltfill = 1;

		   else { 
	     fprintf(stderr,"%s: unknown option: %s. ",MYNAME,argv[i]);
	     printHelp(0);
	   }
         }
       } else if (*(argv[i])=='+' && i<argc-1 ) {  /* parse options */
         j = 0;
         while (*(argv[i]+(++j))) {
           if (*(argv[i]+j)=='a') o->antialias = 0;
           else { 
	     fprintf(stderr,"%s: unknown option: %s. ",MYNAME, argv[i]);
	     printHelp(0);
	   }
	 }
       }
       else  /* No command line "-" */
        *fin=argv[i];
    }
    return;
}

/* ------------------------------------------------------------------------ */

void openFiles(char **fin, char **fout,short verbose){ /* Open files */
  int i;

#ifdef ___GAUDX___

  /* In UDC mode, no need to specify input file or "-o" for 
     the output file.  */
  if (*fout==NULL) {    /* no outfile so far */
    if (*fin==NULL) {
      fprintf (stderr,"%s: missig output file. ",MYNAME,*fin);
      printHelp(0);
    } else {
      *fout = *fin;
    }
  }

#else

  if (*fin==NULL) {
      fprintf (stderr,"%s: missig input file. ",MYNAME,*fin);
      printHelp(0);
      /* 
      *fin = (char *) malloc(sizeof(char)*150);
      fgets(*fin,150,stdin);
      printf("read infile = %s\n",*fin);
      */
  }
  if(strcmp(*fin,"-")==0) infile=stdin;
  else                    infile = fopen(*fin ,"rb");
  if (infile == NULL) {
    *fin=strcat(*fin,"."IN_EXT);
    infile = fopen(*fin,"rb");
    if (infile == NULL) {
      (*fin)[strlen(*fin)-3]='\0';
      fprintf (stderr,"%s: input file %s[."IN_EXT"] not found. ",MYNAME,*fin);
      printHelp(0);
    }
  }

#endif

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
#ifdef ___GAUDX___
    printf("GrADS metafile: (memory buffer)\n");
#else
    printf("GrADS metafile: %s\n",*fin);
#endif
    if(strcmp(*fout,"-")==0) printf("     Output to: stdout\n");
    if(strcmp(*fout,"|")==0) printf("     Output to: pipe \\n"),*fout+1;
    else                     printf("   Output file: %s\n",*fout);
  }
  return;
}

size_t udx_mread (short *ptr, size_t size, size_t nmemb, FILE *stream);

/* ---------------------------------------------------------------------- 

                                   M A I N
 
 ----------------------------------------------------------------------- */
#ifdef ___GAUDX___
int c_gxyat ( int argc, char **argv, struct gacmn *pcm) {
#else
int main (int argc, char *argv[])  {
#endif

   myImage *im;
   myColor Colors[MAXCLRS];
   
	double dx=0,dy=0;
	
   short cmd, opts[7];
   int col=0;
   double sx=SX,sy=SY;
   double sh;
   double x=1, y=1, x1, x2, y1, y2;
   double blowx,blowy;
   double width;
   struct options o;                       /* command line options */
   register int  i;
   short fcnt, new_page;
   char *fin=NULL,*fout=NULL;              /* file names */
   fpos_t infile_pos;
   double Width[12] = {                      /* width (units: .001 inch) */
                       6 ,8 ,10 ,14 ,16 ,20 ,
	               24 ,28 ,30 ,34 ,38 ,42 };
   double ws; /* line width scale */
   double del=0;
   char svg_fname[1024];
   cairo_surface_type_t surface_type;
   cairo_antialias_t    antialias;   

#ifdef ___GAUDX___
   int rc=0;
   rc = setjmp(Env);
   if ( rc ) return rc;
   /* initialize internal pointers for reading metafile
      memory buffer */
   udx_mread (&cmd, -1, -1, infile);
#endif

   o.reverse=o.verbose=o.sx=o.sy=o.pltfill=0;o.ws=1.0,o.antialias=1; /* defaults */


/*                           Initialization                         
                             --------------
*/

  initColors(Colors, MAXCLRS);            /* Initialize colors */

  parseArg(argc,argv,&o,&fin,&fout);    /* Parse command line arguments */
  openFiles(&fin,&fout,o.verbose);      /* open files */

  /* For PS/PDF default sizes for Letter size */
  surface_type = myImageGetSurfType(fout);
  if ( surface_type == CAIRO_SURFACE_TYPE_PDF ||
       surface_type == CAIRO_SURFACE_TYPE_PS ) {
    sx = 11.0 * 72.0;
    sy = 8.5 * 72.0;
  } 

  /* Do anti-aliasing for line drawing if so desidered */
  if ( o.antialias )
    antialias = CAIRO_ANTIALIAS_DEFAULT;
  else
    antialias = CAIRO_ANTIALIAS_NONE;

  /*
                            Translate Metafile
                            ------------------
  */

  new_page = 0; /* this is right! */
  fcnt = 0;
  
  while (1) {

    fread (&cmd, sizeof(short), 1, infile);

    if (cmd==-9) {				/* End of plotting */
      if(o.verbose) printf ("    # of pages: %i\n",fcnt);
      if ( surface_type == CAIRO_SURFACE_TYPE_PDF || 
           surface_type == CAIRO_SURFACE_TYPE_PS   ) 
	   myImageDestroy(im);
      return(0);
    }

    if ( new_page ) {                          /* Setup a new page */
      fcnt++;
      im=myImageNewPage (im,fout,Colors[0],sx,sy,fcnt,surface_type,antialias);
      col=1;
      cairo_set_source_rgba(im->cr,Colors[col].r,Colors[col].g,Colors[col].b,Colors[col].a);
      new_page = 0;
    }

    if (cmd==-11){				/* Draw to */
      fread (opts, sizeof(short), 2, infile);
      x=del+(double)(opts[0]*blowx-dx);
      y=del+(double)((sy+1)-opts[1]*blowy-dy);
      cairo_line_to(im->cr,x,y);
    }

    else if (cmd==-10){				/* Move to */
      cairo_stroke(im->cr);
      fread (opts, sizeof(short), 2, infile);
      x=del+(double)(opts[0]*blowx-dx);
      y=del+(double)((sy+1)-opts[1]*blowy-dy);
      cairo_move_to(im->cr,x,y);
    }

    else if (cmd==-4) {				/* Set line width */
      cairo_stroke(im->cr);
      fread (opts, sizeof(short), 2, infile);
      i = opts[0];
      if (i>12) i=12;
      else if (i<1) i=1;
      width = ws * Width[i-1]/Width[3];
      cairo_set_line_width(im->cr,width);
    }

    else if (cmd==-3) {			       /* Set color */
      cairo_stroke(im->cr);
      fread (opts, sizeof(short), 1, infile);
      col = opts[0];
      if (col<0) col=0;
      if (col>MAXCLRS-1) col=MAXCLRS-1;
      if (!(Colors[col].def)) col=15;
      cairo_set_source_rgba(im->cr,Colors[col].r,Colors[col].g,Colors[col].b,Colors[col].a);
	  //P.Romero
	  //added this test for mask value, and set cairo operator accordingly
		if (rgb_amask[col]==1){
			cairo_set_operator (im->cr,CAIRO_OPERATOR_SOURCE);
		} else{
			cairo_set_operator (im->cr,CAIRO_OPERATOR_OVER);
		}
	}

    else if (cmd==-7){				/* Start fill */
      fread (opts, sizeof(short), 1, infile);
      cairo_stroke(im->cr);
      cairo_set_antialias ( im->cr, CAIRO_ANTIALIAS_NONE );
      del = 0.5;
   }

    else if (cmd==-8){				/* End fill */
      cairo_fill(im->cr);
      cairo_set_antialias ( im->cr, antialias );
      del = 0;
    }

    else if (cmd==-6){				/* Rectangle (background) fill */
      fread (opts, sizeof(short), 4, infile);
      cairo_stroke(im->cr);
	  x1=opts[0];
	  x2=opts[1];
	  y1=opts[2];
	  y2=opts[3];
      x1 = (x1*blowx-dx);
	  x2 = (x2*blowx-dx);
	  y1 = ((sy+1)-((y1*blowy))-dy);
	  y2 = ((sy+1)-((y2*blowy))-dy);
      myImageFilledRectangle(im, x1, y1, x2, y2, Colors[col],rgb_amask[col]);
    }

    else if (cmd==-1) {				/* Start of plotting */
		fread (opts, sizeof(short), 2, infile);
		if(opts[0]<opts[1]) {sh=sx; sx=sy; sy=sh;}
		
		if(o.sx) 
		{
			sx=o.sx;
			if(o.sy==0) sy=sx *(opts[1]/100)/(opts[0]/100);
		}
		if(o.sy) 
		{
			sy=o.sy;
			if(o.sx==0) sx=sy*(opts[0]/100)/(opts[1]/100);
		}
		ws = o.ws * sqrt(sx*sy/(800.*600.)); /* line width scale */
		
		/* picture size in metafile: opts[0] x opts[1] y*/ 
		//scaling was previously off by 1 pixel, top&bottom
		//is this due to how cairo scales its image pixels?
		//we extend the user-defined image size by 1pixel
		//in order to correctly size/"line up" our drawing with canvas
		blowx = (sx+1) / (double)opts[0];  
		blowy = (sy+1) / (double)opts[1];

		//this is for the 'plot fill' option
		//calc's the dimensions of ONLY the plot area
		//adjusts the x&y scale factors
		//sets the dx&dy offsets to align plot origin with image origin
		#ifdef ___GAUDX___
		if(o.pltfill)
		{
			int xmin = (int)(pcm->xsiz1*1000.0+0.5);
			int xmax = (int)(pcm->xsiz2*1000.0+0.5);
			int ymin = (int)(pcm->ysiz1*1000.0+0.5);
			int ymax = (int)(pcm->ysiz2*1000.0+0.5);
			
			int xpsz=xmax-xmin;
			int ypsz=ymax-ymin;
			
			//we add 1 pixel to y scale factor in order to cleanly clip plot's edge
			//frame SHOULD ALWAYS be turned off for this to look good.
			//with frame on, edge defects will be visible
			//not clear why, but its not necessary to add 1px to x scale factor 
			//P.Romero
			blowx=(sx)/xpsz;
			blowy=(sy+1)/ypsz;
			dx=xmin*blowx;
			dy=-1*ymin*blowy;
		}
		#endif

		if(!o.reverse) {
			myColorDef(&Colors[0], 255, 255, 255, 1.0); /* white background */
			myColorDef(&Colors[1],   0,   0,   0, 1.0); /* black foreground */
		}
		/* PS/PDF images are created only once, here; PNG/SVG have
		 new files at each new page */
		if ( surface_type == CAIRO_SURFACE_TYPE_PDF || 
			surface_type == CAIRO_SURFACE_TYPE_PS   ) { 
			if(o.verbose) 
				printf("    Image size: %d x %d points (%2.1f x %2.1f inches)\n",
					(int)sx,(int)sy,sx/72.,sy/72.);
				im = myImageCreate(sx,sy,fout,Colors[0]);
		} else {
			if(o.verbose) 
				printf("    Image size: %d x %d pixels\n",(int)sx,(int)sy);
		}
		new_page = 1;
    }
    else if (cmd==-2) {				/* End of Page */
      cairo_stroke(im->cr);
      if ( surface_type == CAIRO_SURFACE_TYPE_IMAGE ) {
	   myImageWrite(im, fout, fcnt, o.verbose);
           myImageDestroy(im);
      } else if ( surface_type == CAIRO_SURFACE_TYPE_SVG ) {
           myImageFilename(svg_fname,fout,"svg",fcnt);
	   if ( o.verbose ) 
	     printf("     - Writing: SVG file %s\n",svg_fname);
           myImageDestroy(im);
      } else { 
           cairo_show_page(im->cr);
      }
      new_page = 1;
    }
    else if (cmd==-5){				/* Define new color */
      fread (opts, sizeof(short), 4, infile);
      i = opts[0];
      if (i>15 && i<MAXCLRS) 
          myColorDef(&Colors[i],opts[1],opts[2],opts[3],rgb_alpha[i]);
    }
    else if (cmd==-15) {		/* Begin string drawing -- ignore*/
    }
    else if (cmd==-16) {		/* End string drawing -- skip */
      fread (opts, sizeof(short), 7, infile); /* get skip length */
      fseek(infile,
            (long)(opts[0]/sizeof(short)*sizeof(short)+2*sizeof(short)),
             SEEK_CUR); 
    }
    else if (cmd==-20) {		/* Draw button -- ignore */
      fread (opts, sizeof(short), 1, infile);
    }
    else {
#ifdef ___GAUDX___
       printf ("Fatal error: Invalid command \"%i\" found in metafile memory bufer.\n",cmd);
#else
       printf ("Fatal error: Invalid command \"%i\" found in metafile \"%s\".\n",cmd,fin);
       printf ("Is \"%s\" really a GrADS (v1.5 or higher) metafile?\n",fin);
      return(1);
#endif
    }
  }
}


//---------------------------------------------------------------------

// P.Romero
// set_rgba User Defined Command
// 

#ifdef ___GAUDX___

#define manUdcText()   "\
NAME\n\
     %s - define color including alpha channel\n\
\n\
SYNOPSIS\n\
     %s color red green blue alpha [mask]\n\
\n\
DESCRIPTION\n\
     This User Defined Command (UDC) defines *color* given its\n\
     RGB triplet along with its *alpha* channel.\n\
\n\
     color   integer in the range [16-%d]\n\
     red, \n\
     green,\n\
     blue    integers in the range [0-255]\n\
     alpha   float, with alpha ranging from 0.0 (fully transparent)\n\
             to 1.0 (fully opaque).\n\
\n\
    The optional parameter *mask* determines whether the color below\n\
    it is visible or not.\n\
\n\
    mask=0  --- if alpha<1, *color* will be semi-transparent and the\n\
                colors below it will be visible (default)\n\
    mask=1  --- if alpha<1, *color* will be semi-transparent but the\n\
                colors below it will *not* be visible \n\
\n\
EXAMPLE\n\
    In order to emulate the -t option in printim \n\
\n\
       ga-> set rgb 60 125 125 125\n\
       ga-> printim img.png -t 60\n\
\n\
       Specify alpha=0 and mask=1\n\
\n\
       ga-> set_rgba 60 125 125 125 0 1\n\
       ga-> gxyat img.png\n\n"

int c_rgba ( int argc, char **argv, struct gacmn *pcm) {

        int i, cc=0, mm=0;
	double aa=1.0;

        char cmd[256];

        if ( rgb_alpha[0]<0 )
           for ( i=0; i<MAXCLRS; i++ ) rgb_alpha[i] = 1.0;

	if (argc < 5 || argc > 7) goto err;
	
        /* If needed, Initialize alpha channel parameters */
	if (intprs(argv[1],&cc) == NULL ) goto err;

        /* Issue the rugular "set RGB" command */
        snprintf(cmd,255,"set rgb %s %s %s %s",argv[1],argv[2],argv[3],argv[4]);
	if(gaset(cmd,NULL,pcm)) goto err;

        /* alpha channel */
        if (argc>5)
		if (getdbl(argv[5],&aa) == NULL ) goto err;

        /* optional mask */
	if (argc>6)
		if (intprs(argv[6],&mm) == NULL ) goto err;
	
	if ( cc<0 || cc>= MAXCLRS ||
             aa > 1.0 || aa < 0.0 || (mm!=0 && mm!=1 )) goto err;

        /* Record the alpha channel parameters for later use */
	rgb_alpha[cc]=aa;
	rgb_amask[cc]=mm;

        return(0);

err:
        fprintf(stderr,manUdcText(),argv[0],argv[0],MAXCLRS-1);
	return(1);

}
#endif
