#!/usr/bin/env bash
# Added in 2026 for data of very small and very large values. GPLv2; see
# COPYING.
#
# GrADS took the size of the data for granted in many places: tolerances,
# thresholds, and starting values fixed in the units of the data. With the
# same field scaled to 1e-30 or 1e35 this crashed, hung, drew nothing, or
# drew wrong, each checked here against the field at its own size:
#
#   contours of 1e-30       crashed: levels below 1e-15 were all set to 0,
#                           the level loop never ended and overran its table
#   barbs of 1e20           hung: 50 taken from 1e20 leaves 1e20
#   streamlines of 1e-12    drew none: a streamline stopped below 0.1
#   lines and bars of 1e-18 drew none, of 1e-16 wrong: 1 taken from each
#                           value and added back lost all below 2e-16
#   fields of 1e35          "all undefined": min and max began at +-9.99e35,
#                           and no level fell inside set cmin/cmax (+-9.99e33)
#   a constant 1e20 series  "gaaxis internal logic check 24"
#   gxout stat of 1e-15     reported the field as a constant (-5 5 1)
#   fndlvl of 1e-10         returned the lower level, never interpolating
#   gxout grid of 1e-12     printed every value as 0

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
model_ctl="$repo_root/pytests/data/model.ctl"

fail()
{
  printf 'FAIL: %s\n' "$1" >&2
  if [[ -n "${2:-}" ]]; then
    printf '%s\n' "$2" >&2
  fi
  exit 1
}

have_plugin()
{
  find "$build_root/src/.libs" -maxdepth 1 \
    \( -name "$1.so" -o -name "$1.dylib" -o -name "$1.dll" \) \
    -print -quit 2>/dev/null | grep -q .
}

# Commands on standard input; a hang ends after 60 s, which fails.
run_grads()
{
  local rc=0 output
  output="$(OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    timeout 60 "$launcher" -bl -d gxdummy -h "$1" 2>&1)" || rc=$?
  printf '%s\n' "$output"
  return "$rc"
}

# 1. Every kind of plot of the same field at 1e-30, 1e-16, 1e20 and 1e35
#    finishes, says nothing is wrong, and finds its levels.
for scale in 1e-30 1e-16 1e20 1e35; do
  output="$(run_grads gxdummy <<GRADS_COMMANDS
open $model_ctl
set lev 500
set gxout contour
d ta*$scale
set gxout shaded
d ta*$scale
q shades
c
set gxout vector
d ua*$scale;va*$scale
c
set gxout barb
d ua*$scale;va*$scale
c
set gxout stream
d ua*$scale;va*$scale
c
set y 20
set gxout bar
d ta*$scale
c
set gxout line
d ta*$scale
c
set x 1
set y 10
set t 1 5
d const(ts,$scale)
!echo done
quit
GRADS_COMMANDS
)" || fail "GrADS crashed or hung on data of $scale" "$output"
  grep -q '^done' <<< "$output" || fail "GrADS stopped early on data of $scale" "$output"
  if grep -Eqi 'logic check|undefined values|error' <<< "$output"; then
    fail "plotting data of $scale went wrong" "$output"
  fi
  grep -Eq '^Number of levels = ([2-9]|[1-9][0-9]+)$' <<< "$output" ||
    fail "shading of data of $scale found no levels" "$output"
done

# 2. gxout stat and fndlvl give what they give for the field at its own size.
output="$(run_grads gxdummy <<GRADS_COMMANDS
open $model_ctl
set lev 500
set gxout stat
d ta*1e-15
set gxout contour
set lev 1000
set x 10
set y 30
d fndlvl(ta,250,lev=1000,lev=100)
d fndlvl(ta*1e-10,250e-10,lev=1000,lev=100)
quit
GRADS_COMMANDS
)"
grep -q '^Cmin, cmax, cint = 2.3e-13 2.65e-13 5e-15$' <<< "$output" ||
  fail 'gxout stat called a field of 1e-13 a constant' "$output"
levels="$(grep '^Result value' <<< "$output" | awk '{print $4}' | sort -u | wc -l)"
(( levels == 1 )) || fail 'fndlvl found another level for the field at 1e-10' "$output"

