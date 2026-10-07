#!/usr/bin/env bash
# Added in 2026 for axes of very small and large values. GPLv2; see COPYING.
#
# A line graph of values around 1e-10 stopped with "gaaxis internal logic
# check 25": the label interval, about 1e-11, was compared with zero to
# within 1e-8. Checks here: such plots draw, at any scale, along time or a
# space dimension, and with a tiny range on a large value; shading finds
# its levels at 1e-25; and, rendered, an axis of very small or large values
# carries one power of ten at its end (as matplotlib does) where an axis of
# ordinary values, or one with a label format set, carries nothing.

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

run_grads()
{
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 \
    "$launcher" -bl -d gxdummy -h "$1" 2>&1
}

# 1. These all drew nothing but "logic check 25" before.
output="$(run_grads gxdummy <<GRADS_COMMANDS
open $model_ctl
set x 1
set y 10
set t 1 5
d ts*1e-12
c
d ts*1e-30
c
d ts*1e-7
c
d ts*1e-12+1
c
set t 1
set x 1 20
d ts*1e-12
c
set y 1 20
set gxout shaded
d ts*1e-25
q shades
quit
GRADS_COMMANDS
)"
if grep -q 'logic check' <<< "$output"; then
  fail 'a plot of small values hit an internal logic check' "$output"
fi
grep -q '^Number of levels = 1[0-9]' <<< "$output" ||
  fail 'shading of values around 1e-25 found no levels' "$output"

if ! have_plugin libgxpCairo || ! command -v python3 > /dev/null 2>&1; then
  printf 'SKIP: the rendered axis checks need the Cairo printing plug-in and python3\n'
  printf 'Axis scale checks passed (no rendering)\n'
  exit 0
fi

# 2. Rendered: is there ink where the power of ten goes? Pages are 11 by
#    8.5 inches at 100 pixels an inch; the plot area comes from q gxinfo.
test_root="$(mktemp -d /tmp/opengrads-axis-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

cat > "$test_root/ink.py" <<'PYTHON'
import struct, sys, zlib
# ink.py PNG X1 X2 Y1 Y2: dark pixels in that box, in page inches
png, box = sys.argv[1], [float(v) for v in sys.argv[2:6]]
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
assert depth == 8 and ctype in (2, 6), 'unexpected PNG kind'
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
x1, x2, y1, y2 = box
ink = 0
for py in range(max(0, int((8.5 - y2) * 100)), min(h, int((8.5 - y1) * 100))):
    for px in range(max(0, int(x1 * 100)), min(w, int(x2 * 100))):
        r, g, b = rows[py][px * bpp:px * bpp + 3]
        ink += r + g + b < 384
print(ink)
PYTHON

# Draw, print, and report the plot area: "x1 x2 y1 y2"
plot()
{
  local name="$1"
  shift
  {
    printf 'open %s\nset x 1\nset y 10\nset t 1 5\n' "$model_ctl"
    printf '%s\n' "$@"
    printf 'q gxinfo\nprintim %s/%s.png x1100 y850 white\nquit\n' "$test_root" "$name"
  } | run_grads Cairo | awk '/^X Limits/ {x1=$4; x2=$6} /^Y Limits/ {y1=$4; y2=$6}
                             END {print x1, x2, y1, y2}'
}

# ink at the top of the Y axis, left of the plot: over the label column
yfactor()
{
  local x1 x2 y1 y2
  read -r x1 x2 y1 y2 <<< "$(plot "$@")"
  python3 "$test_root/ink.py" "$test_root/$1.png" \
    "$(awk -v v="$x1" 'BEGIN {print v - 1.2}')" "$(awk -v v="$x1" 'BEGIN {print v - 0.05}')" \
    "$(awk -v v="$y2" 'BEGIN {print v + 0.1}')" "$(awk -v v="$y2" 'BEGIN {print v + 0.35}')"
}

# ink under the X axis labels at the right end
xfactor()
{
  local x1 x2 y1 y2
  read -r x1 x2 y1 y2 <<< "$(plot "$@")"
  python3 "$test_root/ink.py" "$test_root/$1.png" \
    "$(awk -v v="$x2" 'BEGIN {print v - 0.8}')" "$x2" \
    "$(awk -v v="$y1" 'BEGIN {print v - 0.45}')" "$(awk -v v="$y1" 'BEGIN {print v - 0.3}')"
}

ink="$(yfactor tiny 'd ts*1e-12')"
(( ink > 5 )) || fail "no power of ten at the top of the Y axis for values of 1e-10 (ink $ink)"
ink="$(yfactor large 'd ts*1e7')"
(( ink > 5 )) || fail "no power of ten at the top of the Y axis for values of 1e9 (ink $ink)"
ink="$(yfactor normal 'd ts')"
(( ink == 0 )) || fail "something was drawn over the Y labels of ordinary values (ink $ink)"
ink="$(yfactor format 'set ylab %g' 'd ts*1e-12')"
(( ink == 0 )) || fail "a power of ten was added to labels with a format set (ink $ink)"
ink="$(xfactor xtiny 'set t 1' 'set x 1 40' 'set xyrev on' 'd ts*1e-9')"
(( ink > 5 )) || fail "no power of ten under the X axis for values of 1e-7 (ink $ink)"
ink="$(xfactor xnormal 'set t 1' 'set x 1 40' 'set xyrev on' 'd ts')"
(( ink == 0 )) || fail "something was drawn under the X labels of ordinary values (ink $ink)"

printf 'Axis scale checks passed: tiny and large values draw, shading levels, one power of ten per axis\n'
