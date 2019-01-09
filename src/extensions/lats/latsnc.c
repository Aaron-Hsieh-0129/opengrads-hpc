/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS netCDF functions
 *
 * Copyright:	1996, Regents of the University of California
 *		This software may not be distributed to others without
 *		permission of the author.
 *
 * Author:      Bob Drach, Lawrence Livermore National Laboratory
 *              drach@llnl.gov
 *
 * Version:     $Id: latsnc.c,v 1.8 2009/10/15 03:42:42 dasilva Exp $
 *
 * Revision History:
 *
 * $Log: latsnc.c,v $
 * Revision 1.8  2009/10/15 03:42:42  dasilva
 * ams: working in progress, still cannot run twice in a row
 *
 * Revision 1.7  2009/10/15 01:44:35  dasilva
 * ams: work in progress
 *
 * Revision 1.6  2009/10/15 01:17:53  dasilva
 * ams: work in progress
 *
 * Revision 1.5  2009/10/13 04:22:39  dasilva
 * ams: nc-4 compression seems to work
 *
 * Revision 1.4  2009/10/13 04:07:58  dasilva
 * ams: nc-4 compression seems to work
 *
 * Revision 1.3  2009/10/13 03:34:20  dasilva
 * ams: work in progress
 *
 * Revision 1.2  2009/10/09 17:45:08  dasilva
 * ams: seems to work
 *
 * Revision 1.1  2009/10/05 13:44:26  dasilva
 * ams: porting LATS to grads v2; work in progress
 *
 * Revision 1.7  2008/02/27 08:51:37  pertusus
 * Use hdf4_netcdf.h instead of netcdf.h, if found.
 * Don't set NC_DFLAGS in netcdf_header.m4, but in netcdf.m4.
 *
 * Revision 1.6  2008/01/19 19:24:47  pertusus
 * Use the pkgconfig result unless dap root was set.
 * change <config.h> to "config.h".
 *
 * Revision 1.5  2007/08/26 23:32:03  pertusus
 * Add standard headers includes.
 *
 * Revision 1.4  2007/08/25 02:39:13  dasilva
 * ams: mods for build with new supplibs; changed dods to dap, renamed dodstn.c to dapstn.c
 *
 * Revision 1.2  2002/10/28 19:08:33  joew
 * Preliminary change for 'autonconfiscation' of GrADS: added a conditional
 * #include "config.h" to each C file. The GNU configure script generates a unique config.h for each platform in place of -D arguments to the compiler.
 * The include is only done when GNU configure is used.
 *
 * Revision 1.1.1.1  2002/06/27 19:44:17  cvsadmin
 * initial GrADS CVS import - release 1.8sl10
 *
 * Revision 1.1.1.1  2001/10/18 02:00:57  Administrator
 * Initial repository: v1.8SL8 plus slight MSDOS mods
 *
 *
 * Revision ????  1997/11/06 18:42:00  da Silva
 * - Changed macro HAVE_NETCDF into GOT_NETCDF to avoid comflict
 *   with NCSA's MFHDF (Multi-file HDF) library. NCSA's uses HAVE_NETCDF
 *   for other purposes.
 *
 * Revision 1.15  1997/10/15 17:53:19  drach
 * - remove name collisions with cdunif
 * - only one vertical dimension with GrADS/GRIB
 * - in sync with Mike's GrADS src170
 * - parameter table in sync with standard model output listing
 *
 * Revision 1.1  1997/01/03 00:12:20  fiorino
 * Initial revision
 *
 * Revision 1.14  1996/10/24 22:41:19  drach
 * - Enabled GRIB-only library build
 *
 * Revision 1.13  1996/10/22 19:05:11  fiorino
 * latsgrib bug in .ctl creator
 *
 * Revision 1.12  1996/10/10 23:15:46  drach
 * - lats_create filetype changed to convention, with options LATS_PCMDI,
 *   LATS_GRADS_GRIB, and LATS_NC3.
 * - monthly data defaults to 16-bit compression
 * - LATS_MONTHLY_TABLE_COMP option added to override 16-bit compression
 * - AMIP II standard parameter file
 * - parameter file incorporates GRIB center and subcenter
 * - if time delta is positive, check that (new_time - old_time)=integer*delta
 *
 * Revision 1.11  1996/08/27 19:44:26  drach
 * - Fixed up minor compiler warnings
 *
 * Revision 1.10  1996/08/20 18:34:10  drach
 * - lats_create has a new argument: calendar
 * - lats_grid: longitude, latitude dimension vectors are now double
 *   precision (double, C).
 * - lats_vert_dim: vector of levels is now double precision (double,
 *   C). lats_vert_dim need not be called for single-value/surface
 *   dimensions, if defined in the parameter table. Multi-valued vertical
 *   dimensions, such as pressure levels, must be defined with
 *   lats_vert_dim.
 * - lats_var: set level ID to 0 for implicitly defined surface
 *   dimension.
 * - lats_write: level value is double precision (double, C).
 * - lats_parmtab: replaces routine lats_vartab.
 * - FORTRAN latserropt added: allows program to set error handling
 *   options.
 * - The parameter file format changed.
 *
 * Revision 1.9  1996/07/12 00:36:27  drach
 * - (GRIB) use undefined flag only when set via lats_miss_XX
 * - (GRIB) use delta when checking for missing data
 * - (GRIB) define maximum and default precision
 * - fixed lats_vartab to work correctly.
 * - Added report of routine names, vertical dimension types
 *
 * Revision 1.8  1996/06/27 01:16:49  drach
 * - Check for POSIX compliance
 *
 * Revision 1.7  1996/05/25 00:27:50  drach
 * - Added tables for vertical dimension types, time statistics, originating
 *   centers, and quality control marks
 * - Modified signatures of lats_create and lats_vert_dim
 *
 * Revision 1.6  1996/05/11 00:08:03  fiorino
 * - Added COARDS compliance
 *
 * Revision 1.5  1996/05/10  22:44:42  drach
 * - Initial version before GRIB driver added:
 * - Made grids, vertical dimensions file-independent
 *
 * Revision 1.4  1996/05/04 01:11:12  drach
 * - Added name, units to lats_vert_dim
 * - Added missing data attribute (latsnc.c)
 *
 * Revision 1.3  1996/05/03 18:59:25  drach
 * - Moved vertical dimension definition from lats_var to lats_vert_dim
 * - Changed lats_miss_double to lats_miss_float
 * - Made time dimension file-dependent, revised lats_write accordingly
 * - Added lats_var_nc, lats_vert_dim_nc
 * - Allow GRIB-only compilation
 * - Added FORTRAN interface
 *
 * Revision 1.2  1996/04/25  23:32:08  drach
 * - Added checks for correct number of times, levels written
 * - Stubbed in statistics routines
 *
 * Revision 1.1  1996/04/25 00:53:02  drach
 * Initial repository version
 *
 *
 */

#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "latsint.h"
#include "latstime.h"

#ifdef GOT_NETCDF
# include "netcdf.h"
#endif

/* ..................................................................................... */

/* The real code is in this file */
#include "latssdf.h"

