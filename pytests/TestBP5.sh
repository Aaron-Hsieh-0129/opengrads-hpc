#!/usr/bin/env bash
# Added in 2026 for the optional ADIOS2 BP5 backend. GPLv2; see COPYING.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
adios2_root="${OPENGRADS_ADIOS2_ROOT:-}"
if [[ -n "$adios2_root" ]]; then
  adios2_config="$adios2_root/bin/adios2-config"
else
  adios2_config="$(command -v adios2-config || true)"
  if [[ -n "$adios2_config" ]]; then
    adios2_root="$(CDPATH= cd -- "$(dirname -- "$adios2_config")/.." && pwd)"
  fi
fi
grads_binary="$build_root/src/grads"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
fixture_writer="$repo_root/pytests/bp5_writer.c"
fixture_ctl="$repo_root/pytests/data/bp5_fixture.ctl"

if [[ ! -x "$adios2_config" ]]; then
  printf 'ADIOS2 config helper not found: %s\n' "$adios2_config" >&2
  exit 1
fi
if [[ ! -x "$grads_binary" && -x "$grads_binary.exe" ]]; then
  grads_binary="$grads_binary.exe"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'ADIOS2-enabled GrADS binary not found: %s\n' "$grads_binary" >&2
  exit 1
fi

test_root="$(mktemp -d /tmp/opengrads-bp5-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

read -r -a adios2_cflags <<< "$("$adios2_config" --serial --c-flags)"
read -r -a adios2_libs <<< "$("$adios2_config" --serial --c-libs)"
"${CC:-gcc}" -std=gnu99 -Wall -Wextra -Werror "${adios2_cflags[@]}" \
  "$fixture_writer" "${adios2_libs[@]}" -o "$test_root/bp5_writer"

cp "$fixture_ctl" "$test_root/bp5_fixture.ctl"
sed 's/^xdef 4 /xdef 5 /' "$fixture_ctl" > "$test_root/bp5_invalid_shape.ctl"
sed 's/^tdef 2 /tdef 4 /' "$fixture_ctl" > "$test_root/bp5_future_times.ctl"
mkdir -p "$test_root/empty" "$test_root/multiple" "$test_root/notime"
LD_LIBRARY_PATH="$adios2_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$test_root/bp5_writer" "$test_root/bp5_fixture.bp"
LD_LIBRARY_PATH="$adios2_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$test_root/bp5_writer" "$test_root/notime/bp5_notime.bp" --no-time
mkdir -p "$test_root/long"
LD_LIBRARY_PATH="$adios2_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$test_root/bp5_writer" "$test_root/long/bp5_six.bp" --steps 6
ln -s "$test_root/bp5_fixture.bp" "$test_root/multiple/first.bp"
ln -s "$test_root/bp5_fixture.bp" "$test_root/multiple/second.bp"

output="$(
  OPENGRADS_BUILD_ROOT="$build_root" \
  OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  OPENGRADS_COLOR=0 \
    "$launcher" -bl -d gxdummy -h gxdummy <<GRADS_COMMANDS
