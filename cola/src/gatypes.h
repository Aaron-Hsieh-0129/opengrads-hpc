/* Modified in 2026 to add a 64-bit HDF5 id type; see COPYING. */
 /************\
 * Data Types * 
 \************/
typedef double        gadouble;
typedef float         gafloat;
typedef int           gaint;
typedef unsigned long gaPixel;
typedef unsigned int  gauint;
typedef long int      galint;
/* Holds an HDF5 hid_t. HDF5 widened hid_t from int to int64_t in 1.10, so a
   gaint truncates it and every id becomes invalid. Declared here rather than
   as hid_t because several signatures are compiled even when HDF5 support is
   not. */
typedef long long     gah5id;

