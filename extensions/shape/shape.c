/*

    Copyright (C) 2007 by Graziano Giuliani <graziano.giuliani@poste.it>
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

shape - GrADS Extensions for Shapefile support

=head1 SYNOPSIS

=over 2

B<shp_polyf> I<SHAPEFILE> [I<OBJECT>]

B<shp_lines> I<SHAPEFILE> [I<OBJECT>]

B<shp_print> I<SHAPEFILE>

=back

=head1 DESCRIPTION

This library of GrADS extensions provides commands for the display and output
of shapefiles. Quoting from the C<Shapefile C Library> documentation by Frank 
Warmerdam:

=over 2

The Shapefile format is a new working and interchange format promulagated by 
ESRI (http://www.esri.com/) for simple vector data with attributes. It
is apparently the only file format that can be edited in ARCView 2/3, and 
can also be exported and imported in Arc/Info.
An excellent white paper on the shapefile format is available from ESRI:

        http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf

The file format actually consists of three files.

 XXX.shp - holds the actual vertices.
 XXX.shx - hold index data pointing to the structures in XXX.shp
 XXX.dbf - holds the attributes in xBase (dBase) format.  

=back

=head1 COMMANDS PROVIDED

=head2 B<shp_polyf> I<SHAPEFILE> [I<OBJECT>]

For each polygon object defined in I<SHAPEFILE>, a filled polygon is
plotted as overlay on GrADS plots using the current line attributes.
Optionally, one can specify a particular I<OBJECT> to plot.

=head2 B<shp_lines> I<SHAPEFILE> [I<OBJECT>]

Each object contained in I<SHAPEFILE> is plotted as a continous line
joining vertexes as overlay on GrADS plots using the current line
attributes. Optionally, one can specify a particular I<OBJECT> to
plot.

=head2 B<shp_print> I<SHAPEFILE>

This function has NOT been implemented yet.

=head1 OBTAINING SHAPEFILES

Here is a very partial list of free (as in beer) shapefiles, as well
for a DVD that you can buy. We do not support or endorse these
products, we list them here for reference only.

=head2 Suggested Installation Procedure

We recommend that you install each triplet
B<(XXX.shp,XXX.shx,XXX.dbf)> of files in your GrADS data directory
(GADDIR), along with the other font and map databases. This way you
will be able to refer to them without entering the full pathname.

=head2 ESRI First Level World Administrative Boundaries 1998 

This shapefile contains the political boundaries for each country, and
states/provinces within each country as of 1998. The data is freely
available from:

 http://geodata.grid.unep.ch/download/admin98_li_shp.zip

=head2 World Map for APRS

This shapefile dataset is based upon a free map published by
TerraSpace, Russia as referenced by Geocommunity.  Antartica's data
came from the Digital Chart of the World and was reprojected into
NAD27 to agree with the rest of the world.

   http://aprsworld.net/gisdata/world/

=head2 GSHHS - A Global Self-consistent, Hierarchical, High-resolution Shoreline Database

GSHHS contain contains shapefiles of varying complexity:

=over 8

=item gshhs_land 

contains the basic coastline shapefile

=item gshhs_lake 

contains the shapefile of lakes within land

=item gshhs_isl  

contains the shapefile of islands within lakes

=item gshhs_pond 

contains the shapefile of ponds within islands within lakes

=back

The data is freely available from NOAA:

   http://www.ngdc.noaa.gov/mgg/shorelines/data/gshhs/version1.5/shapefiles/

=head2 FAO GeoNetwork

The GeoNetwork opensource allows to easily share geographically
referenced thematic information between different FAO Units, other UN
Agencies, NGO`s and other institutions. You can find more information here:

   http://www.fao.org/geonetwork/srv/en/main.home

For example, if you search for I<Administrative Boundaries shapefile>
in this site you get a list of administrative boundaries for many
countries.

=head2 American Geological Institute Global GIS DVD

This DVD is a product of an agreement between the U.S. Geological Survey
(USGS) and The American Geological Institute (AGI) for making the USGS
Global Geographic Information System (GIS) database readily available
to educators and the general public in the form of a DVD based world
atlas. Quoting from the AGI website:

   The USGS Global GIS database contains a wealth of USGS and other
   public domain data, including global coverages of elevation,
   landcover, seismicity, and resources of minerals and energy at a
   nominal scale of 1:1 million

Unfortunately, this DVD is not free and carries a restrictive
license. You can obtain additional information here:

   http://www.agiweb.org/pubs/globalgis/index.html

=head2 If you know of another good source of shapefiles...

Please drop us a note.

=head1 EXAMPLES

For trying these examples you will need to download the freely
available ESRI First Level World Administrative Boundaries 1998 (see 
OBTAINING SHAPEFILES above). Place the files

   admin98.dbf
   admin98.shp
   admin98.shx

either in your local directory or in your GrADS data directory
C<GADDIR> (recommended.)

=head2 Display Administrative Boundaries

Start by disabling the map drawing by GrADS

   ga-> set mpdraw off

Then display something and add a new map:

   ga-> d ts
   ga-> shp_lines admin98

=head2 Display a Solid Basemap

Similarly,

   ga-> d ts
   ga-> shp_polyf admin98

=head2 Changing the color of the map

You can use the C<set line> command of GrADS to change the color, thickness and
line style of the shapefile being drawn:

   ga-> set line 9
   ga-> shp_lines admin98

will plot the map in magenta. See the GrADS documentation for more
information.


=head1 BUGS

Only shapefiles having longitude/latitude coordinates are
supported. Shapefiles in UTM coordinates cannot yet be read by this
extension. The work around is to convert your shapefile to lat/lon
using an utility such as C<ogr2ogr> that comes with I<GDAL>:

    http://www.gdal.org/

=head1 CREDITS

Graziano Giuliani wrote the first version of C<shp_polyf()> as a bult-in patch 
for GrADS v1.b4. Arlindo da Silva converted it to a GrADS extension and introduced 
C<shp_print()> based on his C<gxyat> utility; as of this writing 
C<shp_print()> is just a stub.

The Shapefile C Library by Frank Warmerdam is extensively used for the 
manipulation of shapefiles.

=head1 COPYRIGHT

Copyright (c) 2007 Graziano Giuliani

Copyright (c) 2007 Arlindo da Silva

This is free software released under the GNU General Public
License; see the source for copying conditions.
There is NO  warranty;  not even for MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.

=cut

#else

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "grads.h"
#include "gx.h"

#include <libshp/shapefil.h>

static char message[2048];

/* Buffers to avoid fragmemting memory with multiple malloc/free */
#define NPTS 4096
static gadouble XY[NPTS];

