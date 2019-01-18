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

#if defined(_AIX) || defined(__APPLE_CC__)
#include <unistd.h>  /* for getpid() */
#else
#include <sys/unistd.h>  /* for getpid() */
#endif

#include "grads.h"

static char *xenv(char *str);

/*........................................................................*/

/*

  GrADS User Defined Command for setting and expanding environment variables.

  setenv  VNAME string       Sets environmnent variable VNAME to 'string'
  getenv  VNAME              Prints value of environment value 
  printenv string            Expands environment variables in 'string'
                              and prints the resulting stribg
  env     cmd                Expands any occurence of environment variables
                              in 'cmd' and executes the resulting string.

  Examples:

  1) From the GrADS command line:

     ga-> getenv   HOME
     ga-> setenv   DATA_DIR  $HOME/test_data
     ga-> env open $DATA_DIR/model.ctl 

*/

int c_setenv ( int argc, char **argv, struct gacmn *pcm) {
  char *str, pout[512];   /* Build error msgs here */
  int rc;

  rc = 0;

  if ( argc < 2 ) {
    snprintf(pout,511,"Usage: %s  string\n",argv[0]);
    gaprnt(0,pout);
    return 1;
  }

  str = (char *) malloc ( (strlen(argv[1])+strlen(argv[2])+5) * sizeof(char) );
  if ( !str ) {
    snprintf(pout,511,"%s: cannot allocate memory for environment variable %s\n",argv[0],argv[1]);
    gaprnt(0,pout);
    return 1;
  }

  sprintf(str,"%s=%s", argv[1], argv[2]);
  if ( !putenv(str) ) {
    snprintf(pout,511,"Environment variable %s = %s\n",argv[1],argv[2]);
    gaprnt(2,pout);
    return (rc);
  } else {
    snprintf(pout,511,"%s: cannot set environment variable %s\n",argv[0],argv[1]);
    gaprnt(0,pout);
    return 1;
  }
  
  /* Do NOT free str as it is now part of the "environment" */
  return(0);

}

int c_getenv ( int argc, char **argv, struct gacmn *pcm) {

  char *str, pout[512];   /* Build error msgs here */
  int rc, i, retcod;

  if ( argc < 1 ) {
    snprintf(pout,511,"Usage: %s  VNAME(s)\n",argv[0]);
    gaprnt(0,pout);
    return 1;
  }

  retcod = 0;
  for ( i=1; i<argc; i++ ) {

    if ( (str = getenv(argv[i])) ) {
      snprintf(pout,511,"Environment variable %s = %s\n",argv[i],str);
      gaprnt(2,pout);
    } else {
      snprintf(pout,511,"Environment variable %s = <undef>\n",argv[i]);
      gaprnt(2,pout);
      retcod = 1; /* signal that something went wrong */
    }

  }

  return retcod;

}

int c_xenv ( int argc, char **argv, struct gacmn *pcm) {

  char *str, pout[512], cmd[512];   /* Build error msgs here */
  int rc, i;

  if ( argc < 1 ) {
    snprintf(pout,511,"Usage: %s  string\n",argv[0]);
    gaprnt(0,pout);
    return 1;
  }
  cmd[0] = '\0';
  for ( i=1; i<argc; i++ ) {
        if (i>1) strncat(cmd," ",512);
        strncat(cmd,argv[i],512);
  }  
  cmd[511] = '\0';
  if ( (str = xenv(cmd)) ) {
    snprintf(pout,511,"%s\n",str);
    gaprnt(2,pout);
    free(str);
    return 0;
  } else {
    snprintf(pout,511,"%s: error parsing <%s>\n",argv[0], cmd);
    gaprnt(0,pout);
    return 1;
  }

}

int c_env ( int argc, char **argv, struct gacmn *pcm) {

  char *str, pout[512], cmd[512];   /* Build error msgs here */
  int i, rc;

  rc = 0;
  if ( argc < 1 ) {
    snprintf(pout,511,"Usage: %s  string",argv[0]);
    gaprnt(0,pout);
    return 1;
  }
  cmd[0] = '\0';
  for ( i=1; i<argc; i++ ) {
        if (i>1) strncat(cmd," ",512);
        strncat(cmd,argv[i],512);
  }  
  cmd[511] = '\0';
  if ( (str = xenv(cmd)) ) {
    snprintf(pout,511,"%s\n",str);
    gaprnt(2,pout);
    rc = gacmd(str,pcm,0);
    free(str);
    return rc;
  } else {
    snprintf(pout,511,"%s: error parsing <%s>\n",argv[0], cmd);
    gaprnt(0,pout);
    return 1;
  }

}

