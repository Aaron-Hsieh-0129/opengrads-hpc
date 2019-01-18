/*

    Copyright (C) 2007 by Arlindo da Silva <dasilva@opengrads.org>
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

#ifndef POD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glob.h>
#include <wordexp.h>
#include <regex.h>

#include "pcre.h"
#include "pcrs.h"

#include "grads.h"

static char fexists_ (char *filename)
{
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

pcrs_job *_compile(const char *pattern, const char *options, int *errptr)
{
   pcrs_job *newjob;
   int flags;
   int capturecount;
   const char *error;

   *errptr = 0;

   /* 
    * Handle NULL arguments
    */
   if (pattern == NULL) pattern = "";

   /* 
    * Get and init memory
    */
   if (NULL == (newjob = (pcrs_job *)malloc(sizeof(pcrs_job))))
   {
      *errptr = PCRS_ERR_NOMEM;
      return NULL;
   }
   memset(newjob, '\0', sizeof(pcrs_job));


   /*
    * Evaluate the options
    */
   newjob->options = pcrs_parse_perl_options(options, &flags);
   newjob->flags = flags;

   /*
    * Compile the pattern
    */
   newjob->pattern = pcre_compile(pattern, newjob->options, &error, errptr,NULL);
   if (newjob->pattern == NULL)
   {
      pcrs_free_job(newjob);
      return NULL;
   }

   return newjob;

}

pcrs_job *_compile_command(const char *command, int *errptr ) {

   int i, k, l, quoted = FALSE;
   size_t limit;
   char delimiter;
   char *tokens[4];   
   pcrs_job *newjob;
   
   i = k = l = 0;
   
   /*
    * Tokenize the perl command
    */
   limit = strlen(command);
   if (limit < 2)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      return NULL;
   }
   else
   {
      delimiter = command[0];
   }

   tokens[l] = (char *) malloc(limit + 1);

   for (i = 0; i <= (int)limit; i++)
   {
      
      if (command[i] == delimiter && !quoted)
      {
         if (l == 3)
         {
            l = -1;
            break;
         }
         tokens[0][k++] = '\0';
         tokens[++l] = tokens[0] + k;
         continue;
      }
      
      else if (command[i] == '\\' && !quoted)
      {
         quoted = TRUE;
         if (command[i+1] == delimiter) continue;
      }
      else
      {
         quoted = FALSE;
      }
      tokens[0][k++] = command[i];
   }

   /*
    * Syntax error ?
    */
   if (l != 2)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      free(tokens[0]);
      return NULL;
   }
   newjob = _compile(tokens[1], tokens[2], errptr);
   free(tokens[0]);
   return newjob;
   
}

/*........................................................................*/

/*

  GrADS User Defined Commands for path name manimulations, and file
operations.

*/