#define POLYF 1
#define LINES 2

/* protypes */
static int shp_draw ( int argc, char **argv, struct gacmn *pcm, int which);
static int polyf(char *name, SHPObject *psShape, struct gacmn *pcm, float off);
static int lines(char *name, SHPObject *psShape, struct gacmn *pcm, float off);

/* ........................................................... */

/* Draw polygons from shape file */

int c_polyf ( int argc, char **argv, struct gacmn *pcm) {
  gamscl(pcm);
  gafram(pcm);
  gxclip (pcm->xsiz1, pcm->xsiz2, pcm->ysiz1, pcm->ysiz2);
  return shp_draw ( argc, argv, pcm, POLYF );
}

int c_lines ( int argc, char **argv, struct gacmn *pcm) {
  gamscl(pcm);
  gafram(pcm);
  gxclip (pcm->xsiz1, pcm->xsiz2, pcm->ysiz1, pcm->ysiz2);
  return shp_draw ( argc, argv, pcm, LINES );
}

/* ........................................................... */

/* Draw filed polygons or lines */
static int shp_draw ( int argc, char **argv, struct gacmn *pcm, int which)
{
  SHPHandle hSHP;
  SHPObject *psShape;
  int nShapeType, nshp, nobj;
  double adfMinBound[4], adfMaxBound[4];
  char shpname[1024];
  int rc, narg;

  /* Check args */
  narg = argc - 1;
  if (narg!=1 && narg!=2) 
  {
    sprintf(message,
    "%s: invalid number of arguments; need 1 or 2 but got %d\n", 
     argv[0], narg);
    gaprnt (0,message);
    sprintf(message,"Usage: %s shapefile [object]\n", argv[0]);
    gaprnt (0,message);
    return (1);
  }

  /* Open shapefile: as is or in GADDIR */
  strncpy(shpname,argv[1],1023);
  if ( (hSHP = SHPOpen(shpname,"rb")) == NULL )
  {
    strncpy(shpname,gxgnam(argv[1]),1023);
    hSHP = SHPOpen(shpname,"rb");
  }
  if (hSHP == NULL) 
  {
    sprintf(message,"%s: cannot open shapefile <%s> nor <%s>\n", argv[0], 
            argv[1], gxgnam(argv[1]));
    gaprnt (0,message);
    return (1);
  } 

  SHPGetInfo(hSHP, &nshp, &nShapeType, adfMinBound, adfMaxBound);
  sprintf(message, "Using shapefile %s\n", shpname);
  gaprnt (0,message);
  sprintf(message, "Shapefile Type: %s   # of Shapes: %d\n",
          SHPTypeName( nShapeType ), nshp );
  gaprnt (0,message);
  sprintf(message, "File Bounds: ( %8.3f, %8.3f ) to ( %8.3f, %8.3f )\n",
          adfMinBound[0], adfMinBound[1], adfMaxBound[0], adfMaxBound[1]);
  gaprnt (0,message);

  /* Plot the whole shape file */
  if (narg == 1) 
  {
    int ishp;

    for (ishp = 0; ishp < nshp; ishp ++)
    {
      SHPObject *psShape = NULL;

      psShape = SHPReadObject(hSHP, ishp);
      if (psShape == NULL)
      {
        SHPClose(hSHP);
        return (1);
      }

      if ( which == POLYF ) {
        rc = polyf(argv[0], psShape, pcm, 0.0);
	if ( pcm->dmax[0] > 180.0 ) 
	  rc = polyf(argv[0], psShape, pcm, 360.0);
      }
      else if ( which == LINES ) {
        rc = lines(argv[0], psShape, pcm, 0.0);
	if ( pcm->dmax[0] > 180.0 ) 
	  rc = lines(argv[0], psShape, pcm, 360.0);
      }
      else
        rc = 1;
      if ( rc ) {
        SHPDestroyObject(psShape);
        SHPClose(hSHP);
        return (rc);
      }

      SHPDestroyObject(psShape);
    } /* object loop */ 

    sprintf(message, "Done shapefile %s\n", shpname);
    gaprnt (0,message);

  } /* narg==1 */

  /* Plot only a single object of a given shape object */
  else if (narg == 2)
  {
    /* Get the shape object */
    nobj = atoi(argv[2]);
    sprintf(message, "Doing obj %d\n", nobj);
    gaprnt (0,message);

    if (nobj < 0 || nobj >= nshp)
    {
      SHPClose(hSHP);
      sprintf(message,"%s: invalid object number in shapefile\n",argv[0]);
      gaprnt (0,message);
      return (1);
    }

    psShape = SHPReadObject(hSHP, nobj);

    /* Render this object */
    if ( which == POLYF ) {
      rc = polyf(argv[0], psShape, pcm, 0.0);
      if ( pcm->dmax[0] > 180.0 ) 
           rc = polyf(argv[0], psShape, pcm, 360.0);
    }
    else if ( which == LINES ) {
      rc = lines(argv[0], psShape, pcm, 0.0);
      if ( pcm->dmax[0] > 180.0 ) 
	rc = lines(argv[0], psShape, pcm, 360.0);
    }
    else
      rc = 1;

    if ( rc ) {
      SHPDestroyObject(psShape);
      SHPClose(hSHP);
      return (rc);
    }

  } /* plot object */

  /* All done */
  SHPClose(hSHP);
  return(0);
}

/* ........................................................... */

      static int polyf(char *name, SHPObject *psShape, 
                       struct gacmn *pcm, float off)
{
  int nvert, ivert;
  float shplon, shplat;

  gadouble x, y, *xy;
  int did_alloc, cnt, vcount=0;

  /* We can fill only polygons.... */
  if (psShape->nSHPType != SHPT_POLYGON  &&
      psShape->nSHPType != SHPT_POLYGONZ &&
      psShape->nSHPType != SHPT_POLYGONM) return 0;

  /* Get list of (x,y) coords from this object */
  nvert = psShape->nVertices;
  if (nvert <= 0) return(1);
   
  if ( nvert <= NPTS/2 ) 
  {
    xy = XY; /* use static buffer */
    did_alloc = 0;
  }
  else
  {
    xy = (gadouble *) malloc(2*nvert*sizeof(gadouble));
    if (xy == NULL) 
    {
      sprintf(message,"%s: cannot allocate memory\n", name);
      gaprnt (0,message);
      return (1);
    }
    did_alloc = 1;
  }
  vcount = 0;
  for (ivert = 0; ivert < nvert; ivert ++)
  {
    gxconv(psShape->padfX[ivert]+off, psShape->padfY[ivert],
           xy+vcount, xy+vcount+1, 2);
    vcount += 2;
  }

  /* do the plotting */

  gxcolr (pcm->lincol);
  gxstyl (pcm->linstl);
  gxwide (pcm->linthk);
  if (pcm->ptflg) gxptrn (pcm->ptopt, pcm->ptden, pcm->ptang);
  if ( psShape->nParts == 1)
    gxfill (xy, nvert);
  else
  {
    int pstart, pend, ipart;
    int nparts = psShape->nParts;
    for (ipart = 0; ipart < nparts-1; ipart++)
    {
      pstart=psShape->panPartStart[ipart];
      pend=psShape->panPartStart[ipart+1];
      gxfill(xy+pstart*2, pend-pstart);
    }
    pstart = psShape->panPartStart[nparts-1];
    gxfill(xy+pstart*2, nvert-pstart);
  }
  if (pcm->ptflg) gxptrn (1,1,0);

  if ( did_alloc ) free (xy);

  return(0);
}

/* ........................................................... */

static int lines(char *name, SHPObject *psShape, 
                 struct gacmn *pcm, float off)
{
  int nvert = 0;
  gadouble *xpts = NULL;
  gadouble *ypts = NULL;
  int ivert, ipart;
  int did_alloc;

  nvert = psShape->nVertices;
  if (nvert <= 0) return(1);
 
  if ( nvert <= NPTS/2 ) 
  {
    xpts = XY; /* user static buffer */
    ypts = XY + NPTS/2;
    did_alloc = 0;
  }
  else
  {
    xpts = (gadouble *) malloc(nvert * sizeof(gadouble));
    ypts = (gadouble *) malloc(nvert * sizeof(gadouble));
    if (xpts==NULL)
    {
      sprintf(message,"%s: cannot allocate memory (xpts)\n", name);
      gaprnt (0, message);
      return (1);
    }
    if (ypts==NULL) 
    {
      free(xpts);
      sprintf(message,"%s: cannot allocate memory (ypts)\n", name);
      gaprnt (0, message);
      return (1);
    }
    did_alloc = 1;
  }

  for (ivert = 0; ivert < nvert; ivert ++)
  {
    gxconv(psShape->padfX[ivert]+off, psShape->padfY[ivert],
           xpts+ivert, ypts+ivert, 2);
  }

  /* do the plotting */
  gxcolr (pcm->lincol);
  gxstyl (pcm->linstl);
  gxwide (pcm->linthk);
  for (ivert=0, ipart = 0; ivert<nvert; ivert++)
  {
    if (psShape->panPartStart[ipart] == ivert)
    {
      gxplot(xpts[ivert],ypts[ivert],0);
      gxplot(xpts[ivert],ypts[ivert],1);
      ipart ++;
      continue;
    }
    gxplot(xpts[ivert],ypts[ivert],2);
  }

  /* All done */
  if ( did_alloc ) {
    free (xpts);
    free (ypts);
  }

  return(0);
}

/* ........................................................... */

#endif
