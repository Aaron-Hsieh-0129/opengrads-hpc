/*

    Copyright (C) 2007-2008 by Arlindo da Silva <dasilva@opengrads.org>
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

                               ---
                        About POD Documentation 

The following documentation uses the "Perl On-line Documentation"
(POD) mark up syntax. It is a simple and yet adequate mark up language
for creating basic man pages, and there are converters to html,
MediaWiki, etc. In adittion, the perldoc utility can be used to
display this documentation on the screen by entering:

% perldoc fish

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < fish.c > fish.pod

and place fish.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < fish.pod > fish.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < fish.pod > fish.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make fish.html
% make fish.wiki

*/

#ifndef POD

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "grads.h"

static int Init = 0; /* whether initialized */

static char pout[256];   /* Build error msgs here */
static char expr1[512];
static char expr2[512];
static int get_latlons ( double **lons, double **lats, 
                         struct gagrid *pgr, char *name );

void ftnfish_ ( double lon[], double lat[], 
                int *im, int *jm, int *mbdcnd, double *amiss, double div[], 
                double velp[], int *rc );

/* GrADS v1.x compatibility functions */
int gex_expr1(char *expr, struct gastat *pst);
int gex_setUndef (struct gastat *pst);
#define gaexpr gex_expr1

/* ---------------------------------------------------------------------- */

int f_fish (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  double *val, amiss, *lat, *lon;
  int rc, i, j, im, jm;

/*
      Meridional boundary condition; see fishpak 

      MBDCND = 1  ! BC: solution specified at both poles 
      MBDCND = 5  ! BC: solution specified at TF (South Pole) and
      MBDCND = 7  ! BC: solution specified at TS (North Pole) and
      MBDCND = 9  ! BC: solution unspecified at both poles
 */
  int mbdcnd;     
  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"          Usage:  fish(expr[,MBDCND])\n\n");
    gaprnt(0,"This function returns the solution of the Poisson equation\n");
    gaprnt(0,"with the RHS given by 'expr'. The optional parameter MBDCND\n");
    gaprnt(0,"specifies the meridional boundary condition for the Poisson\n");
    gaprnt(0,"solver Fishpak; the default MBDCND=9 should work in most cases.\n\n");
    gaprnt(0,"Examples:\n");
    gaprnt(0,"   ga-> d fish(hcurl(ua,va))    (streamfunction)\n");
    gaprnt(0,"   ga-> d fish(hdivg(ua,va))    (velocity potential )\n\n");
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  if (pfc->argnum<2) {
    mbdcnd = 9; /* Default as in old psichi classic UDF */
  } else {
    mbdcnd = atoi(pfc->argpnt[1]); 
  }

  if (pst->type==1) {  /* gridded data */

    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;
    amiss = pgr->undef;

    /* generate coordinate variables */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* Run poisson solver */
    ftnfish_ ( lon, lat, &im, &jm, &mbdcnd, &amiss, val, val, &rc );
    free(lon);
    free(lat);
    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
      return (rc);
    }

    rc = gex_setUndef(pst);
    return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

}

/* .................................................................. */

 static
 int get_latlons ( double **lons, double **lats, 
                         struct gagrid *pgr, char *name  ) {

  int i, j, im, jm;
  double dlon, dlat, *lon, *lat;
  double (*conv) (double *, double);

  /* varying dimensions must be lon and lat for fishpak */
  if ( pgr->idim != 0 || pgr->jdim != 1 ) {
    sprintf(pout,"Error from %s: input must be lat/lon grid \n", name);
    gaprnt (0,pout);
    return (1);
  }

  /* For now, force linear scaling because fishpak assumes so */
  if ( pgr->ilinr != 1 || pgr->jlinr != 1 ) {
    sprintf(pout,"Error from %s: lat/lon coords must be LINEAR \n", name);
    gaprnt (0,pout);
    return (1);
  }

  /* Longitudes */
  im   = pgr->isiz;
  lon = (double *) malloc ( sizeof(double)*im );
  if ( lon ) {
    conv = pgr->igrab;
    lon[0] = conv(pgr->ivals,pgr->dimmin[pgr->idim]);
    dlon = *(pgr->ivals);
    for ( i=1; i<im; i++ ) lon[i] = lon[0] + i*dlon;
  } 
  else {
    sprintf(pout,"Error from %s: out of memory (lon)\n", name);
    gaprnt (0,pout);
    return (1);
    }
  
  /* latitudes */
  jm   = pgr->jsiz;
  lat = (double *) malloc ( sizeof(double)*jm );
  if ( lat ) {
    conv = pgr->jgrab;
    lat[0] = conv(pgr->jvals,pgr->dimmin[pgr->jdim]);
    dlat = *(pgr->jvals);
    for ( j=1; j<jm; j++ ) lat[j] = lat[0] + j*dlat;
  }
  else {
    sprintf(pout,"Error from %s: out of memory (lat)\n", name);
    gaprnt (0,pout);
    free (lon);
    return (1);
  }

#if 0
  printf("\n%s:\n ",name);
  printf("%s: Longitudes = ",name);
  for ( i=0; i<im; i++ ) printf("%g ",lon[i]);
  printf("\n%s:\n ",name);
  printf("%s: Latitudes = ",name);
  for ( j=0; j<jm; j++ ) printf("%g ",lat[j]);
  printf("\n%s:\n ",name);
#endif  

  *lons = lon;
  *lats = lat;

  return (0);

}

