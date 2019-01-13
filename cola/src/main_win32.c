/*

    Copyright (C) 2008 by Arlindo da Silva <dasilva@opengrads.org>
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

----------------------------------------------------------------------------

   Simple wrapper to start Win32 GrADS executables.  It is assumed a
standard Win32 GrADS directory structure. If Win32 GrADS is installed
under //progra~1/pcgrads then:

GADDIR = //progra~1/pcgrads/dat
GASCRP = //progra~1/pcgrads/lib
DISPLAY = localhost:7.0

These environment variables are set iff they are not already defined.
The actual grads root directory is derived from the full pathname of the
executable, assuming the executable is under c:/progra~1/pcgrads/win32.

NOTES:

   1. This is a Windows application; is not meant to be portable.
   2. When DISPLAY is not set, Xming is started at display #7 (except if
      in batch mode, "-b" option).

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <windows.h>

static void setargv (int *argcPtr, char ***argvPtr);
int check_batch (int argc, char **argv);
int check_file ( char *fmt, char *str1, char *str2 );
void umount_root(void);
static void setYYYYMMDD(void);

#define MAX_SIZE 2048

static char cmd[MAX_SIZE];
static char prefix[MAX_PATH], prefix32[MAX_PATH]; /* usually c:/PCGrADS */
static char gabin[MAX_PATH], gabin32[MAX_PATH] ; /* usually c:/PCGrADS/win32 */
static char gadset[MAX_PATH];

static char const *mArgv[7];
static char const *xArgv[6];
static char const *gArgv[256];

static char root[] = "/";
static char bin[] = "/bin";

//                                 -------
 
int
main(int argc, char **argv)

