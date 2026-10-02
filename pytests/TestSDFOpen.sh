#!/usr/bin/env bash
# NetCDF sdfopen/xdfopen regression test. GPLv2; see COPYING.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
grads_binary="$build_root/src/grads"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
netcdf_fixture="$repo_root/pytests/data/model.nc"
test_root="$(mktemp -d /tmp/opengrads-sdf-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT
xdf_fixture="$test_root/model.xdf"
printf 'dset %s\n' "$netcdf_fixture" > "$xdf_fixture"

if [[ ! -x "$grads_binary" && -x "$grads_binary.exe" ]]; then
  grads_binary="$grads_binary.exe"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'NetCDF-enabled GrADS binary not found: %s\n' "$grads_binary" >&2
  exit 1
fi
if ! find "$build_root/src/.libs" -maxdepth 1 \
  \( -name 'libgxdummy*.so' -o -name 'libgxdummy*.dylib' -o -name 'libgxdummy*.dll' \) \
  -print -quit | grep -q .; then
  printf 'Headless GrADS plug-in not found in: %s\n' \
    "$build_root/src/.libs" >&2
  exit 1
fi

output="$(
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -bl -d gxdummy -h gxdummy <<GRADS_COMMANDS
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

# Files sdfopen used to refuse: no X (or no Y) coordinate, Cartesian axes in
# metres, dimensions without coordinate variables, and 365-day calendars.
# The fixtures' CDL sources sit next to them in pytests/data/sdf.
sdf_fixtures="$repo_root/pytests/data/sdf"
cat > "$test_root/times.gs" <<'GRADS_SCRIPT'
function main(args)
'q file'
size=sublin(result,5)
nt=subwrd(size,12)
'q calendar'
cal=sublin(result,1)
out=''
t=1
while(t<=nt)
  'set t 't
  'q time'
  out=out' 'subwrd(result,3)
  t=t+1
endwhile
say args': 'cal' |'out
GRADS_SCRIPT

output="$(
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -bl -d gxdummy -h gxdummy <<GRADS_COMMANDS
sdfopen $sdf_fixtures/zonal_mean.nc
q file
set gxout print
set prnopts %g 4 1
set lev 1000 200
set t 2
d u
close 1
sdfopen $sdf_fixtures/cartesian.nc
q dims
set gxout print
set prnopts %g 4 1
set t 2
set z 2
d th
close 1
sdfopen $sdf_fixtures/no_coords.nc
q dims
close 1
sdfopen $sdf_fixtures/profile.nc
q file
set gxout print
set prnopts %g 3 1
set z 1 3
d t
close 1
sdfopen $sdf_fixtures/calendar_noleap.nc
run $test_root/times.gs noleap
sdfopen $sdf_fixtures/calendar_standard.nc
close 1
sdfopen $sdf_fixtures/calendar_standard.nc
run $test_root/times.gs standard
quit
GRADS_COMMANDS
)"

# No X: a single point, and the data reads along Y and Z.
check_text 'SDF file has no X coordinate -- X is a single point, as with XDEF 1 LINEAR 0 1.'
check_text 'Xsize = 1  Ysize = 4  Zsize = 3  Tsize = 2  Esize = 1'
check_text '101 102 103 104'
check_text '109 110 111 112'

# Cartesian X and Y in metres become degrees, as bpopen and a descriptor
# for a Cartesian model give them.
check_text "X and Y are Cartesian; mapped to degrees on GrADS's 6370 km sphere, centred on 0"
check_text 'X is varying   Lon = -0.00269839 to 0.00269839   X = 1 to 4'
check_text 'Y is varying   Lat = -0.00179893 to 0.00179893   Y = 1 to 3'
check_text '113 114 115 116'
check_text '121 122 123 124'

# Dimensions without coordinate variables count grid points.
check_text 'SDF file has no coordinate variable for X dimension x -- X counts grid points, 1 to 4.'
check_text 'X is varying   Lon = 1 to 4   X = 1 to 4'
check_text 'Y is varying   Lat = 1 to 3   Y = 1 to 3'

# A single column: neither X nor Y.
check_text 'SDF file has no Y coordinate -- Y is a single point, as with YDEF 1 LINEAR 0 1.'
check_text 'Xsize = 1  Ysize = 1  Zsize = 3  Tsize = 2  Esize = 1'
check_text '290 280 260'

# The same day numbers land on different dates in the two calendars, and a
# standard file opens once the 365-day one is closed.
check_text 'noleap: 365-day calendar in effect | 00Z28FEB2000 00Z01MAR2000 00Z02MAR2000'
check_text 'SDF Error: this file uses the standard calendar, but the open files use the 365-day one.'
check_text 'standard: standard calendar in effect | 00Z28FEB2000 00Z29FEB2000 00Z01MAR2000'

printf 'SDF regression test passed: NetCDF sdfopen, data read, and xdfopen are available;\n'
printf '  files without X or Y, Cartesian axes, bare dimensions, and both calendars open.\n'