/* .................................................................... */

void gaprnt_(char *name, char *msg ) {
  sprintf(pout,"Error from %s: %s", name, msg);
  gaprnt (0,pout);
}

/* .................................................................... */

/*
   Implements streamfunction or vorticity.
*/

int f_psichi(struct gafunc *pfc, struct gastat *pst) {
  int rc;
  char    *one = "1+0*lat";
  char *cosphi = "cos(0.0174532925199433*lat)";
  char *u, *v;
  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<2) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s(UEXPR,VEXPR[,MBDCND])\n\n", name);
    gaprnt(0,pout);
    gaprnt(0,"The optional parameter MBDCND specifies the meridional boundary\n");
    gaprnt(0,"condition for the Poisson solver Fishpak; the default MBDCND=9\n");
    gaprnt(0,"should work in most cases.\n\n");
    return (1);
  }

#ifdef __CYGWIN32__
  if ( !Init ) {
       // gadudf();
       Init = 1;
  }
#endif

  u = pfc->argpnt[0];
  v = pfc->argpnt[1];

  /* vor = - ( madvu(vgrd,one) - madvv(ugrd,cosphi) / cosphi ) */
  if ( strcmp(name,"fish_psi") == 0 || strcmp(name,"fish_vor") == 0 ) 
       sprintf(expr1,"-(madvu(%s,%s)-madvv(%s,%s)/%s)", 
                      v, one, u, cosphi, cosphi);

  /* div = - ( madvu(ugrd,one) + madvv(vgrd,cosphi) / cosphi ) */
  else
  if ( strcmp(name,"fish_chi") == 0 || strcmp(name,"fish_div") == 0 ) 
    sprintf(expr1,"-(madvu(%s,%s)+madvv(%s,%s)/%s)",
	    u, one, v, cosphi, cosphi );
  else {
    sprintf(pout,"\nError from %s: invalid function name \n\n", name);
    return (1);
  }

  /* Solve poisson equation, if needed */
  if ( strcmp(name,"fish_psi") == 0 ||strcmp(name,"fish_chi") == 0 ) {
    if ( pfc->argnum<3 ) sprintf(expr2,"fish(%s)",    expr1);
    else                 sprintf(expr2,"fish(%s,%s)", expr1, pfc->argpnt[2]);
    rc = gaexpr(expr2,pst);
  } else {
    rc = gaexpr(expr1,pst);
  }

  return(rc);

}