open $test_root/bp5_fixture.ctl
q config
q ctlinfo
set gxout print
set x 4
set y 3
set z 2
set t 1
d temp
set t 2
d temp
set x 2
set y 2
set z 1
set t 1
d ps
set t 2
d ps
set x 3
set y 2
set z 1
set t 1
d temp
reinit
bpopen $test_root
q file
q vars
q ctlinfo
set gxout print
set x 4
set y 3
set z 2
set t 1
d temperature
set t 2
d temperature
set x 2
set y 2
set z 1
set t 1
d surface_pressur
set t 2
d surface_pressur
set x 3
set y 2
set z 1
set t 1
d temperature
set x 1 4
set y 1 3
set z 1
set t 1
set gxout stat
d temperature
set gxout shaded
d temperature
set gxout stat
d surface_pressur
set t 2
d temperature
reinit
open $test_root/bp5_future_times.ctl
q ctlinfo
set gxout print
set x 4
set y 3
set z 2
set t 2
d temp
set t 3
d temp
reinit
bpopen $test_root/bp5_fixture.bp
close 1
bpopen $test_root/bp5_fixture.bp
reinit
bpopen $test_root/empty
bpopen $test_root/multiple
reinit
bpopen $test_root/notime/bp5_notime.bp
q ctlinfo
reinit
open $test_root/bp5_invalid_shape.ctl
reinit
bpopen $test_root/bp5_fixture.bp
set gxout print
set prnopts %g 8 1
set x 1 4
set y 2
set z 2
set t 2
d theta_ref
d terrain
set x 1
set y 1
set z 1 2
d theta_ref
set z 1
set t 1 2
d domain_mean
set t 2
set x 1 4
set y 2
set z 1 2
d temperature-theta_ref
reinit
open $test_root/bp5_fixture.ctl
set gxout print
set prnopts %g 8 1
set x 1 4
set y 2
set z 2
set t 2
d thref
d terrain
set x 1
set y 1
set z 1 2
d thref
set z 1
set t 1 2
d dmean
set t 2
set x 1 4
set y 2
set z 1 2
d temp-thref
reinit
bpopen $test_root/long/bp5_six.bp
set threads 1
set gxout print
set prnopts %.17g 12 1
set x 1 4
set y 1 3
set z 2
d ave(temperature,t=1,t=6)
d ave(temperature+0,t=1,t=6)
d max(surface_pressur,t=1,t=6)
d max(surface_pressur+0,t=1,t=6)
d ave(domain_mean,t=1,t=6)
d ave(domain_mean+0,t=1,t=6)
d ave(terrain,t=2,t=6)
d ave(terrain+0,t=2,t=6)
d sum(temperature,t=1.5,t=5.5,-b)
d sum(temperature+0,t=1.5,t=5.5,-b)
set y 2
set z 1 2
d ave(temperature,t=1,t=6,2)
d ave(temperature+0,t=1,t=6,2)
set threads 4
set gxout print
set prnopts %.17g 12 1
set x 1 4
set y 1 3
set z 2
d ave(temperature,t=1,t=6)
d ave(temperature+0,t=1,t=6)
d max(surface_pressur,t=1,t=6)
d max(surface_pressur+0,t=1,t=6)
d ave(domain_mean,t=1,t=6)
d ave(domain_mean+0,t=1,t=6)
d ave(terrain,t=2,t=6)
d ave(terrain+0,t=2,t=6)
d sum(temperature,t=1.5,t=5.5,-b)
d sum(temperature+0,t=1.5,t=5.5,-b)
set y 2
set z 1 2
d ave(temperature,t=1,t=6,2)
d ave(temperature+0,t=1,t=6,2)
quit
GRADS_COMMANDS
)"

