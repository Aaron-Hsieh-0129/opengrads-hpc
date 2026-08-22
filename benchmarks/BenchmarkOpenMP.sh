#!/usr/bin/env bash
# Reproducible large-grid OpenMP benchmark. GPLv2; see ../COPYING.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-$repo_root/build}"
threads="${OPENGRADS_BENCH_THREADS:-4}"
grid_size="${OPENGRADS_BENCH_SIZE:-2048}"
time_steps="${OPENGRADS_BENCH_TIMES:-6}"
repeats="${OPENGRADS_BENCH_REPEATS:-6}"
trials="${OPENGRADS_BENCH_TRIALS:-3}"
grads_binary="$build_root/src/grads"

for value_name in threads grid_size time_steps repeats trials; do
  value="${!value_name}"
  if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
    printf '%s must be a positive integer, got: %s\n' "$value_name" "$value" >&2
    exit 2
  fi
done
if [[ ! -x "$grads_binary" ]]; then
  printf 'OpenGrADS executable not found: %s\n' "$grads_binary" >&2
  exit 1
fi
if [[ ! -r "$build_root/src/.libs/libgxdummy.so" ]]; then
  printf 'Benchmark device not found: %s\n' \
    "$build_root/src/.libs/libgxdummy.so" >&2
  exit 1
fi
if [[ ! -x /usr/bin/time ]]; then
  printf 'GNU /usr/bin/time is required.\n' >&2
  exit 1
fi

test_root="$(mktemp -d /tmp/opengrads-openmp-bench.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT
bytes=$((grid_size * grid_size * time_steps * 4))
truncate -s "$bytes" "$test_root/large.dat"

{
  printf 'dset ^large.dat\n'
  printf 'title OpenGrADS large OpenMP benchmark\n'
  printf 'undef -9.99e8\n'
  printf 'options little_endian\n'
  printf 'xdef %d linear 0 %s\n' "$grid_size" "$(awk -v n="$grid_size" 'BEGIN { print 360/n }')"
  printf 'ydef %d linear -90 %s\n' "$grid_size" "$(awk -v n="$grid_size" 'BEGIN { print 180/n }')"
  printf 'zdef 1 linear 1 1\n'
  printf 'tdef %d linear 00z01jan2000 1hr\n' "$time_steps"
  printf 'vars 1\n'
  printf 'v 0 99 synthetic-value\n'
  printf 'endvars\n'
} > "$test_root/large.ctl"

make_commands()
{
  local cpu_count="$1"
  local command_file="$2"
  local iteration

  {
    printf 'open %s/large.ctl\n' "$test_root"
    printf 'set threads %d\n' "$cpu_count"
    printf 'q threads\n'
    printf 'set x 1 %d\n' "$grid_size"
    printf 'set y 1 %d\n' "$grid_size"
    printf 'set t 1\n'
    printf 'set gxout print\n'
    printf 'define base=v+1\n'
    for ((iteration=1; iteration<=repeats; iteration++)); do
      printf 'define work=sqrt(abs(base-1))+sin(base)+cos(base)+exp(base)+log(base+1)+pow(base,2)\n'
      printf 'undefine work\n'
      printf 'd amean(base,global)\n'
      printf 'd aave(base,global)\n'
      printf 'd asum(base,global)\n'
      printf 'define work=mean(v,t=1,t=%d)\n' "$time_steps"
      printf 'undefine work\n'
      printf 'define work=sum(v,t=1,t=%d)\n' "$time_steps"
      printf 'undefine work\n'
    done
    printf 'quit\n'
  } > "$command_file"
}

run_once()
{
  local cpu_count="$1"
  local label="$2"
  local command_file="$test_root/commands-$cpu_count.txt"
  local log_file="$test_root/$label.log"
  local time_file="$test_root/$label.time"

  make_commands "$cpu_count" "$command_file"
  /usr/bin/time -f '%e' -o "$time_file" \
    env -u GA_NUM_THREADS OPENGRADS_BUILD_ROOT="$build_root" \
      OPENGRADS_COLOR=0 "$repo_root/opengrads" \
      -blu -d gxdummy -h gxdummy < "$command_file" > "$log_file" 2>&1
  if grep -Eq 'Error|ERROR|Syntax Error|DISPLAY error' "$log_file"; then
    printf 'Benchmark case %s failed. Log follows:\n' "$label" >&2
    sed -n '1,240p' "$log_file" >&2
    exit 1
  fi
  if ! grep -Fq "Calculation threads = $cpu_count (OpenMP enabled)" "$log_file"; then
    printf 'Benchmark did not activate %d thread(s).\n' "$cpu_count" >&2
    exit 1
  fi
  tr -d '[:space:]' < "$time_file"
}

median()
{
  printf '%s\n' "$@" | sort -n | awk '
    { sample[NR]=$1 }
    END {
      if (NR%2) print sample[(NR+1)/2]
      else print (sample[NR/2]+sample[NR/2+1])/2
    }'
}

printf 'Warming file cache and expression paths...\n' >&2
run_once 1 warmup-1 >/dev/null
run_once "$threads" "warmup-$threads" >/dev/null

serial_samples=()
parallel_samples=()
for ((trial=1; trial<=trials; trial++)); do
  serial_samples+=("$(run_once 1 "serial-$trial")")
  parallel_samples+=("$(run_once "$threads" "parallel-$trial")")
  printf 'Trial %d/%d: 1 CPU %ss, %d CPUs %ss\n' \
    "$trial" "$trials" "${serial_samples[-1]}" "$threads" \
    "${parallel_samples[-1]}" >&2
done

serial_median="$(median "${serial_samples[@]}")"
parallel_median="$(median "${parallel_samples[@]}")"
speedup="$(awk -v one="$serial_median" -v many="$parallel_median" \
  'BEGIN { if (many>0) printf "%.2f", one/many; else print "n/a" }')"

printf '\nOpenGrADS OpenMP large-case benchmark\n'
printf 'Grid: %dx%d, time steps: %d, workload repeats: %d, trials: %d\n\n' \
  "$grid_size" "$grid_size" "$time_steps" "$repeats" "$trials"
printf '| Calculation CPUs | Median seconds | Speedup |\n'
printf '|---:|---:|---:|\n'
printf '| 1 (original/single CPU) | %s | 1.00x |\n' "$serial_median"
printf '| %d | %s | %sx |\n' "$threads" "$parallel_median" "$speedup"
