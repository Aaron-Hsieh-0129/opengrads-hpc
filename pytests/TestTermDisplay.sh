#!/usr/bin/env bash
# Added in 2026 for the terminal display. GPLv2; see COPYING.
#
# The terminal display (-d Term) draws without an X server and writes the
# picture whenever GrADS waits for the user, plus each animation frame. These
# checks cover what can be seen without a real iTerm2: the PNG and its size,
# one picture per prompt, live frames, looping GIF animations (decoded by an
# independent decoder when python3 is available), the inline image sequence,
# cleanup, and the viewer's FIFO wake-up, tmux wrapping and multipart
# transfer. Skipped when the build has no Cairo, which the display needs.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
viewer="$repo_root/libexec/grads-termview"
model_ctl="$repo_root/pytests/data/model.ctl"

if [[ ! -r "$build_root/src/.libs/libgxdTerm.so" ]]; then
  printf 'SKIP: terminal display plug-in was not built (needs Cairo)\n'
  exit 0
fi

test_root="$(mktemp -d /tmp/opengrads-term-test.XXXXXX)"
background_pids=()
cleanup()
{
  local pid
  for pid in "${background_pids[@]}"; do
    kill "$pid" 2>/dev/null || true
  done
  rm -rf -- "$test_root"
}
trap cleanup EXIT

fail()
{
  printf 'FAIL: %s\n' "$1" >&2
  if [[ -n "${2:-}" ]]; then
    printf '%s\n' "$2" >&2
  fi
  exit 1
}

# PNG width and height, from the IHDR chunk.
png_size()
{
  local hex
  hex="$(od -An -tx1 -j16 -N8 "$1" | tr -d ' \n')"
  printf '%dx%d\n' "$((16#${hex:0:8}))" "$((16#${hex:8:8}))"
}

# Walk the GIF block structure: "WxH frames looping first-delay trailer".
gif_info()
{
  od -An -v -tu1 "$1" | awk '
    { for (i = 1; i <= NF; i++) b[n++] = $i }
    END {
      if (b[0] != 71 || b[1] != 73 || b[2] != 70) { print "not-a-gif"; exit }
      p = 13
      if (int(b[10] / 128) % 2) p += 3 * 2 ^ (b[10] % 8 + 1)
      frames = 0; loop = 0; delay = -1; trailer = 0
      while (p < n) {
        c = b[p]
        if (c == 59) { trailer = 1; break }
        if (c == 33) {
          if (b[p + 1] == 255 && b[p + 3] == 78 && b[p + 11] == 50) loop = 1
          if (b[p + 1] == 249 && delay < 0) delay = b[p + 4] + 256 * b[p + 5]
          p += 2
        } else if (c == 44) {
          frames++
          packed = b[p + 9]
          p += 10
          if (int(packed / 128) % 2) p += 3 * 2 ^ (packed % 8 + 1)
          p++
        } else { print "bad-block"; exit }
        while (b[p] != 0 && p < n) p += b[p] + 1
        p++
      }
      printf "%dx%d %d %d %d %d\n", b[6] + 256 * b[7], b[8] + 256 * b[9],
        frames, loop, delay, trailer
    }'
}