/* .................................................................... */


  
/*

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

fish.gex - GrADS Extension Library for Calculating Streamfunction/Velocity Potential

=head1 SYNOPSIS

=head3 GrADS Functions:

=over 4

=item

B<fish>(I<UEXPR,VEXPR[,MBDCND]>) - Poisson Solver

=item 

B<fish_psi>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Stream Function

=item 

B<fish_chi>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Velocity Potential

=item 

B<fish_vor>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Relative Vorticity

=item 

B<fish_div>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Divergence 

=back

=head1 DESCRIPTION 

This library provides GrADS extensions (I<gex>) with functions for
computation of streamfunction and velocity potential from zonal and
meridional wind components:

   laplacian(psi) = vorticity       (1)
   laplacian(chi) = divergence      (2)

where C<psi> is the streamfunction and C<chi> is the velocity
potential. (See Wikipedia links below for more information on
streamfunction/velocity potential.)  The vorticity and divergence
computation relies on functions B<madvu> and B<madvv> provided in the
OpenGrADS extension I<Libbjt> by B.-J. Tsuang. The Poisson equations
(1)-(2) above are solved using the classic C<Fishpak> Fortran
library. Documentation for C<FISHPAK> is given in:

Swarztrauber, P. and R. Sweet, 1975: Efficient Fortran Subprograms for
the Solution of Elliptic Partial Differential Equations. I<Technical Note
TN/IA-109>. National Center for Atmospheric Research, Boulder, Colorado 80307.

All functions provided require a global, uniform lat/lon grid. 

=head1 EXAMPLES

For the following examples it is suggested that you open the following
OPeNDAP dataset with a GFS forecast. Fire up C<gradsdods> and then

   ga-> sdfopen http://nomad2.ncep.noaa.gov:9090/dods/gfs/rotating/gfs_00z

=head2 Computing Streamfunction from Vorticity

If you have the relative vorticity field available, say C<vor>, you
can easily compute streamfunction 

   ga-> psi = fish(vor)

=head2 Computing Streamfunction from Wind Components

The first step is to evaluate the streamfunction:

  ga-> set lev 200
  ga-> psi = fish_psi(ugrd,vgrd)

It is often convenient to I<center> the streamfunction by
subtracting its global mean:

  ga-> psi = psi - aave(psi,global)

We can finally display it:

  ga-> set gxout shaded
  ga-> display psi/1e7  
  draw title Streamfunction

=head2 Computing the Rotational Wind 

Although the intrinsc GrADS function C<cdiff> could be used for
numerically diffferentiating the streamfunction, we use here functions
C<mvadv/muadv> from I<Libbjt> because of its handling of the
boundaries:

  ga-> one  = 1 + 0 * lat
  ga-> upsi =   mvadv(one,psi)
  ga-> vpsi = - muadv(one,psi)

We then plot the rotational streamlines on top of the streamfunction:

  ga-> set gxout shaded
  ga-> display psi/1e7  
  ga-> set gxout stream
  ga-> display upsi;vpsi
  ga-> draw title Streamfunction/Rotational Wind

=head2 Computing Velocity Potential from Divergence

If you have the divergence field available, say C<div>, you
can easily compute streamfunction 

   ga-> chi = fish(div)

=head2 Computing Velocity Potential from Wind Components

Start by computing the velocity potential:

  ga-> set lev 200
  ga-> chi = fish_chi(ugrd,vgrd)

It is often convenient to I<center> the velocity potential by
subtracting its global mean:

  ga-> psi = psi - aave(psi,global)

We can finally display it:

  ga-> set gxout shaded
  ga-> display chi/1e6  
  draw title Velocity

=head2 Calculating the Divergent Wind

Although the intrinsc GrADS function C<cdiff> could be used for
numerically diffferentiating the velocity potential, we use here
functions C<mvadv/muadv> from I<Libbjt> because of its handling of the
boundaries:

  ga-> uchi = - muadv(one,chi)
  ga-> vchi = - mvadv(one,chi)

We finally display the divergent wind as vectors on top of the
velocity potential:

  ga-> display chi/1e6
  ga-> set gxout vector
  ga-> set cmin  2
  ga-> set cmax 20
  ga-> display skip(uchi,6,6);vchi
  ga-> draw title Velocity Potential/Divergent Wind

=head2 Computing Vorticity and Divergence

As an alternative to the intrinsic GrADS functions C<hcurl/hdivg>,
functions C<fish_vor> and C<fish_div> uses the advention functions in
I<Libbjt> to numerically evaluate vorticity and divergence

   ga-> vor = fish_vor(ugrd,vgrd)
   ga-> div = fish_div(ugrd,vgrd)

These functions provide a better handling of the boundaries compared
to their intrinsic counterparts.

=head1 FUNCTIONS PROVIDED

=head2  B<fish>(I<UEXPR,VEXPR[,MBDCND]>) - Poisson Solver

=over 4

This function uses routine C<PWSSSP> in C<Fishpak> to solve the
poisson equation. The default parameter C<MBDCND=9> solves a Helmholts
equation with a very small random term to provide a "unique" solution.

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=item I<MBDCND> - optional

Meridional boundary condition; the default C<MBDCND=9> should work in
most cases. See C<FISHPAK> documentation for additional information.

      MBDCND = 1  ! BC: solution specified at both poles 
      MBDCND = 5  ! BC: solution specified at TF (South Pole) and
      MBDCND = 7  ! BC: solution specified at TS (North Pole) and
      MBDCND = 9  ! BC: solution unspecified at both poles

=back

=back

=head2  B<fish_psi>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Stream Function

=over 4

This function computes vorticity as in B<fish_vor> and uses B<fish> to
solve the Poisson equation for the streamfunction C<psi>:

   laplacian(psi) = vorticity

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=item I<MBDCND> - optional

Meridional boundary condition; the default C<MBDCND=9> should work in
most cases. See C<FISHPAK> documentation for additional information.

      MBDCND = 1  ! BC: solution specified at both poles 
      MBDCND = 5  ! BC: solution specified at TF (South Pole) and
      MBDCND = 7  ! BC: solution specified at TS (North Pole) and
      MBDCND = 9  ! BC: solution unspecified at both poles

=back

=back

=head2  B<fish_chi>(I<UEXPR,VEXPR[,MBDCND]>) - Computes Velocity Potential

=over 4

This function computes divergence as in B<fish_div> and uses B<fish> to
solve the Poisson equation for the velocity potential C<chi>:

   laplacian(chi) = divergence

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=item I<MBDCND> - optional

Meridional boundary condition; the default C<MBDCND=9> should work in
most cases. See C<FISHPAK> documentation for additional information.

      MBDCND = 1  ! BC: solution specified at both poles 
      MBDCND = 5  ! BC: solution specified at TF (South Pole) and
      MBDCND = 7  ! BC: solution specified at TS (North Pole) and
      MBDCND = 9  ! BC: solution unspecified at both poles

=back

=back


=head2 B<fish_vor>(I<UEXPR,VEXPR>) - Computes Relative Vorticity

=over 4

This function computes the vorticity using the expression:

   vorticity = - ( madvu(v,one) - madvv(u,cosphi) / cosphi ) 

where C<u> and C<v> are the zonal/meridional wind components, C<one>
is a constant field equal to 1, and C<cosphi> is the cosine of
latitude. The functions B<madvu> and B<madvv> are provided in the OpenGrADS 
extension library I<Libbjt>.

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=back

=back


=head2  B<fish_div>(I<UEXPR,VEXPR>) - Computes Divergence 

=over 4

This function computes the divergence using the expression:

   divergence = - ( madvu(u,one) + madvv(v,cosphi) / cosphi ) 

where C<u> and C<v> are the zonal/meridional wind components, C<one>
is a constant field equal to 1, and C<cosphi> is the cosine of
latitude. The functions B<madvu> and B<madvv> are provided in the OpenGrADS 
extension library I<Libbjt>.


=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=back

=back

=head1 BUGS

The function C<fish> assumes a global and uniform longitude/latitude grid. If
your grid is not uniform, consider using C<re()> for interpolating to
a uniform grid. When doing so, do not make the poles gridpoints. (The
function C<muadv> from I<Libbjt> produces undefined values if the first
and last latitudinal gridpoints are at the poles.)

Undefined values are handled in a less than idea way by the Poisson
solver C<fish>. If undefined values of vortivity/divergence occur at
the first and last latitudinal gridpoint, the following I<polar fix>
is applied depending on whether these gridpoints are the poles or not:

Notice that C<fish()> becomes numerically unstable for horizontal 
resolutions finer that 1/2 degrees or so. In such cases use the
spherical harmonic based B<sh_fish> given in extension I<shfilt>.

=over 4

=item Poles are gridpoints

If the first and last latitudinal gridpoints are at the poles, the
zonal average of the adjascent latitudinal band is computed and this
zonal averaged value is used at the pole.

=item Poles are not gridpoints

The value at the same longitude in the adjascent latitudinal band is
used.

=back

For interior points, undefined values are set to zero before solving
the Poisson equation.


=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://opengrads.org/wiki/index.php?title=User_Defined_Extensions> - OpenGrADS User Defined Extensions

=item *

L<http://www.iges.org/grads/> - Official GrADS Home Page

=item *

L<http://en.wikipedia.org/wiki/Velocity_potential> - Velocity Potential definition on Wikipedia.

=item *

L<http://en.wikipedia.org/wiki/Stream_function> - Stream function
definition on Wikipedia.

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
