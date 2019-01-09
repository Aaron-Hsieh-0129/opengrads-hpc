#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "grads.h"

/* command line options (flags) */
struct options {int reverse, antialias, verbose, sx, sy, ws;};
static char out_ext[] = "shp";
static FILE *infile;
static char  MYNAME[] = "shp_print";
static char VERSION[] = "0.0.0";
static char message[2048];

/* ------------------------ Local Types-------------------------------- */

  /* Image */
  typedef struct
  {
    int flag;
  }
  myImage, *myImagePtr;

#define MXCOL 100         /* Max number of colors */

/* myColor */
typedef struct
{
  int def;
  double r, g, b, a;
}
myColor, *myColorsPtr;

/* ------------------------------------------------------------------------ */
#define manText() "\
\n\
NAME\n\
     TO DO\n\
"
int printHelp(int rc){
  if ( rc ) 
    fprintf(stderr,manText(),VERSION);
  else
    fprintf(stderr,"Enter '%s -h' for additional information.\n", MYNAME);
  exit(1);
}


void myImageInit (myImage *im, double sx, double sy, myColor bkg ) {          

}

/* ------------------------------------------------------------------------ */

myImagePtr myImageCreate (double sx, double sy, char *fout, myColor bkg ) {
                        
  myImage *im;
  char fname[1024];
 
  im = (myImage *) malloc(sizeof(im));

  return im;

}

/* ------------------------------------------------------------------------ */

void myImageDestroy (myImagePtr im)
{
  free(im);
}

/* ------------------------------------------------------------------------ */
void myColorDef(myColor *c, int r, int g, int b, int a) {
  c->r = ( (double) r) / 255.0;
  c->g = ( (double) g) / 255.0;
  c->b = ( (double) b) / 255.0;
  c->a = ( (double) a) / 127.0; /* double check this */
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
       myColorDef ( &Colors[i], r[i], g[i], b[i], 0 );
 }

}

/* ------------------------------------------------------------------------ */

void myImageFilledRectangle(myImagePtr im, double x1, double y1, 
                            double x2, double y2, myColor c )
{

}

/* ------------------------------------------------------------------------ */

void myImageWrite (myImagePtr im, char *fout, int n, int verb)
{

}

/* ------------------------------------------------------------------------ */

myImagePtr myImageNewPage ( myImage *im, char *fout, myColor bkg,
                            double sx, double sy, int fcnt ) {

   char fname[1024];
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
           else if (*(argv[i]+j)=='x') {
	     sscanf(argv[++i],"%d",&(o->sx));break;
	   }
           else if (*(argv[i]+j)=='y') {
	     sscanf(argv[++i],"%d",&(o->sy));break;
	   }
           else if (*(argv[i]+j)=='w') {
	     sscanf(argv[++i],"%d",&(o->ws));break;
	   }
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
    printf("GrADS metafile: (memory buffer)\n");
    if(strcmp(*fout,"-")==0) printf("     Output to: stdout\n");
    else                     printf("   Output file: %s\n",*fout);
  }
  return;
}

size_t udx_mread (short *ptr, size_t size, size_t nmemb, FILE *stream);

/*....................................................................... */