# Decode every frame of a GIF, LZW included, and print "WxH frames". This is
# an independent check of the encoder; it is skipped without python3.
gif_decode()
{
  python3 - "$1" <<'PYTHON'
import sys
d = open(sys.argv[1], 'rb').read()
assert d[:6] == b'GIF89a', 'header'
W = d[6] | d[7] << 8
H = d[8] | d[9] << 8
p = 13
if d[10] & 0x80:
    p += 3 * (2 << (d[10] & 7))
frames = 0
while True:
    c = d[p]
    if c == 0x3b:
        break
    if c == 0x21:
        p += 2
        while d[p]:
            p += d[p] + 1
        p += 1
        continue
    assert c == 0x2c, 'block %d at %d' % (c, p)
    x, y, w, h = [d[p + 1 + 2 * i] | d[p + 2 + 2 * i] << 8 for i in range(4)]
    packed = d[p + 9]
    p += 10
    assert x + w <= W and y + h <= H, 'frame outside the canvas'
    ncolors = 0
    if packed & 0x80:
        ncolors = 2 << (packed & 7)
        p += 3 * ncolors
    mcs = d[p]
    p += 1
    data = bytearray()
    while d[p]:
        data += d[p + 1:p + 1 + d[p]]
        p += d[p] + 1
    p += 1
    clear, eoi = 1 << mcs, (1 << mcs) + 1
    table = [bytes([i]) for i in range(clear)] + [b'', b'']
    cs, prev, out, acc, nb, i = mcs + 1, None, bytearray(), 0, 0, 0
    while True:
        while nb < cs and i < len(data):
            acc |= data[i] << nb
            nb += 8
            i += 1
        assert nb >= cs, 'data ends without an end code'
        code = acc & ((1 << cs) - 1)
        acc >>= cs
        nb -= cs
        if code == clear:
            table = table[:clear + 2]
            cs, prev = mcs + 1, None
            continue
        if code == eoi:
            break
        if code < len(table):
            entry = table[code]
        elif code == len(table) and prev is not None:
            entry = prev + prev[:1]
        else:
            raise AssertionError('invalid code %d' % code)
        out += entry
        if prev is not None and len(table) < 4096:
            table.append(prev + entry[:1])
        prev = entry
        if len(table) == (1 << cs) and cs < 12:
            cs += 1
    assert len(out) == w * h, 'frame has %d pixels, expected %d' % (len(out), w * h)
    assert max(out) < ncolors, 'colour index outside the palette'
    frames += 1
print('%dx%d %d' % (W, H, frames))
PYTHON
}

# The number in "seq", which counts the pictures shown.
seq_number()
{
  local n name
  read -r n name < "$1"
  printf '%s\n' "$n"
}

run_grads()
{
  OPENGRADS_BUILD_ROOT="$build_root" \
  OPENGRADS_COLOR=0 \
  GA_TERM_VIEWER="$viewer" \
    "$launcher" -l -d Term "$@" 2>&1
}

cat > "$test_root/clearloop.gs" <<'GRADS_SCRIPT'
i=1
while (i<=5)
  'clear'
  'set t 'i
  'd ts'
  i=i+1
endwhile
GRADS_SCRIPT

cat > "$test_root/dbuffloop.gs" <<'GRADS_SCRIPT'
'set dbuff on'
i=1
while (i<=5)
  'set t 'i
  'd ts'
  'swap'
  i=i+1
endwhile
GRADS_SCRIPT

# 1. File mode with animation off: the picture, its size, and one picture
#    per prompt however many times a script draws.
pictures="$test_root/pictures"
output="$(
  GA_TERM_MODE=file GA_TERM_DIR="$pictures" GA_TERM_SCALE=1 GA_TERM_SYNC=1 \
  GA_TERM_ANIM=off run_grads -g 400x300 <<GRADS_COMMANDS
open $model_ctl
d ts
!cp $pictures/seq $test_root/seq_after_display
run $test_root/clearloop.gs
!cp $pictures/seq $test_root/seq_after_script
q dims
!cp $pictures/seq $test_root/seq_after_query
quit
GRADS_COMMANDS
)"

[[ -s "$pictures/plot.png" ]] || fail 'no plot.png was written' "$output"
[[ "$(head -c 4 "$pictures/plot.png" | od -An -c | tr -d ' ')" == '211PNG' ]] ||
  fail 'plot.png is not a PNG'
size="$(png_size "$pictures/plot.png")"
[[ "$size" == 400x300 ]] || fail "picture is $size, expected 400x300 from -g"
if command -v python3 > /dev/null 2>&1; then
  python3 - "$pictures/plot.png" <<'PYTHON' || fail 'plot.png does not decode'
import sys, zlib, struct
d = open(sys.argv[1], 'rb').read()
p, idat = 8, b''
while p < len(d):
    n, = struct.unpack('>I', d[p:p + 4])
    kind, body, crc = d[p + 4:p + 8], d[p + 8:p + 8 + n], d[p + 8 + n:p + 12 + n]
    assert struct.unpack('>I', crc)[0] == zlib.crc32(kind + body), 'bad CRC'
    if kind == b'IHDR':
        w, h = struct.unpack('>II', body[:8])
    if kind == b'IDAT':
        idat += body
    p += 12 + n
