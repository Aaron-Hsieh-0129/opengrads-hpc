/* Copyright (C) 1988-2016 by George Mason University. See file COPYRIGHT for more information. */

/* Main program for GrADS (Grid Analysis and Display System).
   This program loops on commands from the user, and calls the
   appropriate routines based on the command entered.             */

/* GrADS originally authored by Brian E. Doty.  Many others have 
   contributed over the years...  

   Jennifer M. Adams 
   Reinhard Budich 
   Luigi Calori 
   Wesley Ebisuzaki 
   Mike Fiorino 
   Graziano Giuliani 
   Matthias Grzeschik
   Tom Holt 
   Don Hooper 
   James L. Kinter 
   Steve Lord 
   Gary Love 
   Karin Meier 
   Matthias Munnich 
   Uwe Schulzweida 
   Arlindo da Silva 
   Michael Timlin 
   Pedro Tsai 
   Joe Wielgosz 
   Brian Wilkinson 
   Katja Winger

   We apologize if we have neglected your contribution --
   but let us know so we can add your name to this list.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
/* If autoconfed, only include malloc.h when it's presen */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#else /* undef HAVE_CONFIG_H */
#include <malloc.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include "grads.h"

#if USEGUI == 1
#include "gagui.h"
#endif

#if READLINE ==1
#include <time.h>
#include <stdlib.h>
#include <readline/history.h>
extern gaint history_length;
void write_command_log(char *logfile);
#endif

struct gacmn gcmn;  
static struct gawgds wgds;   
extern struct gamfcmn mfcmn;