int c_path ( int argc, char **argv, struct gacmn *pcm) {
  char pout[512];   /* Build error msgs here */
  char *result;
  char buffer[4096];
  struct stat stbuf;
  glob_t flist;
  wordexp_t wlist;
  regex_t re;
  pcrs_job *job;
  int i, n, rc;
  size_t m;


  rc = 0;

  if ( argc < 2 ) {
    snprintf(pout,511,"Usage: %s string\n",argv[0]);
    gaprnt(0,pout);
    return 1;
  }

  /* chdir */
  if ( (strcmp(argv[0],"chdir") == 0) || (strcmp(argv[0],"cd") == 0)  ) {

    if ( !chdir(realpath(argv[1],buffer) ) ) {
      snprintf(pout,511,"New directory: %s\n",argv[1]);
      gaprnt(1,pout);
      return (rc);
    } else {
      snprintf(pout,511,"Cannot change directory to: %s\n",argv[1]);
      gaprnt(0,pout);
      return 1;
    }
  
  /* abspath */
  } else if ( strcmp(argv[0],"abspath") == 0 ) {
    if ( (result=realpath(argv[1],buffer)) ) { 
      snprintf(pout,511,"%s\n",result);
      gaprnt(2,pout);
      return (0);
    } else {
      snprintf(pout,511,"Cannot find real path of %s\n",argv[1]);
      gaprnt(0,pout);
      return 1;
    }

  /* basename */
  } else if ( strcmp(argv[0],"basename") == 0  ) {
    result = basename(argv[1]);
    snprintf(pout,511,"%s\n",result);
    gaprnt(2,pout);
    return (0);

  /* dirname */
  } else if ( strcmp(argv[0],"dirname") == 0  ) {
    result = dirname(argv[1]);
    snprintf(pout,511,"%s\n",result);
    gaprnt(2,pout);
    return (0);

  /* exists */
  } else if ( strcmp(argv[0],"exists") == 0  ) {

    if ( fexists_(argv[1]) ) snprintf(pout,511,"yes\n");
    else                     snprintf(pout,511,"no\n");
    gaprnt(2,pout);
    return (0);

  /* isfile */
  } else if ( strcmp(argv[0],"isfile") == 0  ) {

    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
 
      if ( (S_ISREG(stbuf.st_mode)) ) {
          snprintf(pout,511,"yes\n");
      } else {
        snprintf(pout,511,"no\n");
      }
      gaprnt(2,pout);
      return (0);

    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* isdir */
  } else if ( strcmp(argv[0],"isdir") == 0  ) {

    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
 
     if (S_ISDIR(stbuf.st_mode)) {
          snprintf(pout,511,"yes\n");
      } else {
          snprintf(pout,511,"no\n");
      }
      gaprnt(2,pout);
      return (0);

    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* getatime */
  } else if ( strcmp(argv[0],"getatime") == 0  ) {
    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
      snprintf(pout,511,"%s",ctime(&stbuf.st_atime));
      gaprnt(2,pout);
      return (0);
    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* getctime */
  } else if ( strcmp(argv[0],"getctime") == 0  ) {
    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
      snprintf(pout,511,"%s",ctime(&stbuf.st_ctime));
      gaprnt(2,pout);
      return (0);
    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* getmtime */
  } else if ( strcmp(argv[0],"getmtime") == 0  ) {
    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
      snprintf(pout,511,"%s",ctime(&stbuf.st_mtime));
      gaprnt(2,pout);
      return (0);
    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* getatime */
  } else if ( strcmp(argv[0],"getatime") == 0  ) {
    if ( !(stat(realpath(argv[1],buffer),&stbuf)) ) {
      snprintf(pout,511,"%s\n",ctime(&stbuf.st_atime));
      gaprnt(2,pout);
      return (0);
    } else {
        snprintf(pout,511,"*** ERROR *** cannot stat file %s\n",argv[1]);
        gaprnt(0,pout);
        return (1);
    } 

  /* glob */
  } else if ( strcmp(argv[0],"glob") == 0  ) {
    rc = glob(argv[1],0,(int *) NULL, &flist);
    if (rc) {
      switch (rc) {
        case GLOB_NOMATCH: 
          snprintf(pout,511,"*** ERROR *** cannot find matches for %s\n",argv[1]);
          break;
        case GLOB_NOSPACE: 
          snprintf(pout,511,"*** ERROR *** cannot allocate memory for globbing %s\n",argv[1]);
          break;
        default:
          snprintf(pout,511,"*** ERROR *** cannot glob %s\n",argv[1]);
          break;
        }
        gaprnt(0,pout);
        return (1);
    } 
    snprintf(pout,511,"Found %d matches\n",(int) flist.gl_pathc);
    gaprnt(2,pout);
    for ( i=0; i<flist.gl_pathc; i++) {
      snprintf(pout,511,"%s\n",flist.gl_pathv[i]);
      gaprnt(2,pout);
    }
     
    globfree(&flist); /* clean up */ 
    return (0);
  
  /* wordexp */        
  } else if ( strcmp(argv[0],"wordexp") == 0  ) {
    strncpy(buffer,argv[1],4095);
    if ( argc > 2 ) {
      for ( i=2; i<argc; i++ ) {
        snprintf(buffer,4095,"%s %s",buffer,argv[i]);
      }
    }
    rc = wordexp(buffer, &wlist, 0);
    if (rc) {
      switch (rc) {
        case WRDE_BADCHAR:
          snprintf(pout,511,"*** ERROR *** The input string <%s> contains an unquoted invalid character.\n",buffer);
          break;
       case WRDE_BADVAL:
          snprintf(pout,511,"*** ERROR *** The input string <%s> refers to an undefined shell variable.\n",buffer);
          break;
       case WRDE_NOSPACE:
         snprintf(pout,511,"*** ERROR *** out of memory\n");
          break;
       case WRDE_SYNTAX:
          snprintf(pout,511,"*** ERROR *** The input string <%s> has a syntax error\n",buffer);
          break;
       default:
          snprintf(pout,511,"*** ERROR *** cannot perform work expansion for <%s>\n",buffer);
          break;
        }
        gaprnt(0,pout);
        return (1);
    } 
    snprintf(pout,511,"Found %d words\n",(int)wlist.we_wordc);
    gaprnt(2,pout);
    for ( i=0; i<wlist.we_wordc; i++) {
      snprintf(pout,511,"%s\n",wlist.we_wordv[i]);
      gaprnt(2,pout);
    }
      
    wordfree(&wlist); /* clean up */
    return (0);
          
#if 0
  /* POSIX match/imatch ---- DISALED ----*/
  } else if ( (strcmp(argv[0],"match")==0) || (strcmp(argv[0],"imatch")==0) ) {
    if ( argc < 3 ) {
      gaprnt(0,"*** ERROR *** match/imatch requires at least 2 arguments: pattern string");
      return (1);
    }
    if ( argv[0][0]=='i' ) {
      rc = regcomp(&re,argv[1],REG_ICASE);
    } else {
      rc = regcomp(&re,argv[1],0);
    }
    if (rc) {
      snprintf(pout,511,"*** ERROR *** cannot compile regular expression <%s>\n",argv[1]);
      gaprnt(0,pout);
      return (1);
    } 
    rc = regexec(&re,argv[2],0,(regmatch_t *)NULL,0);
    if ( rc ) {
         snprintf(pout,511,"no\n");
    } else {
         snprintf(pout,511,"yes\n");
    } 
    gaprnt(2,pout);
    regfree(&re);
    return(0);     
#endif 

  /* match */
  } else if ( strcmp(argv[0],"match")==0 ) {
    if ( argc < 3 ) {
      gaprnt(0,"*** ERROR *** match requires at least 2 arguments: pattern string");
      return (1);
    }
    job = _compile_command(argv[1],&rc);
    if (rc) {
      snprintf(pout,511,"*** ERROR *** cannot compile regular expression <%s>\n",argv[1]);
      gaprnt(0,pout);
      return (1);
    } 
    rc = pcre_exec(job->pattern,NULL,argv[2],strlen(argv[2]),0,0,NULL,0);
    if ( rc ) {
         snprintf(pout,511,"no\n");
    } else {
         snprintf(pout,511,"yes\n");
    } 
    gaprnt(2,pout);
    pcrs_free_job(job);
    return(0);
      
  /* sed */
  } else if ( strcmp(argv[0],"sed")==0 ) {
    if ( argc < 3 ) {
      gaprnt(0,"*** ERROR *** sed requires at least 2 arguments: sed-command string");
      return (1);
    }
    job = pcrs_compile_command(argv[1],&rc);
    if (rc) {
      snprintf(pout,511,"*** ERROR *** cannot compile sed command <%s>\n",argv[1]);
      gaprnt(0,pout);
      return (1);
    } 
    n = pcrs_execute(job,argv[2],strlen(argv[2]),&result, &m);
    if ( n<0 ) {
      snprintf(pout,511,"*** ERROR *** cannot perform sed on <%s>\n",argv[2]);
      gaprnt(0,pout);
      return (1);
    } else {
      snprintf(pout,511,"Replaced %d matches\n",n);
      gaprnt(2,pout);
      snprintf(pout,511,"%s\n",result);
      gaprnt(2,pout);
      /* free(Result); */
    } 
    pcrs_free_job(job);
    return(0);

    /* unknown */
  } else {
      snprintf(pout,511,"Unknown command <%s>\n",argv[0]);
      gaprnt(0,pout);
      return 1;
  }

  return(0);

}

  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