/*........................................................................*/

#define MAXLEN 1024

/*
  xenv - expands environment variables in string.

  Like perl, if an environment variable is not defined, it sets its contents
  to an empty string (""). The "\" can be used to scape the $ character,
  and the process id can be retrieved with $$.

  Examples:

  str = "Before $HOME after"    expands to "Before /home/dasilva after"
  str = "Before ${HOME} after"  expands to "Before /home/dasilva after"
  str = "Before $(HOME) after"  expands to "Before /home/dasilva after"
  str = "Before \$HOME after"   expands to "Before $HOME after"
  str = "Process id is $$"      expands to "Process id is 25195"

  Note: The user is responsible for free the memory of the returning 
        string.

*/

static char *xenv(char *str) {

  char *ch, pch, *tmp, *name, *xstr, *beg, *end, *val;
  char cpid[32];
  int i, n, pid;

  pid = (int) getpid(); /* current pid */

  tmp  = (char *) malloc(MAXLEN+1);
  name = (char *) malloc(MAXLEN+1);
  xstr = (char *) NULL;

  for ( ch=str, pch=0, i=0, tmp[0]='\0'; *ch; ch++ ) {
   
    /* Start of an env variable */
    if ( *ch == '$' && pch != '\\' ) {  

       ch++; 

       /* protected variable like ${HOME} */
       if ( *ch == '{' ) {         
         beg = ++ch; 
         end = strchr(ch,'}');
         ch = end;
       } 

       /* protected variable like $(HOME) */
       else if ( *ch == '(' ) {         
         beg = ++ch; 
         end = strchr(ch,')');
         ch = end;
       }

       /* Special case: proccess id (pid) */
       else if ( *ch == '$' ) {         
         beg = ch; 
         end = ch+1;
       }

       /* regular env var: $HOME */
       else {                           
         beg = ch; 
	 end = strpbrk(ch," .,;:+-/*~=?!@#$%^&<({[]})>|'`\"\\");
         if (!end) end = ch + strlen(ch);
	 ch = end-1; /* do not egt rid of delimiter */
       }            

       /* Form name of env variable, retrieve its contents and store it 
          Like perl, if env variable is not defined, set it to empty */
       if ( !end ) goto done;
       // printf("Beg/end/ch = [%c] ... [%c] @ [%c]\n", *beg, *end,*ch);
       strncpy(name,beg,n=end-beg); name[n] = '\0';
       // printf("Name: %s = ", name);
       if ( *name == '$' && n==1 )  sprintf(val=cpid,"%d",pid);
       else                         val = getenv(name);
       if ( val ) { 
         // printf("<%s>\n", val);
	 if ( (i=i+strlen(val)) > MAXLEN ) goto done;
	 strcat(tmp,val);
       } 

    }                                    /* found non-scapped dollar */

    /* This is a \ used for scaping $, do not expand */
    else if ( *ch == '\\' && *(ch+1) == '$' ) {
      pch = *ch;
      continue;  
    }

    /* regular part of string */
    else {                                 
      if ( i<MAXLEN ) {
	tmp[i] = *ch; i++;
	tmp[i] = '\0';
      } 
      else goto done;
    }

    pch = *ch;

  } /* loop over input string */

  /* Return string of right size */
  xstr = (char *) malloc(strlen(tmp)+1);
  strcpy(xstr,tmp);

done:
   free(tmp);
   free(name);
   return (char *) xstr;
}
  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

env.gex - GrADS Extensions for Environment Variable Manipulation

=head1 SYNOPSIS

=head3 GrADS Commands:

=over 4

=item

B<env> I<CMD> - Expands environment variables in I<CMD> and executes it

=item 

B<getenv> I<NAME> - Print value of I<NAME>

=item 

B<setenv> I<NAME> I<VALUE> - Set environment variable I<NAME> to I<VALUE>

=item 

B<printenv> I<STRING> - Expands all environment variables in <STRING>

=back

=head3 GrADS Scripting Language Functions:

=over 4

=item 

B<getenv>(I<NAME>) - Return value of environment variable I<NAME>

=item 

B<setenv>(I<NAME>,I<VALUE>) - Set environment variable I<NAME> to I<VALUE>

=item 

B<printenv>(I<STRING>) - Return string with expansion of all environment variables in <STRING>

=back

=head1 DESCRIPTION 