check_line()
{
  local expected="$1"
  if ! grep -Eq "^${expected}[[:space:]]*$" <<< "$output"; then
    printf 'BP5 regression test did not find expected line: %s\n' "$expected" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

check_text()
{
  local expected="$1"
  if ! grep -Fq -- "$expected" <<< "$output"; then
    printf 'BP5 regression test did not find expected text: %s\n' "$expected" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

check_count()
{
  local expected="$1" minimum="$2" found
  found="$(grep -Fc -- "$expected" <<< "$output" || true)"
  if (( found < minimum )); then
    printf 'BP5 regression test expected at least %s occurrences of: %s (found %s)\n' \
      "$minimum" "$expected" "$found" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

check_text 'adios2-bp5'
check_text 'dtype bp5'
check_text "Resolved BP5 dataset: $test_root/bp5_fixture.bp"
check_text "Descriptor: BP5 metadata: $test_root/bp5_fixture.bp"
check_text 'File 1 : OpenGrADS BP5 attribute fixture'
check_text 'Surface pressure [hPa]'
check_text 'Air temperature [K]'
# _mask matches the inferred grid but its name cannot be written in a
# descriptor, so it is skipped with a warning and the field count stays 2.
check_text "BPOPEN warning: skipping '_mask';"
check_text 'BP5 dataset opened without a descriptor: 5 fields, 4x3x2, 2 steps'
check_text 'tdef 4 linear 04:05Z03FEB2001 10mn'
check_text 'Undef count = 1  Valid count = 11'
check_text 'Min, Max = 0 23'
check_text 'Stats[sum,sumsqr,root(sumsqr),n]:     126 2258'
check_text 'Contouring: 0 to 22 interval 2'
check_text 'Min, Max = 900 923'
check_text 'Min, Max = 1000 1023'
check_text 'Stats[sum,sumsqr,root(sumsqr),n]:     12138'
check_text 'BPOPEN error: directory is not a BP5 dataset and contains no BP5 child'
check_text 'BPOPEN error: directory contains multiple BP5 children; specify one explicitly'
check_text "BP5 Open Error: Variable 'temperature' dimension 3 has size 4, expected 5"
check_line '123'
check_line '1123'
check_line '-7777'
check_line '-9[.]99e[+]08'
check_line '1011'

# A descriptor-free open must behave like the descriptor a person would write
# for the same file. The fixture's X and Y are in metres, so both map them to
# degrees on GrADS's sphere, centred on 0; Z stays in metres; T comes from the
# CF time coordinate. Three CTLINFO prints reach the output from the fixture:
# two from descriptors and one from BPOPEN, and all three show the same axes.
check_count 'xdef 4 linear -1.34919e-05 8.99463e-06' 3
check_count 'ydef 3 linear -8.99463e-06 8.99463e-06' 3
check_count 'zdef 2 levels 1000 500' 3
check_count 'tdef 2 linear 04:05Z03FEB2001 10mn' 2
check_text 'X and Y are Cartesian; mapped to degrees on GrADS'"'"'s 6370 km sphere, centred on 0'
check_text "T from 'time': 2 steps from 04:05Z03FEB2001 every 10mn"

# Without a time coordinate, T falls back to counting steps, and says so.
check_text 'No time coordinate found; T counts steps, labelled in 1-minute intervals from 00Z01JAN2000'
check_count 'tdef 2 linear 00Z01JAN2000 1mn' 1

# Fields that are not full grids, the same through bpopen and the descriptor:
# a Z profile repeated along X and Y, a field written at the first step only
# and so the same at every time, and a global value read as a time series.
# A profile combines with a 3-D field directly, here in an x-z section.
check_text '1-D fields, the same along the axes they lack: domain_mean(t), theta_ref(z)'
check_text 'Written once, so the same at every time: terrain, theta_ref'
check_text 'Written once, so the same at every time: thref, terrain'
check_count '310 310 310 310' 2
check_count '4.5 5.5 6.5 7.5' 2
check_count '300 310' 2
check_count '50 51' 2
check_count '710 711 712 713 800 801 802 803' 2

# Time averages of a plain variable read many steps at once, and must match
# the same average taken a step at a time (forced here with "+0"), at one and
# at four calculation threads: x-y maps, an x-z section with an increment, a
# per-step global value, a field written once, and boundary weights.
check_count '2600 2601 2602 2603 2610 2611 2612 2613 2620 2621 2622 2623' 4
check_count '1400 1401 1402 1403 1410 1411 1412 1413 1420 1421 1422 1423' 4
check_count '52.5 52.5 52.5 52.5 52.5 52.5 52.5 52.5 52.5 52.5 52.5 52.5' 4
check_count '0.5 1.5 2.5 3.5 4.5 5.5 6.5 7.5 8.5 9.5 10.5 11.5' 4
check_count '21000 21010 21020 21030 21100 21110 21120 21130 21200 21210 21220 21230' 4
check_count '2010 2011 3012 2013 2110 2111 2112 2113' 4

open_count="$(grep -Fc 'BP5 dataset opened without a descriptor:' <<< "$output")"
if (( open_count < 3 )); then
  printf 'BP5 lifecycle test expected at least 3 successful descriptor-free opens, found %s\n' "$open_count" >&2
  printf '%s\n' "$output" >&2
  exit 1
fi

printf 'BP5 regression test passed: partial TDEF, attributes, descriptor precedence, bulk 2-D/shaded reads, 1-D and written-once fields, many-step time averages, errors, and repeated lifecycle.\n'
