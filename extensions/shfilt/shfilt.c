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

% perldoc shfilt

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < shfilt.c > shfilt.pod

and place shfilt.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < shfilt.pod > shfilt.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < shfilt.pod > shfilt.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make shfilt.html
% make shfilt.wiki

*/

#ifndef POD

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "grads.h"
#include "shfilt.h"

static int Init = 0; /* whether initialized */

static char pout[256];   /* Build error msgs here */
static char expr1[512];
static char expr2[512];
static int get_latlons ( double **lons, double **lats, 
                         struct gagrid *pgr, char *name );

/* Fortran prototypes */

void sphfil_ ( float gfil[], float grid[], 
               float lons[], float lats[], int *nlon, int *nlat, 
               int *n1, int *n2, int *grid_type, int *ier );

void sphpwr_ ( float gpwr[], float grid[], 
               float lons[], float lats[], int *nlon, int *nlat, 
               int *grid_type, int *ier );

void pssolv_ ( float gfil[], float grid[], 
               float lons[], float lats[], int *nlon, int *nlat, 
               int *grid_type, double *amiss, int *ier );

/* ---------------------------------------------------------------------- */

int f_shfilt (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  double *val, amiss, *lat, *lon;           /* GrADS v2 uses doubles */
  float  *f_val, *f_lat, *f_lon;            /* fortran needs floats */
  int rc, i, j, im, jm, n1, n2, grid_type;
  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<2) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s(expr,n1[,n2])\n\n",name);
    gaprnt(0,pout);
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

    if (pfc->argnum<3) {
    n1 = 1;
    n2 = atoi(pfc->argpnt[1]); 
  } else {
    n1 = atoi(pfc->argpnt[1]); 
    n2 = atoi(pfc->argpnt[2]); 
  }

  if (pst->type==1) {  /* gridded data */

    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;

    /* make sure there is no undefs */
    rc = 0;
    for ( i=0; i<im*jm; i++ ) if ( !pgr->umask[i] ) rc = 1;
    if ( rc ) {
      sprintf(pout,"Error from %s: cannot handle undeds\n", name);
      gaprnt (0,pout);
      return(rc);
    }

    /* Heuristic method for detecting a Gaussian Grid */
    if ( pgr->ilinr==1 && pgr->jlinr==0 ) {
      grid_type = GAUSSIAN;
    } else {
      grid_type = LATLON;
    }

    /* generate coordinate variables */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* spherepak requires poles to be gridpoints */
    if ( grid_type == LATLON ) {
      if ( (fabs(lat[0]) -90.)>0.0001 ||
	   (fabs(lat[jm-1])-90.)>0.0001 ) {
	sprintf(pout,
    "Error from %s: first (%f) and last (%f) latitude must be at the poles\n", 
		name, lat[0],lat[jm-1]);
	gaprnt (0,pout);
	return (1);
      } 
    }

    /* Convert doubles to floats for spherepak */
    if ( !(f_val = (float *) malloc(im*jm*sizeof(float))) ) goto memerr;
    if ( !(f_lon = (float *) malloc(   im*sizeof(float))) ) goto memerr;
    if ( !(f_lat = (float *) malloc(   jm*sizeof(float))) ) goto memerr;
    for ( i=0; i<im*jm; i++ ) f_val[i] = (float) val[i];
    for ( i=0; i<im;    i++ ) f_lon[i] = (float) lon[i];
    for ( i=0; i<jm;    i++ ) f_lat[i] = (float) lat[i];

    /* Run the spectral filter in Fortran */
    sphfil_ ( f_val, f_val, f_lon, f_lat, &im, &jm, &n1, &n2, 
              &grid_type, &rc );

    /* Convert result to double */
    for ( i=0; i<im*jm; i++ ) val[i] = (double) f_val[i];

    free(f_val);
    free(lon);
    free(lat);
    free(f_lon);
    free(f_lat);

    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    }
    return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

 memerr:
    sprintf(pout,"Error from %s: not enough memory\n", name);
    gaprnt (0,pout);
    return (1);

}

/* ---------------------------------------------------------------------- */

int f_shpowr (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  double *val, *val1d, amiss, *lat, *lon, *jvals;
  float *f_val, *f_lat, *f_lon;
  double (*conv) (double *, double);
  int rc, i, j, im, jm, grid_type;
  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s(expr)\n\n",name);
    gaprnt(0,pout);
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  if (pst->type==1) {  /* gridded data */

    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;

    /* make sure there is no undefs */
    rc = 0;
    for ( i=0; i<im*jm; i++ ) if ( !pgr->umask[i] ) rc = 1;
    if ( rc ) {
      sprintf(pout,"Error from %s: cannot handle undeds\n", name);
      gaprnt (0,pout);
      return(rc);
    }

    /* Heuristic method for detecting a Gaussian Grid */
    if ( pgr->ilinr==1 && pgr->jlinr==0 ) {
      grid_type = GAUSSIAN;
    } else {
      grid_type = LATLON;
    }

    /* generate coordinate variables */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* spherepak requires poles to be gridpoints */
    if ( grid_type == LATLON ) {
      if ( (fabs(lat[0]) -90.)>0.0001 ||
	   (fabs(lat[jm-1])-90.)>0.0001 ) {
	sprintf(pout,
		"Error from %s: first (%f) and last (%f) latitudes must be at the poles\n", 
		name, lat[0],lat[jm-1]);
	gaprnt (0,pout);
	return (1);
      }
    }

    /* Convert doubles to floats for spherepak */
    if ( !(f_val = (float *) malloc(im*jm*sizeof(float))) ) goto memerr;
    if ( !(f_lon = (float *) malloc(   im*sizeof(float))) ) goto memerr;
    if ( !(f_lat = (float *) malloc(   jm*sizeof(float))) ) goto memerr;
    for ( i=0; i<im*jm; i++ ) f_val[i] = (float) val[i];
    for ( i=0; i<im;    i++ ) f_lon[i] = (float) lon[i];
    for ( i=0; i<jm;    i++ ) f_lat[i] = (float) lat[i];

    /* Get power spectra in Fortran */
    sphpwr_ (f_val, f_val, f_lon, f_lat, &im, &jm, &grid_type,  &rc );

    /* Convert result to double */
    for ( i=0; i<im*jm; i++ ) val[i] = (double) f_val[i];

    free(lon);
    free(lat);
    free(f_lon);
    free(f_lat);
    free(f_val);
    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    }

    /* Make output 1D */
    val1d = (double *) malloc ( jm * sizeof(double) );
    if ( val1d ) {
      i = 0;
      for ( j=0; j<jm; j++ ) {
	val1d[j] = (double) val[i];
        i += im;
      }
      pgr->idim=1; /* latitude */
      pgr->jdim=-1;
      pgr->ilinr = 0; 
      pgr->jlinr = 1; 
      conv = pgr->igrab;
      pgr->igrab = pgr->jgrab;
      pgr->jgrab = conv;
      conv = pgr->iabgr;
      pgr->iabgr = pgr->jabgr;
      pgr->jabgr = conv;
      pgr->ivals = pgr->jvals;
      pgr->isiz = jm;
      pgr->jsiz = 1;
      jvals = pgr->jvals;
      pgr->jvals = pgr->ivals;
      pgr->ivals = jvals;
      jvals = pgr->javals;
      pgr->javals = pgr->iavals;
      pgr->iavals = jvals;
      pgr->grid = val1d;
      free(val);
    } else {
      sprintf(pout,"Error from %s: out of memory (val1d)\n", name);
      gaprnt (0,pout);
      free (lon);
      return (1);
    }

    return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

 memerr:
    sprintf(pout,"Error from %s: not enough memory\n", name);
    gaprnt (0,pout);
    return (1);

}

int f_pssolv (struct gafunc *pfc, struct gastat *pst) {

  struct gagrid *pgr;
  double *val, amiss, *lat, *lon;           /* GrADS v2 uses doubles */
  float  *f_val, *f_lat, *f_lon;            /* fortran needs floats */
  int rc, i, j, im, jm, grid_type;
  char *name = pfc->argpnt[pfc->argnum];

  if (pfc->argnum<1) {
    sprintf(pout,"\nError from %s: Too many or too few args \n\n", name);
    gaprnt(0,pout);
    sprintf(pout,"          Usage:  %s(expr)\n\n",name);
    gaprnt(0,pout);
    return(1);
  }

  /* evaluate expression: should be vorticity or divergence */
  rc = gaexpr(pfc->argpnt[0],pst);
  if (rc) return (rc);

  if (pst->type==1) {  /* gridded data */

    pgr  = pst->result.pgr;
    val  = pgr->grid;
    im   = pgr->isiz;
    jm   = pgr->jsiz;
    amiss  = pgr->undef; // bma added

    /* make sure there is no undefs */
    /* commenting out the next lines, we will fish undef later on, bma */
    /*
    rc = 0;
    for ( i=0; i<im*jm; i++ ) if ( !pgr->umask[i] ) rc = 1;
    if ( rc ) {
      sprintf(pout,"Error from %s: cannot handle undefs\n", name);
      gaprnt (0,pout);
      return(rc);
    }
    */

    /* add this to get the lats and lons */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* Heuristic method for detecting a Gaussian Grid */
    if ( pgr->ilinr==1 && pgr->jlinr==0 ) {
      grid_type = GAUSSIAN;
    } else {
      grid_type = LATLON;
    }

    /* generate coordinate variables */
    rc = get_latlons ( &lon, &lat, pgr, name );
    if (rc) return (rc);

    /* spherepak requires poles to be gridpoints */
    if ( grid_type == LATLON ) {
      if ( (fabs(lat[0]) -90.)>0.0001 ||
	   (fabs(lat[jm-1])-90.)>0.0001 ) {
	sprintf(pout,
    "Error from %s: first (%f) and last (%f) latitude must be at the poles\n", 
		name, lat[0],lat[jm-1]);
	gaprnt (0,pout);
	return (1);
      } 
    }

    /* Convert doubles to floats for spherepak */
    if ( !(f_val = (float *) malloc(im*jm*sizeof(float))) ) goto memerr;
    if ( !(f_lon = (float *) malloc(   im*sizeof(float))) ) goto memerr;
    if ( !(f_lat = (float *) malloc(   jm*sizeof(float))) ) goto memerr;

    for ( i=0; i<im*jm; i++ ) f_val[i] = (float) val[i];
    for ( i=0; i<im;    i++ ) f_lon[i] = (float) lon[i];
    for ( i=0; i<jm;    i++ ) f_lat[i] = (float) lat[i];

    /* Run the spectral filter in Fortran */
    pssolv_ ( f_val, f_val, f_lon, f_lat, &im, &jm, 
              &grid_type, &amiss, &rc );

    /* Convert result to double */
    for ( i=0; i<im*jm; i++ ) val[i] = (double) f_val[i];

    free(f_val);
    free(lon);
    free(lat);
    free(f_lon);
    free(f_lat);

    if ( rc ) {
      sprintf(pout,"Error from %s: rc = %d\n", name, rc );
      gaprnt (0,pout);
    }
    return(rc);

  } else {  /* station data */

    sprintf(pout,"Error from %s: Station data? Are you kidding?\n", 
            name);
    gaprnt (0,pout);
    return (1);

  }

 memerr:
    sprintf(pout,"Error from %s: not enough memory\n", name);
    gaprnt (0,pout);
    return (1);

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
    sprintf(pout,"          Usage:  %s(UEXPR,VEXPR)\n\n", name);
    gaprnt(0,pout);
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
  if ( strcmp(name,"sh_psi") == 0 || strcmp(name,"sh_vor") == 0 ) 
       sprintf(expr1,"-(madvu(%s,%s)-madvv(%s,%s)/%s)", 
                      v, one, u, cosphi, cosphi);

  /* div = - ( madvu(ugrd,one) + madvv(vgrd,cosphi) / cosphi ) */
  else
  if ( strcmp(name,"sh_chi") == 0 || strcmp(name,"sh_div") == 0 ) 
    sprintf(expr1,"-(madvu(%s,%s)+madvv(%s,%s)/%s)",
	    u, one, v, cosphi, cosphi );
  else {
    sprintf(pout,"\nError from %s: invalid function name \n\n", name);
    return (1);
  }

  /* Solve poisson equation, if needed */
  if ( strcmp(name,"sh_psi") == 0 ||strcmp(name,"sh_chi") == 0 ) {
    sprintf(expr2,"sh_fish(%s)", expr1);
    rc = gaexpr(expr2,pst);
  } else {
    rc = gaexpr(expr1,pst);
  }

  return(rc);

}

