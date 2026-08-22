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
