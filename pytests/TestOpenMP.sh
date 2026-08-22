#!/usr/bin/env bash
# OpenMP calculation regression test. GPLv2; see COPYING.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
grads_binary="$build_root/src/grads"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"

if [[ ! -x "$grads_binary" && -x "$grads_binary.exe" ]]; then
  grads_binary="$grads_binary.exe"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'OpenMP-enabled GrADS binary not found: %s\n' "$grads_binary" >&2
  exit 1
fi
if ! find "$build_root/src/.libs" -maxdepth 1 \
  \( -name 'libgxdummy*.so' -o -name 'libgxdummy*.dylib' -o -name 'libgxdummy*.dll' \) \
  -print -quit | grep -q .; then
  printf 'Headless GrADS plug-in not found in: %s\n' \
    "$build_root/src/.libs" >&2
  exit 1
fi

test_root="$(mktemp -d /tmp/opengrads-openmp-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

# Nine 256x256 little-endian float grids: three levels at three times.
# Sparse zero data makes the serial/parallel comparison exactly reproducible.
truncate -s 2359296 "$test_root/large.dat"
cat > "$test_root/large.ctl" <<'CTL'
dset ^large.dat
title OpenMP synthetic regression grid
undef -9.99e8
options little_endian
xdef 256 linear 0 1.40625
ydef 256 linear -90 0.703125
zdef 3 levels 1000 850 500
tdef 3 linear 00z01jan2000 1dy
vars 1
v 3 99 synthetic-value
endvars
CTL

run_case()
{
  local threads="$1"
  local data_file="$2"
  local command_file="$test_root/commands-${threads}.txt"
  local log_file="$test_root/run-${threads}.log"

  cat > "$command_file" <<GRADS_COMMANDS
open $test_root/large.ctl
set threads $threads
q threads
set x 1 256
set y 1 256
set z 1
set t 1
set fwrite -cl $data_file
set gxout fwrite
d sqrt(abs(v-1))+sin(v)+cos(v)+exp(v)+log(v+1)+const(v,2)
d v+2
d v*2
d v/2
d v>0
d v<=0
d pow(v+1,2)
d atan2(v,v+1)
d mag(v,v)
d const(v,7)
d cdiff(v,x)
d cdiff(v,y)
d smth9(v)
d mean(v,t=1,t=3)
d ave(v,z=1,z=3)
d sum(v,t=1,t=3)
d sumg(v,z=1,z=3)
d min(v,t=1,t=3)
d max(v,t=1,t=3)
d minloc(v,t=1,t=3)
d maxloc(v,t=1,t=3)
d gint(v,lev=1000,lev=500)
d amean(v,global)
d aave(v,global)
d asum(v,global)
d atot(v,global)
disable fwrite
quit
GRADS_COMMANDS

  env -u GA_NUM_THREADS OPENGRADS_BUILD_ROOT="$build_root" \
    OPENGRADS_COLOR=0 "$launcher" \
    -blu -d gxdummy -h gxdummy < "$command_file" > "$log_file"

  if ! grep -Fq "Calculation threads = $threads (OpenMP enabled)" "$log_file"; then
    printf 'Thread query did not report %s OpenMP threads.\n' "$threads" >&2
    cat "$log_file" >&2
    exit 1
  fi
  if grep -Eq 'Error|ERROR|Syntax Error|DISPLAY error' "$log_file"; then
    printf 'Calculation failed with %s threads.\n' "$threads" >&2
    cat "$log_file" >&2
    exit 1
  fi
}

printf 'q threads\nquit\n' > "$test_root/default-commands.txt"
default_output="$(
  env -u GA_NUM_THREADS OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -blu -d gxdummy -h gxdummy \
    < "$test_root/default-commands.txt"
)"
if ! grep -Fq 'Calculation threads = 4 (OpenMP enabled)' <<< "$default_output"; then
  printf 'Default OpenMP thread count is not 4.\n%s\n' "$default_output" >&2
  exit 1
fi

environment_output="$(
  GA_NUM_THREADS=3 OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -blu -d gxdummy -h gxdummy \
    < "$test_root/default-commands.txt"
)"
if ! grep -Fq 'Calculation threads = 3 (OpenMP enabled)' <<< "$environment_output"; then
  printf 'GA_NUM_THREADS did not select 3 threads.\n%s\n' "$environment_output" >&2
  exit 1
fi

option_output="$(
  GA_NUM_THREADS=3 OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -blu -j 2 -d gxdummy -h gxdummy \
    < "$test_root/default-commands.txt"
)"
if ! grep -Fq 'Calculation threads = 2 (OpenMP enabled)' <<< "$option_output"; then
  printf 'The -j option did not override GA_NUM_THREADS.\n%s\n' "$option_output" >&2
  exit 1
fi

run_case 1 "$test_root/serial.bin"
run_case 4 "$test_root/parallel.bin"

if ! cmp -s "$test_root/serial.bin" "$test_root/parallel.bin"; then
  printf 'Serial and four-thread calculation results differ.\n' >&2
  exit 1
fi

printf 'OpenMP regression test passed: default/control settings and common calculations agree at 1 and 4 threads.\n'
