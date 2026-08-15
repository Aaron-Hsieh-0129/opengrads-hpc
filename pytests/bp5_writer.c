/*
 * OpenGrADS BP5 test fixture writer.
 * Added in 2026 and distributed under GNU GPL version 2; see COPYING.
 * ADIOS2 is an optional, separately distributed test dependency.
 */
/*
 * Create a small, deterministic BP5 dataset for the OpenGrADS reader test.
 *
 * Shape order is Z,Y,X for temperature and Y,X for surface_pressure.
 * ADIOS2 steps represent the GrADS T axis.
 */

#include <stdio.h>
#include <stdlib.h>

#include <adios2_c.h>

static void fail_error(adios2_error error, const char *operation)
{
  if (error != adios2_error_none) {
    fprintf(stderr, "bp5_writer: %s failed (ADIOS2 error %d)\n",
            operation, (int)error);
    exit(EXIT_FAILURE);
  }
}

static void *require_handle(void *handle, const char *operation)
{
  if (handle == NULL) {
    fprintf(stderr, "bp5_writer: %s returned a null handle\n", operation);
    exit(EXIT_FAILURE);
  }
  return handle;
}

int main(int argc, char **argv)
{
  const size_t temp_shape[3] = {2, 3, 4};
  const size_t temp_start[3] = {0, 0, 0};
  const size_t temp_count[3] = {2, 3, 4};
  const size_t ps_shape[2] = {3, 4};
  const size_t ps_start[2] = {0, 0};
  const size_t ps_count[2] = {3, 4};
  const size_t x_shape[1] = {4}, x_start[1] = {0}, x_count[1] = {4};
  const size_t y_shape[1] = {3}, y_start[1] = {0}, y_count[1] = {3};
  const size_t z_shape[1] = {2}, z_start[1] = {0}, z_count[1] = {2};
  const double x_values[4] = {0.0, 1.0, 2.0, 3.0};
  const double y_values[3] = {-1.0, 0.0, 1.0};
  const double z_values[2] = {1000.0, 500.0};
  const float temperature_fill = -7777.0f;
  const double pressure_missing = -9999.0;
  adios2_step_status status;
  adios2_adios *adios;
  adios2_io *io;
  adios2_variable *temp_var;
  adios2_variable *ps_var;
  adios2_variable *x_var;
  adios2_variable *y_var;
  adios2_variable *z_var;
  adios2_engine *engine;
  float temperature[24];
  double surface_pressure[12];
  size_t step, z, y, x, index;

  if (argc != 2) {
    fprintf(stderr, "usage: %s OUTPUT.bp\n", argv[0]);
    return EXIT_FAILURE;
  }

  adios = require_handle(adios2_init_serial(), "adios2_init_serial");
  io = require_handle(adios2_declare_io(adios, "opengrads_bp5_fixture"),
                      "adios2_declare_io");
  fail_error(adios2_set_engine(io, "BP5"), "adios2_set_engine(BP5)");

  temp_var = require_handle(
      adios2_define_variable(io, "temperature", adios2_type_float, 3,
                             temp_shape, temp_start, temp_count,
                             adios2_constant_dims_true),
      "adios2_define_variable(temperature)");
  ps_var = require_handle(
      adios2_define_variable(io, "surface_pressure", adios2_type_double, 2,
                             ps_shape, ps_start, ps_count,
                             adios2_constant_dims_true),
      "adios2_define_variable(surface_pressure)");
  x_var = require_handle(
      adios2_define_variable(io, "coordinates/x", adios2_type_double, 1,
                             x_shape, x_start, x_count, adios2_constant_dims_true),
      "adios2_define_variable(coordinates/x)");
  y_var = require_handle(
      adios2_define_variable(io, "coordinates/y", adios2_type_double, 1,
                             y_shape, y_start, y_count, adios2_constant_dims_true),
      "adios2_define_variable(coordinates/y)");
  z_var = require_handle(
      adios2_define_variable(io, "coordinates/z_mid", adios2_type_double, 1,
                             z_shape, z_start, z_count, adios2_constant_dims_true),
      "adios2_define_variable(coordinates/z_mid)");
  require_handle(adios2_define_attribute(
                     io, "title", adios2_type_string,
                     "OpenGrADS BP5 attribute fixture"),
                 "adios2_define_attribute(title)");
  require_handle(adios2_define_variable_attribute(
                     io, "long_name", adios2_type_string,
                     "Air temperature", "temperature", "/"),
                 "adios2_define_variable_attribute(temperature/long_name)");
  require_handle(adios2_define_variable_attribute(
                     io, "units", adios2_type_string, "K",
                     "temperature", "/"),
                 "adios2_define_variable_attribute(temperature/units)");
  require_handle(adios2_define_variable_attribute(
                     io, "_FillValue", adios2_type_float,
                     &temperature_fill, "temperature", "/"),
                 "adios2_define_variable_attribute(temperature/_FillValue)");
  require_handle(adios2_define_variable_attribute(
                     io, "long_name", adios2_type_string,
                     "Surface pressure", "surface_pressure", "/"),
                 "adios2_define_variable_attribute(surface_pressure/long_name)");
  require_handle(adios2_define_variable_attribute(
                     io, "units", adios2_type_string, "hPa",
                     "surface_pressure", "/"),
                 "adios2_define_variable_attribute(surface_pressure/units)");
  require_handle(adios2_define_variable_attribute(
                     io, "missing_value", adios2_type_double,
                     &pressure_missing, "surface_pressure", "/"),
                 "adios2_define_variable_attribute(surface_pressure/missing_value)");
  require_handle(adios2_define_variable_attribute(
                     io, "units", adios2_type_string, "meter",
                     "coordinates/x", "/"),
                 "adios2_define_variable_attribute(coordinates/x/units)");
  require_handle(adios2_define_variable_attribute(
                     io, "units", adios2_type_string, "meter",
                     "coordinates/y", "/"),
                 "adios2_define_variable_attribute(coordinates/y/units)");
  require_handle(adios2_define_variable_attribute(
                     io, "units", adios2_type_string, "meter",
                     "coordinates/z_mid", "/"),
                 "adios2_define_variable_attribute(coordinates/z_mid/units)");

  engine = require_handle(adios2_open(io, argv[1], adios2_mode_write),
                          "adios2_open");

  for (step = 0; step < 2; ++step) {
    for (z = 0; z < 2; ++z) {
    if (step == 0) temperature[6] = temperature_fill;
      for (y = 0; y < 3; ++y) {
        for (x = 0; x < 4; ++x) {
          index = (z * 3 + y) * 4 + x;
          temperature[index] =
              (float)(1000 * step + 100 * z + 10 * y + x);
        }
      }
    }
    for (y = 0; y < 3; ++y) {
      for (x = 0; x < 4; ++x) {
        index = y * 4 + x;
        surface_pressure[index] =
            (double)(900 + 100 * step + 10 * y + x);
      }
    }
    if (step == 0) surface_pressure[5] = -9999.0;

    fail_error(adios2_begin_step(engine, adios2_step_mode_append, 0.0,
                                 &status),
               "adios2_begin_step");
    if (status != adios2_step_status_ok) {
      fprintf(stderr, "bp5_writer: begin step returned status %d\n",
              (int)status);
      return EXIT_FAILURE;
    }
    fail_error(adios2_put(engine, temp_var, temperature, adios2_mode_sync),
               "adios2_put(temperature)");
    fail_error(adios2_put(engine, ps_var, surface_pressure, adios2_mode_sync),
               "adios2_put(surface_pressure)");
    fail_error(adios2_put(engine, x_var, x_values, adios2_mode_sync),
               "adios2_put(coordinates/x)");
    fail_error(adios2_put(engine, y_var, y_values, adios2_mode_sync),
               "adios2_put(coordinates/y)");
    fail_error(adios2_put(engine, z_var, z_values, adios2_mode_sync),
               "adios2_put(coordinates/z_mid)");
    fail_error(adios2_end_step(engine), "adios2_end_step");
  }

  fail_error(adios2_close(engine), "adios2_close");
  fail_error(adios2_finalize(adios), "adios2_finalize");
  return EXIT_SUCCESS;
}