path.gex - GrADS Extensions for Pathname Manipulation and Regular Expressions

=head1 SYNOPSIS

=head3 GrADS Commands:

=over 4

=item

B<abspath> I<PATHNAME> - Return the absolute pathname 

=item

B<basename> I<PATHNAME> - Return filename part of a pathname 

=item

B<cd> I<PATHNAME> - Change current directory, same as B<chdir>

=item

B<chdir> I<PATHNAME> - Change current directory, same as B<cd>

=item

B<dirname> I<PATHNAME> - Return directory name part of a pathname 

=item

B<exists> I<PATHNAME> - Check whether a pathname exists 

=item

B<getatime> I<PATHNAME> - Get last access time of a pathname

=item

B<getctime> I<PATHNAME> - Get metadata change time of a pathname

=item

B<getmtime> I<PATHNAME> - Get modification time of a pathname

=item

B<glob> I<PATTERN> - Return a list of paths matching a pathname pattern 

=item

B<isfile> I<PATHNAME> - Test whether a path is a regular file

=item

B<isdir> I<PATHNAME> - Test whether a path is a directory

=item

B<match> I<PATTERN> I<STRING> - Regular expression matching of string

=item

B<sed> I<PATTERN> I<STRING> - SED-like search and replace

=item

B<wordexp> I<STRING> - Perform word expansion like a POSIX shell

