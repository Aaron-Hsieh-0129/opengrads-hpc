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


                        About POD Documentation 

The following documentation uses the "Perl On-line Documentation"
(POD) mark up syntax. It is a simple and yet adequate mark up language
for creating basic man pages, and there are converters to html,
MediaWiki, etc. In adittion, the perldoc utility can be used to
display this documentation on the screen by entering:

% perldoc libipc.c 

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < libipc.c > libipc.pod

and place libipc.pod in a place perldoc can find it.

*/

#ifndef POD

/* gsudf: GrADS user defined function providing a gateway to 
          to implement functions as a .gs script.
          The name of the gs script function must be entered in
          the UDX table, associating it with this C function.
          See example.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grads.h"
#include "gs.h"

static char *gsufile (char *fname, int argc, char **argv, int *retc, int impp);

static char pout[256];   /* Build error msgs here */

void gex_setPDF(struct gastat *pst);

/*.....................................................................*/

int f_gsudf (struct gafunc *pfc, struct gastat *pst) {

  char *expr, *mbuf, fname[1024];
  int rc;
  char *name = pfc->argpnt[pfc->argnum];

    /* run script, expect expression */
    sprintf(fname,"%s.gsf", name);
    expr = gsufile(fname, pfc->argnum, pfc->argpnt, &rc, 1); 
    if ( expr == NULL ) rc = 1;  /* user returns NULL to signal an error */
    if (rc) return(rc);

    gex_setPDF(pst); /* so that newly defined var can be found */

    /* evaluate expression */
    rc = gaexpr(expr,pst);  

    return (rc);

}

/*.....................................................................*/

/* Allocates and fills function argument object */

static struct gsvar * new_parg ( char *str ) {

  struct gsvar *parg;

  /* Get ready to start executing the script.
     Allocate a var block and provide arg string */

  parg = (struct gsvar *)malloc(sizeof(struct gsvar));
  if (parg==NULL) {
    printf ("Memory allocation error:  Script variable (parg)\n");
    return (struct gsvar *) NULL;
  }
  parg->forw = NULL;

  parg->strng = (char *)malloc(1024);
  if (parg->strng==NULL) {
    printf ("Memory allocation error:  Script variable (strng)\n");
    free (parg);
    return (struct gsvar *) NULL;
  }

  strcpy(parg->strng,str);
  return parg;

}

/*.....................................................................*/

/*  Execute a script, from one or more files. 
    Beware: various levels of recursion are used.    

    This is a version of gsfile customized to pass an argument list
    with more than one variable. (ams)
 
 */

static char *gsufile (char *fname, int argc, char **argv, int *retc, int impp){
struct gsvar *parg, *narg;
struct gscmn *pcmn;
char *ch,*res;
int len,rc,i;

  /* Allocate the common structure; this anchors most allocated
     memory for executing this script */

  pcmn = (struct gscmn *)malloc(sizeof(struct gscmn));
  if (pcmn==NULL) {
    printf ("Error executing script file: --> %s <--\n",fname);
    printf ("  Memory allocation error \n");
    *retc = 1;
    return(NULL);
  }
  pcmn->ffdef = NULL;
  pcmn->lfdef = NULL;
  pcmn->frecd = NULL;
  pcmn->lrecd = NULL;
  pcmn->fvar = NULL;
  pcmn->ffnc = NULL;
  pcmn->iob = NULL;
  pcmn->gvar = NULL;
  pcmn->farg = NULL;
  pcmn->fname = fname;  /* Don't free this later */
  pcmn->fprefix = NULL;
  pcmn->ppath = NULL;
  pcmn->rres = NULL;
  pcmn->gsfflg = 1;   /* Do dynamic functions by default as
                         gsudf is  meant to handle functions only */
  res = NULL;

  /* Open, read, and scan the script file. */
  
  rc = gsgsfrd (pcmn,0,fname); /* read as a main script, though */

  if (rc) {
    gsfree(pcmn);
    if (rc==9) {
      if (impp) {      /* This should be handled by caller -- fix later */
        gaprnt (0,"Cannot find gs function: ");
        gaprnt (0,fname);
        gaprnt (0,"\n");
      } else {
        printf ("Error opening script function file: %s\n",fname);
      }
    }
    *retc = 1;
    return(NULL);
  }

  /* Get ready to start executing the script.
     Allocate a var block and provide arg string */

  if ( argc < 1 ) parg = (struct gsvar *) NULL;
  else            parg = new_parg ( argv[0] );
  pcmn->farg = parg;                         /* record first argument */

  for ( i = 1; i<argc; i++ ) {               /* linked list w/ other args */
    narg = new_parg ( argv[i] );             /* next argument */
    if ( narg == NULL ) {
      rc = -1;
      goto retrn;
    }
    parg->forw = narg;
    parg = narg;
  }

  /* Execute the main function. */
  
  rc = gsrunf(pcmn->frecd, pcmn);
  res = pcmn->rres;
  if (rc==999) rc = -1;

  /*  We are done.  Return.  */

retrn:
  gsfree (pcmn);
  *retc = rc;
  return (res);
}


  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

