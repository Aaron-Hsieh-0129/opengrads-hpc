#!/usr/bin/env bash
# NetCDF sdfopen/xdfopen regression test. GPLv2; see COPYING.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
grads_binary="$build_root/src/grads"
netcdf_fixture="$repo_root/pytests/data/model.nc"
test_root="$(mktemp -d /tmp/opengrads-sdf-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT
xdf_fixture="$test_root/model.xdf"
printf 'dset %s\n' "$netcdf_fixture" > "$xdf_fixture"

if [[ ! -x "$grads_binary" ]]; then
  printf 'NetCDF-enabled GrADS binary not found: %s\n' "$grads_binary" >&2
  exit 1
fi
if [[ ! -r "$build_root/src/.libs/libgxdummy.so" ]]; then
  printf 'Headless GrADS plug-in not found: %s\n' \
    "$build_root/src/.libs/libgxdummy.so" >&2
  exit 1
fi

output="$(
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$repo_root/opengrads" -bl -d gxdummy -h gxdummy <<GRADS_COMMANDS
q config
sdfopen $netcdf_fixture
q file
set gxout print
set x 1
set y 1
set z 1
set t 1
d ps
reinit
xdfopen $xdf_fixture
q file
quit
GRADS_COMMANDS
)"

check_text()
{
  local expected="$1"
  if ! grep -Fq -- "$expected" <<< "$output"; then
    printf 'SDF regression test did not find expected text: %s\n' "$expected" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

check_text 'netcdf'
check_text "SDF file $netcdf_fixture is open as file 1"
check_text 'Surface pressure [hPa]'
check_text "Scanning Descriptor File:  $xdf_fixture"
open_count="$(grep -Fc "SDF file $netcdf_fixture is open as file 1" <<< "$output")"
if [[ "$open_count" -ne 2 ]]; then
  printf 'SDF regression test expected two successful opens, found %s.\n%s\n' \
    "$open_count" "$output" >&2
  exit 1
fi
if grep -Fq 'Unknown command' <<< "$output"; then
  printf 'SDF regression test encountered an unknown command.\n%s\n' "$output" >&2
  exit 1
fi

printf 'SDF regression test passed: NetCDF sdfopen, data read, and xdfopen are available.\n'