=back

=head3 GrADS Scripting Language Functions:

=over 4

=item

B<abspath>(I<PATHNAME>) - Return the absolute pathname 

=item

B<basename>(I<PATHNAME>) - Return filename part of a pathname 

=item

B<chdir>(I<PATHNAME>) - Change current directory, same as B<cd>

=item

B<dirname>(I<PATHNAME>) - Return directory name part of a pathname 

=item

B<exists>(I<PATHNAME>) - Return 1 if pathname exists, 0 otherwise 

=item

B<getatime>(I<PATHNAME>) - Get last access time of a pathname

=item

B<getctime>(I<PATHNAME>) - Get metadata change time of a pathname

=item

B<getmtime>(I<PATHNAME>) - Get modification time of a pathname

=item

B<glob>(I<PATTERN>) - Return a list of paths matching a pathname pattern 

=item

B<isfile>(I<PATHNAME>) - Return 1 path is a regular file, 0 otherwise

=item

B<isdir>(I<PATHNAME>) - Return 1 path is a directory, 0 otherwise

=item

B<match>(I<PATTERN>,I<STRING>) - Return 1 if I<PATTERN> matches I<STRING>

=item

B<sed>(I<PATTERN>,I<STRING>) - Return result of SED-like search and replace

=item

B<wordexp>(I<STRING>) - Return string with word expansion like a POSIX shell

=back

=head1 DESCRIPTION 

This library implements GrADS extensions (I<gex>) with commands for
pathname manipulation and perl-compatible regular expression matching,
search and replace of strings. In addition, it also provides
B<glob> and B<wordexp> commands for shell-like file name and
variable expansions; B<wordexp> also provides the back-tick operator for
executing shell commands and capture its output. These features
are available at the GrADS prompt or as GrADS Scription Language functions.

=head1 EXAMPLES

=head3 Using B<abspath> to get full path name of current directory

   ga-> abspath .
   /Users/adasilva

Notice that B<abspath> also expands symlinks, e.g.,

   ga-> ! ln -s /opt xxx
   ga-> abspath xxx
   /opt

=head3 Parsing file and directory names,

   ga-> basename /path/to/somefile.nc4
   somefile.nc4

   ga-> dirname /path/to/somefile.nc4
   /path/to

=head3 Changing directories

If you ever tried to use I<!cd> to change directories you realized that it does not work,

   ga-> !pwd
   /Users/adasilva
   ga-> !cd /opt
   ga-> !pwd
   /Users/adasilva

This is because the directory is only changed in the subprocess
spawned by the "!" operation; it does not propagate to the environment
of of the parent process.  The newly introduced I<chdir> command
allows you to change the current directory,

   ga-> chdir /opt
   New directory: /opt
   ga-> !pwd
   /opt

