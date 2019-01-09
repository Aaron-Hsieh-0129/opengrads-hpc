/* 
 * Include ./configure's header file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* -*-Mode: C;-*-
 * Module:      LATS HDF-4 functions
 *
  */

#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "latsint.h"
#include "latstime.h"

#ifdef GOT_NETCDF
#include "hdf4_netcdf.h"
#endif

#define lats_close_nc     lats_close_sd
#define lats_create_nc    lats_create_sd
#define lats_grid_nc      lats_grid_sd
#define lats_var_nc       lats_var_sd
#define lats_vert_dim_nc  lats_vert_dim_sd
#define lats_write_nc     lats_write_sd
#define lats_stub_nc      lats_stub_sd

/* ..................................................................................... */

/* The real code is in this file */
#include "latssdf.h"