{
    char *cmdLine;

    char appl[MAX_PATH];
    char cmdl[MAX_PATH];

    char gahome[MAX_PATH];
    char gaddir[MAX_PATH];
    char gatmp[MAX_PATH];
    char gapath[MAX_SIZE];
    char gagui[MAX_PATH];
    char gascrp[MAX_PATH];
    char gaudxt[MAX_PATH];
    char display[MAX_PATH];

    char tmp[MAX_SIZE];
    char bintype[MAX_PATH];

    int i, ia, j, batch, slash=0, has_xming, has_udxt, has_sh, gArgc;

//  Tell cygwinn to be quiet about MSDOS file names //
    if ( ! getenv("CYGWIN") ) 
         putenv("CYGWIN=nodosfilewarning");

//  Retrieve name of this executable
//  ---------------------------------
    GetModuleFileName(NULL, tmp, MAX_PATH);   // Windows native call
    GetShortPathName( tmp, cmdl, MAX_PATH);   
    cygwin_conv_to_full_posix_path(cmdl, gabin);   
    strcpy(cmdl,gabin);

//   Determine location of binaries (gabin)
//  --------------------------------------
    for(i=strlen(gabin)-1, ia=0; i>-1; i--)
    {
       if(gabin[i]=='/') slash++;
       if(slash==1 && !ia ) {
          ia = i+1;
          gabin[i] = '\0';
          break;
       }
    }
    cygwin_conv_to_full_win32_path(gabin,gabin32);

//  Determine prefix: assume 5 levels up as in an OpenGrADS Bundle
//  --------------------------------------------------------------
    strcpy(prefix,gabin);
	j = 0;
    for(i=strlen(prefix)-1; i>-1; i--) {
      if(prefix[i]=='/') {
		 j++;
         if (j==4) {
		     prefix[i] = '\0';
		     break;
		}
      }
      prefix[i] = '\0';
    }
    cygwin_conv_to_full_win32_path(prefix,prefix32);

//  Find application name (short win32 name)
//  ---------------------------------------
    for(i=ia, j=0; i<strlen(cmdl); i++, j++) {
        if(cmdl[i]=='.') break;
        appl[j] = cmdl[i];
    }
    appl[j] = '\0';

#ifdef NEVER
//  Determine bintype
//  -----------------
    strcpy(bintype,&appl[5]);
    if ( !strcmp(appl,"grads")    ) strcpy(bintype,"nc");
    if ( !strcmp(bintype,"D~1")   ) strcpy(bintype,"nc");
#endif
	
//  Default environment strings
//  ---------------------------
    sprintf(gaddir,"GADDIR=%s/Resources/SupportData", prefix);
	sprintf(gadset,"GADSET=%s/Resources/SampleDatasets", prefix);
    sprintf(gascrp,"GASCRP=%s/Resources/Scripts", prefix);
    sprintf(gagui, "GAGUI=%s/Resources/Scripts/default.gui", prefix);
    sprintf(gaudxt,"GA2UDXT=%s/gex/udxt", gabin);
    i = strlen(getenv("PATH"))+2*strlen(gabin)+32;
    if ( i > MAX_SIZE ) {
      printf("MAX_SIZE = %d, needed size = %d\n", MAX_SIZE, i);
      printf("Internal error: environment variable PATH is too long; change MAX_SIZE to at least %d and recompile or else shortten your PATH by %d characters\n", i,i-MAX_SIZE);
      return 1;
    }
	
//  Use the system temp directory
//  -----------------------------
    GetTempPath(MAX_PATH,gatmp);
    cygwin_conv_to_full_posix_path(gatmp, tmp);
    sprintf(gatmp,"TMPDIR=%s", tmp);	

//  Check whether Xming is present
//  ------------------------------
    has_xming = check_file("%s/Resources/Xming/rgb.txt", prefix, (char *) NULL);

//  Check whether udxt is present
//  -----------------------------
    has_udxt = check_file("%s/gex/udxt", gabin,(char *) NULL);

//  Check whether we have /bin/sh (usually means cygwin is installed)
//  This is no longer necessary undser cygwin 1.7 because we now mae
//  sure that one can always find /bin/sh.exe when creating the bundle.
//  -------------------------------------------------------------------
    has_sh = check_file("%s/sh.exe", "/bin", (char *) NULL);

//  Set/reset environment variables
//  -------------------------------
    if (getenv("GADDIR")==NULL)      putenv(gaddir);
    if (getenv("GADSET")==NULL)      putenv(gadset);
    if (getenv("GASCRP")==NULL)      putenv(gascrp);
    if (has_udxt && getenv("GA2UDXT")==NULL) {
        putenv(gaudxt);
    } 
    if (getenv("TMPDIR")==NULL)      putenv(gatmp);
    if ( !strcmp(appl,"gradsgui") || !strcmp(appl,"GRADSGUI") ) {
         sprintf(gagui, "GAGUI=%s/Resources/Scripts/default.gui", prefix);
         if (getenv("GAGUI")==NULL)  putenv(gagui);
    } else if ( !strcmp(appl,"merra") || !strcmp(appl,"MERRA") ) {
         sprintf(gagui, "GAGUI=%s/Resources/Scripts/merra.gui", prefix);
	     if (getenv("GAGUI")==NULL)  putenv(gagui);
    } else if ( !strcmp(appl,"nomads") || !strcmp(appl,"NOMADS") ) {
         sprintf(gagui, "GAGUI=%s/Resources/Scripts/nomads.gui", prefix);
	     if (getenv("GAGUI")==NULL)  putenv(gagui);
    } else if ( !strcmp(appl,"geos5") || !strcmp(appl,"GEOS5") ) {
         sprintf(gagui, "GAGUI=%s/Resources/Scripts/geos5.gui", prefix);
	     if (getenv("GAGUI")==NULL)  putenv(gagui);
    }

//  Start Xming if we have it and we are not in batch mode
//  ------------------------------------------------------
    batch = check_batch(argc,argv);
    if (has_xming && !batch) {
          sprintf(display,"DISPLAY=localhost:7.0");
          putenv(display);
          printf("Starting X server under %s\\Resources\\Xming\n", prefix32);
    	  sprintf(cmd,"%s/Resources/Xming/Xming.exe", prefix);
          xArgv[0] = cmd;
          xArgv[1] = ":7";
          xArgv[2] = "-multiwindow";
          xArgv[3] = "+bs";
          xArgv[4] = NULL;
	  spawnv(_P_DETACH,xArgv[0],xArgv);

    } else if (getenv("DISPLAY")==NULL) {
          sprintf(display,"DISPLAY=localhost:0.0");
          putenv(display);
    }
    
//  Make sure current directory and bin dir is part of path
//  -------------------------------------------------------
    sprintf(gapath,"PATH=.:%s:%s",gabin,getenv("PATH"));
    putenv(gapath);

//  It is safer to use Windows HOMEPATH when user has no cygwin //
    sprintf(gahome,"HOME=%s",getenv("HOMEPATH"));
    putenv(gahome);

//  If we do not have /bin/sh.exe, user-mount Contents/ dir as "/"
//  ------------------------------------------------------------
#ifdef NEVER
    if ( ! has_sh ) {

      sprintf(cmd,"%s/mount.exe", gabin);

      printf("User-mode mounting %s as %s ", prefix32, root);
      mArgv[0] = cmd;
      mArgv[1] = "--force";
      mArgv[2] = "user";
      mArgv[3] = prefix32;
      mArgv[4] = root;
      mArgv[5] = NULL;
      spawnv(_P_WAIT,mArgv[0],mArgv);

      printf("and %s as %s ... ", gabin32,  bin);
      mArgv[3] = gabin32;
      mArgv[4] = bin;
      spawnv(_P_WAIT,mArgv[0],mArgv);
      printf("done!\n");

      atexit(umount_root); /* unmount on exit */
    }
#endif

//  Setenv YYYYMMDD with current date
//  --------------------------------- 
    setYYYYMMDD();
	
// Additional parameters for some apps
// -----------------------------------
   gArgv[0] = argv[0];
   if ( !strcmp(appl,"OPENGR~1") || !strcmp(appl,"gradsgui")  || 
        !strcmp(appl,"merra")    || !strcmp(appl,"nomads")    ){
	  gArgv[1] = "-HC";
	  gArgv[2] = "0";
	  gArgc = 3;
   } else {
      gArgc = 1;
   } 
   for ( i=1; (i<argc)&&(i<255); i++ ) {
	     gArgv[gArgc] = argv[i];
	 	 gArgc++;
   }
   gArgv[gArgc+1] = NULL;
   
//  Starting application
//  --------------------
    printf("Starting %s under %s ...\n", appl, gabin32);
 
    return gamain(gArgc,gArgv);

}