=head3 Checking whether a file/directory exists

   ga-> exists /usr/bin/tar
   yes

   ga-> exists /usr/bin/!@#$%^^&
   no

=head3 Checking whether a pathname is a file or directory

   ga-> isfile /usr/bin
   no
   ga-> isfile /usr/bin/tar
   yes

   ga-> isdir /usr/bin
   yes
   ga-> isdir /usr/bin/tar
   no

Notice these commands can only be used with existing files
or directories,

   ga-> isfile /usr/bin/!@#%^&
   *** ERROR *** cannot stat file /usr/bin/!@#%^&

=head3 Getting the modification time of a file/directory

   ga-> getmtime /usr/bin/sed
   Fri Oct 25 17:05:00 2013

Likewise, you can query the last accesss and metadata change times:

   ga-> getatime /usr/bin/sed
   Mon May 26 15:12:53 2014

   ga-> getctime /usr/bin/sed
   Fri Oct 25 17:05:00 2013

=head3 Getting a list of path names matching a pattern,

   ga-> glob '/opt/X11/*'
   Found 6 matches
   /opt/X11/bin
   /opt/X11/etc
   /opt/X11/include
   /opt/X11/lib
   /opt/X11/share
   /opt/X11/var

Use the B<isfile>, B<isdir> to find out if these are files or directories, e.g.,

   ga-> isdir /opt/X11/include
   yes

Notice that B<glob> does not perform expansion of the shell home
diretory character C<~>,

   ga-> glob ~
   *** ERROR *** cannot find matches for ~

Use B<wordexp> if you would like expansion of this character.

=head3 Performing word expansion: files and directories

   ga-> wordexp '/opt/X11/include/*.h'
   Found 5 words
   /opt/X11/include/Xplugin.h
   /opt/X11/include/png.h
   /opt/X11/include/pngconf.h
   /opt/X11/include/pnglibconf.h
   /opt/X11/include/xpyb.h

Unlike B<glob>, B<wordexp> expands C<~>:

   ga-> wordexp ~
   Found 1 words
   /Users/adasilva

When a match cannot be found, B<wordexp> returns the original pattern,

   ga-> glob ~/!@#$%
   *** ERROR *** cannot find matches for ~/!@#$%

   ga-> wordexp ~/!@#$%
   Found 1 words
   /Users/adasilva/!@#$%

=head3 Performing word expansion: environment variables

   ga-> wordexp 'I am $USER and I reside at $HOME'
   Found 8 words
   I
   am
   adasilva
   and
   I
   reside
   at
   /Users/adasilva

=head3 Performing word expansion: back-tick operator

   ga-> wordexp "I am `whoami` and I reside at ~"
   Found 8 words
   I
   am
   adasilva
   and
   I
   reside
   at
   /Users/adasilva

   ga-> wordexp "Today is `date`"
   Found 8 words
   Today
   is
   Mon 
   May
   26
   21:25:12
   EDT
   2014

=head3 Regular expressions: matching

   ga-> match /open/ OpenGrADS
   no
   ga-> match /open/i OpenGrADS
   yes

   ga-> match /^grads/i OpenGrADS
   no
   ga-> match /grads$/i OpenGrADS
   yes

=head3 Regular expressions: search and replace

   ga->sed s/^open/Open/ opengrads
   Replaced 1 matches
   Opengrads
  
   ga-> sed s/GRADS/GrADS/i opengrads
   Replaced 1 matches
   openGrADS

=head3 GrADS Scripting Functions

The following script shows an example 

    function main()

   *  Enable library functions
   *  ------------------------
      rc = gsfallow('on')

   *  Check if a file
   *  ---------------
      if ( isfile('/usr/bin') )
            say '/usr/bin is a file'
      else
            say '/usr/bin is NOT a file'
      endif 

   *  Check if a directory
   *  --------------------
      if ( isdir('/usr/bin') )
            say '/usr/bin is a directory'
      else
            say '/usr/bin a NOT file'
      endif 

   *  Get list of header files
   *  ------------------------
      headers = wordexp('/opt/X11/include/png*.h')
      say 'PNG header files are: ' headers

   *  Search and replace
   *  ------------------
      new = sed('s/grads/GrADS/ig', 'Opengrads provides GRADS extensions')
      say new