assert len(zlib.decompress(idat)) == h * (1 + 3 * w), 'wrong amount of pixel data'
PYTHON
fi

after_display="$(seq_number "$test_root/seq_after_display")"
after_script="$(seq_number "$test_root/seq_after_script")"
after_query="$(seq_number "$test_root/seq_after_query")"
(( after_script == after_display + 1 )) ||
  fail "a script drawing five times wrote $((after_script - after_display)) pictures, expected 1"
(( after_query == after_script )) ||
  fail 'a command that draws nothing wrote a new picture'
grep -Fq 'pictures are written to' <<< "$output" ||
  fail 'file mode did not say where the pictures go' "$output"

# 2. By default, cleared frames are shown as they are made, but a script
#    that clears between pictures does not become a loop.
live="$test_root/live"
GA_TERM_MODE=file GA_TERM_DIR="$live" GA_TERM_SCALE=1 GA_TERM_SYNC=1 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
d ts
!cp $live/seq $test_root/live_before
run $test_root/clearloop.gs
!cp $live/seq $test_root/live_after
quit
GRADS_COMMANDS
shown=$(( $(seq_number "$test_root/live_after") - $(seq_number "$test_root/live_before") ))
(( shown == 5 )) || fail "a five-frame script showed $shown pictures, expected 5"
read -r _ name < "$test_root/live_after"
[[ "$name" == plot.png && ! -e "$live/plot.gif" ]] ||
  fail 'cleared frames turned into an animation in auto mode'

# 3. A double-buffered loop leaves a looping GIF of all its frames, which
#    decodes; GA_TERM_ANIM=gif does the same for cleared frames.
check_animation()
{
  local label="$1" dir="$2" expect="$3" info
  [[ -s "$dir/plot.gif" ]] || fail "$label: no animation was written"
  info="$(gif_info "$dir/plot.gif")"
  [[ "$info" == "$expect" ]] ||
    fail "$label: GIF is \"$info\", expected \"$expect\" (size frames loop delay trailer)"
  if command -v python3 > /dev/null 2>&1; then
    info="$(gif_decode "$dir/plot.gif")" || fail "$label: the GIF does not decode"
    [[ "$info" == "${expect% * * *}" ]] ||
      fail "$label: decoded \"$info\", expected \"${expect% * * *}\""
  fi
}

GA_TERM_MODE=file GA_TERM_DIR="$test_root/dbuff" GA_TERM_SYNC=1 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
check_animation 'double-buffer loop' "$test_root/dbuff" '400x300 5 1 20 1'
read -r _ name < "$test_root/dbuff/seq"
[[ "$name" == plot.gif ]] || fail "the last picture shown is $name, not the animation"

GA_TERM_MODE=file GA_TERM_DIR="$test_root/looping" GA_TERM_SYNC=1 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
set looping on
set t 1 5
d ts
quit
GRADS_COMMANDS
check_animation 'set looping on' "$test_root/looping" '400x300 5 1 20 1'

GA_TERM_MODE=file GA_TERM_DIR="$test_root/gifmode" GA_TERM_SYNC=1 \
GA_TERM_ANIM=gif GA_TERM_ANIM_DELAY=0.5 GA_TERM_ANIM_SCALE=0.5 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/clearloop.gs
quit
GRADS_COMMANDS
check_animation 'GA_TERM_ANIM=gif' "$test_root/gifmode" '200x150 5 1 50 1'

output="$(
  GA_TERM_MODE=file GA_TERM_DIR="$test_root/cut" GA_TERM_SYNC=1 \
  GA_TERM_ANIM_MAX=3 run_grads -g 400x300 <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
)"
check_animation 'GA_TERM_ANIM_MAX=3' "$test_root/cut" '400x300 3 1 20 1'
grep -Fq 'keeps its first 3 frames; 2 more were left out' <<< "$output" ||
  fail 'a cut-short animation gave no warning' "$output"

# 4. Without GA_TERM_SYNC the pictures are written in the background; what
#    was handed over is still written before GrADS exits.
GA_TERM_MODE=file GA_TERM_DIR="$test_root/async" \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
check_animation 'background writing' "$test_root/async" '400x300 5 1 20 1'

# 5. Inline mode: one image per drawn picture, none for a cleared page, an
#    animation as one GIF, and the temporary directory is gone afterwards.
#    Without a terminal the images go to standard output.
mkdir "$test_root/tmp"
output="$(
  TMPDIR="$test_root/tmp" GA_TERM_MODE=inline GA_TERM_SCALE=1 \
    run_grads -g 200x150 <<GRADS_COMMANDS
open $model_ctl
d ts
clear
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
)"
images="$(grep -o $'\033\\]1337;File=inline=1;size=[0-9]*' <<< "$output" | wc -l)"
(( images == 2 )) || fail "inline mode printed $images images, expected 2"
grep -ao $'\033\\]1337;File=[^:]*:R0lGODlh' <<< "$output" > /dev/null ||
  fail 'inline mode did not print the animation as a GIF'
if compgen -G "$test_root/tmp/grads-term-*" > /dev/null; then
  fail 'the temporary picture directory was left behind'
fi

# 6. Outside tmux, asking for tmux falls back to inline.
output="$(
  unset TMUX
  TMPDIR="$test_root/tmp" GA_TERM_MODE=tmux run_grads <<'GRADS_COMMANDS'
quit
GRADS_COMMANDS
)"
grep -Fq 'not running inside tmux' <<< "$output" ||
  fail 'GA_TERM_MODE=tmux outside tmux gave no warning' "$output"

# 7. The viewer wraps the image for tmux, sizes it to the pane, wakes up for
#    a new picture, sends a picture over 1 MiB in parts, and exits with the
#    GrADS process it follows.
view="$test_root/view"
mkdir "$view"
mkfifo "$view/notify"
cp "$pictures/plot.png" "$view/plot.png"
printf '1 plot.png\n' > "$view/seq"
head -c 1100000 /dev/urandom > "$view/plot.gif"
sleep 30 &
follow_pid=$!
background_pids+=("$follow_pid")
TMUX=/tmp/fake,1,0 "$viewer" "$view" "$follow_pid" \
  > "$test_root/viewer.out" 2>/dev/null < /dev/null &
viewer_pid=$!
background_pids+=("$viewer_pid")

wait_for()
{
  local pattern="$1" i
  for i in $(seq 1 50); do
    if grep -aFq "$pattern" "$test_root/viewer.out" 2>/dev/null; then
      return 0
    fi
    sleep 0.1
  done
  return 1
}

wait_for $'\033Ptmux;\033\033]1337;File=inline=1;' ||
  fail 'the viewer did not show the picture, wrapped for tmux'
grep -aEq 'width=[0-9]+;height=[0-9]+;preserveAspectRatio=1:' \
  "$test_root/viewer.out" || fail 'the viewer did not size the image in cells'
printf '2 plot.gif\n' > "$view/seq"
printf '\n' > "$view/notify"
wait_for $'1337;FileEnd' || fail 'the viewer did not send the new picture'
grep -aFq $'\033Ptmux;\033\033]1337;MultipartFile=inline=1;size=1100000;' \
  "$test_root/viewer.out" || fail 'a large picture was not sent in parts'
grep -ao $'1337;FilePart=[A-Za-z0-9+/=]*' "$test_root/viewer.out" |
  sed 's/^1337;FilePart=//' | tr -d '\n' | base64 -d 2>/dev/null |
  cmp -s - "$view/plot.gif" || fail 'the parts do not add up to the picture'

kill "$follow_pid"
for i in $(seq 1 30); do
  kill -0 "$viewer_pid" 2>/dev/null || break
  sleep 0.1
done
if kill -0 "$viewer_pid" 2>/dev/null; then
  fail 'the viewer did not exit after the GrADS process it follows'
fi

printf 'Terminal display checks passed\n'