gsudf.gex - Writing Expression Functions as GrADS Scripts

=head1 SYNOPSIS

  ga-> display myfunc(args)

where B<myfunc> is implemented in a GrADS script file named C<myfunc.gsf>

=head1 DESCRIPTION 

Traditionally, there are two kinds of functions in GrADS: script
functions and expression functions, and up to now these two did not
mix. I<Scripting functions> are user defined and called from any GrADS
script. Such functions are not directly available at the command
line, or cannot be used inside GrADS expressions. I<Expression
functions> are usually used in conjunction with the C<display> and
C<define> commands such as

   ga-> display aave(myfunc(u), global)

where C<aave> is an intrinsic function and C<myfunc> is a user
defined function. 

The dynamic extension I<gsudf> allows you to write I<expression
functions> as GrADS I<script functions>. This is better explained with
the following examples.

=head1 EXAMPLES

=head2 Using I<gsudf> for Computing Wind Speed

Let us reimplement the intrinsic function C<mag> which compute the
wind speed given the C<u> and C<v> components of the wind; we will
call this new function C<speed>. The first step is to create a text
file called C<speed.gsf> with the implementation of this function:

   function speed ( u, v )
      expr = 'sqrt(' u '*' u '+' v '*' v ')'
      return expr

Notice that this function simply returns a string which will
eventually be evaluated by the GrADS expression parser; more on that
later. The second step in the implementation of this function is to
add an entry for this function in your User Defined eXtenfion (UDXT)
file:

      Type  API   Name      Function       Library
     ------ --- ---------  -----------  --------------
      udf    0    speed      f_gsudf     gsudf.gex.so

The generic handler C<f_gsudf> in the shared library C<gsudf.gex>
executes the file C<speed.gsf>, evaluates the string output of this
function using the GrADS expression parser and returns the result. The
net effect is that C<speed.gsf>, with the help of its agent C<gsudf>,
implements a bonafide GrADS expression function.

Finally, you no longer need to initialize the I<gsudf>
package in GrADS v2.0.

   ga-> open model.ctl
   ga-> display speed(ua,va)

=head2 Returning defined variables

Although your GrADS script must always return a string, it does not
mean that all it can do is string maniputation. By using the C<define>
command one can always pre-computed intermediate values, and return
the name of the defined variable. Here is an example where the
dimension environment is modified, a variable defined in this enlarged
dimension environment, and then the dimension reset to its original
state:

   function x_cdiff ( var, dim )
      'set x 0 74'
      'define gsudf = cdiff('var','dim')'
      'set x 1 73'
      return 'gsudf'

This function enlarges the domain as to avoid missing undefined values
on the longitudinal boundaries (but not at the latitudinal boundary.)
A more generic version of this function is included with the source
distribution for this extension.

=head2 Returning an error condition

Returning an empty string will signal an error condition

   function errfunc(arg)
      say 'errfunc: error found, cannot proceed'
      return

=head1 FUNCTION PROVIDED

=head2 B<gsudf> I<[ARGUMENTS]>

=over 4

This function is not directly callable as such from GrADS. Rather it
works as a I<proxy> for the user defined functions written as GrADS
scripts. There 2 steps involved in defining your user defined function
through gsudf:

=over 4

=item 1.

For each function named I<$func>, create a script library file called
I<$func.gsf> and placed in a directory where GrADS can locate scripts
(for example, in a directory listed in your C<GASCRP> environment
variable.) The GrADS script must return a string with a valid GrADS
expression, or an empty string in case of an abnormal exit.

=item 2.

Have a corresponding entry in your User Defined eXtension (UDXT) file, e.g.,

         A
         P   GrADS     Library      Library                 Short
    Type I  Function   Function      Path                Description
    ---- - ---------- ---------- -------------- -------------------------------  
    udf  0   speed     f_gsudf    ^gsudf.gex    "The mag() function as gsUDF"

=back

=back

=head1 BUGS

There is not much in terms of garbage collection. Any local variable
defined by the GrADS script implementing the expression function stays
around until explicitly undefined. To minimize memory usage explicitly
undefine any variabled you defined, and when defined variables are
involved, return your result in defined variable called B<gsudf>.

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

Copyright (C) 2007-2009 Arlindo da Silva; All Rights Reserved.

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif


