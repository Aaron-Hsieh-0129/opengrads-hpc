/*

    Copyright (C) 2026 Aaron Hsieh <b08209006@ntu.edu.tw>
    All Rights Reserved.

    Added in 2026 as part of opengrads-hpc, a fork of OpenGrADS.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; using version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see file COPYING.

*/
/* Optional OpenMP support for independent grid calculations. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "gaomp.h"

#ifndef USEOPENMP
#define USEOPENMP 0
#endif

#if USEOPENMP == 1
#include <omp.h>
#endif

static gaint ga_omp_threads = GA_DEFAULT_THREADS;
static gaint ga_omp_initialized = 0;

void ga_omp_init(void) {
  const char *value;
  char *end;
  long parsed;

  if (ga_omp_initialized) return;
  ga_omp_initialized = 1;

#if USEOPENMP == 1
  value = getenv("GA_NUM_THREADS");
  if (value != NULL && *value != '\0') {
    errno = 0;
    parsed = strtol(value,&end,10);
    if (errno == 0 && *end == '\0' && parsed >= 1 && parsed <= INT_MAX)
      ga_omp_threads = (gaint)parsed;
  }
  omp_set_dynamic(0);
  omp_set_num_threads(ga_omp_threads);
#else
  value = NULL;
  end = NULL;
  parsed = 0;
  ga_omp_threads = 1;
  (void)value;
  (void)end;
  (void)parsed;
#endif
}

gaint ga_omp_set_threads(gaint threads) {
  ga_omp_init();
  if (threads < 1) return 1;
#if USEOPENMP == 1
  ga_omp_threads = threads;
  omp_set_num_threads(ga_omp_threads);
  return 0;
#else
  if (threads == 1) return 0;
  return 2;
#endif
}

gaint ga_omp_get_threads(void) {
  ga_omp_init();
  return ga_omp_threads;
}

gaint ga_omp_enabled(void) {
#if USEOPENMP == 1
  return 1;
#else
  return 0;
#endif
}

gaint ga_omp_parallelize(gaint cells) {
  ga_omp_init();
  return ga_omp_threads > 1 && cells >= GA_PARALLEL_MIN_CELLS;
}
