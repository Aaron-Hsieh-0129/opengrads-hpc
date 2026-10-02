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
/*
 * ADIOS2 BP5 gridded-data backend for OpenGrADS.
 *
 * Added in 2026 for optional ADIOS2 BP5 support. This file is part of
 * OpenGrADS and is distributed under GNU GPL version 2 with the rest of the
 * program; see COPYING. ADIOS2 is an optional, separately distributed
 * dependency. See THIRD_PARTY_NOTICES.md before distributing linked binaries.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <adios2_c.h>

#include "grads.h"
#include "gaomp.h"

#define GA_ADIOS_MAX_DIMS 8

struct gaadios_state {
  adios2_adios *adios;
  adios2_io *io;
  adios2_engine *engine;
  size_t maxsteps;   /* most ADIOS2 steps of any variable without a T dimension */
};

struct gaadios_meta_var {
  const char *name;
  char alias[16];
  adios2_type type;
  size_t rank;
  size_t shape[GA_ADIOS_MAX_DIMS];
  size_t steps;
  char description[161];
  gaint included;
  gaint scalar;          /* a global value: one number per step */
  const char *dims;      /* the descriptor's dimension list */
  size_t levels;         /* the descriptor's level count */
};

/* Names a time coordinate goes by; they are axes, never fields */
static const char *gaadios_time_names[] = {
  "time", "coordinates/time", "Time", "TIME", "times", NULL
};

static char pout[1256];

static const char *gaadios_varname (struct gavar *pvar) {
  if (pvar->longnm[0] != '\0') return pvar->longnm;
  return pvar->abbrv;
}

static gaint gaadios_rank (struct gavar *pvar) {
  return pvar->nh5vardims;
}

static gaint gaadios_numeric_type (adios2_type type) {
  return (type==adios2_type_float ||
          type==adios2_type_double ||
          type==adios2_type_int8_t ||
          type==adios2_type_int16_t ||
          type==adios2_type_int32_t ||
          type==adios2_type_int64_t ||
          type==adios2_type_uint8_t ||
          type==adios2_type_uint16_t ||
          type==adios2_type_uint32_t ||
          type==adios2_type_uint64_t ||
          type==adios2_type_long_double);
}

static size_t gaadios_type_size (adios2_type type) {
  if (type==adios2_type_float) return sizeof(float);
  if (type==adios2_type_double) return sizeof(double);
  if (type==adios2_type_int8_t) return sizeof(int8_t);
  if (type==adios2_type_int16_t) return sizeof(int16_t);
  if (type==adios2_type_int32_t) return sizeof(int32_t);
  if (type==adios2_type_int64_t) return sizeof(int64_t);
  if (type==adios2_type_uint8_t) return sizeof(uint8_t);
  if (type==adios2_type_uint16_t) return sizeof(uint16_t);
  if (type==adios2_type_uint32_t) return sizeof(uint32_t);
  if (type==adios2_type_uint64_t) return sizeof(uint64_t);
  if (type==adios2_type_long_double) return sizeof(long double);
  return 0;
}

static gadouble gaadios_value (void *data, adios2_type type, size_t i) {
  if (type==adios2_type_float) return (gadouble)((float *)data)[i];
  if (type==adios2_type_double) return (gadouble)((double *)data)[i];
  if (type==adios2_type_int8_t) return (gadouble)((int8_t *)data)[i];
  if (type==adios2_type_int16_t) return (gadouble)((int16_t *)data)[i];
  if (type==adios2_type_int32_t) return (gadouble)((int32_t *)data)[i];
  if (type==adios2_type_int64_t) return (gadouble)((int64_t *)data)[i];
  if (type==adios2_type_uint8_t) return (gadouble)((uint8_t *)data)[i];
  if (type==adios2_type_uint16_t) return (gadouble)((uint16_t *)data)[i];
  if (type==adios2_type_uint32_t) return (gadouble)((uint32_t *)data)[i];
  if (type==adios2_type_uint64_t) return (gadouble)((uint64_t *)data)[i];
  if (type==adios2_type_long_double) return (gadouble)((long double *)data)[i];
  return 0.0;
}

static gaint gaadios_string_attribute(adios2_io *io,
                                      const char *variable_name,
                                      const char *attribute_name,
                                      char *value, size_t value_size) {
  adios2_attribute *attribute;
  adios2_bool is_value;
  adios2_type type;
  size_t elements;

  if (!value || value_size<2) return 0;
  if (variable_name)
    attribute = adios2_inquire_variable_attribute(io,attribute_name,
                                                   variable_name,"/");
  else
    attribute = adios2_inquire_attribute(io,attribute_name);
  if (!attribute ||
      adios2_attribute_type(&type,attribute)!=adios2_error_none ||
      type!=adios2_type_string ||
      adios2_attribute_is_value(&is_value,attribute)!=adios2_error_none ||
      is_value!=adios2_true) return 0;

  memset(value,0,value_size);
  elements = 0;
  if (adios2_attribute_data(value,&elements,attribute)!=adios2_error_none ||
      elements!=1) return 0;
  value[value_size-1] = '\0';
  return 1;
}

static gaint gaadios_numeric_attribute(adios2_io *io,
                                       const char *variable_name,
                                       const char *attribute_name,
                                       gadouble *value) {
  adios2_attribute *attribute;
  adios2_bool is_value;
  adios2_type type;
  unsigned char native[sizeof(long double)];
  size_t elements;

  attribute = adios2_inquire_variable_attribute(io,attribute_name,
                                                 variable_name,"/");
  if (!attribute ||
      adios2_attribute_type(&type,attribute)!=adios2_error_none ||
      !gaadios_numeric_type(type) ||
      adios2_attribute_is_value(&is_value,attribute)!=adios2_error_none ||
      is_value!=adios2_true || gaadios_type_size(type)>sizeof(native)) return 0;

  memset(native,0,sizeof(native));
  elements = 0;
  if (adios2_attribute_data(native,&elements,attribute)!=adios2_error_none ||
      elements!=1) return 0;
  *value = gaadios_value(native,type,0);
  return 1;
}

static void gaadios_clean_text(char *text) {
  size_t source, target;
  gaint pending_space;

  target = 0;
  pending_space = 0;
  for (source=0;text[source];source++) {
    if (isspace((unsigned char)text[source]) ||
        !isprint((unsigned char)text[source])) {
      if (target) pending_space = 1;
    }
    else {
      if (pending_space && target<160) text[target++] = ' ';
      pending_space = 0;
      if (target<160) text[target++] = text[source];
    }
  }
  while (target && text[target-1]==' ') target--;
  text[target] = '\0';
}

static void gaadios_make_description(adios2_io *io,
                                     struct gaadios_meta_var *variable) {
  static const char *names[] = {"long_name", "description", "standard_name", NULL};
  char text[4096], units[4096];
  size_t i, used;

  text[0] = '\0';
  for (i=0;names[i];i++) {
    if (gaadios_string_attribute(io,variable->name,names[i],text,
                                 sizeof(text))) break;
  }
  gaadios_clean_text(text);
  if (!text[0]) snprintf(text,sizeof(text),"ADIOS2 variable %s",variable->alias);

  units[0] = '\0';
  if (gaadios_string_attribute(io,variable->name,"units",units,sizeof(units)))
    gaadios_clean_text(units);
  snprintf(variable->description,sizeof(variable->description),"%.160s",text);
  used = strlen(variable->description);
  if (units[0] && used<157)
    snprintf(variable->description+used,sizeof(variable->description)-used,
             " [%.150s]",units);
}

/* The values a variable treats as missing: its two undef values, each with
   GrADS's usual fuzzy tolerance, plus NaN and infinity. */
struct gaadios_missing {
  gadouble low, high, low2, high2;
};

static void gaadios_missing_bounds(struct gavar *pvar, struct gaadios_missing *m) {
  gadouble tolerance;

  tolerance = dequal(pvar->undef,0.0,1.0e-8)==0 ?
              1.0e-5 : fabs(pvar->undef/EPSILON);
  m->low = pvar->undef-tolerance;
  m->high = pvar->undef+tolerance;
  tolerance = dequal(pvar->undef2,0.0,1.0e-8)==0 ?
              1.0e-5 : fabs(pvar->undef2/EPSILON);
  m->low2 = pvar->undef2-tolerance;
  m->high2 = pvar->undef2+tolerance;
}

static inline gaint gaadios_missing_test(const struct gaadios_missing *m,
                                         gadouble value) {
  if (isnan(value) || isinf(value)) return 1;
  if (value>=m->low && value<=m->high) return 1;
  if (value>=m->low2 && value<=m->high2) return 1;
  return 0;
}

static gaint gaadios_is_missing(struct gafile *pfi, struct gavar *pvar,
                                gadouble value) {
  struct gaadios_missing m;

  gaadios_missing_bounds(pvar,&m);
  (void)pfi;
  return gaadios_missing_test(&m,value);
}

static gaint gaadios_expected_size (struct gafile *pfi, struct gavar *pvar,
                                    gadouble unit) {
  if (unit==-100) return pfi->dnum[0];
  if (unit==-101) return pfi->dnum[1];
  if (unit==-102) return pvar->levels;
  if (unit==-103) return pfi->dnum[3];
  if (unit==-104) return pfi->dnum[4];
  return -1;
}

/* Return 1 when the requested T index is currently readable, 0 when the
   descriptor declares a future time that has not been written yet, and -1
   when ADIOS2 metadata cannot be queried. Return 2 for a variable written
   once in a dataset with more steps, such as terrain or a reference
   profile: it holds for every time, and its one step is the one to read. */
static gaint gaadios_time_available (struct gafile *pfi, struct gavar *pvar,
                                     adios2_variable *variable, gaint t) {
  struct gaadios_state *state;
  adios2_shapeid shapeid;
  size_t shape[GA_ADIOS_MAX_DIMS], steps;
  gaint i, rank;

  if (t<1) return 0;
  if (adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none) return -1;
  rank = gaadios_rank(pvar);
  if (shapeid!=adios2_shapeid_global_value) {
    for (i=0;i<rank;i++) {
      if (pvar->units[i]==-103) {
        if (adios2_variable_shape(shape,variable)!=adios2_error_none) return -1;
        return (size_t)t<=shape[i];
      }
    }
  }
  if (adios2_variable_steps(&steps,variable)!=adios2_error_none) return -1;
  if ((size_t)t<=steps) return 1;
  state = (struct gaadios_state *)pfi->adios2;
  if (steps==1 && state && state->maxsteps>1 && t<=pfi->dnum[3]) return 2;
  return 0;
}

static void gaadios_set_undefined (struct gafile *pfi, size_t count,
                                   gadouble *gr, char *gru) {
  size_t i;
  for (i=0;i<count;i++) {
    gr[i] = pfi->undef;
    gru[i] = 0;
  }
}

static void gaadios_make_alias(struct gaadios_meta_var *vars, size_t current) {
  const char *source, *slash;
  char base[16], candidate[16], tail[16];
  size_t i, j, limit, length;
  gaint suffix, collision;

  source = vars[current].name;
  slash = strrchr(source,'/');
  if (slash && slash[1]) source = slash+1;
  j = 0;
  if (!isalpha((unsigned char)source[0])) base[j++] = 'v';
  for (i=0;source[i] && j<15;i++) {
    if (isalnum((unsigned char)source[i]) || source[i]=='_')
      base[j++] = (char)tolower((unsigned char)source[i]);
  }
  if (j==0) base[j++] = 'v';
  base[j] = '\0';
  snprintf(candidate,sizeof(candidate),"%s",base);

  suffix = 2;
  while (1) {
    collision = 0;
    for (i=0;i<current;i++) {
      if (vars[i].included && !strcmp(candidate,vars[i].alias)) {
        collision = 1;
        break;
      }
    }
    if (!collision) break;
    /*
     * Cut the stem to whatever the suffix leaves, rather than to a width
     * guessed from the suffix's size: a guess that is too small gets the
     * suffix trimmed instead, which repeats an earlier candidate and leaves
     * this loop with no way to terminate.
     */
    snprintf(tail,sizeof(tail),"_%d",suffix++);
    length = strlen(tail);
    if (length>sizeof(candidate)-2) length = sizeof(candidate)-2;
    limit = sizeof(candidate)-1-length;
    if (limit>strlen(base)) limit = strlen(base);
    memcpy(candidate,base,limit);
    memcpy(candidate+limit,tail,length);
    candidate[limit+length] = '\0';
  }
  snprintf(vars[current].alias,sizeof(vars[current].alias),"%s",candidate);
}

/*
 * The synthesized descriptor is parsed by the ordinary GrADS reader, so a BP
 * variable name is usable only when it can be written as the long name of a
 * VARS record: it must fit the longnm field, must not start a record that the
 * parser takes for a comment, and must not contain anything the parser treats
 * as a separator or rewrites (it turns '~' into a space).
 */
static gaint gaadios_usable_name(const char *name) {
  size_t i;

  if (!name || !name[0]) return 0;
  if (strlen(name)>256) return 0;
  if (!isalnum((unsigned char)name[0]) && name[0]!='/') return 0;
  for (i=0;name[i];i++) {
    if (isspace((unsigned char)name[i]) ||
        !isprint((unsigned char)name[i]) ||
        name[i]=='~') return 0;
    if (name[i]=='=' && name[i+1]=='>') return 0;
  }
  return 1;
}

static gaint gaadios_has_bp5_metadata(const char *pathname) {
  char marker[4096];
  struct stat status;

  if (snprintf(marker,sizeof(marker),"%s/md.idx",pathname)>=(int)sizeof(marker))
    return 0;
  return stat(marker,&status)==0 && S_ISREG(status.st_mode);
}

/*
 * Accept either a BP5 directory itself or a directory containing exactly one
 * *.bp child.  The latter is useful for model-output directories such as the
 * RCEMIP fixture, whose actual ADIOS2 dataset is vvm_output.bp.
 */
static gaint gaadios_resolve_path(const char *requested, char *pathname,
                                  size_t pathname_size) {
  DIR *directory;
  struct dirent *entry;
  struct stat status;
  char candidate[4096], match[4096];
  size_t length;
  gaint matches;

  if (stat(requested,&status)!=0 || !S_ISDIR(status.st_mode) ||
      gaadios_has_bp5_metadata(requested)) {
    if (snprintf(pathname,pathname_size,"%s",requested)>=(int)pathname_size)
      return 1;
    return 0;
  }

  directory = opendir(requested);
  if (!directory) return 1;
  matches = 0;
  while ((entry=readdir(directory))!=NULL) {
    length = strlen(entry->d_name);
    if (length<3 || strcmp(entry->d_name+length-3,".bp")) continue;
    if (snprintf(candidate,sizeof(candidate),"%s/%s",requested,entry->d_name)
        >=(int)sizeof(candidate)) continue;
    if (!gaadios_has_bp5_metadata(candidate)) continue;
    matches++;
    snprintf(match,sizeof(match),"%s",candidate);
  }
  closedir(directory);

  if (matches==0) {
    gaprnt(0,"BPOPEN error: directory is not a BP5 dataset and contains no BP5 child\n");
    return 1;
  }
  if (matches>1) {
    gaprnt(0,"BPOPEN error: directory contains multiple BP5 children; specify one explicitly\n");
    return 1;
  }
  if (snprintf(pathname,pathname_size,"%s",match)>=(int)pathname_size) return 1;
  gaprnt(2,"Resolved BP5 dataset: ");
  gaprnt(2,pathname);
  gaprnt(2,"\n");
  return 0;
}

/* Read a 1-D coordinate array. On success *found names the variable used,
   so the caller can look at its attributes. */
static gadouble *gaadios_read_axis(adios2_io *io, adios2_engine *engine,
                                   const char **names, size_t expected,
                                   const char **found) {
  adios2_variable *variable;
  adios2_shapeid shapeid;
  adios2_type type;
  adios2_error error;
  gadouble *values;
  void *native;
  size_t shape[1], start[1], count[1], bytes, i, ndims;

  *found = NULL;
  while (*names) {
    variable = adios2_inquire_variable(io,*names);
    if (variable) break;
    names++;
  }
  if (!*names) return NULL;
  if (adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none ||
      shapeid!=adios2_shapeid_global_array ||
      adios2_variable_type(&type,variable)!=adios2_error_none ||
      !gaadios_numeric_type(type) ||
      adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      ndims!=1 ||
      adios2_variable_shape(shape,variable)!=adios2_error_none ||
      shape[0]!=expected) return NULL;

  native = galloc(expected*gaadios_type_size(type),"adios2axisnative");
  values = (gadouble *)galloc(expected*sizeof(gadouble),"adios2axis");
  if (!native || !values) {
    if (native) gree(native,"adios2axisnative");
    if (values) gree(values,"adios2axis");
    return NULL;
  }
  start[0] = 0;
  count[0] = expected;
  error = adios2_set_selection(variable,1,start,count);
  if (error==adios2_error_none)
    error = adios2_set_step_selection(variable,0,1);
  if (error==adios2_error_none)
    error = adios2_get(engine,variable,native,adios2_mode_sync);
  if (error!=adios2_error_none) {
    gree(native,"adios2axisnative");
    gree(values,"adios2axis");
    return NULL;
  }
  bytes = expected;
  for (i=0;i<bytes;i++) values[i] = gaadios_value(native,type,i);
  gree(native,"adios2axisnative");
  *found = *names;
  return values;
}

/*
 * GrADS has no Cartesian horizontal axes: X and Y are longitude and latitude
 * in degrees, and area weighting, the spherical derivatives (hdivg, hcurl), and
 * map drawing all assume it. A descriptor for a Cartesian model therefore
 * writes X and Y as small degree offsets on GrADS's own sphere, centred on 0
 * so that cos(latitude) stays 1 across the domain. A descriptor-free open does
 * the same, so it behaves like that descriptor rather than like metres read as
 * degrees.
 *
 * The radius is the 6.37e6 m GrADS uses internally. With it, the functions
 * that turn degrees back into distance recover the original grid spacing.
 */
#define GA_ADIOS_EARTH_RADIUS 6.37e6

/* Metres per unit when a coordinate's units attribute names a length, or 0. */
static gadouble gaadios_length_unit(adios2_io *io, const char *name) {
  char units[4096];
  size_t i;

  if (!name || !gaadios_string_attribute(io,name,"units",units,sizeof(units)))
    return 0.0;
  gaadios_clean_text(units);
  for (i=0;units[i];i++) units[i] = (char)tolower((unsigned char)units[i]);
  if (!strcmp(units,"m") || !strcmp(units,"meter") ||
      !strcmp(units,"meters") || !strcmp(units,"metre") ||
      !strcmp(units,"metres")) return 1.0;
  if (!strcmp(units,"km") || !strcmp(units,"kilometer") ||
      !strcmp(units,"kilometers") || !strcmp(units,"kilometre") ||
      !strcmp(units,"kilometres")) return 1000.0;
  return 0.0;
}

static void gaadios_map_cartesian(gadouble *values, size_t count,
                                  gadouble metres_per_unit) {
  gadouble centre, scale;
  size_t i;

  centre = 0.5*(values[0]+values[count-1]);
  scale = metres_per_unit /
          (GA_ADIOS_EARTH_RADIUS*3.14159265358979323846/180.0);
  for (i=0;i<count;i++) values[i] = (values[i]-centre)*scale;
}

/* Proleptic Gregorian calendar arithmetic on whole days since 1970-01-01. */
static long long gaadios_days_from_civil(long long y, unsigned m, unsigned d) {
  long long era;
  unsigned yoe, doy, doe;

  y -= m<=2;
  era = (y>=0 ? y : y-399)/400;
  yoe = (unsigned)(y-era*400);
  doy = (153*(m>2 ? m-3 : m+9)+2)/5+d-1;
  doe = yoe*365+yoe/4-yoe/100+doy;
  return era*146097+(long long)doe-719468;
}

static void gaadios_civil_from_days(long long z, long long *y,
                                    unsigned *m, unsigned *d) {
  long long era;
  unsigned doe, yoe, doy, mp;

  z += 719468;
  era = (z>=0 ? z : z-146096)/146097;
  doe = (unsigned)(z-era*146097);
  yoe = (doe-doe/1460+doe/36524-doe/146096)/365;
  doy = doe-(365*yoe+yoe/4-yoe/100);
  mp = (5*doy+2)/153;
  *d = doy-(153*mp+2)/5+1;
  *m = mp<10 ? mp+3 : mp-9;
  *y = (long long)yoe+era*400+(*m<=2);
}

/* Parse CF time units, "<unit> since YYYY-MM-DD[ hh:mm[:ss]]", into the
   length of one unit in seconds and the epoch in seconds since 1970. */
static gaint gaadios_parse_time_units(const char *units, gadouble *unit_seconds,
                                      long long *epoch) {
  char word[32];
  const char *s;
  int y, mo, d, h, mi, n;
  double sec;
  size_t i;

  s = units;
  while (*s && isspace((unsigned char)*s)) s++;
  for (i=0; *s && !isspace((unsigned char)*s) && i<sizeof(word)-1; i++, s++)
    word[i] = (char)tolower((unsigned char)*s);
  word[i] = '\0';
  if (!strcmp(word,"seconds") || !strcmp(word,"second") ||
      !strcmp(word,"secs") || !strcmp(word,"sec") || !strcmp(word,"s"))
    *unit_seconds = 1.0;
  else if (!strcmp(word,"minutes") || !strcmp(word,"minute") ||
           !strcmp(word,"mins") || !strcmp(word,"min"))
    *unit_seconds = 60.0;
  else if (!strcmp(word,"hours") || !strcmp(word,"hour") ||
           !strcmp(word,"hrs") || !strcmp(word,"hr") || !strcmp(word,"h"))
    *unit_seconds = 3600.0;
  else if (!strcmp(word,"days") || !strcmp(word,"day") || !strcmp(word,"d"))
    *unit_seconds = 86400.0;
  else return 0;

  while (*s && isspace((unsigned char)*s)) s++;
  if (strncmp(s,"since",5) && strncmp(s,"SINCE",5) && strncmp(s,"Since",5))
    return 0;
  s += 5;
  y = mo = d = h = mi = 0;
  sec = 0.0;
  n = sscanf(s," %d-%d-%d%*[ T]%d:%d:%lf",&y,&mo,&d,&h,&mi,&sec);
  if (n<3 || mo<1 || mo>12 || d<1 || d>31 || h<0 || h>23 || mi<0 || mi>59 ||
      sec<0.0 || sec>=61.0) return 0;
  *epoch = gaadios_days_from_civil(y,(unsigned)mo,(unsigned)d)*86400LL +
           h*3600LL + mi*60LL + (long long)floor(sec+0.5);
  return 1;
}

/*
 * Build a TDEF line from a CF time coordinate: a variable named time (or
 * coordinates/time) holding one value per step, or a 1-D array of them, with
 * units "<unit> since <date>". Returns 1 with the line in tdef, or 0 when the
 * dataset has no usable time coordinate; either way note says what happened,
 * for the message printed after the open.
 */
static gaint gaadios_time_axis(adios2_io *io, adios2_engine *engine,
                               size_t steps, char *tdef, size_t tdef_size,
                               char *note, size_t note_size) {
  const char **names = gaadios_time_names;
  static const char *months[] = {
    "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"
  };
  adios2_variable *variable;
  adios2_shapeid shapeid;
  adios2_type type;
  char units[4096], calendar[4096], increment[32];
  const char *name;
  void *native;
  gadouble *values, unit_seconds, interval, minutes, gap;
  long long epoch, start, days, year, count;
  unsigned month, day;
  size_t n, ndims, shape[GA_ADIOS_MAX_DIMS], sel_start[1], sel_count[1], i;
  gaint uneven, rounded, hour, minute;

  native = NULL;
  values = NULL;
  for (i=0; names[i]; i++) {
    variable = adios2_inquire_variable(io,names[i]);
    if (variable) break;
  }
  if (!names[i]) {
    snprintf(note,note_size,
             "No time coordinate found; T counts steps, labelled in 1-minute "
             "intervals from 00Z01JAN2000\n");
    return 0;
  }
  name = names[i];

  if (!gaadios_string_attribute(io,name,"units",units,sizeof(units)) ||
      !gaadios_parse_time_units(units,&unit_seconds,&epoch)) {
    snprintf(note,note_size,
             "Time variable '%.100s' has no units of the form '<unit> since "
             "<date>'; T counts steps, labelled in 1-minute intervals from "
             "00Z01JAN2000\n",name);
    return 0;
  }
  if (gaadios_string_attribute(io,name,"calendar",calendar,sizeof(calendar))) {
    gaadios_clean_text(calendar);
    for (i=0;calendar[i];i++)
      calendar[i] = (char)tolower((unsigned char)calendar[i]);
    if (strcmp(calendar,"standard") && strcmp(calendar,"gregorian") &&
        strcmp(calendar,"proleptic_gregorian")) {
      snprintf(note,note_size,
               "Time variable '%.100s' uses the '%.40s' calendar, which a "
               "descriptor-free open does not handle; T counts steps, labelled "
               "in 1-minute intervals from 00Z01JAN2000\n",name,calendar);
      return 0;
    }
  }

  if (adios2_variable_type(&type,variable)!=adios2_error_none ||
      !gaadios_numeric_type(type) ||
      adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none)
    goto unusable;
  if (shapeid==adios2_shapeid_global_value) {
    if (adios2_variable_steps(&n,variable)!=adios2_error_none) goto unusable;
  }
  else if (shapeid==adios2_shapeid_global_array) {
    if (adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
        ndims!=1 ||
        adios2_variable_shape(shape,variable)!=adios2_error_none)
      goto unusable;
    n = shape[0];
  }
  else goto unusable;
  if (n==0) goto unusable;
  if (n>steps) n = steps;

  native = galloc(n*gaadios_type_size(type),"adios2timenative");
  values = (gadouble *)galloc(n*sizeof(gadouble),"adios2time");
  if (!native || !values) goto unusable;
  if (shapeid==adios2_shapeid_global_value) {
    if (adios2_set_step_selection(variable,0,n)!=adios2_error_none)
      goto unusable;
  }
  else {
    sel_start[0] = 0;
    sel_count[0] = n;
    if (adios2_set_selection(variable,1,sel_start,sel_count)!=adios2_error_none ||
        adios2_set_step_selection(variable,0,1)!=adios2_error_none)
      goto unusable;
  }
  if (adios2_get(engine,variable,native,adios2_mode_sync)!=adios2_error_none)
    goto unusable;
  for (i=0;i<n;i++) values[i] = gaadios_value(native,type,i);

  /* A single step has no interval; any increment labels it the same. */
  interval = n>1 ? (values[1]-values[0])*unit_seconds : 60.0;
  if (!(interval>0.0)) {
    snprintf(note,note_size,
             "Time variable '%.100s' does not increase; T counts steps, "
             "labelled in 1-minute intervals from 00Z01JAN2000\n",name);
    goto fallback;
  }
  minutes = interval/60.0;
  if (fabs(minutes-floor(minutes+0.5))>1.0e-6*minutes || minutes<0.5) {
    snprintf(note,note_size,
             "Time variable '%.100s' steps by %g s, which is not a whole number "
             "of minutes and so cannot be a GrADS TDEF increment; T counts "
             "steps, labelled in 1-minute intervals from 00Z01JAN2000\n",
             name,interval);
    goto fallback;
  }
  count = (long long)floor(minutes+0.5);
  uneven = 0;
  for (i=2;i<n;i++) {
    gap = (values[i]-values[i-1])*unit_seconds;
    if (fabs(gap-interval)>1.0e-6*interval) { uneven = 1; break; }
  }

  start = epoch + (long long)floor(values[0]*unit_seconds+0.5);
  rounded = (start%60)!=0;
  start = (long long)floor((double)start/60.0+0.5)*60;
  days = start>=0 ? start/86400 : -((-start+86399)/86400);
  gaadios_civil_from_days(days,&year,&month,&day);
  hour = (gaint)((start-days*86400)/3600);
  minute = (gaint)(((start-days*86400)%3600)/60);

  if (count%1440==0) snprintf(increment,sizeof(increment),"%llddy",count/1440);
  else if (count%60==0) snprintf(increment,sizeof(increment),"%lldhr",count/60);
  else snprintf(increment,sizeof(increment),"%lldmn",count);

  snprintf(tdef,tdef_size,"tdef %lu linear %02d:%02dZ%02u%s%04lld %s",
           (unsigned long)steps,hour,minute,day,months[month-1],year,increment);
  snprintf(note,note_size,
           "T from '%.100s': %lu steps from %02d:%02dZ%02u%s%04lld every %s%s%s\n",
           name,(unsigned long)steps,hour,minute,day,months[month-1],year,
           increment,
           rounded ? "; the first time is rounded to the nearest minute" : "",
           uneven ? "; the steps are NOT evenly spaced, and T is labelled "
                    "with the first interval" : "");
  gree(native,"adios2timenative");
  gree(values,"adios2time");
  return 1;

unusable:
  snprintf(note,note_size,
           "Time variable '%.100s' could not be read as one value per step; T "
           "counts steps, labelled in 1-minute intervals from 00Z01JAN2000\n",
           name);
fallback:
  if (native) gree(native,"adios2timenative");
  if (values) gree(values,"adios2time");
  return 0;
}

static gaint gaadios_axis_is_linear(gadouble *values, size_t count,
                                    gadouble *start, gadouble *increment) {
  gadouble expected, scale, tolerance;
  size_t i;
  if (!values || count<2) return 0;
  *start = values[0];
  *increment = values[1]-values[0];
  scale = fabs(*start)+fabs(*increment)*(gadouble)count+1.0;
  tolerance = scale*1.0e-9;
  for (i=2;i<count;i++) {
    expected = *start + *increment*(gadouble)i;
    if (fabs(values[i]-expected)>tolerance) return 0;
  }
  return 1;
}

static void gaadios_write_axis(FILE *descriptor, const char *dimension,
                               size_t count, gadouble *values) {
  gadouble start, increment;
  size_t i;
  if (!values || count==1) {
    start = values ? values[0] : 1.0;
    fprintf(descriptor,"%sdef %lu linear %.17g 1\n",dimension,
            (unsigned long)count,start);
  }
  else if (gaadios_axis_is_linear(values,count,&start,&increment) &&
           increment>0.0) {
    fprintf(descriptor,"%sdef %lu linear %.17g %.17g\n",dimension,
            (unsigned long)count,start,increment);
  }
  else {
    fprintf(descriptor,"%sdef %lu levels",dimension,(unsigned long)count);
    for (i=0;i<count;i++) {
      if (i && i%8==0) fprintf(descriptor,"\n");
      fprintf(descriptor," %.17g",values[i]);
    }
    fprintf(descriptor,"\n");
  }
}

/* Whether a name is one of the axis coordinates, which are never fields. */
static gaint gaadios_is_axis_name(const char *name, const char **xnames,
                                  const char **ynames, const char **znames) {
  const char **lists[4];
  gaint i, j;

  if (!strncmp(name,"coordinates/",12)) return 1;
  lists[0] = xnames;
  lists[1] = ynames;
  lists[2] = znames;
  lists[3] = gaadios_time_names;
  for (i=0;i<4;i++)
    for (j=0;lists[i][j];j++)
      if (!strcmp(name,lists[i][j])) return 1;
  return 0;
}

/*
 * Scan a BP5 file and synthesize the descriptor metadata in a temporary file.
 * The temporary descriptor is an implementation detail: it is unlinked as
 * soon as gaopen has populated the normal GrADS structures.
 */
gaint gaadios_bpopen(char *args, struct gacmn *pcm) {
  static const char *xnames[] = {
    "coordinates/x", "x", "lon", "longitude", NULL
  };
  static const char *ynames[] = {
    "coordinates/y", "y", "lat", "latitude", NULL
  };
  static const char *znames[] = {
    "coordinates/z_mid", "coordinates/z", "z", "lev", "level", "height", NULL
  };
  adios2_adios *adios;
  adios2_io *io;
  adios2_engine *engine;
  adios2_variable *variable;
  adios2_shapeid shapeid;
  struct gaadios_meta_var *vars;
  char **names;
  char requested[4096], pathname[4096], title[4096];
  char tdefline[256], timenote[512];
  const char *xname, *yname, *zname;
  gadouble xmetres, ymetres;
  gaint havetime;
  char temporary[] = "/tmp/opengrads-bp5-XXXXXX";
  FILE *descriptor;
  gadouble *xvalues, *yvalues, *zvalues;
  size_t name_count, i, j, elements, best_elements, reference;
  size_t xsize, ysize, zsize, steps, included, oned, len;
  char onednames[512];
  gaint matches;
  int fd;
  gaint rc, have_reference;

  adios = NULL;
  io = NULL;
  engine = NULL;
  names = NULL;
  name_count = 0;
  vars = NULL;
  descriptor = NULL;
  xvalues = yvalues = zvalues = NULL;
  title[0] = '\0';
  fd = -1;
  rc = 1;
  getwrd(requested,args,4095);
  if (!requested[0]) {
    gaprnt(0,"BPOPEN error: missing BP5 dataset pathname\n");
    return 1;
  }
  if (strlen(requested)>400) {
    gaprnt(0,"BPOPEN error: dataset pathname is too long\n");
    return 1;
  }
  for (i=0;requested[i];i++) {
    if (isspace((unsigned char)requested[i])) {
      gaprnt(0,"BPOPEN error: paths containing whitespace are not supported\n");
      return 1;
    }
  }
  if (gaadios_resolve_path(requested,pathname,sizeof(pathname))) return 1;

  gaprnt(2,"Scanning BP5 metadata: ");
  gaprnt(2,pathname);
  gaprnt(2,"\n");
  adios = adios2_init_serial();
  if (!adios) goto metadata_error;
  io = adios2_declare_io(adios,"opengrads_bp5_discovery");
  if (!io) goto metadata_error;
  engine = adios2_open(io,pathname,adios2_mode_readRandomAccess);
  if (!engine) goto metadata_error;
  names = adios2_available_variables(io,&name_count);
  if (!names || name_count==0) {
    gaprnt(0,"BPOPEN error: BP5 dataset contains no variables\n");
    goto cleanup;
  }
  vars = (struct gaadios_meta_var *)galloc(
      name_count*sizeof(struct gaadios_meta_var),"adios2metadata");
  if (!vars) {
    gaprnt(0,"BPOPEN error: unable to allocate metadata table\n");
    goto cleanup;
  }
  memset(vars,0,name_count*sizeof(struct gaadios_meta_var));

  reference = 0;
  best_elements = 0;
  have_reference = 0;
  for (i=0;i<name_count;i++) {
    vars[i].name = names[i];
    variable = adios2_inquire_variable(io,names[i]);
    if (!variable ||
        adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none ||
        adios2_variable_type(&vars[i].type,variable)!=adios2_error_none ||
        !gaadios_numeric_type(vars[i].type))
      continue;
    if (shapeid==adios2_shapeid_global_value) {
      /* one number per step; a time series when there is more than one */
      adios2_variable_steps(&vars[i].steps,variable);
      vars[i].scalar = 1;
      continue;
    }
    if (shapeid!=adios2_shapeid_global_array ||
        adios2_variable_ndims(&vars[i].rank,variable)!=adios2_error_none ||
        vars[i].rank<1 || vars[i].rank>3 ||
        adios2_variable_shape(vars[i].shape,variable)!=adios2_error_none) {
      vars[i].rank = 0;
      continue;
    }
    adios2_variable_steps(&vars[i].steps,variable);
    if (vars[i].rank==1) continue;   /* matched to an axis once the grid is known */
    elements = 1;
    for (j=0;j<vars[i].rank;j++) elements *= vars[i].shape[j];
    if (!have_reference ||
        (vars[i].rank==3 && vars[reference].rank!=3) ||
        (vars[i].rank==vars[reference].rank && elements>best_elements)) {
      reference = i;
      best_elements = elements;
      have_reference = 1;
    }
  }
  if (have_reference) {
    xsize = vars[reference].shape[vars[reference].rank-1];
    ysize = vars[reference].shape[vars[reference].rank-2];
    zsize = vars[reference].rank==3 ? vars[reference].shape[0] : 1;
  }
  else {
    /* No 2-D or 3-D field: a column of profiles on a Z coordinate, or time
       series alone. X and Y are single points. */
    xsize = ysize = zsize = 1;
    for (j=0;znames[j];j++) {
      variable = adios2_inquire_variable(io,znames[j]);
      if (variable &&
          adios2_variable_shapeid(&shapeid,variable)==adios2_error_none &&
          shapeid==adios2_shapeid_global_array &&
          adios2_variable_ndims(&elements,variable)==adios2_error_none &&
          elements==1 &&
          adios2_variable_shape(&elements,variable)==adios2_error_none) {
        zsize = elements;
        break;
      }
    }
  }
  steps = 1;
  included = 0;
  oned = 0;
  onednames[0] = '\0';
  for (i=0;i<name_count;i++) {
    if (vars[i].rank==2 &&
        vars[i].shape[0]==ysize && vars[i].shape[1]==xsize) {
      vars[i].included = 1;
      vars[i].dims = "y,x";
      vars[i].levels = 0;
    }
    else if (vars[i].rank==3 &&
             vars[i].shape[0]==zsize && vars[i].shape[1]==ysize &&
             vars[i].shape[2]==xsize) {
      vars[i].included = 1;
      vars[i].dims = "z,y,x";
      vars[i].levels = zsize;
    }
    else if ((vars[i].rank==1 || (vars[i].scalar && vars[i].steps>1)) &&
             !gaadios_is_axis_name(vars[i].name,xnames,ynames,znames)) {
      /* A 1-D field is a profile along whichever axis its length matches,
         and is the same along the others; a global value written every step
         is a time series. A length that fits two axes is left out rather
         than guessed. */
      if (vars[i].scalar) {
        vars[i].dims = "t";
        vars[i].levels = 0;
      }
      else {
        matches = (zsize>1 && vars[i].shape[0]==zsize) +
                  (ysize>1 && vars[i].shape[0]==ysize) +
                  (xsize>1 && vars[i].shape[0]==xsize);
        if (matches>1) {
          snprintf(pout,1255,
                   "BPOPEN warning: skipping '%.200s'; its length %lu fits more than one "
                   "axis, so it needs a descriptor\n",
                   vars[i].name,(unsigned long)vars[i].shape[0]);
          gaprnt(1,pout);
          continue;
        }
        if (matches==0) continue;
        if (zsize>1 && vars[i].shape[0]==zsize) {
          vars[i].dims = "z";
          vars[i].levels = zsize;
        }
        else {
          vars[i].dims = (ysize>1 && vars[i].shape[0]==ysize) ? "y" : "x";
          vars[i].levels = 0;
        }
      }
      vars[i].included = 1;
    }
    if (vars[i].included) {
      if (!gaadios_usable_name(vars[i].name)) {
        vars[i].included = 0;
        snprintf(pout,1255,
                 "BPOPEN warning: skipping '%.200s'; its name cannot be written in a descriptor\n",
                 vars[i].name);
        gaprnt(1,pout);
        continue;
      }
      gaadios_make_alias(vars,i);
      gaadios_make_description(io,&vars[i]);
      if (vars[i].steps>steps) steps = vars[i].steps;
      included++;
      if (vars[i].rank<2) {
        len = strlen(onednames);
        if (oned<8 && len+strlen(vars[i].alias)+8<sizeof(onednames))
          snprintf(onednames+len,sizeof(onednames)-len,"%s%s(%s)",
                   oned ? ", " : "",vars[i].alias,vars[i].dims);
        oned++;
      }
    }
  }
  if (!included) {
    if (have_reference)
      gaprnt(0,"BPOPEN error: no fields match the inferred horizontal grid\n");
    else
      gaprnt(0,"BPOPEN error: no numeric 2-D or 3-D global arrays, profiles on a Z "
               "coordinate, or time series were found\n");
    goto cleanup;
  }

  xvalues = gaadios_read_axis(io,engine,xnames,xsize,&xname);
  yvalues = gaadios_read_axis(io,engine,ynames,ysize,&yname);
  if (zsize>1) zvalues = gaadios_read_axis(io,engine,znames,zsize,&zname);

  /* Z stays in the dataset's own units, as a descriptor writes it; X and Y
     in a length unit become degrees, as a descriptor for a Cartesian model
     writes them. */
  xmetres = xvalues ? gaadios_length_unit(io,xname) : 0.0;
  ymetres = yvalues ? gaadios_length_unit(io,yname) : 0.0;
  if (xmetres>0.0) gaadios_map_cartesian(xvalues,xsize,xmetres);
  if (ymetres>0.0) gaadios_map_cartesian(yvalues,ysize,ymetres);

  havetime = gaadios_time_axis(io,engine,steps,tdefline,sizeof(tdefline),
                               timenote,sizeof(timenote));

  if (gaadios_string_attribute(io,NULL,"title",title,sizeof(title)))
    gaadios_clean_text(title);
  fd = mkstemp(temporary);
  if (fd<0 || !(descriptor=fdopen(fd,"w"))) {
    gaprnt(0,"BPOPEN error: unable to create temporary metadata descriptor\n");
    if (fd>=0) close(fd);
    fd = -1;
    goto cleanup;
  }
  fd = -1;
  fprintf(descriptor,"dset %s\n",pathname);
  fprintf(descriptor,"dtype bp5\n");
  if (title[0]) fprintf(descriptor,"title %.4000s\n",title);
  else fprintf(descriptor,"title Self-describing ADIOS2 BP5 dataset: %s\n",pathname);
  fprintf(descriptor,"undef -9.99e33 _FillValue missing_value\n");
  gaadios_write_axis(descriptor,"x",xsize,xvalues);
  gaadios_write_axis(descriptor,"y",ysize,yvalues);
  gaadios_write_axis(descriptor,"z",zsize,zvalues);
  if (havetime) fprintf(descriptor,"%s\n",tdefline);
  else fprintf(descriptor,"tdef %lu linear 00z01jan2000 1mn\n",
               (unsigned long)steps);
  fprintf(descriptor,"vars %lu\n",(unsigned long)included);
  for (i=0;i<name_count;i++) {
    if (!vars[i].included) continue;
    fprintf(descriptor,"%s=>%s %lu %s %s\n",
            vars[i].name,vars[i].alias,(unsigned long)vars[i].levels,
            vars[i].dims,vars[i].description);
  }
  fprintf(descriptor,"endvars\n");
  if (fclose(descriptor)!=0) {
    descriptor = NULL;
    gaprnt(0,"BPOPEN error: unable to finish temporary metadata descriptor\n");
    goto cleanup;
  }
  descriptor = NULL;

  adios2_close(engine);
  engine = NULL;
  adios2_finalize(adios);
  adios = NULL;
  rc = gaopen(temporary,pcm);
  if (!rc) {
    struct gafile *opened;
    opened = pcm->pfi1;
    while (opened && opened->pforw) opened = opened->pforw;
    if (opened)
      snprintf(opened->dnam,sizeof(opened->dnam),"BP5 metadata: %.4081s",pathname);
    snprintf(pout,1255,
             "BP5 dataset opened without a descriptor: %lu fields, %lux%lux%lu, %lu steps\n",
             (unsigned long)included,(unsigned long)xsize,(unsigned long)ysize,
             (unsigned long)zsize,(unsigned long)steps);
    gaprnt(2,pout);
    if (xmetres>0.0 || ymetres>0.0) {
      snprintf(pout,1255,
               "%s %s Cartesian; mapped to degrees on GrADS's %.0f km sphere, "
               "centred on 0, as a descriptor for a Cartesian model writes them\n",
               xmetres>0.0 && ymetres>0.0 ? "X and Y" : (xmetres>0.0 ? "X" : "Y"),
               xmetres>0.0 && ymetres>0.0 ? "are" : "is",
               GA_ADIOS_EARTH_RADIUS/1000.0);
      gaprnt(2,pout);
    }
    gaprnt(havetime ? 2 : 1,timenote);
    if (oned) {
      snprintf(pout,1255,"1-D fields, the same along the axes they lack: %s%s\n",
               onednames,oned>8 ? ", ..." : "");
      gaprnt(2,pout);
    }
  }
  goto cleanup;

metadata_error:
  snprintf(pout,1255,"BPOPEN error: unable to read ADIOS2 metadata from %.1100s\n",
           pathname);
  gaprnt(0,pout);

cleanup:
  if (descriptor) fclose(descriptor);
  else if (fd>=0) close(fd);
  if (engine) adios2_close(engine);
  if (adios) adios2_finalize(adios);
  if (temporary[0] && strcmp(temporary,"/tmp/opengrads-bp5-XXXXXX"))
    unlink(temporary);
  if (xvalues) gree(xvalues,"adios2axis");
  if (yvalues) gree(yvalues,"adios2axis");
  if (zvalues) gree(zvalues,"adios2axis");
  if (vars) gree(vars,"adios2metadata");
  /*
   * adios2_available_variables hands back malloc'd storage for the array and
   * for every name in it, and nothing in ADIOS2 frees it later, so release it
   * here with free rather than gree: it is not GrADS-allocated memory. The
   * metadata table borrows these names, so this has to come after it is gone.
   */
  if (names) {
    for (i=0;i<name_count;i++)
      if (names[i]) free(names[i]);
    free(names);
  }
  return rc;
}

static gaint gaadios_validate_variable (struct gafile *pfi,
                                        struct gavar *pvar,
                                        struct gaadios_state *state) {
  adios2_variable *variable;
  adios2_shapeid shapeid;
  adios2_type type;
  size_t ndims, shape[GA_ADIOS_MAX_DIMS], steps;
  gaint expected, has_x, has_y, has_z, has_t, has_e, i, rank;
  const char *name;

  name = gaadios_varname(pvar);
  variable = adios2_inquire_variable(state->io,name);
  if (variable==NULL) {
    snprintf(pout,1255,"BP5 Open Error: Variable '%s' was not found in %.1100s\n",
             name,pfi->name);
    gaprnt(0,pout);
    return 1;
  }

  if (adios2_variable_type(&type,variable)!=adios2_error_none ||
      !gaadios_numeric_type(type)) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' is not a supported real numeric type\n",
             name);
    gaprnt(0,pout);
    return 1;
  }
  rank = gaadios_rank(pvar);
  if (adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none ||
      (shapeid!=adios2_shapeid_global_array &&
       shapeid!=adios2_shapeid_global_value)) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' must be a global array or global value\n",
             name);
    gaprnt(0,pout);
    return 1;
  }
  if (shapeid==adios2_shapeid_global_value) {
    /* one number per step: the descriptor describes it as "t" */
    if (rank!=1 || pvar->units[0]!=-103) {
      snprintf(pout,1255,
               "BP5 Open Error: '%s' is a global value, one number per step; "
               "its dimension list must be t\n",name);
      gaprnt(0,pout);
      return 1;
    }
    if (pfi->dnum[4]>1) {
      snprintf(pout,1255,
               "BP5 Open Error: Variable '%s' must include e when EDEF is greater than one\n",
               name);
      gaprnt(0,pout);
      return 1;
    }
    goto attributes;
  }
  if (adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      ndims>GA_ADIOS_MAX_DIMS) {
    snprintf(pout,1255,"BP5 Open Error: Unable to determine dimensions for '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  if (ndims!=(size_t)rank) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' rank is %lu, descriptor specifies %d dimensions\n",
             name,(unsigned long)ndims,rank);
    gaprnt(0,pout);
    return 1;
  }
  if (adios2_variable_shape(shape,variable)!=adios2_error_none) {
    snprintf(pout,1255,"BP5 Open Error: Unable to read shape for '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  has_x = has_y = has_z = has_t = has_e = 0;
  for (i=0;i<rank;i++) {
    if (pvar->units[i]==-100) has_x++;
    if (pvar->units[i]==-101) has_y++;
    if (pvar->units[i]==-102) has_z++;
    if (pvar->units[i]==-103) has_t++;
    if (pvar->units[i]==-104) has_e++;
    if (pvar->units[i]<0 && pvar->units[i]>-100) {
      snprintf(pout,1255,
               "BP5 Open Error: Variable '%s' has invalid negative index %d\n",
               name,(gaint)pvar->units[i]);
      gaprnt(0,pout);
      return 1;
    }
    expected = gaadios_expected_size(pfi,pvar,pvar->units[i]);
    if (expected>=0 && shape[i]!=(size_t)expected &&
        !(pvar->units[i]==-103 && shape[i]<(size_t)expected)) {
      snprintf(pout,1255,
               "BP5 Open Error: Variable '%s' dimension %d has size %lu, expected %d\n",
               name,i+1,(unsigned long)shape[i],expected);
      gaprnt(0,pout);
      return 1;
    }
    if (pvar->units[i]>=0 && (size_t)pvar->units[i]>=shape[i]) {
      snprintf(pout,1255,
               "BP5 Open Error: Fixed index %d is outside dimension %d of '%s'\n",
               (gaint)pvar->units[i],i+1,name);
      gaprnt(0,pout);
      return 1;
    }
  }

  /* A field may lack x or y, such as a reference profile; it is then the
     same everywhere along the axis it lacks. */
  if (has_x>1 || has_y>1 || has_z>1 || has_t>1 || has_e>1) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' repeats an x, y, z, t, or e dimension\n",
             name);
    gaprnt(0,pout);
    return 1;
  }
  if (has_e==0 && pfi->dnum[4]>1) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' must include e when EDEF is greater than one\n",
             name);
    gaprnt(0,pout);
    return 1;
  }
  if (has_t==0) {
    if (adios2_variable_steps(&steps,variable)!=adios2_error_none) {
      snprintf(pout,1255,
               "BP5 Open Error: Unable to determine ADIOS2 steps for '%s'\n",
               name);
      gaprnt(0,pout);
      return 1;
    }
  }
attributes:
  pvar->undef = pfi->undef;
  pvar->undef2 = pfi->undef;
  if (pfi->undefattrflg>0)
    gaadios_numeric_attribute(state->io,name,pfi->undefattr,&pvar->undef);
  if (pfi->undefattrflg>1)
    gaadios_numeric_attribute(state->io,name,pfi->undefattr2,&pvar->undef2);
  return 0;
}

/* Whether a variable takes its T from ADIOS2 steps, which is when it has no
   T array dimension. */
static gaint gaadios_steps_are_time (struct gavar *pvar, adios2_variable *variable) {
  adios2_shapeid shapeid;
  gaint i, rank;

  if (adios2_variable_shapeid(&shapeid,variable)==adios2_error_none &&
      shapeid==adios2_shapeid_global_value) return 1;
  rank = gaadios_rank(pvar);
  for (i=0;i<rank;i++) if (pvar->units[i]==-103) return 0;
  return 1;
}

/* A variable written once in a dataset whose other variables have more steps
   (terrain, a reference profile) holds for every time. Record the most steps
   any variable has, which is what tells such a variable apart, and name the
   variables read that way. */
static void gaadios_note_static (struct gafile *pfi, struct gaadios_state *state) {
  adios2_variable *variable;
  struct gavar *pvar;
  size_t steps, len;
  gaint i, listed, more;
  char names[512];

  state->maxsteps = 0;
  pvar = pfi->pvar1;
  for (i=0;i<pfi->vnum;i++,pvar++) {
    variable = adios2_inquire_variable(state->io,gaadios_varname(pvar));
    if (!variable || !gaadios_steps_are_time(pvar,variable) ||
        adios2_variable_steps(&steps,variable)!=adios2_error_none) continue;
    if (steps>state->maxsteps) state->maxsteps = steps;
  }
  if (state->maxsteps<2 || pfi->dnum[3]<2) return;

  names[0] = '\0';
  listed = more = 0;
  pvar = pfi->pvar1;
  for (i=0;i<pfi->vnum;i++,pvar++) {
    variable = adios2_inquire_variable(state->io,gaadios_varname(pvar));
    if (!variable || !gaadios_steps_are_time(pvar,variable) ||
        adios2_variable_steps(&steps,variable)!=adios2_error_none || steps!=1)
      continue;
    len = strlen(names);
    if (listed<8 && len+strlen(pvar->abbrv)+3<sizeof(names)) {
      snprintf(names+len,sizeof(names)-len,"%s%s",listed ? ", " : "",pvar->abbrv);
      listed++;
    }
    else more++;
  }
  if (!listed) return;
  if (more) {
    snprintf(pout,1255,"Written once, so the same at every time: %s, and %d more\n",
             names,more);
  }
  else {
    snprintf(pout,1255,"Written once, so the same at every time: %s\n",names);
  }
  gaprnt(2,pout);
}

gaint gaadios_open (struct gafile *pfi) {
  struct gaadios_state *state;
  struct gavar *pvar;
  size_t sz;
  gaint i;

  sz = sizeof(struct gaadios_state);
  state = (struct gaadios_state *)galloc(sz,"adios2state");
  if (state==NULL) {
    gaprnt(0,"BP5 Open Error: Unable to allocate ADIOS2 state\n");
    return 1;
  }
  state->adios = NULL;
  state->io = NULL;
  state->engine = NULL;
  state->maxsteps = 0;
  pfi->adios2 = state;

  state->adios = adios2_init_serial();
  if (state->adios==NULL) {
    gaprnt(0,"BP5 Open Error: adios2_init_serial failed\n");
    gaadios_close(pfi);
    return 1;
  }
  state->io = adios2_declare_io(state->adios,"opengrads_bp5_reader");
  if (state->io==NULL) {
    gaprnt(0,"BP5 Open Error: adios2_declare_io failed\n");
    gaadios_close(pfi);
    return 1;
  }
  state->engine = adios2_open(state->io,pfi->name,adios2_mode_readRandomAccess);
  if (state->engine==NULL) {
    snprintf(pout,1255,"BP5 Open Error: Unable to open %.1100s\n",pfi->name);
    gaprnt(0,pout);
    gaadios_close(pfi);
    return 1;
  }

  pvar = pfi->pvar1;
  for (i=0;i<pfi->vnum;i++,pvar++) {
    if (gaadios_validate_variable(pfi,pvar,state)) {
      gaadios_close(pfi);
      return 1;
    }
  }
  gaadios_note_static(pfi,state);
  return 0;
}

void gaadios_close (struct gafile *pfi) {
  struct gaadios_state *state;
  state = (struct gaadios_state *)pfi->adios2;
  if (state==NULL) return;
  if (state->engine) adios2_close(state->engine);
  if (state->adios) adios2_finalize(state->adios);
  gree(state,"adios2state");
  pfi->adios2 = NULL;
}

gaint gaadios_read_row (struct gafile *pfi, struct gavar *pvar,
                        gaint x, gaint y, gaint z, gaint t, gaint e,
                        gaint len, gadouble *gr, char *gru) {
  struct gaadios_state *state;
  adios2_variable *variable;
  adios2_shapeid shapeid;
  adios2_type type;
  adios2_error error;
  size_t ndims, start[GA_ADIOS_MAX_DIMS], count[GA_ADIOS_MAX_DIMS];
  size_t bytes, i, nread;
  gaint rank, has_t, has_x, scalar, time_status, yy, zz;
  void *native;
  gadouble value;
  const char *name;

  state = (struct gaadios_state *)pfi->adios2;
  if (state==NULL || state->engine==NULL) {
    gaprnt(0,"BP5 I/O Error: Dataset is not open\n");
    return 1;
  }
  name = gaadios_varname(pvar);
  variable = adios2_inquire_variable(state->io,name);
  if (variable==NULL) {
    snprintf(pout,1255,"BP5 I/O Error: Variable '%s' is unavailable\n",name);
    gaprnt(0,pout);
    return 1;
  }
  rank = gaadios_rank(pvar);
  if (adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none ||
      adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      adios2_variable_type(&type,variable)!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Metadata changed for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }
  /* a global value is one number per step, described by the list "t" */
  scalar = (shapeid==adios2_shapeid_global_value);
  if (scalar ? ndims!=0 : ndims!=(size_t)rank) {
    snprintf(pout,1255,"BP5 I/O Error: Metadata changed for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  time_status = gaadios_time_available(pfi,pvar,variable,t);
  if (time_status<0) {
    snprintf(pout,1255,"BP5 I/O Error: Unable to query time metadata for '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }
  if (time_status==0) {
    gaadios_set_undefined(pfi,(size_t)len,gr,gru);
    return 0;
  }

  yy = pfi->yrflg ? pfi->dnum[1]-y : y-1;
  if (pfi->zrflg && pvar->levels>0) zz = pvar->levels-z;
  else zz = z-1;
  has_t = has_x = 0;
  if (!scalar) {
    for (i=0;i<ndims;i++) {
      count[i] = 1;
      if (pvar->units[i]==-100) {
        start[i] = x-1;
        count[i] = len;
        has_x = 1;
      }
      else if (pvar->units[i]==-101) start[i] = yy;
      else if (pvar->units[i]==-102) start[i] = zz;
      else if (pvar->units[i]==-103) {
        start[i] = t-1;
        has_t = 1;
      }
      else if (pvar->units[i]==-104) start[i] = e-1;
      else start[i] = (size_t)pvar->units[i];
    }
  }
  /* without an X dimension one value holds along the whole row */
  nread = has_x ? (size_t)len : 1;

  error = scalar ? adios2_error_none :
          adios2_set_selection(variable,ndims,start,count);
  if (error==adios2_error_none) {
    if (has_t) error = adios2_set_step_selection(variable,0,1);
    else error = adios2_set_step_selection(variable,
                                           time_status==2 ? 0 : (size_t)(t-1),1);
  }
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Invalid selection for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  bytes = nread*gaadios_type_size(type);
  native = galloc(bytes,"adios2row");
  if (native==NULL) {
    gaprnt(0,"BP5 I/O Error: Unable to allocate row buffer\n");
    return 1;
  }
  error = adios2_get(state->engine,variable,native,adios2_mode_sync);
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Read failed for variable '%s'\n",name);
    gaprnt(0,pout);
    gree(native,"adios2row");
    return 1;
  }

  for (i=0;i<(size_t)len;i++) {
    value = gaadios_value(native,type,has_x ? i : 0);
    if (gaadios_is_missing(pfi,pvar,value)) {
      gr[i] = pfi->undef;
      gru[i] = 0;
    }
    else {
      gr[i] = value;
      gru[i] = 1;
    }
  }
  gree(native,"adios2row");
  return 0;
}

/*
 * How one grid request maps onto a variable's native array: the selection to
 * make, and where each grid point sits in what that selection returns. A
 * request may vary in any two of X, Y and Z, so a vertical section is one
 * read rather than one read per level; a variable lacking a varying axis (a
 * profile on an x-y map) repeats its value along it, through a stride of 0.
 */
struct gaadios_plan {
  adios2_variable *variable;
  adios2_type type;
  size_t ndims;
  size_t start[GA_ADIOS_MAX_DIMS], count[GA_ADIOS_MAX_DIMS];
  size_t nvalues;            /* native values per step */
  size_t isiz, jsiz;         /* the grid's shape */
  size_t istride, jstride;   /* native stride along the grid's i and j */
  gaint irev, jrev;          /* native order runs backwards along i or j */
  gaint scalar;              /* a global value, one number per step */
  gaint tdim;                /* native position of an explicit T dimension */
};

/* Native index of grid index idx (1-based) along dimension dim, and whether
   native order runs backwards there; -1 when outside the variable. */
static long gaadios_native_index(struct gafile *pfi, struct gavar *pvar,
                                 gaint dim, gaint idx, gaint *rev) {
  *rev = 0;
  if (dim==0 || dim==4) {
    if (idx<1 || idx>pfi->dnum[dim]) return -1;
    return idx-1;
  }
  if (dim==1) {
    if (idx<1 || idx>pfi->dnum[1]) return -1;
    *rev = pfi->yrflg ? 1 : 0;
    return pfi->yrflg ? pfi->dnum[1]-idx : idx-1;
  }
  if (idx<1 || idx>pvar->levels) return -1;
  *rev = pfi->zrflg ? 1 : 0;
  return pfi->zrflg ? pvar->levels-idx : idx-1;
}

/* Returns 0 with the plan made, -1 when the request is not one the plan
   covers (the row reader then handles it), or 1 on an error. */
static gaint gaadios_plan_grid(struct gafile *pfi, struct gavar *pvar,
                               struct gagrid *pgrid, struct gaadios_plan *plan) {
  struct gaadios_state *state;
  adios2_shapeid shapeid;
  size_t stride[GA_ADIOS_MAX_DIMS];
  gaint rank, k, g, rev, lo, hi, idim, jdim, irev, jrev;
  long native;
  const char *name;

  idim = pgrid->idim;
  jdim = pgrid->jdim;
  if (idim>2 || jdim>2 || pfi->ppflag || pfi->tmplat || pgrid->toff) return -1;
  state = (struct gaadios_state *)pfi->adios2;
  if (state==NULL || state->engine==NULL) {
    gaprnt(0,"BP5 I/O Error: Dataset is not open\n");
    return 1;
  }
  name = gaadios_varname(pvar);
  plan->variable = adios2_inquire_variable(state->io,name);
  rank = gaadios_rank(pvar);
  if (!plan->variable ||
      adios2_variable_shapeid(&shapeid,plan->variable)!=adios2_error_none ||
      adios2_variable_ndims(&plan->ndims,plan->variable)!=adios2_error_none ||
      adios2_variable_type(&plan->type,plan->variable)!=adios2_error_none ||
      (shapeid==adios2_shapeid_global_value ? plan->ndims!=0 :
                                              plan->ndims!=(size_t)rank)) {
    snprintf(pout,1255,"BP5 I/O Error: Metadata changed for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }
  plan->scalar = (shapeid==adios2_shapeid_global_value);
  plan->isiz = idim>=0 ? (size_t)(pgrid->dimmax[idim]-pgrid->dimmin[idim]+1) : 1;
  plan->jsiz = jdim>=0 ? (size_t)(pgrid->dimmax[jdim]-pgrid->dimmin[jdim]+1) : 1;
  plan->istride = plan->jstride = 0;
  plan->irev = plan->jrev = 0;
  plan->tdim = -1;
  plan->nvalues = 1;
  if (plan->scalar) return 0;

  irev = jrev = 0;
  for (k=0;k<rank;k++) {
    plan->count[k] = 1;
    g = -1;
    if (pvar->units[k]==-100) g = 0;
    else if (pvar->units[k]==-101) g = 1;
    else if (pvar->units[k]==-102) g = 2;
    else if (pvar->units[k]==-104) g = 4;
    else if (pvar->units[k]==-103) {
      plan->tdim = k;
      plan->start[k] = 0;
      continue;
    }
    else {
      plan->start[k] = (size_t)pvar->units[k];
      continue;
    }
    if (g==idim || g==jdim) {
      lo = pgrid->dimmin[g];
      hi = pgrid->dimmax[g];
      if (gaadios_native_index(pfi,pvar,g,lo,&rev)<0 ||
          gaadios_native_index(pfi,pvar,g,hi,&rev)<0) return -1;
      plan->start[k] = (size_t)(rev ? gaadios_native_index(pfi,pvar,g,hi,&rev)
                                    : lo-1);
      plan->count[k] = (size_t)(hi-lo+1);
      if (g==idim) irev = rev;
      else jrev = rev;
    }
    else {
      native = gaadios_native_index(pfi,pvar,g,pgrid->dimmin[g],&rev);
      if (native<0) return -1;
      plan->start[k] = (size_t)native;
    }
  }
  stride[rank-1] = 1;
  for (k=rank-2;k>=0;k--) stride[k] = stride[k+1]*plan->count[k+1];
  for (k=0;k<rank;k++) {
    plan->nvalues *= plan->count[k];
    g = pvar->units[k]==-100 ? 0 : pvar->units[k]==-101 ? 1 :
        pvar->units[k]==-102 ? 2 : -1;
    if (g>=0 && g==idim) { plan->istride = stride[k]; plan->irev = irev; }
    if (g>=0 && g==jdim) { plan->jstride = stride[k]; plan->jrev = jrev; }
  }
  return 0;
}

/* Select time t for a planned read; time_status is gaadios_time_available's
   answer, 2 meaning the variable's single step stands for every time. */
static adios2_error gaadios_plan_select(struct gaadios_plan *plan, gaint t,
                                        gaint time_status) {
  adios2_error error;

  if (plan->scalar)
    return adios2_set_step_selection(plan->variable,
                                     time_status==2 ? 0 : (size_t)(t-1),1);
  if (plan->tdim>=0) plan->start[plan->tdim] = (size_t)(t-1);
  error = adios2_set_selection(plan->variable,plan->ndims,plan->start,plan->count);
  if (error!=adios2_error_none) return error;
  if (plan->tdim>=0) return adios2_set_step_selection(plan->variable,0,1);
  return adios2_set_step_selection(plan->variable,
                                   time_status==2 ? 0 : (size_t)(t-1),1);
}

/* Copy one step's native values into a GrADS grid and its undef mask. The
   common types get their own loop, so the per-value work is a load and the
   missing-value test. */
#define GAADIOS_CONVERT(fetch)                                            \
  for (j=0;j<plan->jsiz;j++) {                                           \
    nj = plan->jrev ? plan->jsiz-1-j : j;                                \
    for (i=0;i<plan->isiz;i++) {                                         \
      ni = plan->irev ? plan->isiz-1-i : i;                              \
      index = ni*plan->istride + nj*plan->jstride;                       \
      value = (fetch);                                                   \
      out = j*plan->isiz+i;                                              \
      if (gaadios_missing_test(&missing,value)) {                        \
        gr[out] = pfi->undef;                                            \
        gru[out] = 0;                                                    \
      }                                                                  \
      else {                                                             \
        gr[out] = value;                                                 \
        gru[out] = 1;                                                    \
      }                                                                  \
    }                                                                    \
  }

static void gaadios_plan_convert(struct gafile *pfi, struct gavar *pvar,
                                 struct gaadios_plan *plan, void *native,
                                 gadouble *gr, char *gru) {
  struct gaadios_missing missing;
  size_t i, j, ni, nj, index, out;
  gadouble value;

  gaadios_missing_bounds(pvar,&missing);
  if (plan->type==adios2_type_float) {
    GAADIOS_CONVERT((gadouble)((const float *)native)[index])
  }
  else if (plan->type==adios2_type_double) {
    GAADIOS_CONVERT(((const double *)native)[index])
  }
  else {
    GAADIOS_CONVERT(gaadios_value(native,plan->type,index))
  }
}

#undef GAADIOS_CONVERT

gaint gaadios_read_grid (struct gafile *pfi, struct gavar *pvar,
                          struct gagrid *pgrid, gadouble *gr, char *gru) {
  struct gaadios_state *state;
  struct gaadios_plan plan;
  adios2_error error;
  size_t type_size;
  gaint rc, t, time_status;
  void *native;
  const char *name;

  rc = gaadios_plan_grid(pfi,pvar,pgrid,&plan);
  if (rc) return rc;
  state = (struct gaadios_state *)pfi->adios2;
  name = gaadios_varname(pvar);
  t = pgrid->dimmin[3];
  time_status = gaadios_time_available(pfi,pvar,plan.variable,t);
  if (time_status<0) {
    snprintf(pout,1255,"BP5 I/O Error: Unable to query time metadata for '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }
  if (time_status==0) {
    gaadios_set_undefined(pfi,plan.isiz*plan.jsiz,gr,gru);
    pgrid->undef = pfi->undef;
    return 0;
  }
  type_size = gaadios_type_size(plan.type);
  if (!type_size || plan.nvalues>SIZE_MAX/type_size) {
    gaprnt(0,"BP5 I/O Error: Requested grid buffer is too large\n");
    return 1;
  }
  if (gaadios_plan_select(&plan,t,time_status)!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Invalid grid selection for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }
  native = galloc(plan.nvalues*type_size,"adios2grid");
  if (!native) {
    gaprnt(0,"BP5 I/O Error: Unable to allocate grid buffer\n");
    return 1;
  }
  error = adios2_get(state->engine,plan.variable,native,adios2_mode_sync);
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Grid read failed for variable '%s'\n",name);
    gaprnt(0,pout);
    gree(native,"adios2grid");
    return 1;
  }
  gaadios_plan_convert(pfi,pvar,&plan,native,gr,gru);
  pgrid->undef = pfi->undef;
  gree(native,"adios2grid");
  return 0;
}

/*
 * Read n grids shaped like pgrid, at times t0, t0+incr, ..., into gr and gru
 * one after another. The reads are queued as deferred gets and issued
 * together, so ADIOS2 serves them on its reader threads instead of one
 * request at a time; the copies into GrADS grids then run on the calculation
 * threads. Each grid is exactly what gaadios_read_grid returns for that time.
 * Returns -1 for a request it does not cover, so the caller reads step by step.
 */
gaint gaadios_read_steps (struct gafile *pfi, struct gavar *pvar,
                          struct gagrid *pgrid, gaint t0, gaint n, gaint incr,
                          gadouble *gr, char *gru) {
  struct gaadios_state *state;
  struct gaadios_plan plan;
  size_t type_size, points;
  gaint rc, k, t, queued, *status;
  char *native;
  const char *name;

  if (n<1 || incr<1) return -1;
  rc = gaadios_plan_grid(pfi,pvar,pgrid,&plan);
  if (rc) return rc;
  state = (struct gaadios_state *)pfi->adios2;
  name = gaadios_varname(pvar);
  points = plan.isiz*plan.jsiz;
  type_size = gaadios_type_size(plan.type);
  if (!type_size || plan.nvalues>SIZE_MAX/type_size/(size_t)n) return -1;
  status = (gaint *)galloc(sizeof(gaint)*(size_t)n,"adios2steps");
  native = (char *)galloc(plan.nvalues*type_size*(size_t)n,"adios2stepdata");
  if (!status || !native) {
    if (status) gree(status,"adios2steps");
    if (native) gree(native,"adios2stepdata");
    return -1;
  }

  rc = 0;
  queued = 0;
  /* Consecutive steps, all written: one selection spanning them, which
     ADIOS2 returns step after step in one buffer. */
  if (incr==1 && plan.tdim<0) {
    for (k=0;k<n;k++) {
      t = t0+k;
      status[k] = (t<1 || t>pfi->dnum[3]) ? 0 :
                  gaadios_time_available(pfi,pvar,plan.variable,t);
      if (status[k]!=1) break;
    }
    if (k==n) {
      if (gaadios_plan_select(&plan,t0,1)!=adios2_error_none ||
          adios2_set_step_selection(plan.variable,(size_t)(t0-1),(size_t)n)
            !=adios2_error_none ||
          adios2_get(state->engine,plan.variable,native,adios2_mode_sync)
            !=adios2_error_none) {
        snprintf(pout,1255,"BP5 I/O Error: Grid read failed for variable '%s'\n",name);
        gaprnt(0,pout);
        rc = 1;
        goto done;
      }
      goto convert;
    }
  }
  for (k=0;k<n && !rc;k++) {
    t = t0+k*incr;
    status[k] = (t<1 || t>pfi->dnum[3]) ? 0 :
                gaadios_time_available(pfi,pvar,plan.variable,t);
    if (status[k]<0) {
      snprintf(pout,1255,"BP5 I/O Error: Unable to query time metadata for '%s'\n",name);
      gaprnt(0,pout);
      rc = 1;
    }
    else if (status[k]>0) {
      if (gaadios_plan_select(&plan,t,status[k])!=adios2_error_none ||
          adios2_get(state->engine,plan.variable,
                     native+(size_t)k*plan.nvalues*type_size,
                     adios2_mode_deferred)!=adios2_error_none) {
        snprintf(pout,1255,"BP5 I/O Error: Invalid grid selection for variable '%s'\n",name);
        gaprnt(0,pout);
        rc = 1;
      }
      else queued++;
    }
  }
  /* Complete whatever was queued even after an error: the engine writes into
     these buffers when the gets are performed, so they must outlive that. */
  if (queued && adios2_perform_gets(state->engine)!=adios2_error_none && !rc) {
    snprintf(pout,1255,"BP5 I/O Error: Grid read failed for variable '%s'\n",name);
    gaprnt(0,pout);
    rc = 1;
  }
  if (rc) goto done;

convert:
#if USEOPENMP == 1
#pragma omp parallel for schedule(static) \
    if(ga_omp_parallelize(points*(size_t)n>(size_t)INT_MAX ? INT_MAX : (gaint)(points*(size_t)n)))
#endif
  for (k=0;k<n;k++) {
    if (status[k]==0)
      gaadios_set_undefined(pfi,points,gr+(size_t)k*points,gru+(size_t)k*points);
    else
      gaadios_plan_convert(pfi,pvar,&plan,native+(size_t)k*plan.nvalues*type_size,
                           gr+(size_t)k*points,gru+(size_t)k*points);
  }

done:
  gree(status,"adios2steps");
  gree(native,"adios2stepdata");
  return rc;
}