This library implements GrADS extensions (I<gex>) with commands for
setting and retriving environment variables. In addition it can
exapand all environment variables in a GrADS command string and then
execute the command after the expansion takes places. These features
are available at the GrADS prompt or as GrADS Scription Language functions.

Environment variables can be used in GrADS in a variety of ways. Prior
to starting GrADS you can set up environment variables with the
locations of your data directories and use this environment variable
when opening your files, for example. Another useful application is
for holding truly global information which persists throughout the
extent of your GrADS session. You can use this technique for
communicating between GrADS scripts that are invoked from different
C<run> commands.

=head1 EXAMPLES

=head3 Using B<getenv> to print the value of a list of environment variables

   ga-> getenv USER HOME TMPDIR
   Environment variable USER = dasilva
   Environment variable HOME = /Users/dasilva
   Environment variable TMPDIR = <undef>

Notice that environment variable C<TMPDIR> was not defined in this case.

=head3 Using B<setenv> to set the value of a given environment variable

   ga-> setenv OPENGRADS /share/dasilva/opengrads
   Environment variable OPENGRADS = /share/dasilva/opengrads

=head3 Using B<printenv> expand environment variables in a string

   ga-> printenv $HOME/src/grads/test_data
   /Users/dasilva/src/grads/test_data

Notice that undefined variables exapand to a blank string:

   ga-> printenv Undefined variable FOO = <$FOO>
   Undefined variable FOO = <>

=head3 Get the process id (PID) for creating temporary files, etc

   ga-> printenv /tmp/tempfile.$$
   /tmp/tempfile.21386

=head3 Using B<env> to expand environment variables, then execute a GrADS command

  ga-> env open $OPENGRADS/test_data/model.ctl

Notice that you can use B<@> as a short hand for B<env>:

  ga-> @ open $OPENGRADS/test_data/model.ctl

=head3 Setting environment variables based on another env variable

Using the B<@> short-hand for B<env>:

   ga-> @ setenv FILE $HOME/data/model.ctl
   setenv FILE /Users/dasilva/data/model.ctl
   Environment variable FILE = /Users/dasilva/data/model.ctl

   ga-> @ open $FILE
   open /Users/dasilva/data/model.ctl
   Scanning description file:  /Users/dasilva/data/model.ctl
   Data file /Users/dasilva/data/model.grb is open as file 1
   LON set to 0 360
   LAT set to -90 90
   LEV set to 1000 1000
   Time values set: 1987:1:1:0 1987:1:1:0

=head3 Saving complicated expressions in environment variables (like aliases)

You can use env variables to save long expressions:

   ga-> setenv EXPR ua;va;sqrt(ua*ua+va*va)
   Environment variable EXPR = ua;va;sqrt(ua*ua+va*va)

   ga-> @ display $EXPR
   display ua;va;sqrt(ua*ua+va*va)

=head1 COMMANDS PROVIDED

The following commands are loosely based on the C-Shell syntax:

=head2 B<env> I<CMD> 

=over 4

This command expands environment variables in string I<CMD> and
executes it as a GrADS command after expansion. Example:

   ga-> env open $HOME/data/model.ctl
   open /Users/dasilva/data/model.ctl
   Scanning description file:  /Users/dasilva/data/model.ctl
   Data file /Users/dasilva/data/model.grb is open as file 1
   LON set to 0 360
   LAT set to -90 90
   LEV set to 1000 1000
   Time values set: 1987:1:1:0 1987:1:1:0

=back

=head2 B<getenv> I<NAMEs> 

=over 4

This command takes a list of environment variables and prints the
value of each one. Example:

   ga-> getenv USER HOME DISPLAY
   Environment variable USER = dasilva
   Environment variable HOME = /Users/dasilva
   Environment variable DISPLAY = :0.0

=back

=head2 B<setenv> I<NAME> I<VALUE>

=over 4

This command sets environment variable I<NAME> to I<VALUE>. Example:

   ga-> setenv DATADIR /archive/data
   Environment variable DATADIR = /archive/data

=back

=head2 B<printenv> I<STRING> 

=over 4

This command expands all environment variables in I<STRING>, 
printing the result to the screeen. Example:

   ga-> printenv My home directory is $HOME
   My home directory is /Users/dasilva

=back

=head2 B<xenv> I<STRING>

=over 4

Same as B<printenv>. 

=back

=head2 B<@> I<STRING>

=over 4

Same as B<env>. 

=back

=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=back

=head1 AUTHOR 

Arlindo da Silva (dasilva@opengrads.org)

=head1 COPYRIGHT

Copyright (C) 2007-2008 Arlindo da Silva; All Rights Reserved.

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif
