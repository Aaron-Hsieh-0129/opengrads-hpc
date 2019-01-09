/*

    Copyright (c) 1995-2003 by Mike Fiorino <mfiorino@gmail.com>
    Copyright (c) 2003-2007 by Ben-Jei Tsuang <btsuang@yahoo.com>
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

% perldoc re

Or else, run this file through cpp to extract the POD fragments:

% cpp -DPOD -P < re.c > re.pod

and place re.pod in a place perldoc can find it, like somewhere in your path.
To generate HTML documentation:

% pod2html --header < re.pod > re.html

To generate MediaWiki documentation:

% pod2wiki --style mediawiki < re.pod > re.wiki

If you have "pod2html" and "pod2wini" installed (if not, get them from
CPAN), there are targets in the gex.mk fragment for these:

% make re.html
% make re.wiki

*/

#ifdef POD

=pod

=head1 NAME

re.gex - A 2D regridding function for GrADS

=head1 SYNOPSIS

=head3 Short-hand

=over 2

B<re>(expr,dlon), while dlon=dlat 

B<re>(expr,dlon,dlat,['ig',nyig],['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min]

B<re>(expr,dlon,gYY,['ig',nyig],['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min]

=back

=head3 Linear Y-mapping

=over 2

B<re>(expr,nx,'linear',lon,dlon,ny,'linear',lat,dlat,
['ig',nyig],['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min]

=back

=head3 Gaussian Y-mapping

=over 2

B<re>(expr,nx,'linear',lon,dlon,ny,'gaus',gstart,njog,
['ig',nyig],['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min]

=back

=head1 DESCRIPTION

The regrid function C<re> solves a common problem of transforming
I<horizontal> 2-D gridded fields from/to different resolutions/grid
types for quantitative intercomparison.  For example, a model monthly
mean precipitation field on a T126 gaussian grid can be compared to an
observed climatology on a 2.5x2.5 grid using C<re>. The function C<re>
offers many transform options ranging from simple bilinear
interpolation to box averaging with I<voting.>  Additional methods can
be added to C<re> as needed.

C<re> transforms two-dimensional (2-D) lat/lon GrADS grids from
one grid type/resolution to another.  The input is any 2-D
lat/lon grid defined by the current GrADS lat/lon dimension
environment.  C<re> handles input grids which are cyclically
continuous in longitude and excludes undefined input grid values
from participation in the transform.  If a valid transform cannot
be made (i.e., insufficient defined data points), the output grid
is set to undefined.  C<re> supports two output grid types: 1)
lat/lon with fixed increments; and 2) gaussian.  Four transforms
are available: 1) box averaging for regridding fine to coarse
grids; 2) box averaging with "voting" for noncontinuous/index
data such, as soil type; 3) bilinear interpolation; and 4)
4-point bessel interpolation.

=head1 REQUIRED PARAMETERS

=over 4

=item I<expr>: 

Any valid GrADS grid expression (e.g., z or ave(z.3(t+0,t=120,1yr)),
real number, 'undef' or '-u'.

=item I<nx>: 

The number of points in longitude (integer)

=item I<lon>: 

Beginning longitude (center of the lower left hand corner grid
or the grid (1,1)) of the output domain (float)

=item I<dlon>:

Delta longitude (dlon) or number of gaussian longitudes on the GLOBE (float)

=item I<ny>: 

The number of points in latitude (integer)

=item I<lat>:

Beginning latitude (center of the lower left hand corner grid
or the grid (1,1)) of the output domain (float)

=item I<dlat>: 

Delta latitude (dlat) or the number of gaussian latitudes on the GLOBE (float)

=item I<gstart>:

The first gaussian grid number. If the data span all latitudes, start would 
be 1, indicating the southernmost gaussian grid latitude (integer)

=item I<njog>:	

The number of GLOBAL gaussian latitudes on the output grid. (integer)

=item I<'linear'>:

Linear mapping (string)

=item I<'gaus'>:

Gaussian latitide mapping (string)

=item I<Note>:

All strings are CASE INSENSITIVE.

=back

=head1 OPTIONAL PARAMETERS

=head3 Input Gaussian Grid:

=over 4

=item I<'ig', nyig>:

Input grid is gaussian with I<nyig> being the number of
gaussian latitudes (e.g., ig92 for the NMC T62
grid).  I<nyig> must be >= 8 and a multiple of four.
This parameter is used to invoke a more precise
calculation of the boundary between gaussian grid boxes.

=back

=head3 Interpolation Method:

=over 4

=item I<'ba'>	

Box averaging (the default, while regrids to coarse resoultion)

=item I<'bl'>:

Bi-linear interpolation (the default, when regridding to a finer
resolution)

=item I<'bs'>:

3rd order Bessel interpolation 

=item I<'vt', vtmax, vtmin>:

I<Vote> interpolation or box averaging with voting. The parameters 
I<(vtmax,vtmin)> (range: [0-1]) set the fraction of an output grid box 
that must be covered by defined input grid data for a "winner" to be chosen in
the election.  The default is I<vtmax>=I<vtmin>=1/2.

The parameter I<vtmin> must be the same as I<vtmax> except for three or more
candidates.  The fraction for two candidates is midway between vtmax
and vtmin.

When there is only one candidate, I<vtmax> is the minimum fraction 
of an output grid point hat must be covered by defined input grid 
data for a "winner" to be chosen in the election.  

Specifying I<vtmax> = I<vtmin> = 1 would require that 100% of the output 
grid box must be covered by a single, unique value from the input grid
whereas I<vtmax> = I<vtmin> = 0 would allow a winner to be chosen if ANY
candidates where running.  The default value of 0.5 means that a
simple majority is required to have a winner.

=item I<'ma', fraction>:

This option applies ONLY to box averaging without voting when the 
input grid has undefined points. The parameter I<fraction> (range: [0-1])
specifies the minimum area which must be covered with DEFINED
data boxes to be considered a valid interpolation.  The old
I<regrid> v1.0 assumed I<fraction> was 0 or that if ANY input grid
boxes contained defined data which intersected the output grid
produced a box average.  This was clearly too liberal and I<fraction> is
now set by default to 50% or that half the output grid box must
be covered with defined data to produced a defined output grid
point.

=back

=head1 EXAMPLES

=over 4

=item 1)

Regrid a global T62 gaussian grid (192x94) to a 2.5 deg lat/lon by box
averaging,

   open /reanl1/pilot20/fluxgrb8508.ctl
   set x 1 192
   set y 1 94
   define p25=re(p,144,linear,0,2.5,72,linear,-88.75,2.5,ba)

or 
   set lon 0 360
   set lat -90 90
   define p25=re(p,2.5,2.5,ba) 

or more simply,

   define p25=re(p,2.5)

Note: The lat/lon dimension environment is set using grid
coordinates (x,y) to make the input and output grids global. To
minimize future graphics/analysis calculations with the regridded
field, we use the GrADS define function to store the grid in
memory where it can be referenced as any other GrADS grid.


=item 2) 

Regrid a 4x5 SiB vegetation type to a R15 (48x40) gaussian
grid using box averaging with "voting."  Require that at least
60% of the output grid box must be covered with a single
candidate value from the input grid for an election to occur.
Otherwise the output grid box is set to undefined.  Relax the
one-candidate election requirement to 20% when there are three or
more candidates,

  open /export/sgi18/fiorino/data/sib/sib.param.annual.ctl
  set lon 0 360
  set lat -90 90
  define i21=re(index,48,linear,0,7.5,40,gaus,1,40,vt,0.60,0.20)
  set gxout grfill
  d index
  d i21

B<Note> : During candidate selection, undefined input grid points do
not contribute to the fraction of the output grid box covered
with input grid boxes.  The best way to display index type data
in GrADS is to use the "grid fill" display option (set gxout
grfill).  GrADS will draw the grid box and color it according to
the grid value.

=item 3) 

Regrid 1x1 Aviation run 500 mb z to 2.5x2.5 grid for the region
(-140, 20) to (-40, 70) using bessel interpolation,

  open /export/sgi39/wd22sl/grads/avn/avn.93092800.fcst.ctl
  set lev 500
  set lat -180 180
  set lon -90 90

  d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5,bs)

or

  set lat 20 70
  set lon -140 -40

  d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5,bs)

or

  d re(z,2.5,2.5,bs)


B<Note>: The above three regrid commands produce exactly the same results.
Box averaging would be more appropriate when regridding to
a coarser grid.

=item 4) 

Regrid 1x1 Aviation run 500 mb z to 2.5x2.5 grid using box averaging and
setting the grid to start at a specific lat/lon,

  open /export/sgi39/wd22sl/grads/avn/avn.93092800.fcst.ctl
  set lev 500
  set lat -20 70
  set lon -140 -40
  d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5,ba)
  set lat 30 50
  set lon -50 50
  d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5,ba)

B<Note>: The above two regrids produce DIFFERENT results
since the input domain does not cover the entire output domain. Missing
values will be filled for the uncovered regions.

=back

=head1 RESTRICTIONS

=over 4

=item 1) 

There is no restriction in the dimension of input/output grids while
there is sufficient memory. Note that there was a restriction
of input/output grids of dimension 730x380 (~T225) in version 2.0
and earlier.

=item 2) 

Any valid GrADS grid can be regridded.  However, GrADS (V1.5)
currently only supports lat/lon grids where the mapping between
grid and world coordinates is one dimensional, i.e.,
longitude(i,j)=f(i) vice longitude(i,j)=f(i,j).

=item 3) 

Only two output grid types have been implemented: 1) lat/lon
(dlat does NOT have to equal dlon); and 2) gaussian grids.
Mercator output grids could be added as lon(i,j)=f(i) and
lat(i,j)=f(j) in this projection.

=back

=head1 METHODOLOGY

The first step in the regrid transform is to define a
relationship between the input and output grids within a common
frame of reference or coordinate system.  regrid bases the
inter-grid relationship on "world" coordinates, and the GrADS map
between grid coordinates (i,j) and world coordinates (lon, lat).
As noted above, the world-grid mapping in GrADS is
one-dimensional.  Thus, the world-grid map of an input GrADS grid
takes the form,

   lat(i,j)=f(j) and lon(i,j)=g(i).

By specifying a similar mapping for an output GrADS grid of the
form

   LAT(I,J)=F(J) and LON(I,J)=G(I),

as defined by the input parameters X1, X2 and X3-6, regrid
calculates,

   X(I)=i(G(I)) and Y(J)=j(F(J)),

where i(G(I)) is the location of the output grid with respect to
the input grid dimension i and j(F(J)) for j.

For simplicity, and greater generality, regrid assumes that the
grid point is at the center of a rectangular grid box and maps
the location of the boundaries of the output grid box to that of
the input grid box.  By default the boundaries are assumed to lie
midway between grid points and while this is not strictly true
for a gaussian grid near the poles, it is close nonetheless.  The
boundaries for gaussian grids can be calculated by specifying
ig XXX in options.  The reason why this cannot be automatic is that
GrADS does not directly support gaussian grids (i.e., there is no
ydef gauss 40 option in the data descriptor .ctl file, just
linear and levels).

Given the inter-grid map X(I) and Y(J), regrid uses two basic
methods for doing the transform: 1) box averaging; or 2)
interpolation.  Box averaging is simply the area-weighted
integral of all input grid boxes which intersect an output grid
box, divided by the area of the output grid box.  This approach
is most appropriate: 1) for transforming from fine (e.g., dlon =
1 deg) to coarse grids (e.g., dlon = 2.5 deg); and 2) when
approximate conservation of an integral quantity (e.g., global
average) is desired.

Box averaging is also useful for regridding noncontinuous,
parametric or "index" data.  For example, suppose you have been
given a 0.5x0.5 deg global grid of vegetation type and want to
use these data in an R43 global model.  The intuitive solution is
to assign the output grid the value of the intersecting input
grid box(es) which account(s) for the greatest percentage of the
output grid box surface area.  In the example of vegetation data,
if 70% of the output grid box is covered by deciduous forest,
then it might be reasonable to call the output grid deciduous
forest.  However, if there were 5 distinct vegetation types or
"candidates" available, then regrid, being an American function,
holds an "election" and select a "winner" based on the candidate
covering the greatest percentage of the total surface area in the
output grid box.  Of course, coming from an imperfect democracy,
the election can be "rigged" for a desired effect....

This grid transform strategy is invoked using the "vote" option
in box averaging (vt in C1).  Conditions on the percentage of the
output grid box (number of candidates and what it takes to get
elected) can be finely controlled by the X4 and X5 parameters.

Perhaps the most conventional way of regridding meteorological
data (e.g., 500 mb geopotential heights) is interpolation because
weather data tend to be continuous .  regrid features a 4x4 point
bessel interpolation routine developed at Fleet Numerical
Meteorology and Oceanography Center (courtesy of D. Hensen,
FNMOC).  While this routine is in wide use at FNMOC, the regrid
implementation has been substantially improved to handle more
general input grids.

First, bilinear interpolation is performed at all points to
produce a "first guess."  Improvements to the bilinear 
"first guess" are made using the higher-order terms in the bessel
interpolator, but only if the bessel option is set (i.e., bs in
options).  Second, an undefined value in the 2x2 bilinear stencil
causes the output grid to be undefined.  If the bilinear value is
defined, but any of the points in the larger 4x4 bessel stencil
are undefined, the output grid is assigned the bilinear value.
The third improvement is that the routine handles grids which are
cyclically continuous in longitude.

It is generally recommended that you use the bessel option when
interpolating because the higher-order terms in the polynomial
interpolation function produce a closer fit to the original data
in regions of rapid changes (e.g., large gradients of the
gradient around low pressure centers).

By default, the box averaging is used while the resolution of input
grid is finer than the out grid. Otherwise, the bessel interlopation
is used.

=head1 ADDITIONAL NOTES

=head2 Using regridded fields in other GrADS functions

The only down side to a regridded field is that its dimension
environment cannot be controlled by its "grid" coordinate system.
The best way to understand this is by an example. Suppose you
regrid a T62 global Gaussian grid (192x94) to a uniform 2.5 deg
grid (144x73) using box averaging and the GrADS define
capability, e.g.,

    define p25=re(p,2.5,2.5,ba)

You now want to calculate the global average of the original
field p and the defined regridded field p25.  The only
unambiguous way (using all grid boxes) of doing this calculation
for p would be,

   d aave(p,x=1,x=192,y=1,y=94)

and not,

   d aave(p,lon=0,lon=360,lat=-90,lat=90)

This is because the cyclic continuity feature in GrADS would
count grid boxes at the prime meridian twice, i.e., GrADS would
really be doing,

   d aave(p,x=1,x=193,y=1,y=94)

Trying to find the global average of the 2.5 deg regridded field
p25 using,

   d aave(p25,x=1,x=144,y=1,y=73)

would not yield a global average even though p25 IS 144x73!
However,

   d aave(p25,x=1,x=192,y=1,y=94)

would because GrADS converts the grid coordinate range to
(x=1,x=192) to world coordinates (lon=0,lon=360-1.875) and grabs
all grid boxes in p25 within that range when putting together the
data for the areal averaging calculation.  Despite this
restriction on grid coordinates, you can access specific chunks
of a regridded defined variable using world coordinates.  Suppose
you want to look at the latitudinal variation of the u wind
component at 180 deg and 500 mb on a 1 deg grid, e.g.,

   set lev 500
   set lon 180
   set lat -90 90
   d u

if the you had,

   define u5=regrid2(u,5)

you could then,

   d u5(lon=175)

but not,

   d u5(x=1)
 

=head2 Diagnostic messages from regrid

regrid sends information on the transform process
(starting/ending lat/lon, number of grid points and the
regridding method) to the terminal window where you are running
GrADS.  Additionally, errors during the call to regrid (e.g.,
trying to regrid a two-dimensional longitude-pressure cross
section) will be displayed, the process terminated, and control
returned back to GrADS.

=head1 HISTORY

I<Regrid>, a Grid Analysis and Display System (GrADS) function
developed originally for the Development Division of the former
National Meteorological Center (now NOAA`s National Centers for
Environmental Predection, NCEP), was substantially improved by Mike
Fiorino at the Program for Climate Model Diagnosis and Intercomparison
(PCMDI), Lawrence Livermore National Laboratory, from 1995 to about
2003. Fiorino introduced a simpler calling syntax and made it
available through the Internet as a GrADS User Defined Function (UDF).

Starting in 2003 Ben-Jei Tsuang converted the original I<regrid> from
Fortran to C and implemented it as an intrinsic function in GrADS
v1.8, making his source code patches available to the
community. However, this regrid patch was not adopted by COLA for
GrADS v1.9. The porting was done with the utility C<f2c> to convert
original f77 code to C, and the resulting C code was modified to
eliminate the dependency on the C<f2c> library. Among the improvements
Tsuang made to regrid are:

=over 4 

=item 

Removed the restriction on the sizes of input/output dimensions.

=item 

The function was renamed from C<regrid()> to C<re()>, and the
arguments were reorganized to a syntax that closely followed the
conventions used by the standard GrADS file descriptor. In this new
syntax, the size and the locations of output grids can be exactly
described.

=item 

The function is now aware of the longitudinal cyclic continuity in GrADS 
(where the first and last longitudinal grid point is repeated), and does 
not generate an extra-x column as its predecessor C<regrid>.
This feature is useful when using fwrite to create binary files.

=item 


The output domain can be smaller or larger than the input domain.
If the input domain is smaller than the output domain missing values
will be filled for the uncovered regions.

=item 

Due to the cyclic continuity feature in GrADS, you may not see your 
results correctly on the screen when using C<display>. It is strongly 
suggested to use C<lats4d> or C<fwrite> to write your results to a 
disk file, and then visualize your results afterwards.
For convenience, the function writes out XDEF, YDEF and UNDEF records
which can be cut-and-pasted to create a ctl file when using this 
function in conjunction with C<fwrite> or C<lats4d>.

=back

In 2006 Arlindo da Silva implemented C<re()> as dynamic user defined 
function and it became a poster child for the new User Defined Extensions 
(UDXTs) in GrADS. In 2008 adjustments were made for GrADS v2.

=head1 COPYRIGHT

Copyright (c) 1995-2003 by Mike Fiorino <mfiorino@gmail.com>

Copyright (c) 2003-2007 by Ben-Jei Tsuang <btsuang@yahoo.com>

This is free software; see the source for copying conditions.  There is
NO  warranty;  not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.

=cut

#else

#include <stdlib.h>
#include <math.h>
#include "grads.h"

#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define TRUE_ (1)
#define FALSE_ (0)
#define lDiag FALSE_

#define UNDEF (-9.9e33)

		 static char pout[256];   // Build error msgs here
static int allocateAxisBoundary(int ny, gadouble *yin, int *nyp1, gadouble **yinb);
static int allocateLinearAxis(int ny, gadouble rlatbego, gadouble dyout, gadouble **yout,
			      gadouble **youtb, gadouble **gyout);
static int box_ave(gadouble *fld_in, gadouble *area_in, gadouble *area_out, gadouble *area_min,
		   gadouble *undef, gadouble *gxout, gadouble *gyout,
		   int *nii, int *nji, int *nio, int *njo,
		   gadouble *fld_out, int *iunit_diag, int *vote, int *istat,
		   gadouble *rmin_vote_max, gadouble *rmin_vote_min);
static int bssl_interp(gadouble *fld_in, gadouble *undef,
		       gadouble *xoutb, gadouble *youtb,
		       gadouble *gxout, gadouble *gyout,
		       int *nii, int *nji, int *nio, int *njo,
		       gadouble *fld_out, int *iunit_diag,
		       int *cyclicxi, int *spole_point, int *npole_point, int *bessel,
		       int *istat);
static int bsslzr(double *pbes, int knum);
static int bsslz1(double *bes, int n);
static int fix_poles(gadouble *fld_out, int *nio, int *njo, gadouble *undef,
		     int *spole_point, int *npole_point);
static int gauss_lat_nmc(gadouble *gaul, int k);
static int gauss_lat_pcmdi(gadouble **gaul, gadouble **gaulb, int k);
static int gauaw(double *pa, double *pw, int nj);
static int allocateGaussianLatitude(struct gastat *pst, int njog, gadouble ymin, gadouble ymax, int *nwaves,
				    gadouble *dyout, gadouble **yout, gadouble **youtb, gadouble **gyout, int *ny);
static int allocateGaussianLatitude2(struct gastat *pst, int njog, int glatbego, int ny, int *nwaves,
				     gadouble *dyout, gadouble **yout, gadouble **youtb, gadouble **gyout);
static int help();
static int helpCommand();
static int helpOptions();
static int helpExamples();
static int in_out_boundaries(gadouble *xin, gadouble *yin, gadouble *xout, gadouble *yout, 
			     int cyclicxi, int niip1, int njip1, int niop1,
			     int njop1, gadouble *gxout, gadouble *gyout, gadouble undef, int iLonLat);
static int indexx(int n, gadouble *arrin, int *indx);
static int reallocate(gadouble **a, int *ni, int *nj, int ninew, int njnew);
static int sfc_area(gadouble *fld, gadouble *rlon, gadouble *rlat, gadouble undef, int ni, int nj,
		    gadouble *area, int iLonLat);

static int verbose = 1;   /*ams by default only print error messages and help ams*/

static void gaprnt_ ( int level, char *msg );  /* wrapper to make it shut up sometimes */


int ffre(struct gafunc *pfc, struct gastat *pst) {	
  struct gagrid *inpgr;
  struct gagrid *outpgr;
  struct dt dtim;
  gadouble (*conv) (gadouble *, gadouble);
  gadouble rvals[20],*v;
  char *m; /* undef mask */
  int insiz,outsiz;
  char firstarg[80],xmapping[80],ymapping[80];
  int glatbego;	// starting gaussian number
  char carg[80];
  int jend;
  gadouble xbeg, ybeg;
  gadouble *glat;
  gadouble *glatb;
  gadouble area_min;
  int njog;
  int bVote;
  int iLonLat;	// LatLon Coordinate ?
  gadouble eps_glat__;
  gadouble rlatbego;
  int cyclicxi;
  gadouble rlonbego, rlatendo;
  int niip1, njip1, iregrid_type;
  int i, j, jj;
  int yrbeg;
  int mobeg;
  int dabeg;
  int hrbeg;
  int mnbeg;
  int iarg;
  int istat;
  char regrid_method[40];
  gadouble rmin_vote_min, rmin_vote_max;
  gadouble pi, dxin, dyin;
  gadouble *xin, *xinb;
  gadouble *yin, *yinb;
  gadouble *fld_in;
  gadouble *area_in;
  int bBessel;
  int njglat, jsglat, niifix, iunit_diag;
  int nwaves;
  gadouble deg2rad, rad2deg;
  int wrapxi;
  int jstest;
  int iregrid_method;
  int iout_grid_type;
  gadouble dmn;
  gadouble dmo;
  gadouble *xout, *xoutb;
  gadouble *yout, *youtb;
  gadouble *gxout;
  gadouble *gyout;
  gadouble *fld_out;
  gadouble *area_out;	// was [800][450]
  gadouble dxout, dyout;
  int npole_point, spole_point;
  gadouble value;
  int bValue;		// logical Value mode; 
  int bShorthand;	// logical Shorthand mode;

  // 0. Initialization

  xin=NULL;
  xinb=NULL;
  yin=NULL;
  yinb=NULL;
  fld_in=NULL;
  area_in=NULL;
  xout=NULL;
  xoutb=NULL;
  gxout=NULL;
  yout=NULL;
  youtb=NULL;
  gyout=NULL;
  area_out=NULL;
  fld_out=NULL;
  glat=NULL;
  glatb=NULL;
	
  outpgr = (struct gagrid *)malloc(sizeof(struct gagrid));
  if (outpgr==NULL) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","re");
    gaprnt_(2,pout);
    return 1;
  }
  outpgr->ilinr = 1;
  outpgr->jlinr = 1;
  outpgr->idim = 0;
  outpgr->jdim = 1;
  outpgr->undef = UNDEF;	// default undef
  outpgr->alocf = 0;
  outpgr->iwrld = 0; outpgr->jwrld = 0;
  //		   constants 
  pi = (gadouble)3.141592654;
  deg2rad = pi / (gadouble)180.;
  rad2deg = (gadouble)180. / pi;
  //		   defaults 
  //ams iLonLat = pst->pfid->iLonLat;
  iLonLat = 1;  // Let's assume this to avoid conflict in v1.9
  if (lDiag) {
    sprintf (pout,"OK from 0.0: iLonLat=%d\n",iLonLat);
    gaprnt_(2,pout);
  }
  bValue = FALSE_;		// logical Value mode; 
  bShorthand = FALSE_;		// logical Shorthand mode; 
  iout_grid_type = 1;
  //		   pole points 
  cyclicxi = FALSE_;
  wrapxi = FALSE_;
  npole_point = FALSE_;
  spole_point = FALSE_;
  //		   initialize flag for using calculated gauss lat weights 
  jsglat = -888;
  dxin = -999.0;
  dyin = -999.0;
  //		   the default method is box averaging without voting on 
  //		   a unform grid 
  iregrid_type = 1;
  iregrid_method = 1;
  bBessel = FALSE_;
  bVote = FALSE_;
  //		   minimum fractional area covered by data in box averaging 
  //		   to have a defined point 
  area_min = (gadouble).5;
  //		   voting defaults -- 
  //		   50% of grid box must be covered regardless of the number 
  //		   of candidates 
  rmin_vote_max = (gadouble).5;
  rmin_vote_min = (gadouble).5;
  // Check number of args 
  if (lDiag) {
    sprintf (pout,"OK from 0.1: %d args (should be >= 2)\n", pfc->argnum);
    gaprnt_(2,pout);
  }
  //	nargs = pfc->argnum;
  if (2>pfc->argnum) {
    sprintf (pout,"\nOnly %d args (should be >= 2)\n", pfc->argnum);
    gaprnt_(2,pout);
    help();
    goto err;
  }
  else if (2==pfc->argnum) {bShorthand=TRUE_;}
  else {
    getwrd(xmapping,pfc->argpnt[2],80);
    lowcas(xmapping);
    if (!cmpwrd("linear",xmapping)) { 
      bShorthand=TRUE_;
    }
    else {
      bShorthand=FALSE_;
    }
  }
  //
  iarg=0;
  if (getdbl(pfc->argpnt[iarg],&value)!=NULL) {
    iarg++;
    // constant input field 
    bValue=TRUE_;
  }
  else {
    getwrd(firstarg,pfc->argpnt[iarg],80);
    lowcas(firstarg);
    if (cmpwrd("undef",firstarg)||cmpwrd("-u",firstarg)) { 
      iarg++;
      bValue=TRUE_;
      value=outpgr->undef;
    } else if (!gaexpr(pfc->argpnt[iarg],pst)) {
      iarg++;
      if (pst->type==0) {
	gaprnt_(2,"Error from REGRID: 1st argument must be a grid\n");
	goto err;
      }
      bValue=FALSE_;
      inpgr = pst->result.pgr;
      if (inpgr->idim!=0 || inpgr->jdim!=1) {
	gaprnt_(2,"Error from REGRID: Grid must vary in X, Y\n");
	goto err;
      }
      //		   the first record contains number of args 
      //		   and other parameters for future implementations 
      outpgr->undef = inpgr->undef;
      if (inpgr->idim>-1 && inpgr->ilinr==1) {
	// Linear scaling info 
	if (inpgr->idim==3) {
	  gr2t (inpgr->ivals,inpgr->dimmin[3],&dtim);
	  rvals[9] = -999.0;
	  rvals[10] = -999.0; 			  //mf - make udf return time values as advertized mf
	  yrbeg = dtim.yr;
	  mobeg = dtim.mo;
	  dabeg = dtim.dy;
	  hrbeg = dtim.hr;
	  mnbeg = dtim.mn;
	  dmn = *(inpgr->ivals+6);
	  dmo = *(inpgr->ivals+5);
	} else {
	  conv = inpgr->igrab;
	  xbeg = conv(inpgr->ivals,inpgr->dimmin[inpgr->idim]);
	  dxin = *(inpgr->ivals);
	}
      }
      else if (inpgr->idim>-1) {	 // nonlinear scaling info 
	xbeg = pst->dmin[0];
	dxin = (pst->dmax[0]-pst->dmin[0])/inpgr->isiz;
      }
      if (inpgr->jdim>-1 && inpgr->jlinr==1) {	// linear scaling info
	if (inpgr->jdim==3) {
	  gr2t (inpgr->jvals,inpgr->dimmin[3],&dtim); 	//	mf - bug change inpgr->ivals ot inpgr->jvals mf
	  rvals[9] = -999.0;
	  rvals[10] = -999.0;
	  yrbeg = dtim.yr;
	  mobeg = dtim.mo;
	  dabeg = dtim.dy;
	  hrbeg = dtim.hr;
	  mnbeg = dtim.mn;
	  dmn = *(inpgr->jvals+6);
	  dmo = *(inpgr->jvals+5);
	} else {
	  conv = inpgr->jgrab;
	  ybeg = conv(inpgr->jvals,inpgr->dimmin[inpgr->jdim]);
	  dyin = *(inpgr->jvals);
	}
      }
      else if (inpgr->jdim>-1) {	 // nonlinear scaling info 
	ybeg = pst->dmin[1];
	dyin = (pst->dmax[1]-pst->dmin[1])/inpgr->jsiz;
      }
      //		   grid record #2 the data 
      insiz = inpgr->isiz*inpgr->jsiz;				 // Write header 
      if (lDiag) {sprintf (pout,"OK from 0.15\n");gaprnt_(2,pout);}
      fld_in = (gadouble *)malloc(sizeof(gadouble)*insiz);
      if (fld_in==NULL) {
	sprintf (pout,"Error from %s: Memory Allocation Error\n","fld_in");
	gaprnt_(2,pout);
	goto err;
      }
      if (lDiag) {sprintf (pout,"OK from 0.2\n");gaprnt_(2,pout);}
      area_in = (gadouble *)malloc(sizeof(gadouble)*insiz);
      if (area_in==NULL) {
	sprintf (pout,"Error from %s: Memory Allocation Error\n","area_in");
	gaprnt_(2,pout);
	goto err;
      }
      for (i=0;i<insiz;i++) {
	if ( inpgr->umask[i] ) fld_in[i]=inpgr->grid[i];
	else                   fld_in[i]=inpgr->undef;
      }
      if (lDiag) {sprintf (pout,"OK from 0.3\n");gaprnt_(2,pout);}
      if (inpgr->idim>-1) {				   // write i dim scaling 
	xin = (gadouble *)malloc(sizeof(gadouble)*(inpgr->isiz+1));	// plus 1 to incl. boundary
	v = xin;
	if (inpgr->idim<3) {
	  conv = inpgr->igrab;
	  for (i=inpgr->dimmin[inpgr->idim];i<=inpgr->dimmax[inpgr->idim];i++) {
	    *v = conv(inpgr->ivals,(gadouble)i);
	    v++;
	  }
	} else {
	  for (i=inpgr->dimmin[inpgr->idim];i<=inpgr->dimmax[inpgr->idim];i++) {
	    *v = (gadouble)i;
	    v++;
	  }
	}
	if (lDiag) {sprintf (pout,"OK from 0.31\n");gaprnt_(2,pout);}
	if (!allocateAxisBoundary(inpgr->isiz,xin,&niip1,&xinb)) goto err;
	if (lDiag) {sprintf (pout,"OK from 0.35\n");gaprnt_(2,pout);}
      }
      if (lDiag) {sprintf (pout,"OK from 0.4\n");gaprnt_(2,pout);}
      if (inpgr->jdim>-1) {				 // write j dim scaling 
	yin = (gadouble *)malloc(sizeof(gadouble)*inpgr->jsiz);
	if (NULL==yin) {
	  sprintf (pout,"Error from %s: Memory Allocation Error\n","yin");
	  gaprnt_(2,pout);
	  goto err;
	}
	v = yin;
	if (inpgr->jdim<3) {
	  conv = inpgr->jgrab;
	  for (i=inpgr->dimmin[inpgr->jdim];i<=inpgr->dimmax[inpgr->jdim];i++) {
	    *v = conv(inpgr->jvals,(gadouble)i);
	    v++;
	  }
	} else {
	  for (i=inpgr->dimmin[inpgr->jdim];i<=inpgr->dimmax[inpgr->jdim];i++) {
	    *v = (gadouble)i;
	    v++;
	  }
	}
	if (!allocateAxisBoundary(inpgr->jsiz,yin,&njip1,&yinb)) goto err;
      }
      if (lDiag) {sprintf (pout,"OK from 0.5\n");gaprnt_(2,pout);}
    } else {
      sprintf (pout,"Error from %s: Invalid Argument\n","expr");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be a valid GrADS expression, a real value, -u or 'undef' string.\n",
	       iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
  }
  if (lDiag) {
    sprintf (pout,"OK from 1.0\n");
    gaprnt_(2,pout);
  }	
  // ------------------------------------------------------------ 
  //		   read arguments after the grid -- two floats 
  //		   #1 - dx or # gaussian lats 
  //		   #2 - dy or # gaussian lons 
  //		   if only 1 then dx=dy 
  //		   if only 2 then method = box averaging 
  // ------------------------------------------------------------ 
  //
  // longitude
  //
  if (bShorthand) {
    if (getdbl(pfc->argpnt[iarg],&dxout)==NULL) {
      sprintf (pout,"Error from %s: Invalid Argument\n","dlon");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be a value\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
    iarg++;
    //ams rlonbego=pst->dmin[0]+dxout/2.;
    rlonbego=pst->dmin[0];
    outpgr->isiz=(pst->dmax[0]-pst->dmin[0])/dxout;
  }
  else {
    //	nx
    if (intprs(pfc->argpnt[iarg],&outpgr->isiz)==NULL) {
      sprintf (pout,"Error from %s: Invalid Argument\n","nx");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be an integer\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }	
    iarg++;
    // xmapping
    getwrd(xmapping,pfc->argpnt[iarg],80);
    iarg++;
    lowcas(xmapping);
    if (!cmpwrd("linear",xmapping)) { 
      sprintf (pout,"Error from %s: Invalid mapping\n","lon");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be 'linear' only.\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
    if (lDiag) {
      sprintf (pout,"OK from 1.2\n");
      gaprnt_(2,pout);
    }
    // lon
    if (getdbl(pfc->argpnt[iarg],&rlonbego)==NULL) {
      sprintf (pout,"Error from %s: Invalid Argument\n","lon");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be a value\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
    iarg++;
    if (lDiag) {
      sprintf (pout,"OK from 1.3\n");
      gaprnt_(2,pout);
    }
    // dlon
    if (getdbl(pfc->argpnt[iarg],&dxout)==NULL) {
      sprintf (pout,"Error from %s: Invalid Argument\n","dlon");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be a value\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
    iarg++;
    if (lDiag) {sprintf (pout,"OK from 1.4\n");gaprnt_(2,pout);}
  }
  if (!allocateLinearAxis(outpgr->isiz, rlonbego, dxout, &xout, &xoutb, &gxout)) goto err;
  //
  if (lDiag) {sprintf (pout,"OK from 1.5\n");gaprnt_(2,pout);}
  //
  // latitude
  //
  if (bShorthand) {
    // dlat
    if (2==pfc->argnum) {
      dyout=dxout;
      //ams rlatbego=pst->dmin[1]+dyout/2.;
      rlatbego=pst->dmin[1];
      //ams outpgr->jsiz=(pst->dmax[1]-pst->dmin[1])/dyout;
      outpgr->jsiz=1+(pst->dmax[1]-pst->dmin[1])/dyout;

      if(!allocateLinearAxis(outpgr->jsiz, rlatbego, dyout, &yout, &youtb, &gyout)) goto err;
      if (lDiag) {
	sprintf (pout,"OK from 1.501\n");gaprnt_(2,pout);
	sprintf (pout,"jsiz=%d\n",outpgr->jsiz);gaprnt_(2,pout);
	sprintf (pout,"yout[0]=%g\n",yout[0]);gaprnt_(2,pout);
	sprintf (pout,"yout[ny-1]=%g\n",yout[outpgr->jsiz-1]);gaprnt_(2,pout);
      }
    }
    else if (getdbl(pfc->argpnt[iarg],&dyout)!=NULL) {
      iarg++;
      //ams rlatbego=pst->dmin[1]+dyout/2.;
      rlatbego=pst->dmin[1];
      //ams outpgr->jsiz=(pst->dmax[1]-pst->dmin[1])/dyout;
      outpgr->jsiz=1+(pst->dmax[1]-pst->dmin[1])/dyout;
      if(!allocateLinearAxis(outpgr->jsiz, rlatbego, dyout, &yout, &youtb, &gyout)) goto err;
    }
    else {
      // gXX??
      getwrd(ymapping,pfc->argpnt[iarg],80);
      iarg++;
      lowcas(ymapping);
      if (cmpch("g",ymapping,1)) {
	// not gXX
	sprintf (pout,"  Expecting arg%d: %s to be a value or gnn\n",iarg+1,pfc->argpnt[iarg]);
	gaprnt_(2,pout);
	goto err;
      }
      else {
	if (lDiag) {sprintf (pout,"OK from 1.6\n");gaprnt_(2,pout);}
	// start with g
	//	Gaussian Output Grid 
	//	calculate the gaussian latitudes and 
	//	the grid spacing for the gaussian longitudes 
	outpgr->jlinr = 0;	// nonlinear
	iout_grid_type = 2;	// gaussin latitide
				// gaussian number
	if (intprs(ymapping+1,&njog)==NULL) {
	  sprintf (pout,"Error from %s: Invalid Argument\n","njog");
	  gaprnt_(2,pout);
	  sprintf (pout,"  Expecting arg%d: %s to be an integer\n",iarg,ymapping+1);
	  gaprnt_(2,pout);
	  goto err;
	}
	if (lDiag) {sprintf (pout,"OK from 1.65, njog=%d\n",njog);gaprnt_(2,pout);}
	if(!allocateGaussianLatitude(pst,njog,pst->dmin[1],pst->dmax[1],&nwaves,&dyout,
				     &yout,&youtb,&gyout,&outpgr->jsiz)) goto err;
	if (lDiag) {sprintf (pout,"OK from 1.7\n");gaprnt_(2,pout);}
	rlatbego = yout[0];
	if (lDiag) {
	  sprintf (pout,"OK from 1.8\n");
	  gaprnt_(2,pout);
	}
      }
    }
  }
  else {	
    //	ny
    if (intprs(pfc->argpnt[iarg],&outpgr->jsiz)==NULL) {
      sprintf (pout,"Error from %s: Invalid Argument\n","ny");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be an integer\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }	
    iarg++;
    if (lDiag) {
      sprintf (pout,"OK from 1.51\n");
      gaprnt_(2,pout);
    }
    getwrd(ymapping,pfc->argpnt[iarg],80);
    iarg++;
    lowcas(ymapping);
    if (lDiag) {
      sprintf (pout,"OK from 1.7\n");
      gaprnt_(2,pout);
    }
    if (cmpwrd("gaus",ymapping)) { 
      //		   Gaussian Output Grid 
      //		   calculate the gaussian latitudes and 
      //		   the grid spacing for the gaussian longitudes 
      outpgr->jlinr = 0;
      iout_grid_type = 2;
      // gaussian start number
      if (intprs(pfc->argpnt[iarg],&glatbego)==NULL) {
	sprintf (pout,"Error from %s: Invalid Argument\n","glatbego");
	gaprnt_(2,pout);
	sprintf (pout,"  Expecting arg%d: %s to be an integer\n",iarg+1,pfc->argpnt[iarg]);
	gaprnt_(2,pout);
	goto err;
      }
      iarg++;
      // gaussian number
      if (intprs(pfc->argpnt[iarg],&njog)==NULL) {
	sprintf (pout,"Error from %s: Invalid Argument\n","njog");
	gaprnt_(2,pout);
	sprintf (pout,"  Expecting arg%d: %s to be an integer\n",iarg+1,pfc->argpnt[iarg]);
	gaprnt_(2,pout);
	goto err;
      }
      iarg++;
      if (lDiag) {
	sprintf (pout,"OK from 1.8\n");
	gaprnt_(2,pout);
      }
      if (!allocateGaussianLatitude2(pst,njog,glatbego,outpgr->jsiz,&nwaves,&dyout,
				     &yout,&youtb,&gyout)) goto err;
      rlatbego = yout[0];
      ///
	}
    else if (cmpwrd("linear",ymapping)) { 
      if (lDiag) {
	sprintf (pout,"OK from 1.8\n");
	gaprnt_(2,pout);
      }
      iout_grid_type = 1;
      // lat
      if (getdbl(pfc->argpnt[iarg],&rlatbego)==NULL) {
	sprintf (pout,"Error from %s: Invalid Argument\n","lat");
	gaprnt_(2,pout);
	sprintf (pout,"  Expecting arg%d: %s to be a value\n",iarg+1,pfc->argpnt[iarg]);
	gaprnt_(2,pout);
	goto err;
      }
      iarg++;
      // dlat
      if (getdbl(pfc->argpnt[iarg],&dyout)==NULL) {
	sprintf (pout,"Error from %s: Invalid Argument\n","dlat");
	gaprnt_(2,pout);
	sprintf (pout,"  Expecting arg%d: %s to be a value\n",iarg+1,pfc->argpnt[iarg]);
	gaprnt_(2,pout);
	goto err;
      }
      iarg++;
      if (lDiag) {
	sprintf (pout,"OK from 1.9\n");
	gaprnt_(2,pout);
      }
      if (!allocateLinearAxis(outpgr->jsiz, rlatbego, dyout, &yout, &youtb, &gyout)) goto err;
    }
    else {
      sprintf (pout,"Error from %s: Invalid mapping\n","lat");
      gaprnt_(2,pout);
      sprintf (pout,"  Expecting arg%d: %s to be 'linear' or 'gaus'.\n",iarg+1,pfc->argpnt[iarg]);
      gaprnt_(2,pout);
      goto err;
    }
  }
  outsiz = outpgr->isiz * outpgr->jsiz;
  if (lDiag) {
    sprintf (pout,"size of %d\n",outsiz);
    gaprnt_(2,pout);
    sprintf (pout,"OK from 1.91\n");
    gaprnt_(2,pout);
  }
  fld_out = (gadouble *)malloc(sizeof(gadouble)*outsiz);
  if (lDiag) {
    sprintf (pout,"OK from 1.92\n");
    gaprnt_(2,pout);
  }
  if (NULL==fld_out) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","fld_out");
    gaprnt_(2,pout);
    goto err;
  }
  if (lDiag) {
    sprintf (pout,"OK from 2.0\n");
    gaprnt_(2,pout);
  }
  // initial: by setting undefine
  for (i=0; i<outsiz; i++) {
    if (bValue) {
      fld_out[i]=value;
    }
    else {
      fld_out[i]=outpgr->undef;
    }
  }
  if (lDiag) {
    sprintf (pout,"OK from 2.1\n");
    gaprnt_(2,pout);
  }
  area_out = (gadouble *)malloc(sizeof(gadouble)*outsiz);
  if (NULL==area_out) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","area_out");
    gaprnt_(2,pout);
    goto err;
  }
  // initial: by setting undefine
  for (i=0; i<outsiz; i++) {
    area_out[i]=outpgr->undef;
  }
  // default methods
  if (!bValue) {
    if (dxout*dyout>=dxin*dyin) {
      iregrid_method = 1;		// box averaging
      bVote = FALSE_;
    }
    else {
      iregrid_method = 2;		// bilinear interp
      bBessel = FALSE_;
    }
    if (lDiag) {
      sprintf (pout,"dxout*dyout=%g, dxin*dyin=%g\n",dxout*dyout, dxin*dyin);
      gaprnt_(2,pout);
      sprintf (pout,"OK from 2.2\n");
      gaprnt_(2,pout);
    }
    // options
    // ------------------------------------------------------------ 
    //		   read the 10th argument -- the char option string 
    //		   this specifies: 
    //		   1)  we are telling regrid that the input grid (ig)
    //		   is gaussian and the resolution (number of gaussian latitudes) 
    //		   2)  interp method (ba, bl, bs)
    // ------------------------------------------------------------ 
    //		   number of char options
    while (iarg<pfc->argnum) {
      getwrd(carg,pfc->argpnt[iarg],80);
      iarg++;
      lowcas(carg);
      if (cmpwrd("ig",carg)) {
	eps_glat__ = (gadouble).01;
	if (intprs(pfc->argpnt[iarg],&njglat)==NULL) {
	  sprintf (pout,"Error from %s: Invalid Argument\n","ig");
	  gaprnt_(2,pout);
	  sprintf (pout,"  Expecting arg%d: %s to be an integer\n",
		   iarg+1,pfc->argpnt[iarg]);
	  gaprnt_(2,pout);
	  sprintf (pout,"eg:\nregrid(.,...,ig,94,...)\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"input grid is gaussian with XXX the number of\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"gaussian latitudes (e.g., ig,92 for the NMC T62\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"grid).  XXX must be >= 8 and an even number.\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"This param is used to invoke a more precise\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"calculation of the boundary between gaussian grid boxes\n");
	  gaprnt_(2,pout);
	  goto err;
	}	
	iarg++;
	//		   make sure this IS a valid gaussian grid 
	//		   and can be calculated by the routine 
	if (njglat % 2 != 0 || njglat < 8) {
	  sprintf (pout,"number of input gaussian lats invalid\n(not a factor of 2) bypass precise weighting\n");
	  gaprnt_(2,pout);
	} else {
	  //		   get the latitudes and boundaries using the pcmdi routine 
	  if (!gauss_lat_pcmdi(&glat, &glatb, njglat)) goto err;
	  if (lDiag) {
	    {sprintf(pout,"glat:\n");gaprnt(1,pout);}
	    for (j=0; j<njglat; j++) {
	      sprintf(pout,"%g ",glat[j]);gaprnt(1,pout);
	      if (0==((j+1)%10)) {sprintf(pout,"\n");gaprnt(1,pout);}
	    }
	    sprintf(pout,"\nglatb:\n");gaprnt(1,pout);
	    for (j=0; j<=njglat; j++) {
	      sprintf(pout,"%g ",glatb[j]);gaprnt(1,pout);
	      if (0==((j+1)%10)) {sprintf(pout,"\n");gaprnt(1,pout);}
	    }
	    sprintf(pout,"\nyin:\n");gaprnt(1,pout);
	    for (j=0; j<inpgr->jsiz; j++) {
	      sprintf(pout,"%g ",yin[j]);gaprnt(1,pout);
	      if (0==((j+1)%10)) {sprintf(pout,"\n");gaprnt(1,pout);}
	    }
	    {sprintf(pout,"\n");gaprnt(1,pout);}
	  }
	  //		   now see if we can match the input grid to the assumed 
	  //		   gaussian grid 
	  jstest = 0;
	  while (yin[jstest] < (gadouble)-90.) {
	    jstest++;
	  }
	  for (j = 0; j < njglat; j++) {
	    if (fabs(yin[jstest] - glat[j]) <= eps_glat__) {
	      jsglat = j;
	      sprintf (pout,"input gaussian grid matched! Gaussian boundary is used.\n");
	      gaprnt_(2,pout);
	      break;
	    }
	  }
	  if (-888 == jsglat) {
	    sprintf (pout,"input gaussian grid UNMATCHED! Linear boundary is used.\n");
	    gaprnt_(2,pout);
	  }
	}
      }
      else if (cmpwrd("ma",carg)) {
	//		   
	//***	   min area for defined data
	  //
	  if (getdbl(pfc->argpnt[iarg],&area_min)==NULL) {
	    sprintf (pout,"Error from %s: Invalid Argument\n","ma");
	    gaprnt_(2,pout);
	    sprintf (pout,"  Expecting arg%d: %s to be a value within [0,1]\n",
		     iarg+1,pfc->argpnt[iarg]);
	    gaprnt_(2,pout);
	    sprintf (pout,"eg:\nregrid(.,...,ma,0.5,...)\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"minimum area [0-1] which must be covered with DEFINED\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"data boxes to be considered a valid interpolation.  Applies ONLY\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"to box averaging without voting when the input grid has undefined\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"points.  regrid v1.0 assumed am was 0 or that if ANY input grid\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"boxes contained defined data which intersected the output grid\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"produced a box average.  This was clearly too liberal and ma is\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"now set by default to 50% or that half the output grid box must\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"be covered with defined data to produced a defined output grid\n");
	    gaprnt_(2,pout);
	    sprintf (pout,"point.\n");
	    gaprnt_(2,pout);
	    goto err;
	  }	
	iarg++;
	//		   
	//		   decrease area_min slightly for 100%
	//		   because of numerical differences between
	//		   sum of intersections and the actual sfc area
	//		   of the output grid boxes
	//
	if(0.96<=area_min) area_min=(gadouble)0.96;
	iregrid_method = 1;
	bVote = FALSE_;
      }
      else if (cmpwrd("ba",carg)) { 
	iregrid_method = 1;
	bVote = FALSE_;
      }
      else if (cmpwrd("bl",carg)) { 
	iregrid_method = 2;
	bBessel = FALSE_;
      }
      else if (cmpwrd("bs",carg)) { 
	iregrid_method = 2;
	bBessel = TRUE_;
      }
      else if (cmpwrd("vt",carg)) { 
	iregrid_method = 1;
	bVote = TRUE_;
	// rmin_vote_max
	if (getdbl(pfc->argpnt[iarg],&rmin_vote_max)==NULL) {
	  sprintf (pout,"Error from %s: Invalid Argument\n","rmin_vote_max");
	  gaprnt_(2,pout);
	  sprintf (pout,"  Expecting arg%d: %s to be a value within [0,1]\n",
		   iarg+1,pfc->argpnt[iarg]);
	  gaprnt_(2,pout);
	  sprintf (pout,"eg, re(....,vt,0.6,0.2,...)\tfor Box averaging with voting\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    60% minimum fraction of an output grid point when there is only one candidate[0-1]\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    20% minimum fraction for three or more candidates.  The fraction for two\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    candidates is midway between 0.2 and 0.6.\n");
	  gaprnt_(2,pout);
	  goto err;
	}
	iarg++;
	// rmin_vote_min
	if (getdbl(pfc->argpnt[iarg],&rmin_vote_min)==NULL) {
	  sprintf (pout,"Error from %s: Invalid Argument\n","rmin_vote_min");
	  gaprnt_(2,pout);
	  sprintf (pout,"  Expecting arg%d: %s to be a value within [0,1]\n",
		   iarg+1,pfc->argpnt[iarg]);
	  gaprnt_(2,pout);
	  sprintf (pout,"eg, re(....,vt,0.6,0.2,...)\tfor Box averaging with voting\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    60% minimum fraction of an output grid point when there is only one candidate[0-1]\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    20% minimum fraction for three or more candidates.  The fraction for two\n");
	  gaprnt_(2,pout);
	  sprintf (pout,"    candidates is midway between 0.2 and 0.6.\n");
	  gaprnt_(2,pout);
	  goto err;
	}
	iarg++;
      }
      else if (cmpwrd("ll",carg)) { 
	iLonLat = 1;
      }
      else if (cmpwrd("utm",carg)) { 
	iLonLat = 2;
      }
      else {
	sprintf (pout,"Invalid character option: %s in arg %d\n\n", carg, iarg);
	gaprnt_(2,pout);
	helpOptions();
	goto err;
      }
    }
    if (lDiag) {sprintf(pout,"OK from 3.0\n");gaprnt_(2,pout);}
    if (1==iLonLat) {
      //	lon-lat coordinate ?
      //	bounds check 		
      //ams if (dxout*outpgr->isiz > (gadouble)360.) {
      if (dxout*(outpgr->isiz-1) > (gadouble)360.) {
	sprintf (pout,"bounds check failure: longitude range over 360 deg: %g\n",dxout*outpgr->isiz);
	gaprnt_(2,pout);
	goto err;
      }
      if (lDiag) {sprintf(pout,"OK from 3.02\n");gaprnt_(2,pout);}
      //	bounds check 		
      rlatendo = yout[outpgr->jsiz - 1];
      if (lDiag) {sprintf(pout,"OK from 3.03\n");gaprnt_(2,pout);}
      if ((gadouble)90.<rlatendo||(gadouble)-90.>rlatbego) {
	sprintf (pout,"latbego=%g\n",rlatbego);
	gaprnt_(2,pout);
	sprintf (pout,"latendo=%g\n",rlatendo);
	gaprnt_(2,pout);
	sprintf (pout,"bounds check failure: (latbego, latendo) not within (90S, 90N)\n");
	gaprnt_(2,pout);
	goto err;
      }
      if (lDiag) {sprintf(pout,"OK from 3.04\n");gaprnt_(2,pout);}

/* mf 20090329 -- relax this constraint; handle exceeding poles in sfc_area and bssl_interp
  issue is calculating the interp coefficents and output the pole point if output grid on pole

      for (j = 0; j <=outpgr->jsiz; j++) {
	if (youtb[j] < (gadouble)-90.) {
	  youtb[j] = (gadouble)-90.;
	}
	if (youtb[j] > (gadouble)90.) {
	  youtb[j] = (gadouble)90.;
	}
      }
*/
      if (lDiag) {sprintf(pout,"OK from 3.05\n");gaprnt_(2,pout);}
      // ------------------------------------------------------------ 
      //		   input grid parameters 
      //		   check for cyclic continuity in x 
      // ------------------------------------------------------------ 
      //		   make sure the x dimension is longitude and is uniform 
      if (inpgr->idim == 0 && inpgr->ilinr == 1) {
	if (((inpgr->isiz - 1) * dxin) >= (gadouble)360.) {
	  wrapxi = TRUE_;
	  cyclicxi = TRUE_;
	  niifix = (gadouble)360. / dxin;
	}
	else if ((inpgr->isiz * dxin) == (gadouble)360.) {
	  wrapxi = FALSE_;
	  cyclicxi = TRUE_;
	}
      }
      if (lDiag) {sprintf (pout,"OK from 3.1\n");gaprnt_(2,pout);}
      // ------------------------------------------------------------ 
      //		   if wrapped in x, then trim the grid 
      //		   dump the input data into a dummy array 
      //		   and then load the trimmed grid into the original field array 
      // ------------------------------------------------------------ 
      if (wrapxi) {
	if(!reallocate(&fld_in,&(inpgr->isiz),&(inpgr->jsiz),niifix,
		       inpgr->jsiz)) goto err;
	//		   set the input size to the new trimmed x dimension 
      }
      if (lDiag) {
	sprintf (pout,"OK from 3.2\n");
	gaprnt_(2,pout);
      }
      if (rlatendo == (gadouble)90.) {
	npole_point = TRUE_;
      }
      if (rlatbego == (gadouble)-90.) {
	spole_point = TRUE_;
      }
    }
    // ------------------------------------------------------------ 
    //		   set up the input grid 
    // ------------------------------------------------------------ 
    //		   boundaries of input grid boxes 
    niip1 = inpgr->isiz + 1;
    njip1 = inpgr->jsiz + 1;
    insiz = inpgr->isiz * inpgr->jsiz;
    //		   x input grid box boundaries 
    if (jsglat != -888) {
      //	fixed boundary with the calculated gaussian boundaries 
      jend = min(njip1,njglat+1);
      for (j = jstest; j < jend; j++) {
	jj = jsglat + (j - jstest);
	yinb[j] = glatb[jj];
      }
    }
    if (lDiag) {
      sprintf (pout,"OK from 4.0 grid\n");
      gaprnt_(2,pout);
    }
    // ------------------------------------------------------------ 
    //		   input-output grid box relationship 
    // ------------------------------------------------------------
    //		   make sure longitudes of the input/output grids 
    //		   are within 360 deg
    if (1==iLonLat) {
      while (fabs(xinb[0]-xoutb[0])>=(gadouble)360.) {
	if (xinb[0]-xoutb[0]>=360.) {
	  for (i = 0; i < niip1; ++i) {
	    xinb[i] -= (gadouble)360.;
	  }
	}
	else {
	  for (i = 0; i < niip1; ++i) {
	    xinb[i] += (gadouble)360.;
	  }
	}
      }
    }
    //		   calculate the location of the output grid box boundaries 
    //		   w.r.t. the input grid box boundaries
    if (lDiag) {
      sprintf (pout,"OK from 11.1 arg\n"); gaprnt_(2,pout);
    }
    in_out_boundaries(xinb, yinb, xoutb, youtb, cyclicxi,
		      niip1, njip1, outpgr->isiz+1, outpgr->jsiz+1, gxout, gyout, outpgr->undef, iLonLat);
    if (lDiag) {
      sprintf (pout,"I: gxout=\n"); gaprnt_(2,pout);
      for (j=0; j<=outpgr->isiz; j++) {
	sprintf(pout,"%g ",gxout[j]); gaprnt(2,pout);
	if (0==(j+1)%10) {
	  sprintf(pout,"\n"); gaprnt(2,pout);
	}
      }
      sprintf(pout,"\n"); gaprnt(2,pout);
      sprintf (pout,"I: gyout=\n"); gaprnt_(2,pout);
      for (j=0; j<=outpgr->jsiz; j++) {
	sprintf(pout,"%g ",gyout[j]); gaprnt(2,pout);
	if (0==(j+1)%10) {
	  sprintf(pout,"\n"); gaprnt(2,pout);
	}
      }
      sprintf(pout,"\n"); gaprnt(2,pout);
    }
    if (lDiag) {
      sprintf (pout,"OK from 11.2 arg\n"); gaprnt_(2,pout);
    }
    //		   calculate sfc area of each grid box of input grid
    sfc_area(fld_in, xinb, yinb, outpgr->undef, inpgr->isiz, inpgr->jsiz, area_in,
	     iLonLat);
    if (lDiag) {
      sprintf (pout,"OK from 11.3\n");
      gaprnt_(2,pout);
    }
    sfc_area(fld_out, xoutb, youtb, outpgr->undef, outpgr->isiz, outpgr->jsiz, area_out,
	     iLonLat);
    if (lDiag) {
      sprintf (pout,"OK from 11.4\n");
      gaprnt_(2,pout);
    }
    // ------------------------------------------------------------ 
    //		   do the regrid 
    // ------------------------------------------------------------ 
    //		   box averaging or "clumping" with a "voting" option 
    //		   where the output grid equals the value of the 
    //		   input grid which accounts for the most area.  voting 
    //		   is used for discontinuos data such as soil type 
    if (iregrid_method == 1) {
      box_ave(fld_in, area_in, area_out, &area_min, &outpgr->undef, gxout,
	      gyout, &inpgr->isiz, &inpgr->jsiz, &outpgr->isiz, &outpgr->jsiz, fld_out,
	      &iunit_diag, &bVote, &istat, &rmin_vote_max, &rmin_vote_min);
      //		   FNOC bilinear/bessel interpolation 
      if (lDiag) {
	sprintf (pout,"OK from 11.5 arg\n");
	gaprnt_(2,pout);
      }		
    } else if (iregrid_method == 2) {
      bssl_interp(fld_in, &outpgr->undef, 
		  xoutb, youtb, 
		  gxout, gyout, 
		  &inpgr->isiz, &inpgr->jsiz,
		  &outpgr->isiz, &outpgr->jsiz, fld_out, &iunit_diag, &cyclicxi, &spole_point,
		  &npole_point, &bBessel, &istat);
      if (lDiag) {
	sprintf (pout,"OK from 11.6 arg\n");
	gaprnt_(2,pout);
      }
			
    }
    if (1==iLonLat) {
      //		   check for pole points 
      if (spole_point || npole_point) {
	fix_poles(fld_out, &outpgr->isiz, &outpgr->jsiz, &outpgr->undef, &spole_point, &
		  npole_point);
      }
    }
    if (lDiag) {
      sprintf (pout,"OK from 12 arg\n");
      gaprnt_(2,pout);
    }		
    gafree(pst);	 // free old expr 
    if (lDiag) {
      sprintf (pout,"OK from 13\n");
      gaprnt_(2,pout);
    }
  }
  // ------------------------------------------------------------ 
  //		   write out return info for GrADS 
  // ------------------------------------------------------------ 
  for (i=0; i<4; i++) outpgr->dimmin[i] = 1;
  if ( outpgr->idim<-1 || outpgr->idim>3 ) goto ferr;
  if ( outpgr->jdim<-1 || outpgr->jdim>3 ) goto ferr;
  if ( outpgr->ilinr<0 || outpgr->ilinr>1) goto ferr;
  if ( outpgr->jlinr<0 || outpgr->jlinr>1) goto ferr;
  if ( outpgr->jdim>-1 && outpgr->idim>outpgr->jdim) goto derr;
  if ( outpgr->idim==-1 && outpgr->isiz!=1) goto ferr;
  if ( outpgr->jdim==-1 && outpgr->jsiz!=1) goto ferr;
  if ( outpgr->isiz<1) goto ferr;
  if ( outpgr->jsiz<1) goto ferr;
  for (i=0; i<4; i++) outpgr->dimmin[i] = 1;
  if (outpgr->idim>-1) outpgr->dimmax[outpgr->idim] = outpgr->isiz;
  if (outpgr->jdim>-1) outpgr->dimmax[outpgr->jdim] = outpgr->jsiz;
	
  // Set up linear scaling info
	
  if (outpgr->idim>-1 && outpgr->ilinr==1) {	   /* Linear scaling info */
    if (outpgr->idim==3) {
      v = (gadouble *)malloc(sizeof(gadouble)*8);
      if (v==NULL) goto merr;
      *v = yrbeg;
      *(v+1) = mobeg;
      *(v+2) = dabeg;
      *(v+3) = hrbeg;
      *(v+4) = mnbeg;
      *(v+6) = dmn;
      *(v+5) = dmo;
      *(v+7) = -999.9;
      outpgr->ivals = v;
      outpgr->iavals = v;
    } else {
      v = (gadouble *)malloc(sizeof(gadouble)*6);
      if (v==NULL) goto merr;
      *v = dxout;
      *(v+1) = rlonbego-dxout;
      *(v+2) = -999.9;
      outpgr->ivals = v;
      *(v+3) = 1.0 /dxout;
      *(v+4) = -1.0 * (rlonbego-dxout) / dxout;
      *(v+5) = -999.9;
      outpgr->iavals = v+3;
      outpgr->iabgr = liconv;
      outpgr->igrab = liconv;
    }
  }
  if (outpgr->jdim>-1 && outpgr->jlinr==1) {	   /* Linear scaling info */
    if (outpgr->jdim==3) {
      v = (gadouble *)malloc(sizeof(gadouble)*8);
      if (v==NULL) goto merr;
      *v = yrbeg;
      *(v+1) = mobeg;
      *(v+2) = dabeg;
      *(v+3) = hrbeg;
      *(v+4) = mnbeg;
      *(v+6) = dmn;
      *(v+5) = dmo;
      *(v+7) = -999.9;
      outpgr->jvals = v;
      outpgr->javals = v;
    } else {
      v = (gadouble *)malloc(sizeof(gadouble)*6);
      if (v==NULL) goto merr;
      *v = dyout;
      *(v+1) = rlatbego-dyout;
      *(v+2) = -999.9;
      outpgr->jvals = v;
      *(v+3) = 1.0 / dyout;
      *(v+4) = -1.0 * (rlatbego-dyout) / dyout;
      *(v+5) = -999.9;
      outpgr->javals = v+3;
      outpgr->jabgr = liconv;
      outpgr->jgrab = liconv;
    }
  }
  if (lDiag) {
    sprintf (pout,"OK from 14\n");
    gaprnt_(2,pout);
  }		
	
  /* Read in the data */
  v = (gadouble *)malloc(sizeof(gadouble)*outsiz);
  m = (char *) malloc(sizeof(char)*outsiz);
  if (v==NULL || m==NULL) {
    goto merr;
  }
  for (i=0; i<outsiz; i++) {
    v[i]=fld_out[i];
    if ( v[i] == outpgr->undef ) m[i]=0; // mask for grads v2
    else                         m[i]=1;
  }
  outpgr->grid  = v;
  outpgr->umask = m;

  /* Read in non-linear scaling info, if any */
	
  if (outpgr->idim>-1 && outpgr->ilinr==0) {
    v = (gadouble *)malloc(sizeof(gadouble)*(outpgr->isiz+2));
    if (v==NULL) {
      goto merr;
    }
    *v = outpgr->isiz;
    for (i=0; i<outpgr->isiz; i++) {
      v[1+i]=xout[i];
    }
    *(v+outpgr->isiz+1) = -999.9;
    outpgr->ivals = v;
    outpgr->iavals = v;
    outpgr->iabgr = lev2gr;
    outpgr->igrab = gr2lev;
  }
  if (outpgr->jdim>-1 && outpgr->jlinr==0) {
    v = (gadouble *)malloc(sizeof(gadouble)*(outpgr->jsiz+2));
    if (v==NULL) {
      goto merr;
    }
    *v = outpgr->jsiz;
    for (i=0; i<outpgr->jsiz; i++) {
      v[1+i]=yout[i];
    }
    *(v+outpgr->jsiz+1) = -999.9;
    outpgr->jvals = v;
    outpgr->javals = v;
    outpgr->jabgr = lev2gr;
    outpgr->jgrab = gr2lev;
  }
  // ------------------------------------------------------------ 
  //		   Regrid Successfully 
  // ------------------------------------------------------------ 
  //		   output undef, XDEF, YDEF template to GrADS 
	
  if (iregrid_method == 1 && ! bVote) {
    sprintf (regrid_method, "box averaging                  ");
  }
  if (iregrid_method == 1 && bVote) {
    sprintf (regrid_method, "box averaging with VOTING      ");
  }
  if (iregrid_method == 2 && ! bBessel) {
    sprintf (regrid_method, "bilinear interpolation         ");
  }
  if (iregrid_method == 2 && bBessel) {
    sprintf (regrid_method, "bessel interpolation           ");
  }
  if(1==iout_grid_type) {
    if(1==iLonLat) {
      sprintf (pout,"\nRegrid Successful! The output grid is UNIFORM lat/lon:\n\n");
      gaprnt_(2,pout);
    }
    else if(2==iLonLat) {
#ifdef USEUTM
      sprintf (pout,"\nRegrid Successful! The output grid is UNIFORM UTM:\n\n");
      gaprnt_(2,pout);
      if (cmpwrd("user",pst->pfid->utmZone)) { 
	sprintf (pout,"UTMDEF\t%g %g %g %g %g",pst->pfid->ppvals[4],
		 pst->pfid->ppvals[5],pst->pfid->ppvals[6],pst->pfid->ppvals[7],
		 pst->pfid->ppvals[8]);
	gaprnt_(2,pout);
      } else {
	sprintf (pout,"UTMDEF\t%s",pst->pfid->utmZone);
	gaprnt_(2,pout);
      }
      if (cmpwrd("user",pst->pfid->datum)) { 
	sprintf (pout," %g %g\n",pst->pfid->ppvals[9],pst->pfid->ppvals[10]);
	gaprnt_(2,pout);
      } else {
	sprintf (pout," %s\n",pst->pfid->datum);
	gaprnt_(2,pout);
      }
#else
      sprintf (pout,"UTM support disabled in this version\n");
      gaprnt_(2,pout);
#endif

    } else {
      sprintf (pout,"\nRegrid Successful!\n\n");
      gaprnt_(2,pout);
    }
    sprintf (pout,"UNDEF\t%e\n",outpgr->undef);
    gaprnt_(2,pout);
    sprintf (pout,"XDEF\t%d\tLINEAR\t%g\t%g\n",outpgr->isiz,rlonbego,dxout);
    gaprnt_(2,pout);
    sprintf (pout,"YDEF\t%d\tLINEAR\t%g\t%g\n",outpgr->jsiz,rlatbego,dyout);
    gaprnt_(2,pout);
  }
  else if(2==iout_grid_type) {
    if(1==iLonLat) {
      sprintf (pout,"\nRegrid Successful! The output grid is ~ T%d GAUSSIAN lat/lon:\n\n",nwaves);
      gaprnt_(2,pout);
    }
    else if(2==iLonLat) {
#ifdef USEUTM
      sprintf (pout,"\nRegrid Successful! The output grid is ~ T%d GAUSSIAN UTM:\n\n",nwaves);
      gaprnt_(2,pout);
      if (cmpwrd("user",pst->pfid->utmZone)) { 
	sprintf (pout,"UTMDEF\t%g %g %g %g %g",pst->pfid->ppvals[4],
		 pst->pfid->ppvals[5],pst->pfid->ppvals[6],pst->pfid->ppvals[7],
		 pst->pfid->ppvals[8]);
	gaprnt_(2,pout);
      } else {
	sprintf (pout,"UTMDEF\t%s",pst->pfid->utmZone);
	gaprnt_(2,pout);
      }
      if (cmpwrd("user",pst->pfid->datum)) { 
	sprintf (pout," %g %g\n",pst->pfid->ppvals[9],pst->pfid->ppvals[10]);
	gaprnt_(2,pout);
      } else {
	sprintf (pout," %s\n",pst->pfid->datum);
	gaprnt_(2,pout);
      }
#else
      sprintf (pout,"UTM support disabled in this version\n");
      gaprnt_(2,pout);
#endif
    }
    else {
      sprintf (pout,"\nRegrid Successful! The output grid is ~ T%d GAUSSIAN:\n\n",nwaves);
      gaprnt_(2,pout);
    }
    sprintf (pout,"UNDEF\t%e\n",outpgr->undef);
    gaprnt_(2,pout);
    sprintf (pout,"XDEF\t%d\tLINEAR\t%g\t%g\n",outpgr->isiz,rlonbego,dxout);
    gaprnt_(2,pout);
    sprintf (pout,"YDEF\t%d\tLEVELS\n",outpgr->jsiz);
    gaprnt_(2,pout);
    for (j=0; j<outpgr->jsiz; j++) {
      sprintf(pout,"%g ",yout[j]);
      gaprnt(2,pout);
      if (0==(j+1)%10) {
	sprintf(pout,"\n");
	gaprnt(2,pout);
      }
    }
    sprintf(pout,"\n");
    gaprnt(2,pout);
  }
  sprintf (pout,"regrid method is: %s\n",regrid_method);
  gaprnt_(2,pout);
  if (bVote) {
    sprintf (pout,"vote parameters:  max fract area = %g, min frac area = %g\n",
	     rmin_vote_max,rmin_vote_min);
    gaprnt_(2,pout);
  }
  sprintf (pout,"\n");gaprnt_(2,pout);
  // ------------------------------------------------------------ 
  //		   We are done.  Return. 
  // ------------------------------------------------------------ 
  pst->result.pgr = outpgr;	// connet to new expr
  pst->type = 1;
  if (lDiag) {
    sprintf (pout,"OK from 15\n");
    gaprnt_(2,pout);
  }		
  if (lDiag) {
    sprintf (pout,"NULL=%x\n",NULL);gaprnt_(2,pout);
    sprintf (pout,"xin=%x, xinb=%x\n",xin,xinb);gaprnt_(2,pout);
    sprintf (pout,"yin=%x, yinb=%x\n",yin,yinb);gaprnt_(2,pout);
    sprintf (pout,"fld_in=%x, area_in=%x\n",fld_in,area_in);gaprnt_(2,pout);
    sprintf (pout,"xout=%x, xoutb=%x, gxout=%x\n",xout,xoutb,gxout);gaprnt_(2,pout);
    sprintf (pout,"yout=%x, youtb=%x, gyout=%x\n",yout,youtb,gyout);gaprnt_(2,pout);
    sprintf (pout,"fld_out=%x, area_out=%x\n",fld_out,area_out);gaprnt_(2,pout);
    sprintf (pout,"glat=%x, glatb=%x\n",glat,glatb);gaprnt_(2,pout);
  }		
  if (NULL!=glat) free (glat);
  if (NULL!=glatb) free (glatb);
  if (NULL!=area_out) free (area_out);
  if (NULL!=fld_out) free (fld_out);
  if (NULL!=gyout) free (gyout);
  if (NULL!=yout) free (yout);
  if (NULL!=youtb) free (youtb);
  if (NULL!=gxout) free (gxout);
  if (NULL!=xout) free (xout);
  if (NULL!=xoutb) free (xoutb);
  if (NULL!=yinb) free (yinb);
  if (NULL!=yin) free (yin);
  if (NULL!=xinb) free (xinb);
  if (NULL!=xin) free (xin);
  if (NULL!=area_in) free (area_in);
  if (NULL!=fld_in) free (fld_in);
  gaprnt_(2,"re: all done!\n");
  return (0); 	
 err:
  if (lDiag) {
    sprintf (pout,"NULL=%x\n",NULL);gaprnt_(0,pout);
    sprintf (pout,"xin=%x, xinb=%x\n",xin,xinb);gaprnt_(0,pout);
    sprintf (pout,"yin=%x, yinb=%x\n",yin,yinb);gaprnt_(0,pout);
    sprintf (pout,"fld_in=%x, area_in=%x\n",fld_in,area_in);gaprnt_(0,pout);
    sprintf (pout,"xout=%x, xoutb=%x, gxout=%x\n",xout,xoutb,gxout);gaprnt_(0,pout);
    sprintf (pout,"yout=%x, youtb=%x, gyout=%x\n",yout,youtb,gyout);gaprnt_(0,pout);
    sprintf (pout,"fld_out=%x, area_out=%x\n",fld_out,area_out);gaprnt_(0,pout);
    sprintf (pout,"glat=%x, glatb=%x\n",glat,glatb);gaprnt_(0,pout);
  }	
  if (NULL!=glat) free (glat);
  if (NULL!=glatb) free (glatb);
  if (NULL!=area_out) free (area_out);
  if (NULL!=fld_out) free (fld_out);
  if (NULL!=gyout) free (gyout);
  if (NULL!=yout) free (yout);
  if (NULL!=youtb) free (youtb);
  if (NULL!=gxout) free (gxout);
  if (NULL!=xout) free (xout);
  if (NULL!=xoutb) free (xoutb);
  if (NULL!=yinb) free (yinb);
  if (NULL!=yin) free (yin);
  if (NULL!=xinb) free (xinb);
  if (NULL!=xin) free (xin);
  if (NULL!=area_in) free (area_in);
  if (NULL!=fld_in) free (fld_in);
  free (outpgr);
  //	gagfre (outpgr);
  gafree (pst);
  return (1); 
  //		   error conditions 
 merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n","re");
  gaprnt_(0,pout);
  goto err;
 ferr:
  sprintf (pout,"Error from %s: Invalid transfer file format\n","re");
  gaprnt_(0,pout);
  sprintf (pout,"  File name: %s\n","re");
  gaprnt_(0,pout);
  goto err;
	
 derr:
  sprintf (pout,"Error from %s: Invalid dimension environment ","re");
  gaprnt_(0,pout);
  gaprnt_(0,"in result grid\n");
  goto err;
}
static int allocateAxisBoundary(int ny, gadouble *yin, int *nyp1, gadouble **yinb) {
  //		   y input grid box boundaries 
  //		   use the input lat -> j map for the boundaries 
  int j;
  *nyp1=ny+1;
  if (lDiag) {sprintf (pout,"OK from 0.32\n");gaprnt_(2,pout);}
  *yinb = (gadouble *)malloc(sizeof(gadouble)*(*nyp1));	// plus 1 to incl. boundary
  if (lDiag) {sprintf (pout,"OK from 0.33\n");gaprnt_(2,pout);}
  if (NULL==(*yinb)) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","y-yinb");
    gaprnt_(2,pout);
    sprintf (pout,"with size: %d bytes\n",sizeof(gadouble)*(*nyp1));
    gaprnt_(2,pout);
    goto err;
  }
  if (lDiag) {sprintf (pout,"OK from 0.34\n");gaprnt_(2,pout);}
  for (j=1; j<ny; j++) {
    (*yinb)[j] = (yin[j-1] + yin[j]) * (gadouble).5;
  }
  if (lDiag) {sprintf (pout,"OK from 0.35\n");gaprnt_(2,pout);}
  (*yinb)[0] = yin[0] - (yin[1] - yin[0]) * (gadouble).5;
  (*yinb)[ny] = yin[ny-1] + (yin[ny-1] - yin[ny-2]) * (gadouble).5;
  return TRUE_;
 err:
  return FALSE_;
}
static int allocateLinearAxis(int ny, gadouble rlatbego, gadouble dyout, gadouble **yout,
			      gadouble **youtb, gadouble **gyout) {
  int j;
  if (lDiag) {sprintf (pout,"OK from allocateLinearAxis 1.0\n");gaprnt_(2,pout);}
  //		 error if less than 2 grid 
  if (ny < 2) {
    sprintf (pout,"at least 2 grid after regrid.\n");
    gaprnt_(2,pout);
    return FALSE_;
  }
  //		   boundaries of the output boxes 
  *yout = (gadouble *)malloc(sizeof(gadouble)*ny);	// plus 1 to incl. boundary
  *youtb = (gadouble *)malloc(sizeof(gadouble)*(ny+1));	// plus 1 to incl. boundary
  *gyout = (gadouble *)malloc(sizeof(gadouble)*(ny+1));	// plus 1 to incl. boundary
  if (NULL==(*yout)||NULL==(*youtb)||NULL==(*gyout)) {
    goto merr;
  }
  if (lDiag) {sprintf (pout,"OK from allocateLinearAxis 2.0\n");gaprnt_(2,pout);}
  for (j=0; j<ny; j++) {
    (*yout)[j]  = rlatbego + j * dyout;
  }
  for (j = 0; j<=ny; j++) {
    (*youtb)[j] = rlatbego + j * dyout - dyout * (gadouble).5;
  }
  return TRUE_;
 merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n","allocateLinearAxis");
  gaprnt_(0,pout);
  if (NULL!=(*yout)) free (*yout);
  if (NULL!=(*youtb)) free (*youtb);
  if (NULL!=(*gyout)) free (*gyout);
  return FALSE_;
}
static int help() {
  helpCommand();
  helpOptions();
  helpExamples();
  return 0;	
}
static int helpCommand() {
  sprintf (pout,"\nUsages (re):\n\n"); gaprnt_(1,pout);
  sprintf (pout,"\tre(expr,dlon), while dlon=dlat\n\n"); gaprnt_(1,pout);
  sprintf (pout,"\tre(expr,dlon,dlat,['ig',nyig],['ba'|'bl'|'bs'|'vt',vtmax,vtmin|\n\t\t'ma',min])\n\n");gaprnt_(1,pout);
  sprintf (pout,"\tre(expr,nx,'linear',lon,dlon,ny,'linear',lat,dlat,['ig',nyig],\n\t\t['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min])\n\n"); gaprnt_(1,pout);
  sprintf (pout,"\tre(expr,nx,'linear',lon,dlon,ny,'gaus',start,njog,['ig',nyig],\n\t\t['ba'|'bl'|'bs'|'vt',vtmax,vtmin|'ma',min])\n\n");	gaprnt_(1,pout);
  sprintf (pout,"where expr can be GrADS expression, value or undef, such as z.1(t=2),\n\t\t4.3, -u, 'undef'.\n\n"); gaprnt_(1,pout);
  return 0;	
}
static int helpOptions() {
  sprintf (pout,"\nValid options include:\n\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,ig,94,...)\tfor input grid with 94 levels of\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tGLOBAL gaussian latitudes such as NCEP realaysis data.\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tinput grid is gaussian with XXX the number of\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tgaussian latitudes (e.g., ig,92 for the NMC T62\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tgrid).  XXX must be >= 8 and an even number.\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tThis param is used to invoke a more precise\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tcalculation of the boundary between gaussian grid boxes\n");
  gaprnt_(1,pout);
  sprintf (pout,"\nOr one of the followings:\n\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,ba,...)\tfor box averaging (default, while regrid\n");
  gaprnt_(1,pout);
  sprintf (pout,"        to coarser grid)\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,ma,0.5,...)\tfor at least 50%% of minimum area coverage\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tminimum area [0-1] which must be covered with DEFINED\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tdata boxes to be considered a valid interpolation.  Applies ONLY\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tto box averaging without voting when the input grid has undefined\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tpoints.  regrid v1.0 assumed am was 0 or that if ANY input grid\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tboxes contained defined data which the output grid\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tproduced a box average.  This was clearly too liberal and ma is\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tnow set by default to 50%% or that half the output grid box must\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tbe covered with defined data to produced a defined output grid\n");
  gaprnt_(1,pout);
  sprintf (pout,"\tpoint.\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,vt,0.6,0.2,...)\tfor Box averaging with voting\n");
  gaprnt_(1,pout);
  sprintf (pout,"\t0.6: 60%% minimum fraction of an output grid point when there is\n");
  gaprnt_(1,pout);
  sprintf (pout,"\t     only one candidate[0-1]\n");
  gaprnt_(1,pout);
  sprintf (pout,"\t0.2: 20%% minimum fraction for three or more candidates. The\n");
  gaprnt_(1,pout);
  sprintf (pout,"\t      fraction for two candidates is midway between 0.2 and 0.6.\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,bl,...)\tfor bi-linear interpolation (default, while regrid\n");
  gaprnt_(1,pout);
  sprintf (pout,"        to finer grid)\n");
  gaprnt_(1,pout);
  sprintf (pout,"re(....,bs,...)\tfor 3rd order Bessel interpolation\n\n");
  gaprnt_(1,pout);
  return 0;
}

static int helpExamples() {
  {sprintf (pout,"Examples:\n\n"); gaprnt_(1,pout);}
  {sprintf (pout,"set lat -90 90\nset lon 0 360\n");gaprnt_(1,pout);}   
  {sprintf (pout,"define p25=re(p,144,linear,1.25,2.5,72,linear,-88.75,2.5,ba)\n");gaprnt_(1,pout);}
  {sprintf (pout,"define p25=re(p,2.5,2.5,ba)\n");gaprnt_(1,pout);}
  {sprintf (pout,"define p25=re(p,2.5)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"Note: The above three regrid commands produce exactly the same results.\n\n");gaprnt_(1,pout);}
  {sprintf (pout,"set lat -90 90\nset lon -180 180\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"set lat 20 70\nset lon -140 -40\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(z,40,linear,-138.75,2.5,20,linear,21.25,2.5)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(z,2.5,2.5)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"Note: The above three regrid commands produce exactly the same results.\n\n");gaprnt_(1,pout);}
  {sprintf (pout,"d re(index,48,linear,0,7.5,40,gaus,1,40,vt,0.60,0.20)\n\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(index,48,linear,0,7.5,40,gaus,1,40,ma,0.5)\n\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(tmpsfc,144,linear,1.25,2.5,40,gaus,1,40,ig,94,bs)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"d re(tmpsfc,144,linear,1.25,2.5,40,gaus,1,40,bs)\n");gaprnt_(1,pout);}   
  {sprintf (pout,"Note: The first command is better since it prvovides input gaussian latitude.\n\n");gaprnt_(1,pout);}
  {sprintf (pout,"d re(tmpsfc,50,linear,1.25,2.5,20,gaus,5,40,ig,94)\n\n");gaprnt_(1,pout);}   
  return 0;
}
static int	trimVector(gadouble **yout, int ny) {
  // trim the memory of yout to a size ny
  int j;
  gadouble *dumx;
  dumx = (gadouble *)malloc(sizeof(gadouble)*ny);
  if (dumx==NULL) goto merr;
  for (j=0; j<ny; j++) {
    dumx[j] = (*yout)[j];
  }
  // swap
  free (*yout);
  *yout=dumx;
  return (0);
 merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n","trimVector(dumx)");
  gaprnt_(0,pout);
  return 1;
}
static int allocateGaussianLatitude(struct gastat *pst, int njog, gadouble ymin, gadouble ymax, int *nwaves,
				    gadouble *dyout, gadouble **yout, gadouble **youtb, gadouble **gyout,
				    int *ny) {
  gadouble *dumx, *dumxb;
  int j;
  gadouble xxx;	// dummy variable
  *yout = (gadouble *)malloc(sizeof(gadouble)*njog);
  *youtb = (gadouble *)malloc(sizeof(gadouble)*(njog+1));	// plus 1 incl. boundary
  *gyout = (gadouble *)malloc(sizeof(gadouble)*(njog+1));	// plus 1 incl. boundary
  if (NULL==(*yout)||NULL==(*youtb)||NULL==(*gyout)) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","allocateGaussianLatitude");
    gaprnt_(2,pout);
    goto err;
  }
  if (lDiag) {sprintf (pout,"OK from allocateGaussianLatitude 1.0\n");gaprnt_(2,pout);}
  *nwaves = njog / 3 - 1;
  *dyout = (gadouble)180. / (gadouble) njog;
  if (!gauss_lat_pcmdi(&dumx, &dumxb, njog)) goto err;

#ifdef USEUTM
  if (2==pst->pfid->iLonLat) {
    // convert to UTM coordinate
    for (j=0; j<njog; j++) {
      ll2utm(pst->pfid->ppvals,pst->pfid->ppvals[4],dumx[j],&xxx,&dumx[j]);
    }
    for (j=0; j<=njog; j++) {
      ll2utm(pst->pfid->ppvals,pst->pfid->ppvals[4],dumxb[j],&xxx,&dumxb[j]);
    }
  }
#endif

  if (lDiag) {sprintf (pout,"OK from allocateGaussianLatitude 2.0\n");gaprnt_(2,pout);}
  for (j=0,(*ny)=0; j<njog; j++) {
    if (dumx[j]>=ymin&&dumx[j]<=ymax) {
      (*yout)[*ny] = dumx[j];
      (*youtb)[*ny] = dumxb[j];
      (*ny)++;
    }			
  }
  if (lDiag) {sprintf (pout,"OK from allocateGaussianLatitude 3.0\n");gaprnt_(2,pout);}
  (*youtb)[0]=ymin;
  (*youtb)[*ny]=ymax;
  if (lDiag) {sprintf (pout,"OK from allocateGaussianLatitude 4.0\n");gaprnt_(2,pout);}
  trimVector(yout,*ny);
  trimVector(youtb,(*ny)+1);
  trimVector(gyout,(*ny)+1);
  if (lDiag) {sprintf (pout,"OK from allocateGaussianLatitude 5.0\n");gaprnt_(2,pout);}
  free(dumxb);
  free(dumx);
  return TRUE_;
 err:
  if (NULL!=(*yout)) free (*yout);
  if (NULL!=(*youtb)) free (*youtb);
  if (NULL!=(*gyout)) free (*gyout);
  return FALSE_;
}

static int allocateGaussianLatitude2(struct gastat *pst, int njog, int glatbego, int ny, int *nwaves,
				     gadouble *dyout, gadouble **yout, gadouble **youtb, gadouble **gyout) {
  gadouble *dumx;
  gadouble *dumxb;
  gadouble xxx;	// dummy variable
  int j, jj;
  //		   limits of the input grid (handles gaussian --> gaussian) 
  //		   bounds check 		
  if (glatbego-1+ny > njog) {
    sprintf (pout,"bounds check failure: gaussian latendo over 90N\n");
    gaprnt_(2,pout);
    sprintf (pout,"glatbego-1+nj(%g)>njog(%g)\n",glatbego-1+ny,njog);
    gaprnt_(2,pout);
    return FALSE_;
  }
  *yout = (gadouble *)malloc(sizeof(gadouble)*ny);
  *youtb = (gadouble *)malloc(sizeof(gadouble)*(ny+1));	// plus 1 incl. boundary
  *gyout = (gadouble *)malloc(sizeof(gadouble)*(ny+1));	// plus 1 incl. boundary
  if (NULL==(*yout)||NULL==(*youtb)||NULL==(*gyout)) {
    sprintf (pout,"Error from %s: Memory Allocation Error\n","allocateGaussianLatitude2");
    gaprnt_(2,pout);
    goto err;
  }
  *nwaves = njog / 3 - 1;
  *dyout = (gadouble)180. / (gadouble) njog;
  //		dyoutg = dyout;
  if(!gauss_lat_pcmdi(&dumx, &dumxb, njog)) goto err;
#ifdef USEUTM
  if (2==pst->pfid->iLonLat) {
    // convert to UTM coordinate
    for (j=0; j<njog; j++) {
      ll2utm(pst->pfid->ppvals,pst->pfid->ppvals[4],dumx[j],&xxx,&dumx[j]);
    }
    for (j=0; j<=njog; j++) {
      ll2utm(pst->pfid->ppvals,pst->pfid->ppvals[4],dumxb[j],&xxx,&dumxb[j]);
    }
  }
#endif
  if (lDiag) {
    sprintf (pout,"OK from 1.8\n");
    gaprnt_(2,pout);
  }
  for (j=glatbego-1,jj=0; j<glatbego-1+ny; j++, jj++) {
    (*yout)[jj] = dumx[j];
    (*youtb)[jj] = dumxb[j];
  }
  (*youtb)[jj] = dumxb[j];
  free(dumx);
  free(dumxb);
  return TRUE_;
 err:
  if (NULL!=(*yout)) free (*yout);
  if (NULL!=(*youtb)) free (*youtb);
  if (NULL!=(*gyout)) free (*gyout);
  return FALSE_;
}
static int box_ave(gadouble *fld_in, gadouble *area_in, gadouble *area_out, gadouble *area_min,
		   gadouble *undef, gadouble *gxout, gadouble *gyout,
		   int *nii, int *nji, int *nio, int *njo,
		   gadouble *fld_out, int *iunit_diag, int *vote, int *istat,
		   gadouble *rmin_vote_max, gadouble *rmin_vote_min) {
  // System generated locals 
  int fld_in_dim1, fld_in_offset, area_in_dim1, area_in_offset, 
    fld_out_dim1, fld_out_offset, area_out_dim1, area_out_offset, 
    i__1, i__2, i__3, i__4;
	
  // Local variables 
/// #define MAXGRID 6144
  gadouble *fld_cand__, *area_box__, *dxdy_box__,*area_cand__,*dxdy_cand__,*fld_box__;
  int *ifld_rank__;
  int maxgrid;


  gadouble area_max__;
  int icnt;
  gadouble tot_area__;
  int i, j;
  int ncand, iamax;
  int i1, i2;
  gadouble dxout, dyout, x0, rmin_vote__;
  int ib, jb, ie, je, ii, jj;
  gadouble dx, dy;
  int ii0, it1, it2, ibb, jbb, iee, jee;
  gadouble rmin_dxdy_vote__[3], eps, tot_fld__;
  int cyclicx;
  // return 0;
	
  // Parameter adjustments 
  area_in_dim1 = *nii;
  area_in_offset = area_in_dim1 + 1;
  area_in -= area_in_offset;
  fld_in_dim1 = *nii;
  fld_in_offset = fld_in_dim1 + 1;
  fld_in -= fld_in_offset;
  --gxout;
  fld_out_dim1 = *nio;
  fld_out_offset = fld_out_dim1 + 1;
  fld_out -= fld_out_offset;
  --gyout;
  area_out_dim1 = *nio;
  area_out_offset = area_out_dim1 + 1;
  area_out -= area_out_offset;
	
  // Function Body 
  *istat = 1;
	
  //		   tolerance for checking whether grid is undefined (zero sfc area) 
	
  eps = (gadouble)1e-12;
	
  //		   minimum fractional area of the output grid box 
  //		   in order to have a winner in the voting 
	
  rmin_dxdy_vote__[0] = *rmin_vote_max;
  rmin_dxdy_vote__[1] = (*rmin_vote_max + *rmin_vote_min) * (gadouble).5;
  rmin_dxdy_vote__[2] = *rmin_vote_min;
  ibb = 1;
  iee = *nio;
  jbb = 1;
  jee = *njo;
  i__1 = jee;
  for (j = jbb; j <= i__1; j++) {
    i__2 = iee;
    for (i = ibb; i <= i__2; ++i) {
      //		   check for undefined regions			
      if ( (gxout[i]==gxout[i+1])||(gyout[j]==gyout[j+1]) ){
	fld_out[i + j * fld_out_dim1] = *undef;
	continue;
      }	
      ib = (int) gxout[i] + 1;
      ie = (int) gxout[i + 1] + 1;
      jb = (int) gyout[j] + 1;
      je = (int) gyout[j + 1] + 1;
      //		   cyclic continuity in x 
      //		   check for exceeding n pole 
			
      if (je > *nji) {
	je = *nji;
      }
      //		   cyclic continuity in x		
      cyclicx = FALSE_;
      if (ie < ib) {
	ie += *nii;
	cyclicx = TRUE_;
      }
      //		   initialize the counter for intersecting grid boxes 
      icnt = 0;
      maxgrid=(ie-ib+1)*(je-jb+1);
      fld_cand__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      area_box__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      dxdy_box__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      area_cand__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      dxdy_cand__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      fld_box__ = (gadouble *)malloc(sizeof(gadouble)*maxgrid);
      ifld_rank__ = (int *)malloc(sizeof(int)*maxgrid);
      if (NULL==(fld_cand__)||NULL==(area_box__)||NULL==(dxdy_box__)||NULL==(area_cand__)||NULL==(dxdy_cand__)||NULL==(fld_box__)
        ||NULL==(ifld_rank__)) {
        sprintf (pout,"Error from %s: Memory Allocation Error\n","box_ave");
        gaprnt_(2,pout);
        goto err;
      }      
      //		   CASE 1:	only one input grid box in output grid box 
			
      if (ib == ie && jb == je) {
	icnt = 1;
	dxdy_box__[icnt - 1] = (gadouble)1.;
	area_box__[icnt - 1] = area_in[ib + jb * area_in_dim1];
	if (area_box__[icnt - 1] == (gadouble)0.) {
	  dxdy_box__[icnt - 1] = (gadouble)0.;
	}
	fld_box__[icnt - 1] = fld_in[ib + jb * fld_in_dim1];
      } else if (ib == ie) {
	//		   CASE 2:	intersecting boxes in y only 
	ii = ib;
	dx = gxout[i + 1] - gxout[i];
	dxout = (gadouble)1.;
	i__3 = je;
	for (jj = jb; jj <= i__3; ++jj) {
	  ++icnt;
	  if (icnt > maxgrid) {
	    *istat = 0;
	    goto err;
	  }
	  if (jj == jb) {
	    dy = (gadouble) jj - gyout[j];
	  } else if (jj == je) {
	    dy = gyout[j + 1] - (gadouble) (jj - 1);
	  } else {
	    dy = (gadouble)1.;
	  }
	  if (jj == jb || jj == je) {
	    dyout = dy / (gyout[j + 1] - gyout[j]);
	  } else {
	    dyout = (gadouble)1.;
	  }
	  dxdy_box__[icnt - 1] = dxout * dyout;
	  area_box__[icnt - 1] = dx * dy * area_in[ii + jj * 
						   area_in_dim1];
	  if (area_in[ii + jj * area_in_dim1] == (gadouble)0.) {
	    dxdy_box__[icnt - 1] = (gadouble)0.;
	  }
	  fld_box__[icnt - 1] = fld_in[ii + jj * fld_in_dim1];
	}
      } else if (jb == je) {
	//		   CASE 3:	intersecting boxes in x only 
	jj = jb;
	dy = gyout[j + 1] - gyout[j];
	dyout = (gadouble)1.;
	i__3 = ie;
	for (ii = ib; ii <= i__3; ++ii) {
	  ++icnt;
	  if (icnt > maxgrid) {
	    *istat = 0;
	    goto err;
	  }
	  ii0 = ii;
	  if (cyclicx && ii0 > *nii) {
	    ii0 = ii - *nii;
	  }
	  if (ii == ib) {
	    dx = (gadouble) ii - gxout[i];
	  } else if (ii == ie) {
	    x0 = (gadouble) (ii - 1);
	    if (cyclicx) {
	      x0 = (gadouble) ii0 - (gadouble)1.;
	    }
	    dx = gxout[i + 1] - x0;
	  } else {
	    dx = (gadouble)1.;
	  }
	  if (ii == ib || ii == ie) {
	    dxout = dx / (gxout[i + 1] - gxout[i]);
	  } else {
	    dxout = (gadouble)1.;
	  }
	  dxdy_box__[icnt - 1] = dxout * dyout;
	  area_box__[icnt - 1] = dx * dy * area_in[ii0 + jj * 
						   area_in_dim1];
	  if (area_in[ii0 + jj * area_in_dim1] == (gadouble)0.) {
	    dxdy_box__[icnt - 1] = (gadouble)0.;
	  }
	  fld_box__[icnt - 1] = fld_in[ii0 + jj * fld_in_dim1];
	}
      } else {
	//		   CASE 4:	intersecting boxes in both directions 
	i__3 = je;
	for (jj = jb; jj <= i__3; ++jj) {
	  if (jj == jb) {
	    dy = (gadouble) jj - gyout[j];
	  } else if (jj == je) {
	    dy = gyout[j + 1] - (gadouble) (jj - 1);
	  } else {
	    dy = (gadouble)1.;
	  }
	  if (jj == jb || jj == je) {
	    dyout = dy / (gyout[j + 1] - gyout[j]);
	  } else {
	    dyout = (gadouble)1.;
	  }
	  i__4 = ie;
	  for (ii = ib; ii <= i__4; ++ii) {
	    ++icnt;
	    if (icnt > maxgrid) {
	      *istat = 0;
	      goto err;
	    }
	    ii0 = ii;
	    if (cyclicx && ii0 > *nii) {
	      ii0 = ii - *nii;
	    }
	    if (ii == ib) {
	      dx = (gadouble) ii - gxout[i];
	    } else if (ii == ie) {
	      x0 = (gadouble) (ii - 1);
	      if (cyclicx) {
		x0 = (gadouble) ii0 - (gadouble)1.;
	      }
	      dx = gxout[i + 1] - x0;
	    } else {
	      dx = (gadouble)1.;
	    }
	    if (ii == ib || ii == ie) {
	      dxout = dx / (gxout[i + 1] - gxout[i]);
	    } else {
	      dxout = (gadouble)1.;
	    }
	    dxdy_box__[icnt - 1] = dxout * dyout;
	    area_box__[icnt - 1] = dx * dy * area_in[ii0 + jj * 
						     area_in_dim1];
	    if (area_in[ii0 + jj * area_in_dim1] == (gadouble)0.) {
	      dxdy_box__[icnt - 1] = (gadouble)0.;
	    }
	    fld_box__[icnt - 1] = fld_in[ii0 + jj * fld_in_dim1]
	      ;
	  }
	}
      }
      //		   integrate or vote for the average value 
      if (*vote) {
	//		   voting routine; first get total area 
	tot_area__ = (gadouble)0.;
	i__3 = icnt;
	for (ii = 1; ii <= i__3; ++ii) {
	  tot_area__ += area_box__[ii - 1];
	}
	if (tot_area__ <= eps) {
	  fld_out[i + j * fld_out_dim1] = *undef;
	  goto L100;
	}
	if (icnt == 1) {
	  //		   USSR election -- only one "candidate" to vote for 
	  //		   check if the the total area is greater 
	  //		   than the minimum required to hold the election (e.g., 0.5) 
	  if (dxdy_box__[0] < rmin_dxdy_vote__[0]) {
	    fld_out[i + j * fld_out_dim1] = *undef;
	  } else {
	    fld_out[i + j * fld_out_dim1] = fld_box__[0];
	  }
	  goto L100;
	} else if (icnt == 2) {
	  //		   USA election -- two-party, two-candidate race; area wins 
	  if (dxdy_box__[0] == (gadouble)0. || dxdy_box__[1] == (gadouble)
	      0.) {
	    rmin_vote__ = rmin_dxdy_vote__[0];
	  } else if (fld_box__[0] == fld_box__[1]) {
	    rmin_vote__ = rmin_dxdy_vote__[0];
	  } else {
	    rmin_vote__ = rmin_dxdy_vote__[1];
	  }
	  if (dxdy_box__[0] >= dxdy_box__[1] && dxdy_box__[0] > 
	      rmin_vote__) {
	    fld_out[i + j * fld_out_dim1] = fld_box__[0];
	  } else if (dxdy_box__[1] > rmin_vote__) {
	    fld_out[i + j * fld_out_dim1] = fld_box__[1];
	    //		   case where both candidates are the same in the two-person race 
	  } else if (fld_box__[0] == fld_box__[1] && dxdy_box__[0] 
		     + dxdy_box__[1] > rmin_vote__) {
	    fld_out[i + j * fld_out_dim1] = fld_box__[1];
	  } else {
	    fld_out[i + j * fld_out_dim1] = *undef;
	  }
	} else {
					
	  //		   a wide open election - three or more candidates	
	  //		   sort the data by surface area using 
	  //		   the numercial recipes routine --  indexx 
					
	  indexx(icnt, fld_box__, ifld_rank__);
					
	  //		   the indexes are in reverse order, with the 
	  //		   biggest fld element in the last element of the array 
	  //		   first check if the biggest is in the majority 
	  //		   set up the candidates 
					
	  ncand = 1;
	  it1 = ifld_rank__[0];
	  area_cand__[ncand - 1] = area_box__[it1 - 1];
	  fld_cand__[ncand - 1] = fld_box__[it1 - 1];
	  dxdy_cand__[ncand - 1] = dxdy_box__[it1 - 1];
	  i__3 = icnt;
	  for (ii = 2; ii <= i__3; ++ii) {
	    i1 = ii - 1;
	    i2 = ii;
	    it1 = ifld_rank__[i1 - 1];
	    it2 = ifld_rank__[i2 - 1];
	    if (fld_box__[it1 - 1] == fld_box__[it2 - 1]) {
	      area_cand__[ncand - 1] += area_box__[it2 - 1];
	      dxdy_cand__[ncand - 1] += dxdy_box__[it2 - 1];
	    } else {
	      ++ncand;
	      area_cand__[ncand - 1] = area_box__[it2 - 1];
	      dxdy_cand__[ncand - 1] = dxdy_box__[it2 - 1];
	      fld_cand__[ncand - 1] = fld_box__[it2 - 1];
	    }
	  }
					
	  //		   if one candidate, all done 
					
	  if (ncand == 1) {
	    if (dxdy_cand__[0] > rmin_dxdy_vote__[2]) {
	      fld_out[i + j * fld_out_dim1] = fld_cand__[0];
	    } else {
	      fld_out[i + j * fld_out_dim1] = *undef;
	    }
	    goto L100;
	  } else {
						
	    //		   the candidate with the most area is the winner 
						
	    area_max__ = (gadouble)0.;
	    i__3 = ncand;
	    for (ii = 1; ii <= i__3; ++ii) {
	      if (area_cand__[ii - 1] > area_max__) {
		iamax = ii;
		area_max__ = area_cand__[ii - 1];
	      }
	    }
	    if (ncand <= 2) {
	      rmin_vote__ = rmin_dxdy_vote__[ncand - 1];
	    } else {
	      rmin_vote__ = rmin_dxdy_vote__[2];
	    }
	    if (dxdy_cand__[iamax - 1] > rmin_vote__) {
	      fld_out[i + j * fld_out_dim1] = fld_cand__[
							 iamax - 1];
	    } else {
	      fld_out[i + j * fld_out_dim1] = *undef;
	    }
	    goto L100;
	  }
	}
      } else {
			
	//		   area integrate 
			
	tot_fld__ = (gadouble)0.;
	tot_area__ = (gadouble)0.;
			
	i__3 = icnt;
	for (ii = 1; ii <= i__3; ++ii) {
	  tot_fld__ += fld_box__[ii - 1] * area_box__[ii - 1];
	  tot_area__ += area_box__[ii - 1];
	}
	if (tot_area__ > area_out[i + j * area_out_dim1] * *
	    area_min) {
	  fld_out[i + j * fld_out_dim1] = tot_fld__ / 
	    tot_area__;
	} else {
	  fld_out[i + j * fld_out_dim1] = *undef;
	}
      }
    L100:
      if (NULL!=(fld_cand__)) free (fld_cand__);
      if (NULL!=(area_box__)) free (area_box__);
      if (NULL!=(dxdy_box__)) free (dxdy_box__); 
      if (NULL!=(area_cand__)) free (area_cand__); 
      if (NULL!=(dxdy_cand__)) free (dxdy_cand__); 
      if (NULL!=(fld_box__)) free (fld_box__);
      if (NULL!=(ifld_rank__)) free (ifld_rank__);
      ;
    }
  }
  return 0;
 err:
  if (NULL!=(fld_cand__)) free (fld_cand__);
  if (NULL!=(area_box__)) free (area_box__);
  if (NULL!=(dxdy_box__)) free (dxdy_box__); 
  if (NULL!=(area_cand__)) free (area_cand__); 
  if (NULL!=(dxdy_cand__)) free (dxdy_cand__); 
  if (NULL!=(fld_box__)) free (fld_box__);
  if (NULL!=(ifld_rank__)) free (ifld_rank__); 
  sprintf (pout,"box_ave error.\n");
  gaprnt_(0,pout);
  return 1;
} // box_ave 

static int bssl_interp(gadouble *fld_in, gadouble *undef, 
		       gadouble *xoutb, gadouble *youtb,
		       gadouble *gxout, gadouble *gyout,
		       int *nii, int *nji, int *nio, int *njo,
		       gadouble *fld_out, int *iunit_diag,
		       int *cyclicxi, int *spole_point, int *npole_point, int *bessel,
		       int *istat) {
  // System generated locals 
  int fld_in_dim1, fld_in_offset, fld_out_dim1, fld_out_offset, i__1, 
    i__2, i__3;
	
  // Local variables 
  int jchk, iok_bilinear__;
  gadouble fijm1, fijp1, fijp2, udel2;
  gadouble y1,y2,gy1,gy2,gycenter,gymid;
  int niim1, njim1, i, j, k;
  gadouble r__, s, u;
  int i1, j1;
  gadouble r1, r2, r3, s1, s2, s3;
  int ic, jc, ii;
  gadouble fr[4];
  int im1, ip2, ibb, jbb, iee, jee;
  gadouble del;
  int j1m1;
  gadouble fij;
  int j1p1, j1p2;
  gadouble del3;
  int icp1, jcp1;
	
  // Parameter adjustments 
  fld_in_dim1 = *nii;
  fld_in_offset = fld_in_dim1 + 1;
  fld_in -= fld_in_offset;
  --gxout;
  fld_out_dim1 = *nio;
  fld_out_offset = fld_out_dim1 + 1;
  fld_out -= fld_out_offset;
  --gyout;
	
  // Function Body 
  jchk = 2;
  *istat = 1;
  //		   convert the box boundaries to grid point center 
  i__1 = *nio;
  for (i = 1; i <= i__1; ++i) {		
    //		   check for crossing the boundaries 
    //		   the only way for this to occur is if the field is 
    //		   cyclically continuous in x 
    if (gxout[i + 1] < gxout[i]) {
      gxout[i] = (gxout[i] - (gadouble) (*nii) + gxout[i + 1]) * (
								  gadouble).5 + (gadouble).5;
      if (gxout[i] < (gadouble)1.) {
	gxout[i] = (gadouble) (*nii) + gxout[i];
      }
    } else {
      gxout[i] = (gxout[i] + gxout[i + 1]) * (gadouble).5 + (gadouble)
	.5;
      if (gxout[i] < (gadouble)1.) {
	gxout[i] = (gadouble) (*nii) + gxout[i];
      }
    }
  }
  i__1 = *njo;
  for (j = 1; j <= i__1; j++) {


/* original code 
//		   check if a pole points on the input grid 		
    gyout[j] = (gyout[j] + gyout[j + 1]) * (gadouble).5 + (gadouble).5;
		if (*spole_point && gyout[j] < (gadouble)1.) {
			gyout[j] = (gadouble)1.;
		}
		if (*npole_point && gyout[j] > *nji + (gadouble).5) {
			gyout[j] = (gadouble) (*nji);
		}
*/

// mf 20090329 -- make work like re2  -- youtb in re.c is like youb in ftn_re2.F

    y1=youtb[j-1];
    y2=youtb[j];
    gy1=gyout[j];
    gy2=gyout[j+1];
    gycenter=(gyout[j]+gyout[j+1])*0.5;

    if(gycenter < 0.5) {
       gymid=gycenter+0.5;
    } else {
       gymid=gycenter+0.5;
    }

    gyout[j]=gymid;

    if (*spole_point && (y1 <= -90.0) && (y2>y1)) {
      gyout[j] = (gadouble)1.;
    }

    if (*npole_point && (y2 >= 90.0) && (y1 < y2) ) {
      gyout[j] = (gadouble) (*nji);
    }


/* mf 20090329 -- from ftn_libmf.F

        if(spole_point.and.(y1.le.-90.and.y2.gt.y1)) then
          if(verb) write(iunit_diag,*) 'ddddddddd SSSSSS pole corr'
          gyout(j)=1.0
          jc=1
          s=gyout(j)-jc
        endif

        if(npole_point.and.(y2.ge.90.and.y1.lt.y2)) then
          if(verb) write(iunit_diag,*) 'dddddddddd NNNNNN pole corr'
          gyout(j)=real(nji)
          jc=nji
          s=gyout(j)-jc
        endif

        if(npole_point.and.gyout(j).gt.(real(nji)+0.5)) 
     $       gyout(j)=real(nji)

*/


  }
  ibb = 1;
  iee = *nio;
  jbb = 1;
  jee = *njo;
  niim1 = *nii - 1;
  njim1 = *nji - 1;
  i__1 = jee;
  for (j = jbb; j <= i__1; j++) {
    i__2 = iee;
    for (i = ibb; i <= i__2; ++i) {
      ic = (int) gxout[i];
      jc = (int) gyout[j];
      icp1 = ic + 1;
      if (*cyclicxi && icp1 > *nii) {
	icp1 -= *nii;
      }
      jcp1 = jc + 1;

 /*
      if ( (gxout[i]==gxout[i+1])||(gyout[j]==gyout[j+1]) ){
	fld_out[i + j * fld_out_dim1] = *undef;
	continue;
 */

      if( (jc < 1 || jc > *nji) || (!(*cyclicxi) && (ic < 1 || ic > *nii)) ) {
	fld_out[i + j * fld_out_dim1] = *undef;
	goto L100;
      }	
      // ------------------------------------------------ 			
      //		   bilinear/bessel interpolation based on the FNOC routine 
      //		   bssl5 by D. Hensen, FNOC 			
      // ------------------------------------------------ 			
      r__ = gxout[i] - ic;
      s = gyout[j] - jc;			
      //		   interior check 
			
      if (jc >= 2 && jc < njim1 && *cyclicxi || ic >= 2 && jc >= 2 && 
	  ic < niim1 && jc < njim1) {
	goto L10;
      }
			
      //		   border zone check 
			
      if (jc < *nji && *cyclicxi || ic < *nii && jc < *nji) {
	goto L5;
      }
			
      // ------------------------------------------------ 
			
      //		   top and right edge processing 
			
      // ------------------------------------------------ 
			
      if (ic == *nii && jc == *nji) {
	fld_out[i + j * fld_out_dim1] = fld_in[*nii + *nji * 
					       fld_in_dim1];
      } else if (ic == *nii) {
	if (fld_in[ic + jc * fld_in_dim1] != *undef && fld_in[ic+ jcp1 * fld_in_dim1] != *undef) {
	  fld_out[i + j * fld_out_dim1] = ((gadouble)1. - s) * 
	    fld_in[ic + jc * fld_in_dim1] + s * fld_in[ic + jcp1 * fld_in_dim1];
	} else {
	  fld_out[i + j * fld_out_dim1] = *undef;
	}
      } else if (jc == *nji) {
	if (fld_in[ic + jc * fld_in_dim1] != *undef && fld_in[icp1 + jc * fld_in_dim1] != *undef) {
	  fld_out[i + j * fld_out_dim1] = ((gadouble)1. - r__) * 
	    fld_in[ic + jc * fld_in_dim1] + r__ * fld_in[icp1 + jc * fld_in_dim1];
	} else {
	  fld_out[i + j * fld_out_dim1] = *undef;
	}
      } else if (jc == 1) {
	if (fld_in[ic + jc * fld_in_dim1] != *undef && fld_in[icp1 + jc * fld_in_dim1] != *undef) {
	  fld_out[i + j * fld_out_dim1] = ((gadouble)1. - r__) * 
	    fld_in[ic + jc * fld_in_dim1] + r__ * fld_in[icp1 + jc * fld_in_dim1];
	} else {
	  fld_out[i + j * fld_out_dim1] = *undef;
	}
      }
      goto L100;
    L5:
			
      // ------------------------------------------------ 
      //		   border zone; bilinear 
      // ------------------------------------------------ 
			
      iok_bilinear__ = 1;
      if (fld_in[ic + jc * fld_in_dim1] == *undef || fld_in[icp1 + jc * fld_in_dim1] == *undef || 
	  fld_in[ic + jcp1 * fld_in_dim1] == *undef || fld_in[icp1 + jcp1 * fld_in_dim1] == *undef) {
	iok_bilinear__ = 0;
      }
      if (iok_bilinear__ == 0) {
	fld_out[i + j * fld_out_dim1] = *undef;
      } else {
	fld_out[i + j * fld_out_dim1] = ((gadouble)1. - s) *
	  (((gadouble)1. - r__) * fld_in[ic + jc * fld_in_dim1] + 
	   r__ * fld_in[icp1 + jc * fld_in_dim1]) + s * ((( gadouble)1. - r__) * fld_in[ic + jcp1 * fld_in_dim1] +  r__ * fld_in[icp1 + jcp1 * fld_in_dim1]);
      }
      goto L100;
    L10:
			
      // ------------------------------------------------ 			
      //		   interior zone 
      // ------------------------------------------------ 
      //		   first check if bilinear is OK 
			
      iok_bilinear__ = 1;
      if (fld_in[ic + jc * fld_in_dim1] == *undef || fld_in[icp1 + jc * fld_in_dim1] == *undef || 
	  fld_in[ic + jcp1 * fld_in_dim1] == *undef || fld_in[icp1 + jcp1 * fld_in_dim1] == *undef) {
	iok_bilinear__ = 0;
      }
      if (iok_bilinear__ == 0) {
	fld_out[i + j * fld_out_dim1] = *undef;
	goto L100;
      } else {
				
	//		   bilinear value is the first guess 
				
	fld_out[i + j * fld_out_dim1] = ((gadouble)1. - s) * (((
								gadouble)1. - r__) * fld_in[ic + jc * fld_in_dim1] + 
							      r__ * fld_in[icp1 + jc * fld_in_dim1]) + s * (((
													      gadouble)1. - r__) * fld_in[ic + jcp1 * fld_in_dim1] + 
													    r__ * fld_in[icp1 + jcp1 * fld_in_dim1]);
				
	//		   exit if only doing bilinear 
				
	if (! (*bessel)) {
	  goto L100;
	}
      }
			
      //		   interpolate 4 columns (i-1,i,i+1,i+2) to j+s and store in fr(1) 
      //		   through fr(4) 
			
      r1 = r__ - (gadouble).5;
      r2 = r__ * (r__ - (gadouble)1.) * (gadouble).5;
      r3 = r1 * r2 * (gadouble).3333333333334;
      s1 = s - (gadouble).5;
      s2 = s * (s - (gadouble)1.) * (gadouble).5;
      s3 = s1 * s2 * (gadouble).3333333333334;
			
      k = 0;
      im1 = ic - 1;
      ip2 = ic + 2;
			
      i__3 = ip2;
      for (ii = im1; ii <= i__3; ++ii) {
	++k;
	i1 = ii;
	if (*cyclicxi && i1 < 1) {
	  i1 = *nii - i1;
	}
	if (*cyclicxi && i1 > *nii) {
	  i1 -= *nii;
	}
	j1 = jc;
	j1p1 = j1 + 1;
	j1p2 = j1 + 2;
	j1m1 = j1 - 1;
	fij = fld_in[i1 + j1 * fld_in_dim1];
	fijp1 = fld_in[i1 + j1p1 * fld_in_dim1];
	fijp2 = fld_in[i1 + j1p2 * fld_in_dim1];
	fijm1 = fld_in[i1 + j1m1 * fld_in_dim1];
				
	//		   exit if any value undefined 
				
	if (fij == *undef || fijp1 == *undef || fijp2 == *undef || 
	    fijm1 == *undef) {
	  goto L100;
	}
	u = (fij + fijp1) * (gadouble).5;
	del = fijp1 - fij;
	udel2 = (fijp2 - fijp1 + fijm1 - fij) * (gadouble).5;
	del3 = fijp2 - fijp1 - del * (gadouble)2. + fij - fijm1;
	fr[k - 1] = u + s1 * del + s2 * udel2 + s3 * del3;
      }
			
      //		   interpolate the fr row to ii+r 
			
      u = (fr[1] + fr[2]) * (gadouble).5;
      del = fr[2] - fr[1];
      udel2 = (fr[3] - fr[2] + fr[0] - fr[1]) * (gadouble).5;
      del3 = fr[3] - fr[2] - del * (gadouble)2. + fr[1] - fr[0];
      fld_out[i + j * fld_out_dim1] = u + r1 * del + r2 * udel2 + 
	r3 * del3;
    L100:
      ;
    }
  }
  return 0;
} // bssl_interp 

static int indexx(int n, gadouble *arrin, int *indx) {
  // System generated locals 
  int i__1;
	
  // Local variables 
  int i, j, l;
  gadouble q;
  int indxt, ir;
	
  // Parameter adjustments 
  --indx;
  --arrin;
	
  // Function Body 
  i__1 = n;
  for (j = 1; j <= i__1; j++) {
    indx[j] = j;
  }
  l = n / 2 + 1;
  ir = n;
 L10:
  if (l > 1) {
    --l;
    indxt = indx[l];
    q = arrin[indxt];
  } else {
    indxt = indx[ir];
    q = arrin[indxt];
    indx[ir] = indx[1];
    --ir;
    if (ir == 1) {
      indx[1] = indxt;
      return 0;
    }
  }
  i = l;
  j = l + l;
 L20:
  if (j <= ir) {
    if (j < ir) {
      if (arrin[indx[j]] < arrin[indx[j + 1]]) {
	j++;
      }
    }
    if (q < arrin[indx[j]]) {
      indx[i] = indx[j];
      i = j;
      j += j;
    } else {
      j = ir + 1;
    }
    goto L20;
  }
  indx[i] = indxt;
  goto L10;
} // indexx 

static int in_out_boundaries(gadouble *xin, gadouble *yin, gadouble *xout, gadouble *yout, 
			     int cyclicxi, int niip1, int njip1, int niop1,
			     int njop1, gadouble *gxout, gadouble *gyout, gadouble undef, int iLonLat) {
  //	Purpose:	
  //		   calculate the location of grid box boundaries of 
  //		   an "output" grid w.r.t. an "input" grid 
  //		   used in a 2-D regriding process, i.e., 
  //		   input --> output 
  //	Input variables: 
  //		   xin - longitudes of the input grid 
  //		   yin - latitudes of the input grid 
  //		   xout - longtitudes of the output grid 
  //		   yout - latitudes of the output grid 
  //		   cyclicxi - flag whether the input grid is cyclically continuous 
  //		   in x 
  //		   nii - size of the x dimension of the input grid 
  //		   nji - size of the y dimension of the input grid 
  //		   nio - size of the x dimension of the output grid 
  //		   njo - size of the y dimension of the output grid 
  //		   niip1= nii+1, etc. 
  //		   iunit_diag - unit number to write diagnositics 
  //	Output variables: 
	
  //		   gxout - x location of the output grid in input grid units 
  //		   gyout - y location of the output grid in input grid units 
  //		   locate the output grid w.r.t. input grid in x 
  //		   ALLOW FOR CYCLIC CONTINUITY IN X!! 
  int i, j, ii, jj;
  for (i = 0; i < niop1; i++) {
    if (1==iLonLat) {
      if ((xout[i]>=xin[0]&&xout[i]<=xin[niip1-1])||
	  (xout[i]>=xin[0]+360.&&xout[i]<=xin[niip1-1]+360.) ||
	  (xout[i]>=xin[0]-360.&&xout[i]<=xin[niip1-1]-360.) ){
	// inside the range
	for (ii=0; ii<niip1; ii++) {
	  if (xout[i]>=xin[ii]&&xout[i]<=xin[ii+1]) {
	    gxout[i] =(gadouble)ii+(xout[i]-xin[ii])/(xin[ii+1]-xin[ii]);
	    break;
	  }
	  else if (xout[i]>=xin[ii]+360.&&xout[i]<=xin[ii+1]+360.) {
	    gxout[i] =(gadouble)ii+(xout[i]-360.-xin[ii])/(xin[ii+1]-xin[ii]);
	    break;
	  }
	  else if (xout[i]>=xin[ii]-360.&&xout[i]<=xin[ii+1]-360.) {
	    gxout[i] =(gadouble)ii+(xout[i]+360.-xin[ii])/(xin[ii+1]-xin[ii]);
	    break;
	  }
	}
      }
      else {
	if (cyclicxi) {
	  if (xout[i]<xin[0]) {
	    gxout[i] =(gadouble)(-1)+(xout[i]-xin[niip1]-360.)/(xin[0]-xin[niip1]-360.);
	  }
	  else {
	    gxout[i] =(gadouble)niip1+(xout[i]-xin[niip1])/(xin[0]+360.-xin[niip1]);
	  }
	}
	else {
	  if (xout[i]<xin[0]) {
	    gxout[i] =0.;
	  }
	  else {
	    gxout[i] =(gadouble)niip1-1.;
	  }
	  //				gxout[i] = undef;
	}
      }
    }
    else {
      if (xout[i]>=xin[0]&&xout[i]<=xin[niip1-1]) {
	// inside the range
	ii=0;
	while (!((xout[i]>=xin[ii])&&(xout[i]<=xin[ii+1]))) {ii++;}
	if (xin[ii+1]==xin[ii]) {
	  gxout[i] =(gadouble)ii;
	  {sprintf (pout,"warning! xin[%d] and xin[%d] grid overlapping!\n",ii,ii+1);
	    gaprnt_(2,pout);}
	}
	else {
	  gxout[i] =(gadouble)ii+(xout[i]-xin[ii])/(xin[ii+1]-xin[ii]);
	}
      }
      else {
	if (xout[i]<xin[0]) {
	  gxout[i] =0.;
	}
	else {
	  gxout[i] =(gadouble)niip1-1.;
	}
      }
    }
  }
  //		   locate the output grid w.r.t. input grid in y 
  //		   NO CYCLIC CONTINUITY!! 
  for (j = 0; j < njop1; j++) {
    if (yout[j]>=yin[0]&&yout[j]<=yin[njip1-1]) {
      jj=0;
      while (!((yout[j]>=yin[jj])&&(yout[j]<=yin[jj+1]))) {jj++;}
      if (yin[jj+1]==yin[jj]) {
	gyout[j] =(gadouble)jj;
	{sprintf (pout,"warning! yin[%d] and yin[%d] grid overlapping!\n",jj,jj+1);
	  gaprnt_(2,pout);}
      }
      else {
	gyout[j] =(gadouble)jj+(yout[j]-yin[jj])/(yin[jj+1]-yin[jj]);
      }
    }
    else {
      if (yout[j]<yin[0]) {
	gyout[j] =0.;
      }
      else {
	gyout[j] =(gadouble)njip1-1.;
      }
      //			gyout[j] = undef;
    }
  }
  return 0;
} // in_out_boundaries 

static int avg_pole(gadouble *a, int *m, int n) {
  // System generated locals 
  int a_dim1, a_offset, i__1;
  // Local variables 
  gadouble ave_2__;
  int i;
  gadouble ave_nm1__;
  // sss		routine to replace pole value with average 
  // sss		at the penultimate point 
  // Parameter adjustments 
  a_dim1 = *m;
  a_offset = a_dim1 + 1;
  a -= a_offset;
	
  // Function Body 
  ave_2__ = (gadouble)0.;
  ave_nm1__ = (gadouble)0.;
  i__1 = *m;
  for (i = 1; i <= i__1; ++i) {
    ave_2__ += a[i + (a_dim1 << 1)];
    ave_nm1__ += a[i + (n - 1) * a_dim1];
  }
  ave_2__ /= *m;
  ave_nm1__ /= *m;
  i__1 = *m;
  for (i = 1; i <= i__1; ++i) {
    a[i + a_dim1] = ave_2__;
    a[i + n * a_dim1] = ave_nm1__;
  }
  return 0;
} // avg_pole 

static int bsslz1(double *bes, int n) {
  // Initialized data 
	
  double pi = 3.14159265358979;
  double bz[50] = { 2.4048255577,5.5200781103,8.6537279129,
		    11.7915344391,14.9309177086,18.0710639679,21.2116366299,
		    24.3524715308,27.493479132,30.6346064684,33.7758202136,
		    36.9170983537,40.0584257646,43.1997917132,46.3411883717,
		    49.4826098974,52.6240518411,55.765510755,58.9069839261,
		    62.0484691902,65.1899648002,68.3314693299,71.4729816036,
		    74.6145006437,77.7560256304,80.8975558711,84.0390907769,
		    87.1806298436,90.3221726372,93.4637187819,96.605267951,
		    99.7468198587,102.888374254,106.029930916,109.171489649,
		    112.31305028,115.454612653,118.59617663,121.737742088,
		    124.879308913,128.020877005,131.162446275,134.304016638,
		    137.44558802,140.587160352,143.728733573,146.870307625,
		    150.011882457,153.153458019,156.295034268 };
	
  // System generated locals 
  int i__1;
  // Local variables 
  int j, nn;
  // Parameter adjustments 
  --bes;	
  // Function Body 
  nn = n;
  if (n <= 50) {
    goto L12;
  }
  bes[50] = bz[49];
  i__1 = n;
  for (j = 51; j <= i__1; j++) {
    // L5: 
    bes[j] = bes[j - 1] + pi;
  }
  nn = 49;
 L12:
  i__1 = nn;
  for (j = 1; j <= i__1; j++) {
    // L15: 
    bes[j] = bz[j - 1];
  }
  return 0;
} // bsslz1 

static int fix_poles(gadouble *fld_out, int *nio, int *njo, gadouble *undef,
		     int *spole_point, int *npole_point) {
  // System generated locals 
  int fld_out_dim1, fld_out_offset, i__1;
	
  // Local variables 
  int i, icntn, icnts;
  gadouble rmeann, rmeans;
	
  // Parameter adjustments 
  fld_out_dim1 = *nio;
  fld_out_offset = fld_out_dim1 + 1;
  fld_out -= fld_out_offset;
	
  // Function Body 
  rmeans = (gadouble)0.;
  rmeann = (gadouble)0.;
  icnts = 0;
  icntn = 0;
  i__1 = *nio;
  for (i = 1; i <= i__1; ++i) {
    if (fld_out[i + fld_out_dim1] != *undef) {
      rmeans += fld_out[i + fld_out_dim1];
      ++icnts;
    }
    if (fld_out[i + *njo * fld_out_dim1] != *undef) {
      rmeann += fld_out[i + *njo * fld_out_dim1];
      ++icntn;
    }
  }
  if (icnts > 0) {
    rmeans /= (gadouble) icnts;
  } else {
    rmeans = *undef;
  }
  if (icntn > 0) {
    rmeann /= (gadouble) icntn;
  } else {
    rmeann = *undef;
  }
  if (*spole_point) {
    i__1 = *nio;
    for (i = 1; i <= i__1; ++i) {
      fld_out[i + fld_out_dim1] = rmeans;
    }
  }
  if (*npole_point) {
    i__1 = *nio;
    for (i = 1; i <= i__1; ++i) {
      fld_out[i + *njo * fld_out_dim1] = rmeann;
    }
  }
  return 0;
} // fix_poles 


// $$$	subprogram documentation block 
//				  . 	 .	  . 									  . 
// subprogram:	  gaulat	  calculates gaussian grid latitudes 
//	 prgmmr: s. j. lord 	  org: w/nmc22	  date: 91-06-06 

// abstract: calculates gaussian grid latitudes 

// program history log: 
//	 91-06-06  s. j. lord - copied from kanamitsu library 
//	 930921 m.fiorino - changed from colatitude to latitude 
//	 yy-mm-dd  modifier1   description of change 
//	 yy-mm-dd  modifier2   description of change 

// usage:	 call pgm-name(inarg1, inarg2, wrkarg, outarg1, ... ) 
//	 input argument list: 
//	   inarg1	- generic description, including content, units, 
//	   inarg2	- type.  explain function if control variable. 

//	 output argument list:		(including work arrays) 
//	   wrkarg	- generic description, etc., as above. 
//	   outarg1	- explain completely if error return 
//	   errflag	- even if many lines are needed 

//	 input files:	(delete if no input files in subprogram) 
//	   ddname1	- generic name & content 

//	 output files:	(delete if no output files in subprogram) 
//	   ddname2	- generic name & content as above 
//	   ft06f001 - include if any printout 

// remarks: list caveats, other helpful hints or information 

// attributes: 
//	 language: indicate extensions, compiler options 
//	 machine:  nas, cyber, whatever 

// $$$ 
static int gauss_lat_nmc(gadouble *gaul, int nj) {
  int j;
  double radi;
  double *pa, *pw;
  pa= (double *)malloc(sizeof(double)*nj);
  pw= (double *)malloc(sizeof(double)*nj);
  //		   get the gaussian latitudes and integration weights 
  gauaw(pa, pw, nj);
  radi = (double)180. / (atan((double)1.) * (double)4.);
  for (j = 0; j < nj; j++) {
    gaul[j] = acos(pa[j]) * radi - (double)90.;
  }
  if (lDiag) {
    sprintf (pout,"gaussian lat (deg) for jmax=%d\n",nj);gaprnt_(2,pout);
    for (j=0; j<nj; j++) {
      sprintf (pout,"%g ",gaul[j]);gaprnt_(2,pout);
    }
    sprintf (pout,"\n");gaprnt_(2,pout);
  }	
  free (pw);
  free (pa);
  return 0;
} // gauss_lat_nmc 

static int gauss_lat_pcmdi(gadouble **gaul, gadouble **gaulb, int nj) {
  // Local variables 
  int j;
  double pi, r2d;
  double *pa, *pb, *pt, *pw;

  *gaul = (gadouble *)malloc(sizeof(gadouble)*nj);
  *gaulb = (gadouble *)malloc(sizeof(gadouble)*(nj+1));	// plus 1 incl. boundary
  pa= (double *)malloc(sizeof(double)*nj);
  pw= (double *)malloc(sizeof(double)*nj);
  pb= (double *)malloc(sizeof(double)*(nj+1));	// plus 1 to incl. boundary
  pt= (double *)malloc(sizeof(double)*(nj+1));	// plus 1 to incl. boundary
  if (NULL==(*gaul)||NULL==(*gaulb)||NULL==pa||NULL==pw||NULL==pb||NULL==pt) goto merr; 
  //		   get the gaussian latitudes and integration weights 
  // Function Body 
  if (lDiag) {sprintf (pout,"OK from gauss_lat_pcmdi 1.0:pa=%x, nj=%d\n",pa,nj);gaprnt_(2,pout);}
  gauaw(pa, pw, nj);
  if (lDiag) {sprintf (pout,"OK from gauss_lat_pcmdi 2.0\n");gaprnt_(2,pout);}
  //		   reverse direction so j increase northward for output 
  for (j = 0; j < nj; j++) pt[j] = pa[nj-j-1];
  for (j = 0; j < nj; j++) pa[j] = pt[j];
  for (j = 0; j < nj; j++) pt[j] = pw[nj-j-1];
  for (j = 0; j < nj; j++) pw[j] = pt[j];
  pi = atan(1.) * (double)4.;
  r2d = (double)180. / pi;
  //		   integrate to get the latitude boundaries of the gauss grid boxes 	
  pb[0] = -pi * (double).5;	// at south pole
  for (j = 1; j < nj/2; j++) {
    pb[j] = asin(pw[j-1] + sin(pb[j-1]));
  }
  for (j = 0; j < nj/2; j++) {
    (*gaulb)[j] = r2d * pb[j];
  }
  (*gaulb)[nj/2] = 0.;	// middle one should be at equator
  if (lDiag) {sprintf (pout,"OK from gauss_lat_pcmdi 3.0\n");gaprnt_(2,pout);}
  for (j = nj; j > nj/2; j--) {
    (*gaulb)[j] = -(*gaulb)[nj-j];	// symmetric
  }
  for (j = 0; j < nj; j++) {
    (*gaul)[j] = r2d * asin(pa[j]);
  }
  if (lDiag) {sprintf (pout,"OK from gauss_lat_pcmdi 4.0\n");gaprnt_(2,pout);}
  if (lDiag) {
    {sprintf(pout,"2nd Method:\n");gaprnt(1,pout);}
    {sprintf(pout,"gaul:\n");gaprnt(1,pout);}
    for (j=0; j<nj; j++) {
      sprintf(pout,"%g ",(*gaul)[j]);gaprnt(1,pout);
      if (0==((j+1)%10)) {sprintf(pout,"\n");gaprnt(1,pout);}
    }
    {sprintf(pout,"\ngaulb:\n");gaprnt(1,pout);}
    for (j=0; j<=nj; j++) {
      sprintf(pout,"%g ",(*gaulb)[j]);gaprnt(1,pout);
      if (0==((j+1)%10)) {sprintf(pout,"\n");gaprnt(1,pout);}
    }
    {sprintf(pout,"\n");gaprnt(1,pout);}
  }//
  free (pw);
  free (pt);
  free (pb);
  free (pa);
  return TRUE_;
 merr:
  if (NULL==(*gaul)) free (*gaul);
  if (NULL==(*gaulb)) free (*gaulb); 
  if (NULL==pw) free (pw);
  if (NULL==pt) free (pt);
  if (NULL==pb) free (pb);
  if (NULL==pa) free (pa);
  return FALSE_;
} // gauss_lat_pcmdi 

static int gauaw(double *pa, double *pw, int nj) {
  // Initialized data 
  double eps = 1e-13;
  // Builtin functions 
  // Local variables 
  int iter;
  double avsp, c;
  int j;
  double pkmrk, fk, fn;
  int nj2;
  double pi, pk;
  int is;
  double sp, xz;
  double pkm1, pkm2;
	
  // **** 	*gauaw* - compute abscissas and weights for *gaussian integration. 
  //		   purpose. 
  //		   -------- 
  //		   *gauaw* is called to compute the abscissas and weights requir 
  //		   to perform *gaussian integration. 
  // **		interface. 
  //		   ---------- 
  //		   *call* *gauaw(pa,pw,nj)* 
  //		   *pa* 	- array, length at least nj,* to receive abscis 
  //		   abscissas. 
  //		   *pw* 	- array, length at least nj,* to receive 
  //		   weights. 
  //		   method. 
  //		   ------- 
  //		   the zeros of the *bessel functions are used as starting 
  //		   approximations for the abscissas. newton iteration is used to 
  //		   improve the values to within a tollerence of *eps.* 
  //		   external. 
  //		   --------- 
  //		   *bsslzr* - routine to obtain zeros of *bessel functions. 
  //		   reference. 
  //		   ---------- 

  // Function Body 
  //		   ------------------------------------------------------------------ 
  // *		1.	   set constants and find zeros of bessel function. 
  //		   --- --------- --- ---- ----- -- ------ --------- 
  pi = atan(1.) * (double)4.;
  c = ((double)1. - pow((double)2. / pi,2)) * (double).25;
  fk = (double) nj;
  nj2 = nj / 2;
  if (lDiag) {sprintf (pout,"OK from gauaw 1.0:pa=%x, nj2=%d\n",pa,nj2);gaprnt_(2,pout);}
  bsslzr(pa, nj2);
  if (lDiag) {sprintf (pout,"OK from gauaw 2.0\n");gaprnt_(2,pout);}
  for (is = 0; is < nj2; ++is) {
    xz = cos(pa[is] / sqrt(pow(fk+(double).5,2)+ c));
    // *		giving the first approximation for *xz.* 
    iter = 0;		
    //		   ------------------------------------------------------------------ 
    // *		2.	   compute abscissas and weights. 
    //		   ------- --------- --- ------- 
    // *		2.1 	set values for next iteration. 
    do {
      pkm2 = (double)1.;
      pkm1 = xz;
      ++iter;
      if (iter > 10) {
	goto err;
      }
      // *		2.2 	computation of the *legendre polynomial. 
      for (j = 2; j <= nj; ++j) {
	fn = (double) j;
	pk = ((fn * (double)2. - (double)1.) * xz * pkm1 - (fn - (double)1.) 
	      * pkm2) / fn;
	pkm2 = pkm1;
	pkm1 = pk;
      }
      pkm1 = pkm2;
      pkmrk = fk * (pkm1 - xz * pk) / ((double)1.-pow(xz,2));
      sp = pk / pkmrk;
      xz -= sp;
      avsp = fabs(sp);
    } while (avsp > eps);
    // *		2.3 	abscissas and weights. 
    pa[is] = xz;
    pw[is] = ((double)1.-pow(xz,2))*(double)2./ pow(fk*pkm1,2);
  }
  if (lDiag) {sprintf (pout,"OK from gauaw 3.0\n");gaprnt_(2,pout);}
  // *		2.4 	odd nj* computation of weight at the equator. 
  if (nj != (nj2 << 1)) {
    pa[nj2] = (double)0.;
    pk = (double)2. /pow(fk,2);
    for (j = 2; j <= nj; j += 2) {
      fn = (double) j;
      pk *= pow(fn,2) / pow(fn-(double)1.,2);
    }
    pw[nj2] = pk;
  }
  if (lDiag) {sprintf (pout,"OK from gauaw 4.0\n");gaprnt_(2,pout);}
  // *		2.5 	use symmetry to obtain remaining values. 
  for (j=0; j<nj2; j++) {
    pa[nj-j-1] = -pa[j];
    pw[nj-j-1] = pw[j];
  }
  if (lDiag) {sprintf (pout,"OK from gauaw 5.0\n");gaprnt_(2,pout);}
  return 0;
  //		   ------------------------------------------------------------------	
  // *		3.	   error processing. 
  //		   ----- -----------	
 err:
  sprintf (pout,"  gauaw failed to converge after 10 iterations.\n");
  gaprnt_(0,pout);
  return 1;
} // gauaw 

static int bsslzr(double *pbes, int knum) {
  // Initialized data 
  double zbes[50] = { 2.4048255577,5.5200781103,8.6537279129,
		      11.7915344391,14.9309177086,18.0710639679,21.2116366299,
		      24.3524715308,27.493479132,30.6346064684,33.7758202136,
		      36.9170983537,40.0584257646,43.1997917132,46.3411883717,
		      49.4826098974,52.6240518411,55.765510755,58.9069839261,
		      62.0484691902,65.1899648002,68.3314693299,71.4729816036,
		      74.6145006437,77.7560256304,80.8975558711,84.0390907769,
		      87.1806298436,90.3221726372,93.4637187819,96.605267951,
		      99.7468198587,102.8883742542,106.0299309165,109.1714896498,
		      112.3130502805,115.4546126537,118.5961766309,121.737742088,
		      124.8793089132,128.0208770059,131.1624462752,134.3040166383,
		      137.4455880203,140.5871603528,143.7287335737,146.8703076258,
		      150.011882457,153.1534580192,156.2950342685 };
  // System generated locals 
  // Builtin functions 
  //	double atan();
  // Local variables 
  int inum, j;
  double pi;
  // **** 	*bsslzr* - routine to return zeros of the j0 *bessel function. 
  //		   purpose. 
  //		   -------- 
  //		   *bsslzr* returns knum* zeros, or if knum>50,* knum* 
  //		   approximate zeros of the *bessel function j0. 
  // **		interface. 
  //		   ---------- 
  //		   *call* *nsslzr(pbes,knum)* 
  //		   *pbes*	- array, dimensioned knum,* to receive the 
  //		   values. 
  //		   knum*	- number of zeros requested. 
  //		   method. 
  //		   ------- 
  //		   the first 50 values are obtained from a look up table. any 
  //		   additional values requested are interpolated. 
  //		   externals. 
  //		   ---------- 
  //		   none. 
  //		   reference. 
  //		   ---------- 
  // call 	comcon 
  // Function Body 
  //		   ------------------------------------------------------------------ 
  // *		1.	   extract values from look up table. 
  //		   ------- ------ ---- ---- -- ------ 
  //		   set api 
  pi = atan(1.) * (double)4.;
  inum = min(knum,50);
  if (lDiag) {sprintf (pout,"OK from bsslzr 1.0:pa=%x, nj2=%d, inum=%d\n",
		       pbes,knum,inum);gaprnt_(2,pout);}
  for (j=0; j<inum; j++) {
    pbes[j] = zbes[j];
  }
  if (lDiag) {sprintf (pout,"OK from bsslzr 2.0\n");gaprnt_(2,pout);}
  //		   ------------------------------------------------------------------ 
  // *		2.	   interpolate remaining values. 
  //		   ----------- --------- ------- 
  for (j=50; j<knum; j++) {
    pbes[j]=pbes[j-1]+pi;
  }
  if (lDiag) {sprintf (pout,"OK from bsslzr 3.0\n");gaprnt_(2,pout);}
  //		   ------------------------------------------------------------------ 
  return 0;
} // bsslzr 

static int sfc_area(gadouble *fld, gadouble *rlon, gadouble *rlat, gadouble undef, int ni, int nj,
		    gadouble *area, int iLonLat) {
  // Local variables 
  gadouble dlat, dlon, rlat1, rlat0;
  int i, j;
  gadouble deg2rad;
  if (1==iLonLat) {
    // for lon/lat coordinate
    deg2rad=3.14115926/180.0;
    for (i=0; i<ni; i++) {
      for (j=0; j<nj; j++) {

/* mf 20090329 -- make sure box doesn't go past poles; relaxed contraint on the latitude grid bounds
   exceeding the poles to control interpolation at and near the poles
*/
        rlat1=rlat[j+1];
	rlat0=rlat[j];
	if(rlat1 >  90.0) rlat1=90.0;
	if(rlat0 >  90.0) rlat0=90.0;
	if(rlat1 < -90.0) rlat1=-90.0;
	if(rlat0 < -90.0) rlat0=-90.0;

	dlon=(rlon[i+1]-rlon[i])*deg2rad;
	dlat=sin(rlat1*deg2rad)-sin(rlat0*deg2rad);
	area[i+j*ni]=dlon*dlat;
	if(undef==fld[i+j*ni]) area[i+j*ni]=0.0;
      }
    }	  
  }
  else {
    // for utm coordinate
    for (j=0; j<nj; j++) {
      for (i=0; i<ni; i++) {
	dlon=rlon[i+1]-rlon[i];
	dlat=rlat[j+1]-rlat[j];
	area[i+j*ni]=dlon*dlat;
	if(undef==fld[i+j*ni]) area[i+j*ni]=0.0;
      }
    }	  
  }
  return 0;
}

static int reallocate(gadouble **a, int *ni, int *nj, int ninew, int njnew) {
  // trim array 'a' from (ni*nj) to (ninew*njnew);
  // Local variables 
  int i, j, siz;
  gadouble *dum1;
  // Function Body 
  siz=ninew*njnew;
  dum1 = (gadouble *)malloc(sizeof(gadouble)*siz);
  if (dum1==NULL) goto merr;
  for (j = 0; j < njnew; j++) {
    for (i = 0; i < ninew; ++i) {
      dum1[i + j * ninew] = (*a)[i + j *(*ni)];
    }
  }
  free (*a);	// free old array
  (*a)=dum1;	// swap array
  //	set the input size to the new trimmed x/y dimension 
  (*ni)=ninew;
  (*nj)=njnew;
  return TRUE_;;
 merr:
  sprintf (pout,"Error from %s: Memory Allocation Error\n","wrapxi_dum1");
  gaprnt_(0,pout);
  sprintf (pout,"with size: %d bytes\n",sizeof(gadouble)*siz);
  gaprnt_(0,pout);
  return FALSE_;
} // trim_grid 

static void gaprnt_ ( int level, char *msg ) {

  if ( level <= verbose ) gaprnt(level,msg);

}

/* .................................................................. */
int ffre_dbg(struct gafunc *pfc, struct gastat *pst) {	
  int rc;
  verbose = 2; /* turn on verbosity */ 
  rc = ffre(pfc,pst);
  verbose = 1;
  return (rc);
}

/* .................................................................. */

#if 0
int re (struct gaufb *ufb, struct gafunc *pfc, struct gastat *pst) {

  if ( strcmp(ufb->oname,"quiet")  == 0 ) verbose = 0; /* only eror messages  */
  if ( strcmp(ufb->oname,"parrot") == 0 ) verbose = 2; /* talks like a parrot */
  else                                    verbose = 1; /* error messages/help */

  return ffre ( pfc, pst );

}
#endif

#endif