// ----------------------------------------------------------------------

int check_batch (int argc, char **argv) {
  int i, j, batflg = 0;
  if (argc>1) {
    for (i=1; i<argc; i++) {
      if (*(argv[i])=='-') {
	j = 1;
	while (*(argv[i]+j)) {
	  if (*(argv[i]+j)=='b') batflg = 1;
	  j++;
	}
      } 
    }
  }
  return batflg;
}

// ----------------------------------------------------------------------

int check_file ( char *fmt, char *str1, char *str2 ) {
  struct stat s;
  int has_it;
  char fname[MAX_PATH];
  if ( str2 == NULL ) sprintf(fname, fmt, str1);
  else                sprintf(fname, fmt, str1, str2);
  if (stat (fname,   &s) == 0) has_it=1; 
  else                         has_it=0;
  // printf("--- DEBUG --- File name is <%s>, has_it = %d\n",fname, has_it);
  return has_it;
}

// ----------------------------------------------------------------------

/* Used to unmount "/" and "/bin" at exit */
void umount_root(void) {
  sprintf(cmd,"%s/umount.exe", gabin);

  printf("User-mode unmounting %s from %s ", root,prefix32);
  mArgv[0] = cmd;
  mArgv[1] = "-u";
  mArgv[2] = root;
  mArgv[3] = NULL;
  spawnv(_P_WAIT,mArgv[0],mArgv);

  printf("and %s from %s ... ", bin,gabin32);
  mArgv[0] = cmd;
  mArgv[1] = "-u";
  mArgv[2] = bin;
  mArgv[3] = NULL;
  spawnv(_P_WAIT,mArgv[0],mArgv);
  printf("done!\n");

}
    
// ------------------------------------------------------------------------
void setYYYYMMDD() {
  struct timeval tp;
  struct tm *now;
  char YYYYMMDD[9];
  gettimeofday(&tp,NULL);
  now = localtime((time_t *) &tp.tv_sec);
  snprintf(YYYYMMDD,9,"%4d%02d%02d",
          now->tm_year+1900,now->tm_mon+1,now->tm_mday);
  setenv("YYYYMMDD",YYYYMMDD,1);
}