/* .................................................................. */

 static
 int get_latlons ( double **lons, double **lats, 
                   struct gagrid *pgr, char *name  ) {

  int i, j, im, jm;
  double dlon, dlat, *lon, *lat;
  double (*conv) (double *, double);

  /* varying dimensions must be lon and lat for spherepak */
  if ( pgr->idim != 0 || pgr->jdim != 1 ) {
    sprintf(pout,"Error from %s: input must be lat/lon grid \n", name);
    gaprnt (0,pout);
    return (1);
  }

  /* Longitudes */
  im   = pgr->isiz;
  j = 0;
  lon = (double *) malloc ( sizeof(double)*im );
  if ( lon ) {
    conv = pgr->igrab;
    for (i=pgr->dimmin[pgr->idim];i<=pgr->dimmax[pgr->idim];i++) 
      lon[j++] = conv(pgr->ivals,(double) i );
  } 
  else {
    sprintf(pout,"Error from %s: out of memory (lon)\n", name);
    gaprnt (0,pout);
    return (1);
    }
  
  /* latitudes */
  j = 0;
  jm   = pgr->jsiz;
  lat = (double *) malloc ( sizeof(double)*jm );
  if ( lat ) {
    conv = pgr->jgrab;
    for (i=pgr->dimmin[pgr->jdim];i<=pgr->dimmax[pgr->jdim];i++) 
      lat[j++] = conv(pgr->jvals,(double) i );
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

                         -----------------
                         POD Documentation
                         -----------------
*/

#else

=pod

=head1 NAME

shfilt.gex - GrADS Extension Library with Spherical Harmonic Utilities

=head1 SYNOPSIS

=head3 GrADS Functions:

=over 4

=item 

display B<sh_filt>(I<EXPR,N1[,N2]>) - Spherical Harmonic Filter

=item 

display B<sh_power>(I<EXPR>) - Spherical Harmonic Power Spectra

=item 

display B<sh_fish>(I<EXPR>) - Poisson Solver

=item 

display B<sh_psi>(I<UEXPR,VEXPR>) - Computes Stream Function

=item 

display B<sh_chi>(I<UEXPR,VEXPR>) - Computes Velocity Potential

=item 

display B<sh_vor>(I<UEXPR,VEXPR>) - Computes Relative Vorticity

=item 

display B<sh_div>(I<UEXPR,VEXPR>) - Computes Divergence 

=back

=head3 GrADS Script:

=over 4

=item 

run B<power.gs> I<EXPR> N - Plot Spherical Harmonic Power Spectra

=back

=head1 DESCRIPTION 

This library provides GrADS extensions (I<gex>) with functions for
spherical harmonic filtering and calculation of spherical harmonic
power spectra. The numerical calculations rely on the excellent
I<Spherepak> library by John C. Adams and Paul N. Swarztrauber.
Quoting from the Speherepak website:

=over 4

SPHEREPACK 3.1 is a collection of FORTRAN programs that facilitates
computer modeling of geophysical processes. The package contains
programs for computing certain common differential operators including
divergence, vorticity, gradients, and the Laplacian of both scalar and
vector functions. Programs are also available for inverting these
operators. For example, given divergence and vorticity, the package
can be used to compute the velocity components. The Laplacian can also
be inverted and therefore the package can be used to solve both the
scalar and vector Poisson equations. Its use in model development is
demonstrated by a sample program that solves the time-dependent
non-linear shallow-water equations. Accurate solutions are obtained
via the spectral method that uses both scalar and vector spherical
harmonic transforms that are available to the user. The package also
contains utility programs for computing the associated Legendre
functions, Gauss points and weights, and multiple fast Fourier
transforms. Programs are provided for both equally-spaced and Gauss
distributed latitudinal points as well as programs that transfer data
between these grids.

=back

The current GrADS extensions only begin to explore the capabilties of
Spherepak. The function B<sh_filt> takes a scalar global 2D field on
the sphere, expands it in terms of spherical harmonics, and
reconstructs it includng only (total) wavenumbers in the range [N1,N2]
specified by the user. Additionally, function B<sh_power> returns the
power spectra in terms of total waenumbers. This is accomplished by
returning a 1D array (fixed longitude, varying latitude) with the
spectra as a function of total wavenumber. The GrADS script
B<power.gs> is useful to plot this power spectra and should be used in
place of the function B<sh_power()>.

This library also provides functions for computation of streamfunction 
and velocity potential from zonal and meridional wind components using 
a spectral method:

   laplacian(psi) = vorticity       (1)
   laplacian(chi) = divergence      (2)

where C<psi> is the streamfunction and C<chi> is the velocity
potential. (See Wikipedia links below for more information on
streamfunction/velocity potential.)  The vorticity and divergence
computation relies on functions B<madvu> and B<madvv> provided in the
OpenGrADS extension I<Libbjt> by B.-J. Tsuang. The Poisson equations
(1)-(2) above are solved using a spectral method in Spherepak. These
functions complement similar functions in the OpenGrADS I<fish> extension
which become numerically unstable for resolutions finer than 1/2 deg. 

=head1 EXAMPLES

=head2 Filtering surface pressure. 

The example expands the surface pressure field in terms of spherical
harmonics and reconstructs it retaining only 10 wavenumber.

 open model
 set gxout shaded
 d sh_filt(ps,10)

=head2 Power spectra of surface temperature

The example expands the surface temperature field in terms of
spherical harmonics and plots it as function of the 32 first total
numbers.

 run power.gs ts 32

=head2 Computing Streamfunction from Vorticity

If you have the relative vorticity field available, say C<vor>, you
can easily compute streamfunction 

   ga-> psi = sh_fish(vor)

=head2 Computing Streamfunction from Wind Components

The first step is to evaluate the streamfunction:

  ga-> set lev 200
  ga-> psi = sh_psi(ugrd,vgrd)

It is often convenient to I<center> the streamfunction by
subtracting its global mean:

  ga-> psi = psi - aave(psi,global)

We can finally display it:

  ga-> set gxout shaded
  ga-> display psi/1e7  
  draw title Streamfunction

Notice that if you would like to compare the results of 
C<sh_psi> and C<fish_psi> you must first remove the global
mean of both.

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

   ga-> chi = sh_fish(div)

=head2 Computing Velocity Potential from Wind Components

Start by computing the velocity potential:

  ga-> set lev 200
  ga-> chi = sh_chi(ugrd,vgrd)

It is often convenient to I<center> the velocity potential by
subtracting its global mean:

  ga-> psi = psi - aave(psi,global)

We can finally display it:

  ga-> set gxout shaded
  ga-> display chi/1e6  
  draw title Velocity

Notice that if you would like to compare the results of 
C<sh_chi> and C<fish_chi> you must first remove the global
mean of both.

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
C<sh_vor> and C<sh_div> uses the advention functions in
I<Libbjt> to numerically evaluate vorticity and divergence

   ga-> vor = sh_vor(ugrd,vgrd)
   ga-> div = sh_div(ugrd,vgrd)

These functions provide a better handling of the boundaries compared
to their intrinsic counterparts. 

=head3 IMPORTANT

You cannot C<hcurl/hdivg> with C<sh_fish>, you must use 
C<sh_vor/sh_div> instead.


=head1 FUNCTIONS PROVIDED

=head2  B<sh_filt>(I<EXPR,N1[,N2]>)

=over 4

This function takes a scalar global 2D field on the sphere, expands it
in terms of spherical harmonics, and reconstructs it includng only
(total) wavenumbers in the range [N1,N2] specified by the user.

=over 8

=item I<EXPR> 

GrADS expressions with scalar expression to be filtered.

=item I<N1[,N2]> 

When both I<N1> and I<N2> are specified, only spherical harmonics with
total wavenumber in the range I<[N1,N2]> will be retained. When only
I<N1> is specified, only spherical harmonics with total wavenumber in
the range I<[1,N1]> will be retained.

=back

=back

=head2  B<sh_power>(I<EXPR>)

=over 4

This function returns the power spectra in terms of total
waenumbers. This is accomplished by returning a 1D array (fixed
longitude, varying latitude) with the spectra as a function of total
wavenumber. The GrADS script B<power.gs> is useful to plot this power
spectra and should be used in place of the function B<sh_power()>.

=back

=head2  B<sh_fish>(I<EXPR>)

=over 4

This function returns the inverse laplacian of I<EXPR> using a spectral method. 
This is particularly useful when the finite difference implementation in B<fish()>
fails for high resolution grids.

=back

=head2  B<sh_psi>(I<UEXPR,VEXPR>) - Computes Stream Function

=over 4

This function computes vorticity as in B<sh_vor> and uses B<sh_fish> to
solve the Poisson equation for the streamfunction C<psi>:

   laplacian(psi) = vorticity

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=back

=back

=head2  B<sh_chi>(I<UEXPR,VEXPR>) - Computes Velocity Potential

=over 4

This function computes divergence as in B<sh_div> and uses B<sh_fish> to
solve the Poisson equation for the velocity potential C<chi>:

   laplacian(chi) = divergence

=over 8

=item I<UEXPR,VEXPR> - required

GrADS expressions with zonal and meridional wind components

=back

=back


=head2 B<sh_vor>(I<UEXPR,VEXPR>) - Computes Relative Vorticity

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


=head2  B<sh_div>(I<UEXPR,VEXPR>) - Computes Divergence 

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

Do not use the GrADS intrinsic functions B<hcurl> and B<hdivg>
with B<sh_fish>. Use the provided functions B<sh_vor> and 
B<sh_div> instead.

=head1 SEE ALSO

=over 4

=item *

L<http://opengrads.org/> - OpenGrADS Home Page

=item *

L<http://cookbooks.opengrads.org/index.php?title=Main_Page> -
OpenGrADS Cookbooks

=item *

L<http://www2.cisl.ucar.edu/resources/legacy/spherepack> - Spherepack Home Page

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

Arlindo da Silva (dasilva@opengrads.org) with contributions from Ben Auer (Poisson Solver.)

=head1 COPYRIGHT

Copyright (C) 2008-2009 Arlindo da Silva; All Rights Reserved.

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#endif