int cmd_print ( int argc, char **argv, struct gacmn *pcm) {

   myImage *im;
   myColor Colors[MXCOL];

   short cmd, opts[7];
   int col=0;
   double sx,sy,sh;
   double blowx,blowy;
   double x=1, y=1, x1, x2, y1, y2;
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

   int rc;


   /* Just a stub for now */
   sprintf(message,"%s: not implemented yet\n", argv[0]);
   gaprnt (0,message);
   return (1);


   /* initialize internal pointers for reading metafile
      memory buffer */
   udx_mread (&cmd, -1, -1, infile);

   o.reverse=o.verbose=o.sx=o.sy=0;o.ws=1.0,o.antialias=1; /* defaults */


/*                           Initialization                         
                             --------------
*/

  initColors(Colors, MXCOL);            /* Initialize colors */

  parseArg(argc,argv,&o,&fin,&fout);    /* Parse command line arguments */
  openFiles(&fin,&fout,o.verbose);      /* open files */

  /*
                            Translate Metafile
                            ------------------
  */

  new_page = 0; /* this is right! */
  fcnt = 0;
  while (1) {

    udx_mread (&cmd, sizeof(short), 1, infile);

    if (cmd==-9) {				/* End of plotting */
      if(o.verbose) printf ("    # of pages: %i\n",fcnt);
      return(0);
    }

    if ( new_page ) {                          /* Setup a new page */
      fcnt++;
      im=myImageNewPage (im,fout,Colors[0],sx,sy,fcnt);
      col=1;
      new_page = 0;
    }

    if (cmd==-11){				/* Draw to */
      udx_mfread (opts, sizeof(short), 2, infile);
      x=del+(double)opts[0]*blowx;
      y=del+(double)(sy-opts[1]*blowy);
    }

    else if (cmd==-10){				/* Move to */
      udx_mread (opts, sizeof(short), 2, infile);
      x=del+(double)opts[0]*blowx;
      y=del+(double)(sy-opts[1]*blowy);
    }

    else if (cmd==-4) {				/* Set line width */
      udx_mread (opts, sizeof(short), 2, infile);
      i = opts[0];
      if (i>12) i=12;
      else if (i<1) i=1;
      width = ws * Width[i-1]/Width[3];
    }

    else if (cmd==-3) {			       /* Set color */
      udx_mread (opts, sizeof(short), 1, infile);
      col = opts[0];
      if (col<0) col=0;
      if (col>MXCOL-1) col=MXCOL-1;
      if (!(Colors[col].def)) col=15;
    }

    else if (cmd==-7){				/* Start fill */
      udx_mread (opts, sizeof(short), 1, infile);
      del = 0.5;
   }

    else if (cmd==-8){				/* End fill */
      del = 0;
    }

    else if (cmd==-6){				/* Rectangle fill */
      udx_mread (opts, sizeof(short), 4, infile);
      x1 = ((double) opts[0] )*blowx;
      y1 = (double) (sy-opts[3]*blowy);
      x2 = ((double) opts[1])*blowx;
      y2 = (double) (sy-opts[2]*blowy);
      myImageFilledRectangle(im, x1, y1, x2, y2, Colors[col]);
    }

    else if (cmd==-1) {				/* Start of plotting */
      udx_mread (opts, sizeof(short), 2, infile);
      if(opts[0]<opts[1]) {sh=sx; sx=sy; sy=sh;}
      if(o.sx) {
	sx=o.sx;
	if(o.sy==0) sy=sx *(opts[1]/100)/(opts[0]/100);
      }
      if(o.sy) {
	sy=o.sy;
	if(o.sx==0) sx=sy*(opts[0]/100)/(opts[1]/100);
      }
      ws = o.ws * sqrt(sx*sy/(800.*600.)); /* line width scale */
      /* picture size in metafile: opts[0] x opts[1] */ 
      blowx=(float)sx/(float)opts[0];  
      blowy=(float)sy/(float)opts[1];

      if(!o.reverse) {
	myColorDef(&Colors[0], 255, 255, 255, 0); /* white background */
	myColorDef(&Colors[1],   0,   0,   0, 0); /* black foreground */
      }
      new_page = 1;
    }

    else if (cmd==-2) {				/* End of Page */
      new_page = 1;

    }

    else if (cmd==-5){				/* Define new color */
      udx_mread (opts, sizeof(short), 4, infile);
      i = opts[0];
      if (i>15 && i<MXCOL) 
          myColorDef(&Colors[i],opts[1],opts[2],opts[3],0);
    }

    else if (cmd==-15) {		/* Begin string drawing -- ignore*/
    }

    else if (cmd==-16) {		/* End string drawing -- skip */
      udx_mread (opts, sizeof(short), 7, infile); /* get skip length */
    }

    else if (cmd==-20) {		/* Draw button -- ignore */
      udx_mread (opts, sizeof(short), 1, infile);
    }

    else {
      printf ("Fatal error: Invalid command \"%i\" found in metafile memory bufer.\n",cmd);
      printf ("Is \"%s\" really a GrADS (v1.5 or higher) metafile?\n",fin);
      return(1);
    }

  }

}