Running this script produces the following output:

   /usr/bin is NOT a file
   /usr/bin is a directory
   PNG header files are: /opt/X11/include/png.h /opt/X11/include/pngconf.h /opt/X11/include/pnglibconf.h
   OpenGrADS provides GrADS extensions

=head1 REGULAR EXPRESSIONS

A regular expression (or I<regex> for short) is a sequence of
characters that forms a search pattern, primarily used in pattern
matching with strings, or in I<find and replace> type of
operations. The B<match> and B<sed> functions provided here rely
extensively on regular expressions, in particular Perl Compatible
Regular Expressions as implmented in the PCRE Library
(L<http://www.pcre.org>). If you are not familiar with regular
expressions you may want to start with this Quick
Tutorial (L<http://www.regular-expressions.info/quickstart.html>), or
consult the references under SEE ALSO below for additional
information.

=head1 COMMANDS PROVIDED

The following commands are loosely based on the C-Shell syntax:

=head2 B<abspath> I<PATHNAME> 

=over 4

Return the absolute pathname associated with I<PATHNAME>. It will
expand the C<.> and C<..> directories as well as follow
symlinks. However, it will not expand the shell character C<~>.

=back

=head2 B<basename> I<PATHNAME> 

=over 4

Return file name part of I<PATHNAME>. This is a simple string manipulation, 
there is no need for the pathname involved to exist.

=back

=head2 B<cd> I<PATHNAME> 

=over 4

Change current directory, same as B<chdir>. An error is returned if 
I<PATHNAME> does not exists.

=back

=head2 B<chdir> I<PATHNAME>

=over 4

Change current directory, same as B<cd>.

=back

=head2 B<dirname> I<PATHNAME>

=over 4

Return directory name part of I<PATHNAME>. This is a simple string manipulation, 
there is no need for the pathname involved to exist.

=back

=head2 B<exists> I<PATHNAME> 

=over 4

Check whether I<PATHNAME> exists. The pathname can be a file or a directory.

=back

=head2 B<getatime> I<PATHNAME> 

=over 4

Return last access time of I<PATHNAME>.

=back

=head2 B<getctime> I<PATHNAME> 

=over 4

Return metadata change time of I<PATHNAME>.

=back

=head2 B<getmtime> I<PATHNAME> 

=over 4

Return modification time of I<PATHNAME>.

=back

=head2 B<glob> I<PATTERN> 

=over 4

Return a list of files or directories matching a pathname pattern. The I<PATTERN> is a shell-like
pattern involving wildcards, etc., not a regular expression.
 
=back

=head2 B<isfile> I<PATHNAME>

=over 4

Test whether I<PATHNAME> is a regular file. An error is returned if the pathname
does not exists.

=back

=head2 B<isdir> I<PATHNAME>

=over 4

Test whether I<PATHNAME> is a directory. An error is returned if the pathname
does not exists.

=back

=head2 B<match> I<PATTERN> I<STRING> 

=over 4

Check whether the (perl-compatible) regular expression I<PATTERN> matches I<STRING>. 

=back

=head2 B<sed> I<PATTERN> I<STRING>

Return string with the result of a search and replace operation on I<STRING> expressed in the
(perl-compatible) regular expression I<PATTERN>. See EXAMPLES above.

=over 4

=back

=head2 B<wordexp> I<STRING> 

=over 4

Perform word expansion like a POSIX shell. Word expansion means the
process of splitting a string into words and substituting for
variables, commands, and wildcards just as the shell does. See EXAMPLES above.

=back

=head1 GrADS SCRIPT FUNCTIONS PROVIDED

As shown in the SYNOPSIS above, each of the command above are provided as 
a GrADS script function. To enable these Library Functions be sure to add 
this to the top of your own script:

          gsfallow('on')

=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org> - OpenGrADS Home Page

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=back

=head1 AUTHOR 

Arlindo da Silva (dasilva@opengrads.org)

=head1 COPYRIGHT

Copyright (C) 2014 Arlindo da Silva; All Rights Reserved.

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif
