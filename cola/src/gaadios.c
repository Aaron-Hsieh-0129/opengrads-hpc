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
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <adios2_c.h>

#include "grads.h"

#define GA_ADIOS_MAX_DIMS 8

struct gaadios_state {
  adios2_adios *adios;
  adios2_io *io;
  adios2_engine *engine;
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
};

static char pout[1256];

static const char *gaadios_varname (struct gavar *pvar) {
  if (pvar->longnm[0] != '\0') return pvar->longnm;
  return pvar->abbrv;
}

static gaint gaadios_rank (struct gavar *pvar) {
  gaint rank;
  rank = 0;
  while (rank<GA_ADIOS_MAX_DIMS && pvar->units[rank]!=-999) rank++;
  return rank;
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

static gaint gaadios_is_missing(struct gafile *pfi, struct gavar *pvar,
                                gadouble value) {
  gadouble tolerance, low, high;

  if (isnan(value) || isinf(value)) return 1;
  tolerance = dequal(pvar->undef,0.0,1.0e-8)==0 ?
              1.0e-5 : fabs(pvar->undef/EPSILON);
  low = pvar->undef-tolerance;
  high = pvar->undef+tolerance;
  if (value>=low && value<=high) return 1;
  tolerance = dequal(pvar->undef2,0.0,1.0e-8)==0 ?
              1.0e-5 : fabs(pvar->undef2/EPSILON);
  low = pvar->undef2-tolerance;
  high = pvar->undef2+tolerance;
  if (value>=low && value<=high) return 1;
  (void)pfi;
  return 0;
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

static void gaadios_make_alias(struct gaadios_meta_var *vars, size_t current) {
  const char *source, *slash;
  char base[16], candidate[16];
  size_t i, j, limit;
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
    limit = 15;
    if (suffix<10) limit -= 2;
    else if (suffix<100) limit -= 3;
    else limit -= 4;
    snprintf(candidate,sizeof(candidate),"%.*s_%d",(int)limit,base,suffix++);
  }
  snprintf(vars[current].alias,sizeof(vars[current].alias),"%s",candidate);
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

static gadouble gaadios_axis_scale(adios2_io *io, const char *variable_name) {
  adios2_attribute *attribute;
  adios2_bool is_value;
  adios2_type type;
  adios2_error error;
  char units[4096];
  size_t i, length;

  attribute = adios2_inquire_variable_attribute(io,"units",variable_name,"/");
  if (!attribute ||
      adios2_attribute_type(&type,attribute)!=adios2_error_none ||
      type!=adios2_type_string ||
      adios2_attribute_is_value(&is_value,attribute)!=adios2_error_none ||
      is_value!=adios2_true) return 1.0;
  memset(units,0,sizeof(units));
  error = adios2_attribute_data(units,&length,attribute);
  if (error!=adios2_error_none) return 1.0;
  for (i=0;units[i];i++) units[i] = (char)tolower((unsigned char)units[i]);
  if (!strcmp(units,"m") || !strcmp(units,"meter") ||
      !strcmp(units,"meters") || !strcmp(units,"metre") ||
      !strcmp(units,"metres")) return 0.001;
  return 1.0;
}


static gadouble *gaadios_read_axis(adios2_io *io, adios2_engine *engine,
                                   const char **names, size_t expected) {
  adios2_variable *variable;
  adios2_shapeid shapeid;
  adios2_type type;
  adios2_error error;
  gadouble *values;
  void *native;
  size_t shape[1], start[1], count[1], bytes, i, ndims;
  gadouble scale;

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
  scale = gaadios_axis_scale(io,*names);
  bytes = expected;
  for (i=0;i<bytes;i++) values[i] = gaadios_value(native,type,i)*scale;
  gree(native,"adios2axisnative");
  return values;
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
  char temporary[] = "/tmp/opengrads-bp5-XXXXXX";
  FILE *descriptor;
  gadouble *xvalues, *yvalues, *zvalues;
  size_t name_count, i, j, elements, best_elements, reference;
  size_t xsize, ysize, zsize, steps, included;
  int fd;
  gaint rc, have_reference;

  adios = NULL;
  io = NULL;
  engine = NULL;
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
        shapeid!=adios2_shapeid_global_array ||
        adios2_variable_type(&vars[i].type,variable)!=adios2_error_none ||
        !gaadios_numeric_type(vars[i].type) ||
        adios2_variable_ndims(&vars[i].rank,variable)!=adios2_error_none ||
        vars[i].rank<2 || vars[i].rank>3 ||
        adios2_variable_shape(vars[i].shape,variable)!=adios2_error_none)
      continue;
    adios2_variable_steps(&vars[i].steps,variable);
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
  if (!have_reference) {
    gaprnt(0,"BPOPEN error: no numeric 2-D or 3-D global arrays were found\n");
    goto cleanup;
  }

  xsize = vars[reference].shape[vars[reference].rank-1];
  ysize = vars[reference].shape[vars[reference].rank-2];
  zsize = vars[reference].rank==3 ? vars[reference].shape[0] : 1;
  steps = 1;
  included = 0;
  for (i=0;i<name_count;i++) {
    if (vars[i].rank==2 &&
        vars[i].shape[0]==ysize && vars[i].shape[1]==xsize) {
      vars[i].included = 1;
    }
    else if (vars[i].rank==3 &&
             vars[i].shape[0]==zsize && vars[i].shape[1]==ysize &&
             vars[i].shape[2]==xsize) {
      vars[i].included = 1;
    }
    if (vars[i].included) {
      if (strlen(vars[i].name)>256) {
        vars[i].included = 0;
        continue;
      }
      gaadios_make_alias(vars,i);
      gaadios_make_description(io,&vars[i]);
      if (vars[i].steps>steps) steps = vars[i].steps;
      included++;
    }
  }
  if (!included) {
    gaprnt(0,"BPOPEN error: no fields match the inferred horizontal grid\n");
    goto cleanup;
  }

  xvalues = gaadios_read_axis(io,engine,xnames,xsize);
  yvalues = gaadios_read_axis(io,engine,ynames,ysize);
  if (zsize>1) zvalues = gaadios_read_axis(io,engine,znames,zsize);

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
  fprintf(descriptor,"tdef %lu linear 00z01jan2000 1mn\n",
          (unsigned long)steps);
  fprintf(descriptor,"vars %lu\n",(unsigned long)included);
  for (i=0;i<name_count;i++) {
    if (!vars[i].included) continue;
    fprintf(descriptor,"%s=>%s %lu %s %s\n",
            vars[i].name,vars[i].alias,
            (unsigned long)(vars[i].rank==3 ? zsize : 0),
            vars[i].rank==3 ? "z,y,x" : "y,x",vars[i].description);
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
  if (adios2_variable_shapeid(&shapeid,variable)!=adios2_error_none ||
      shapeid!=adios2_shapeid_global_array) {
    snprintf(pout,1255,"BP5 Open Error: Variable '%s' must be a global array\n",name);
    gaprnt(0,pout);
    return 1;
  }
  if (adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      ndims>GA_ADIOS_MAX_DIMS) {
    snprintf(pout,1255,"BP5 Open Error: Unable to determine dimensions for '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  rank = gaadios_rank(pvar);
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
    if (expected>=0 && shape[i]!=(size_t)expected) {
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

  if (has_x!=1 || has_y!=1) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' must map exactly one x and one y dimension\n",
             name);
    gaprnt(0,pout);
    return 1;
  }
  if (has_z>1 || has_t>1 || has_e>1) {
    snprintf(pout,1255,
             "BP5 Open Error: Variable '%s' repeats a z, t, or e dimension\n",name);
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
    if (adios2_variable_steps(&steps,variable)!=adios2_error_none ||
        steps<(size_t)pfi->dnum[3]) {
      snprintf(pout,1255,
               "BP5 Open Error: Variable '%s' has fewer ADIOS2 steps than TDEF\n",
               name);
      gaprnt(0,pout);
      return 1;
    }
  }
  pvar->undef = pfi->undef;
  pvar->undef2 = pfi->undef;
  if (pfi->undefattrflg>0)
    gaadios_numeric_attribute(state->io,name,pfi->undefattr,&pvar->undef);
  if (pfi->undefattrflg>1)
    gaadios_numeric_attribute(state->io,name,pfi->undefattr2,&pvar->undef2);
  return 0;
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
  adios2_type type;
  adios2_error error;
  size_t ndims, start[GA_ADIOS_MAX_DIMS], count[GA_ADIOS_MAX_DIMS];
  size_t bytes, i;
  gaint rank, has_t, yy, zz;
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
  if (adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      ndims!=(size_t)rank ||
      adios2_variable_type(&type,variable)!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Metadata changed for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  yy = pfi->yrflg ? pfi->dnum[1]-y : y-1;
  if (pfi->zrflg && pvar->levels>0) zz = pvar->levels-z;
  else zz = z-1;
  has_t = 0;
  for (i=0;i<ndims;i++) {
    count[i] = 1;
    if (pvar->units[i]==-100) {
      start[i] = x-1;
      count[i] = len;
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

  error = adios2_set_selection(variable,ndims,start,count);
  if (error==adios2_error_none) {
    if (has_t) error = adios2_set_step_selection(variable,0,1);
    else error = adios2_set_step_selection(variable,t-1,1);
  }
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Invalid selection for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  bytes = (size_t)len*gaadios_type_size(type);
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
    value = gaadios_value(native,type,i);
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

gaint gaadios_read_grid (struct gafile *pfi, struct gavar *pvar,
                          struct gagrid *pgrid, gadouble *gr, char *gru) {
  struct gaadios_state *state;
  adios2_variable *variable;
  adios2_type type;
  adios2_error error;
  size_t ndims, start[GA_ADIOS_MAX_DIMS], count[GA_ADIOS_MAX_DIMS];
  size_t native_index, output_index, points, type_size;
  size_t gx, gy, native_y, nx, ny, i;
  gaint rank, has_t, xdim, ydim, yy, zz, t, e;
  void *native;
  gadouble value;
  const char *name;

  if (pgrid->idim!=0 || pgrid->jdim!=1 || pfi->ppflag || pfi->tmplat ||
      pgrid->toff || pgrid->dimmin[0]<1 ||
      pgrid->dimmax[0]>pfi->dnum[0] || pgrid->dimmin[1]<1 ||
      pgrid->dimmax[1]>pfi->dnum[1]) return -1;

  state = (struct gaadios_state *)pfi->adios2;
  if (state==NULL || state->engine==NULL) {
    gaprnt(0,"BP5 I/O Error: Dataset is not open\n");
    return 1;
  }
  name = gaadios_varname(pvar);
  variable = adios2_inquire_variable(state->io,name);
  rank = gaadios_rank(pvar);
  if (!variable ||
      adios2_variable_ndims(&ndims,variable)!=adios2_error_none ||
      ndims!=(size_t)rank ||
      adios2_variable_type(&type,variable)!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Metadata changed for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  nx = (size_t)pgrid->isiz;
  ny = (size_t)pgrid->jsiz;
  if (ny && nx>SIZE_MAX/ny) {
    gaprnt(0,"BP5 I/O Error: Requested grid is too large\n");
    return 1;
  }
  points = nx*ny;
  type_size = gaadios_type_size(type);
  if (!type_size || points>SIZE_MAX/type_size) {
    gaprnt(0,"BP5 I/O Error: Requested grid buffer is too large\n");
    return 1;
  }

  yy = pfi->yrflg ? pfi->dnum[1]-pgrid->dimmax[1] :
                     pgrid->dimmin[1]-1;
  if (pfi->zrflg && pvar->levels>0) zz = pvar->levels-pgrid->dimmin[2];
  else zz = pgrid->dimmin[2]-1;
  t = pgrid->dimmin[3];
  e = pgrid->dimmin[4];
  has_t = 0;
  xdim = ydim = -1;
  for (i=0;i<ndims;i++) {
    count[i] = 1;
    if (pvar->units[i]==-100) {
      xdim = (gaint)i;
      start[i] = pgrid->dimmin[0]-1;
      count[i] = nx;
    }
    else if (pvar->units[i]==-101) {
      ydim = (gaint)i;
      start[i] = yy;
      count[i] = ny;
    }
    else if (pvar->units[i]==-102) start[i] = zz;
    else if (pvar->units[i]==-103) {
      start[i] = t-1;
      has_t = 1;
    }
    else if (pvar->units[i]==-104) start[i] = e-1;
    else start[i] = (size_t)pvar->units[i];
  }
  if (xdim<0 || ydim<0) return -1;

  error = adios2_set_selection(variable,ndims,start,count);
  if (error==adios2_error_none) {
    if (has_t) error = adios2_set_step_selection(variable,0,1);
    else error = adios2_set_step_selection(variable,t-1,1);
  }
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Invalid grid selection for variable '%s'\n",name);
    gaprnt(0,pout);
    return 1;
  }

  native = galloc(points*type_size,"adios2grid");
  if (!native) {
    gaprnt(0,"BP5 I/O Error: Unable to allocate grid buffer\n");
    return 1;
  }
  error = adios2_get(state->engine,variable,native,adios2_mode_sync);
  if (error!=adios2_error_none) {
    snprintf(pout,1255,"BP5 I/O Error: Grid read failed for variable '%s'\n",name);
    gaprnt(0,pout);
    gree(native,"adios2grid");
    return 1;
  }

  for (gy=0;gy<ny;gy++) {
    native_y = pfi->yrflg ? ny-1-gy : gy;
    for (gx=0;gx<nx;gx++) {
      if (xdim>ydim) native_index = native_y*nx+gx;
      else native_index = gx*ny+native_y;
      output_index = gy*nx+gx;
      value = gaadios_value(native,type,native_index);
      if (gaadios_is_missing(pfi,pvar,value)) {
        gr[output_index] = pfi->undef;
        gru[output_index] = 0;
      }
      else {
        gr[output_index] = value;
        gru[output_index] = 1;
      }
    }
  }
  pgrid->undef = pfi->undef;
  gree(native,"adios2grid");
  return 0;
}
