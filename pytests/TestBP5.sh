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
mkdir -p "$test_root/empty" "$test_root/multiple"
LD_LIBRARY_PATH="$adios2_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  "$test_root/bp5_writer" "$test_root/bp5_fixture.bp"
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
open $test_root/bp5_invalid_shape.ctl
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

check_text 'adios2-bp5'
check_text 'dtype bp5'
check_text "Resolved BP5 dataset: $test_root/bp5_fixture.bp"
check_text "Descriptor: BP5 metadata: $test_root/bp5_fixture.bp"
check_text 'File 1 : OpenGrADS BP5 attribute fixture'
check_text 'Surface pressure [hPa]'
check_text 'Air temperature [K]'
check_text 'xdef 4 linear 0 0.001'
check_text 'BP5 dataset opened without a descriptor: 2 fields, 4x3x2, 2 steps'
check_text 'tdef 4 linear 00Z01JAN2000 60mn'
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

open_count="$(grep -Fc 'BP5 dataset opened without a descriptor:' <<< "$output")"
if (( open_count < 3 )); then
  printf 'BP5 lifecycle test expected at least 3 successful descriptor-free opens, found %s\n' "$open_count" >&2
  printf '%s\n' "$output" >&2
  exit 1
fi

printf 'BP5 regression test passed: partial TDEF, attributes, descriptor precedence, bulk 2-D/shaded reads, errors, and repeated lifecycle.\n'