if ! have_plugin libgxpCairo || ! command -v python3 > /dev/null 2>&1; then
  printf 'SKIP: the rendered checks need the Cairo printing plug-in and python3\n'
  printf 'Extreme value checks passed (no rendering)\n'
  exit 0
fi

# 3. Rendered, the scaled field covers the plot as the field itself does.
test_root="$(mktemp -d /tmp/opengrads-extreme-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

# dark pixels inside the plot area of a page printed at 100 pixels an inch
cat > "$test_root/ink.py" <<'PYTHON'
import struct, sys, zlib
png, x1, x2, y1, y2 = sys.argv[1], *map(float, sys.argv[2:6])
d = open(png, 'rb').read()
p, idat = 8, b''
while p < len(d):
    n, = struct.unpack('>I', d[p:p + 4])
    kind, body = d[p + 4:p + 8], d[p + 8:p + 8 + n]
    if kind == b'IHDR':
        w, h, depth, ctype = struct.unpack('>IIBB', body[:10])
    if kind == b'IDAT':
        idat += body
    p += 12 + n
bpp = 3 if ctype == 2 else 4
raw, stride = zlib.decompress(idat), w * bpp
rows, prev, q = [], bytearray(stride), 0
for y in range(h):
    f, line = raw[q], bytearray(raw[q + 1:q + 1 + stride])
    q += 1 + stride
    for i in range(stride):
        a = line[i - bpp] if i >= bpp else 0
        b, c = prev[i], prev[i - bpp] if i >= bpp else 0
        if f == 1: line[i] = (line[i] + a) & 255
        elif f == 2: line[i] = (line[i] + b) & 255
        elif f == 3: line[i] = (line[i] + (a + b) // 2) & 255
        elif f == 4:
            pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
            line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
    rows.append(line)
    prev = line
ink = 0
for py in range(int((8.5 - y2) * 100) + 3, int((8.5 - y1) * 100) - 3):
    for px in range(int(x1 * 100) + 3, int(x2 * 100) - 3):
        r, g, b = rows[py][px * bpp:px * bpp + 3]
        ink += r + g + b < 384
print(ink)
PYTHON

# ink of a plot: plot NAME COMMANDS...; the plot area from q gxinfo
ink()
{
  local name="$1" x1 x2 y1 y2
  shift
  read -r x1 x2 y1 y2 <<< "$(
    {
      printf 'open %s\nset lev 500\nset grads off\nset grid off\nset mpdraw off\n' "$model_ctl"
      printf '%s\n' "$@"
      printf 'q gxinfo\nprintim %s/%s.png x1100 y850 white\nquit\n' "$test_root" "$name"
    } | run_grads Cairo | awk '/^X Limits/ {x1=$4; x2=$6} /^Y Limits/ {y1=$4; y2=$6}
                               END {print x1, x2, y1, y2}')"
  python3 "$test_root/ink.py" "$test_root/$name.png" "$x1" "$x2" "$y1" "$y2"
}

# the scaled plot has at least half the ink of the plot at the field's size
alike()
{
  local what="$1" one="$2" scaled="$3"
  (( one > 50 )) || fail "$what: the reference plot drew nothing ($one)"
  (( scaled * 2 > one )) || fail "$what: drew $scaled dark pixels against $one"
}

alike 'streamlines of 1e-12' "$(ink s1 'set gxout stream' 'd ua;va')" \
  "$(ink s12 'set gxout stream' 'd ua*1e-12;va*1e-12')"
alike 'bars of 1e-18' "$(ink b1 'set y 20' 'set gxout bar' 'd ta')" \
  "$(ink b18 'set y 20' 'set gxout bar' 'd ta*1e-18')"
alike 'a line of 1e-30' "$(ink l1 'set y 20' 'd ta')" \
  "$(ink l30 'set y 20' 'd ta*1e-30')"
alike 'contours of 1e-30' "$(ink c1 'set clab off' 'd ta')" \
  "$(ink c30 'set clab off' 'd ta*1e-30')"
alike 'shading of 1e35' "$(ink h1 'set gxout shaded' 'd ta')" \
  "$(ink h35 'set gxout shaded' 'd ta*1e35')"

printf 'Extreme value checks passed: every kind of plot at 1e-30 to 1e35, stat, fndlvl\n'