#ifdef OPENGRADS
int Main (int argc, char *argv[])  {
#else
int main (int argc, char *argv[])  {
#endif

void command_line_help(void) ;
void gxdgeo (char *);
void gxend (void);
void gatxti(gaint on, gaint cs);
char *gatxtl(char *str,gaint level);

char cmd[1024];
gaint rc,i,j,land,port,cmdflg,hstflg,gflag,xwideflg,killflg,ratioflg;
gaint metabuff,size=0,g2size=0;
gaint txtcs=-2;
gaint ipcflg = 0; /* for IPC friendly interaction via pipes */
char *icmd,*arg,*rc1;
void gasigcpu() ;
gaint wrhstflg=0; 
gadouble aspratio;
char *logfile,*userhome=NULL;

/*--- common block sets before gainit ---*/
gcmn.batflg = 0;
land = 0;
port = 0;
cmdflg = 0;
metabuff = 0;
hstflg = 1; 
gflag = 0;
xwideflg = 0;
icmd = NULL;
arg = NULL;
rc1 = NULL;
killflg = 0;
ratioflg = 0;
aspratio = -999.9;

#if READLINE == 1
#ifdef __GO32__  /* MSDOS case */ 
logfile= (char *) malloc(22);
logfile= "c:\windows\grads.log";
#else  /* Unix */
userhome=getenv("HOME");
if (userhome==NULL) {
  logfile=NULL;
}
else {
  logfile= (char *) malloc(strlen(userhome)+12);
  if(logfile==NULL) {
    printf("Memory allocation error for logfile name.\n");
  }
  else {
    strcpy(logfile,userhome);
    strcat(logfile,"/.grads.log");
  }
}
#endif /* __GO32__ */
#endif /* READLINE == 1 */

if (argc>1) {
  for (i=1; i<argc; i++) {
    if (*(argv[i])=='-' &&
	*(argv[i]+1)=='h' && *(argv[i]+2)=='e' && *(argv[i]+3)=='l' && *(argv[i]+4)=='p') {
      command_line_help();     /* answer a cry for help */
      return(0);
    } else if (cmdflg==1) {    /* next arg is the command to execute */
      icmd = argv[i];
      cmdflg = 0;
    } else if (metabuff) {     /* next arg is the metafile buffer size */
      arg = argv[i];
      rc1 = intprs(arg,&size);
      if (rc1!=NULL) metabuff = 0;
    } else if ((txtcs==-1) && (*argv[i]!='-')) {  /* next arg is optional colorization scheme */
      txtcs = (gaint) strtol(argv[i], (char **)NULL, 10);
      if (txtcs>2) {
        printf("Valid colorization schemes are 0, 1, or 2. Colorization option %d ignored. \n",txtcs);
        txtcs = -2;
      }
    } else if (ratioflg) {    /* next arg is aspect ratio */
        aspratio = atof(argv[i]);
        ratioflg = 0;
    } else if (gflag) {        /* next arg is the geometry string */
      gxdgeo(argv[i]);
      gflag=0;
    } else if (wrhstflg && *(argv[i])!='-') {   /* next arg is optional log file name */
        logfile=argv[i];
    } else if (*(argv[i])=='-') {
      j = 1;
      while (*(argv[i]+j)) {
	if      (*(argv[i]+j)=='a') ratioflg = 1;    /* aspect ratio to follow */
	else if (*(argv[i]+j)=='b') gcmn.batflg = 1; /* batch mode */
	else if (*(argv[i]+j)=='c') cmdflg = 1;      /* command to follow */
	else if (*(argv[i]+j)=='C') txtcs = -1;      /* text color scheme */
	else if (*(argv[i]+j)=='E') hstflg = 0;      /* disable command line editing */
	else if (*(argv[i]+j)=='g') gflag = 1;       /* geometry specification to follow */
	else if (*(argv[i]+j)=='H') wrhstflg = 1;    /* write history to log file */
	else if (*(argv[i]+j)=='l') land = 1;        /* landscape mode */
	else if (*(argv[i]+j)=='m') metabuff = 1;    /* metafile buffer size to follow */
	else if (*(argv[i]+j)=='p') port = 1;        /* portrait mode */
	else if (*(argv[i]+j)=='W') xwideflg = 1;    /* use software to control wide lines (undocumented) */
	else if (*(argv[i]+j)=='u') {                /* unbuffer output: needed for IPC via pipes */
	  hstflg = 0;                                /* no need for readline in IPC mode */
	  ipcflg = 1;
	  setvbuf(stdin,  (char *) NULL,  _IONBF, 0 );
	  setvbuf(stdout, (char *) NULL,  _IONBF, 0 );
	  setvbuf(stderr, (char *) NULL,  _IONBF, 0 );
	}
	else if (*(argv[i]+j)=='x') killflg = 1;     /* quit after finishing (usually used with -c) */
	else printf ("Unknown command line option: %c\n",*(argv[i]+j));
	j++;
      }
    } else printf ("Unknown command line keyword: %s\n",argv[i]);
  }
}

if (txtcs > -2) gatxti(1,txtcs); /* turn on text colorizing */

if (ratioflg==1) printf ("Note: -a option was specified, but no aspect ratio was provided\n");
if (cmdflg==1)   printf ("Note: -c option was specified, but no command was provided\n");
if (gflag==1)    printf ("Note: -g option was specified, but no geometry specification was provided\n");
if (metabuff==1) printf ("Note: -m option was specified, but no metafile buffer size was provided\n");

if (ipcflg) printf("\n<IPC>" );  /* delimit splash screen */

printf ("\nGrid Analysis and Display System (GrADS) Version %s\n",gatxtl(GRADS_VERSION,0));
printf ("Copyright (C) 1988-2016 by George Mason University\n");
printf ("GrADS comes with ABSOLUTELY NO WARRANTY\n");
printf ("See file COPYRIGHT for more information\n\n");

gacfg(0);

#ifdef OPENGRADS
gaudi(&gcmn); /* Initialize OpenGrADS User Defined Extensions */
#endif


if (!land && !port && aspratio<-990) {
  nxtcmd (cmd,"Landscape mode? ('n' for portrait): ");
  if (cmd[0]=='n') port = 1;
}
if (port) {
  gcmn.xsiz = 8.5;
  gcmn.ysiz = 11.0;
} else {
  gcmn.xsiz = 11.0;
  gcmn.ysiz = 8.5;
}
if (aspratio>-990) { /* user has specified aspect ratio */
  if (aspratio>0.2 && aspratio < 5.0) {   /* range is limited here. */
    if (aspratio < 1.0) {
      gcmn.xsiz = 11.0*aspratio;
      gcmn.ysiz = 11.0;
    } else {
      gcmn.ysiz = 11.0/aspratio;
      gcmn.xsiz = 11.0;
    }
  }
  else {
    gaprnt(1,"Warning: Aspect ratio must be between 0.2 and 5.0 -- defaulting to landscape mode\n");
  }
}

if(xwideflg) gxwdln();

gainit();
mfcmn.cal365=-999;
mfcmn.warnflg=2;
mfcmn.winx=-999;      /* Window x  */         
mfcmn.winy=-999;      /* Window y */     
mfcmn.winw=0;         /* Window width */ 
mfcmn.winh=0;         /* Window height */ 
mfcmn.winb=0;         /* Window border width */
gcmn.pfi1 = NULL;                     /* No data sets open      */
gcmn.pfid = NULL;
gcmn.fnum = 0;
gcmn.dfnum = 0;
gcmn.undef = -9.99e8;         /* default undef value */
gcmn.fseq = 10; 
gcmn.pdf1 = NULL;
gcmn.sdfwname = NULL;
gcmn.sdfwtype = 1;
gcmn.sdfwpad = 0;
gcmn.sdfrecdim = 0;
gcmn.sdfchunk = 0;
gcmn.sdfzip = 0;
gcmn.sdfprec = 8;
gcmn.ncwid = -999;
gcmn.xchunk = 0;
gcmn.ychunk = 0;
gcmn.zchunk = 0;
gcmn.tchunk = 0;
gcmn.echunk = 0;
gcmn.attr = NULL;
gcmn.ffile = NULL;
gcmn.sfile = NULL;
gcmn.fwname = NULL;
gcmn.gtifname = NULL;    /* for GeoTIFF output */
gcmn.tifname = NULL;     /* for KML output */
gcmn.kmlname = NULL;     /* for KML output */
gcmn.kmlflg = 1;         /* default KML output is an image file */
gcmn.shpfname = NULL;    /* for shapefile output */
gcmn.shptype = 2;        /* default shape type is line */
gcmn.fwenflg = BYTEORDER;
gcmn.fwsqflg = 0;        /* default is stream */
gcmn.fwexflg = 0;        /* default is not exact -- old bad way */
gcmn.gtifflg = 1;        /* default geotiff output format is float */
if (size) gcmn.hbufsz = size;
if (g2size) gcmn.g2bufsz = g2size;
gcmn.cachesf = 1.0;      /* global scale factor for netcdf4/hdf5 cache */
gcmn.fillpoly = -1;      /* default is to not fill shapefile polygons */
gcmn.marktype = 3;       /* default is to draw points as closed circe */
gcmn.marksize = 0.05;    /* default mark size */
for (i=0; i<32; i++) {
  gcmn.clct[i] = NULL;  /* initialize collection pointers */
  gcmn.clctnm[i] = 0;
}


gafdef();

gagx(&gcmn);

#if !defined(__CYGWIN32__) && !defined(__GO32__)
  signal(CPULIMSIG, gasigcpu) ;  /* CPU time limit signal; just exit   -hoop */
#endif

#if READLINE == 1
if (wrhstflg && logfile != NULL) {
  printf("Command line history in %s\n",logfile);
  history_truncate_file(logfile,256); 
  read_history(logfile); /* read last 256 cmd */
}
#endif

if (icmd) rc = gacmd(icmd,&gcmn,0);
else      rc = 0;
signal(2,gasig);  /* Trap cntrl c */

#if USEGUI == 1
if (!ipcflg) 
  gagui_main (argc, argv);   /*ams Initializes GAGUI, and if the environment
                               variable GAGUI is set it starts a GUI
                               script. Otherwise, it just returns. ams*/
#endif
if (ipcflg) printf("\n<RC> %d </RC>\n</IPC>\n",rc);

/* Main command line loop */
while (rc>-1) {

  if (killflg) return(99);

#if READLINE == 1
#if defined(MM_NEW_PROMPT) 
  char prompt[13];
  if (hstflg) {
    snprintf(prompt,12,"ga[%d]> ",history_length+1);
    rc=nxrdln(&cmd[0],prompt);
  }
#else
  if (hstflg) rc=nxrdln(&cmd[0],"ga-> ");
#endif
  else rc=nxtcmd(&cmd[0],"ga> ");
#else
  rc=nxtcmd(&cmd[0],"ga> ");
#endif
  
  if (rc < 0) {
    strcpy(cmd,"quit");   /* on EOF, just quit */
    printf("[EOF]\n");
  }

  if (ipcflg) printf("\n<IPC> %s", cmd );  /* echo command in IPC mode */

  gcmn.sig = 0;
  rc = gacmd(cmd,&gcmn,0);

  if (ipcflg) printf("\n<RC> %d </RC>\n</IPC>\n",rc);
}

/* All done */
gxend();

#if READLINE == 1
 if (wrhstflg) write_command_log(logfile);
#endif

exit(0);

}

/* query the global cache scale factor */
gadouble qcachesf (void) {
  return(gcmn.cachesf); 
}

/* Initialize most gacmn values.  Values involving real page size,
   and values involving open files, are not modified   */

void gainit (void) {
gaint i;

  gcmn.wgds = &wgds;
  gcmn.wgds->fname = NULL;
  gcmn.wgds->opts = NULL;
  gcmn.hbufsz = 1000000;
  gcmn.g2bufsz = 10000000;
  gcmn.loopdim = 3;
  gcmn.csmth = 0;
  gcmn.cterp = 1;
  gcmn.cint = 0;
  gcmn.cflag = 0;
  gcmn.ccflg = 0;
  gcmn.cmin = -9.99e33;
  gcmn.cmax = 9.99e33;
  gcmn.arrflg = 0;
  gcmn.arlflg = 1;
  gcmn.ahdsiz = 0.05;
  gcmn.hemflg = -1;
  gcmn.aflag = 0;
  gcmn.axflg = 0;
  gcmn.ayflg = 0;
  gcmn.rotate = 0;
  gcmn.xflip = 0;
  gcmn.yflip = 0;
  gcmn.gridln = -9;
  gcmn.zlog = 0;
  gcmn.log1d = 0;
  gcmn.coslat = 0;
  gcmn.numgrd = 0;
  gcmn.gout0 = 0;
  gcmn.gout1 = 1;
  gcmn.gout1a = 0;
  gcmn.gout2a = 1;
  gcmn.gout2b = 4;
  gcmn.goutstn = 1;
  gcmn.cmark = -9;
  gcmn.grflag = 1;
  gcmn.grstyl = 5;
  gcmn.grthck = 4;
  gcmn.grcolr = 15;
  gcmn.blkflg = 0;
  gcmn.dignum = 0;
  gcmn.digsiz = 0.07;
  gcmn.reccol = 1;
  gcmn.recthk = 3;
  gcmn.lincol = 1;
  gcmn.linstl = 1;
  gcmn.linthk = 3;
  gcmn.mproj = 2;
  gcmn.mpdraw = 1;
  gcmn.mpflg = 0;
  gcmn.mapcol = -9; gcmn.mapstl = 1; gcmn.mapthk = 1;
  for (i=0; i<256; i++) {
    gcmn.mpcols[i] = -9;
    gcmn.mpstls[i] = 1;
    gcmn.mpthks[i] = 3;
  }
  gcmn.mpcols[0] = -1;  gcmn.mpcols[1] = -1; gcmn.mpcols[2] = -1;
  for (i=0; i<8; i++) {
    if (gcmn.mpdset[i]) gree(gcmn.mpdset[i],"g1");
    gcmn.mpdset[i] = NULL;
  }
  gcmn.mpdset[0] = (char *)galloc(7,"mpdset");
  *(gcmn.mpdset[0]+0) = 'l'; 
  *(gcmn.mpdset[0]+1) = 'o';
  *(gcmn.mpdset[0]+2) = 'w'; 
  *(gcmn.mpdset[0]+3) = 'r';
  *(gcmn.mpdset[0]+4) = 'e'; 
  *(gcmn.mpdset[0]+5) = 's';
  *(gcmn.mpdset[0]+6) = '\0';
  for (i=1;i<8;i++) gcmn.mpdset[i]=NULL;

  gcmn.strcol = 1;
  gcmn.strthk = 3;
  gcmn.strjst = 0;
  gcmn.strrot = 0.0;
  gcmn.strhsz = 0.1;
  gcmn.strvsz = 0.12;
  gcmn.anncol = 1;
  gcmn.annthk = 5;
  gcmn.tlsupp = 0;
  gcmn.xlcol = 1;
  gcmn.ylcol = 1;
  gcmn.xlthck = 4;
  gcmn.ylthck = 4;
  gcmn.xlsiz = 0.11;
  gcmn.ylsiz = 0.11;
  gcmn.xlflg = 0;
  gcmn.ylflg = 0;
  gcmn.xtick = 1;
  gcmn.ytick = 1;
  gcmn.xlint = 0.0;
  gcmn.ylint = 0.0;
  gcmn.xlpos = 0.0;
  gcmn.ylpos = 0.0;
  gcmn.ylpflg = 0;
  gcmn.yllow = 0.0;
  gcmn.xlside = 0;
  gcmn.ylside = 0;
  gcmn.clsiz = 0.09;
  gcmn.clcol = -1;
  gcmn.clthck = -1;
  gcmn.stidflg = 0;
  gcmn.grdsflg = 1;
  gcmn.timelabflg = 1;
  gcmn.stnprintflg = 0;
  gcmn.fgcnt = 0;
  gcmn.barflg = 0;
  gcmn.bargap = 0;
  gcmn.barolin = 0;
  gcmn.clab = 1;
  gcmn.clskip = 1;
  gcmn.xlab = 1;
  gcmn.ylab = 1;
  gcmn.clstr = NULL;
  gcmn.xlstr = NULL;
  gcmn.ylstr = NULL;
  gcmn.xlabs = NULL;
  gcmn.ylabs = NULL;
  gcmn.dbflg = 0;
  gcmn.rainmn = 0.0;
  gcmn.rainmx = 0.0;
  gcmn.rbflg = 0;
  gcmn.miconn = 0;
  gcmn.impflg = 0;
  gcmn.impcmd = 1;
  gcmn.strmden = 5;
  gcmn.strmarrd = 0.4;
  gcmn.strmarrsz = 0.05;
  gcmn.strmarrt = 1;
  gcmn.frame = 1;
  gcmn.pxsize = gcmn.xsiz;
  gcmn.pysize = gcmn.ysiz;
  gcmn.vpflag = 0;
  gcmn.xsiz1 = 0.0;
  gcmn.xsiz2 = gcmn.xsiz;
  gcmn.ysiz1 = 0.0;
  gcmn.ysiz2 = gcmn.ysiz;
  gcmn.paflg = 0;
  for (i=0; i<10; i++) gcmn.gpass[i] = 0;
  gcmn.btnfc = 1;
  gcmn.btnbc = 0;
  gcmn.btnoc = 1;
  gcmn.btnoc2 = 1;
  gcmn.btnftc = 1;
  gcmn.btnbtc = 0;
  gcmn.btnotc = 1;
  gcmn.btnotc2 = 1;
  gcmn.btnthk = 3;
  gcmn.dlgpc = -1;
  gcmn.dlgfc = -1;
  gcmn.dlgbc = -1;
  gcmn.dlgoc = -1;
  gcmn.dlgth = 3;
  gcmn.dlgnu = 0;
  for (i=0; i<15; i++) gcmn.drvals[i] = 1;
  gcmn.drvals[1] = 0; gcmn.drvals[5] = 0;
  gcmn.drvals[9] = 0;
  gcmn.drvals[14] = 1;
  gcmn.sig = 0;
  gcmn.lfc1 = 2;
  gcmn.lfc2 = 3;
  gcmn.wxcols[0] = 2; gcmn.wxcols[1] = 10; gcmn.wxcols[2] = 11;
  gcmn.wxcols[3] = 7; gcmn.wxcols[4] = 15;
  gcmn.wxopt = 1;
  gcmn.ptflg = 0;
  gcmn.ptopt = 1;
  gcmn.ptden = 5;
  gcmn.ptang = 0;
  gcmn.statflg = 0;
  gcmn.prstr = NULL;  gcmn.prlnum = 8; 
  gcmn.prbnum = 1; gcmn.prudef = 0;
  gcmn.dwrnflg = 1;
  gcmn.xexflg = 0; gcmn.yexflg = 0;
  gcmn.cachesf = 1.0; 
  gcmn.dblen = 12;
  gcmn.dbprec = 6;
  gcmn.loopflg = 0;
  gcmn.aaflg = 1;

}

void gasig(gaint i) {
  if (gcmn.sig) exit(0);
  gcmn.sig = 1;
}

gaint gaqsig (void) {
  return(gcmn.sig);
}

#if READLINE == 1
/* write command history to log file */
void write_command_log(char *logfile) {
   char QuitLabel[60];
   time_t thetime;
   struct tm *ltime;
   if ((thetime=time(NULL))!=0) {  
      ltime=localtime(&thetime);
      strftime(QuitLabel,59,"quit # (End of session: %d%h%Y, %T)",ltime);
      remove_history(where_history());
      add_history(QuitLabel);
   }
   write_history(logfile);  
   return;
}
#endif

/* output command line options */

void command_line_help (void) {
printf("\nCommand line options for GrADS version " GRADS_VERSION ": \n\n");
printf("    -help       Prints out this help message \n");
printf("    -a ratio    Sets the aspect ratio of the real page \n");
printf("    -b          Enables batch mode (graphics window is not opened) \n");
printf("    -c cmd      Executes the command 'cmd' after startup \n");
printf("    -C N        Enables colorization of text with color scheme N (default scheme is 0)\n");
printf("    -E          Disables command line editing \n");
printf("    -g WxH+X+Y  Sets size of graphics window \n");
printf("    -H fname    Enables command line logging to file 'fname' (default fname is $HOME/.grads.log) \n");
printf("    -l          Starts in landscape mode with real page size 11 x 8.5 \n");
printf("    -p          Starts in portrait mode with real page size 8.5 x 11 \n");
printf("    -m NNN      Sets metafile buffer size to NNN (must be an integer) \n");
printf("    -u          Unbuffers stdout/stderr, disables command line editing \n");
printf("    -W          Uses X server to draw wide lines (faster) instead of software (better) \n");
printf("    -x          Causes GrADS to automatically quit after executing supplied command (used with -c) \n");
printf("\n    Options that do not require arguments may be concatenated \n");
printf("\nExamples:\n");
printf("   grads -pb \n");
printf("   grads -lbxc \"myscript.gs\" \n");
printf("   grads -Ca 1.7778 \n");
printf("   grads -C 2 -a 1.7778 \n");
printf("   grads -pHm 5000000 -g 1100x850+70+0 \n");
printf("   grads -pH mysession.log -m 5000000 -g 1100x850+70+0 \n\n");
}

/* For CPU time limit signal */
void gasigcpu(gaint i) {  
    exit(1) ;
}
