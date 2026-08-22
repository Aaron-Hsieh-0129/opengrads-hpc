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
/* OpenMP calculation controls for GrADS. */

#ifndef GAOMP_H
#define GAOMP_H

#include "gatypes.h"

#define GA_DEFAULT_THREADS 4
#define GA_PARALLEL_MIN_CELLS 32768

void ga_omp_init(void);
gaint ga_omp_set_threads(gaint);
gaint ga_omp_get_threads(void);
gaint ga_omp_enabled(void);
gaint ga_omp_parallelize(gaint);

#endif
