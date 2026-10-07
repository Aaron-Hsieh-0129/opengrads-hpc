#!/usr/bin/env bash
# Added in 2026 for the terminal display. GPLv2; see COPYING.
#
# The terminal display (-d Term) draws without an X server and writes the
# picture whenever GrADS waits for the user, plus each animation frame. These
# checks cover what can be seen without a real iTerm2: the PNG and its size,
# one picture per prompt, live frames, looping GIF animations (decoded by an
# independent decoder when python3 is available), the inline image sequence,
# cleanup, the viewer's FIFO wake-up, tmux wrapping and multipart transfer,
# and, inside a real tmux (plain, in small steps, and -CC), that every
# picture arrives whole at its pane. Skipped when the build has no Cairo,
# which the display needs.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
viewer="$repo_root/libexec/grads-termview"
model_ctl="$repo_root/pytests/data/model.ctl"

case "${OPENGRADS_RELEASE_PLATFORM:-$(uname -s)}" in
  Darwin*) plugin_ext=dylib ;;
  *) plugin_ext=so ;;
esac
if [[ ! -r "$build_root/src/.libs/libgxdTerm.$plugin_ext" ]]; then
  printf 'SKIP: terminal display plug-in was not built (needs Cairo)\n'
  exit 0
fi

test_root="$(mktemp -d /tmp/opengrads-term-test.XXXXXX)"
background_pids=()
cleanup()
{
  local pid
  # bash 3.2, on macOS, takes an empty array for an unset one under set -u
  for pid in ${background_pids[@]+"${background_pids[@]}"}; do
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

# 2. Frames are shown as they are made: a script that clears between
#    pictures shows each one.
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

# 3. By default a double-buffered loop shows every frame, in order, with no
#    GIF afterwards; written in the background, no frame is skipped.
GA_TERM_MODE=file GA_TERM_DIR="$test_root/frames" \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
read -r n name < "$test_root/frames/seq"
(( n == 6 )) || fail "a blank page and five frames made $n pictures, expected 6"
[[ "$name" == plot.png && ! -e "$test_root/frames/plot.gif" ]] ||
  fail 'a double-buffered loop left a GIF without GA_TERM_ANIM=gif'

# 4. With GA_TERM_ANIM=gif a double-buffered loop also leaves a looping GIF
#    of all its frames, which decodes.
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

GA_TERM_MODE=file GA_TERM_DIR="$test_root/dbuff" GA_TERM_SYNC=1 GA_TERM_ANIM=gif \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
check_animation 'double-buffer loop' "$test_root/dbuff" '400x300 5 1 20 1'
read -r _ name < "$test_root/dbuff/seq"
[[ "$name" == plot.gif ]] || fail "the last picture shown is $name, not the animation"

GA_TERM_MODE=file GA_TERM_DIR="$test_root/looping" GA_TERM_SYNC=1 GA_TERM_ANIM=gif \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
set looping on
set t 1 5
d ts
quit
GRADS_COMMANDS
check_animation 'set looping on' "$test_root/looping" '400x300 5 1 20 1'

GA_TERM_MODE=file GA_TERM_DIR="$test_root/gifopts" GA_TERM_SYNC=1 \
GA_TERM_ANIM=gif GA_TERM_ANIM_DELAY=0.5 GA_TERM_ANIM_SCALE=0.5 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
check_animation 'GIF delay and scale' "$test_root/gifopts" '200x150 5 1 50 1'

output="$(
  GA_TERM_MODE=file GA_TERM_DIR="$test_root/cut" GA_TERM_SYNC=1 GA_TERM_ANIM=gif \
  GA_TERM_ANIM_MAX=3 run_grads -g 400x300 <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
)"
check_animation 'GA_TERM_ANIM_MAX=3' "$test_root/cut" '400x300 3 1 20 1'
grep -Fq 'keeps its first 3 frames; 2 more were left out' <<< "$output" ||
  fail 'a cut-short animation gave no warning' "$output"

GA_TERM_MODE=file GA_TERM_DIR="$test_root/async" GA_TERM_ANIM=gif \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
check_animation 'GIF written in the background' "$test_root/async" '400x300 5 1 20 1'

# 5. Inline mode: one image per drawn picture, none for a cleared page, the
#    last frame of an animation (a GIF with GA_TERM_ANIM=gif), and the
#    temporary directory is gone afterwards. Without a terminal the images
#    go to standard output.
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
if grep -aq $'\033\\]1337;File=[^:]*:R0lGODlh' <<< "$output"; then
  fail 'inline mode printed a GIF without GA_TERM_ANIM=gif'
fi
output="$(
  TMPDIR="$test_root/tmp" GA_TERM_MODE=inline GA_TERM_SCALE=1 GA_TERM_ANIM=gif \
    run_grads -g 200x150 <<GRADS_COMMANDS
open $model_ctl
run $test_root/dbuffloop.gs
quit
GRADS_COMMANDS
)"
grep -aq $'\033\\]1337;File=[^:]*:R0lGODlh' <<< "$output" ||
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

# 8. In --hold mode the viewer only keeps the pane: it prints nothing (GrADS
#    writes all the pane shows, so nothing can land after a picture), and
#    exits with GrADS.
sleep 30 &
follow_pid=$!
background_pids+=("$follow_pid")
"$viewer" --hold "$view" "$follow_pid" > "$test_root/hold.out" 2>/dev/null < /dev/null &
hold_pid=$!
background_pids+=("$hold_pid")
sleep 0.5
kill "$follow_pid"
for i in $(seq 1 30); do
  kill -0 "$hold_pid" 2>/dev/null || break
  sleep 0.1
done
if kill -0 "$hold_pid" 2>/dev/null; then
  fail 'the --hold viewer did not exit after GrADS'
fi
if [[ -s "$test_root/hold.out" ]]; then
  fail 'the --hold viewer printed something' "$(od -c "$test_root/hold.out" | head -5)"
fi

if ! command -v python3 > /dev/null 2>&1; then
  printf 'SKIP: Ctrl-C and tmux checks need python3\n'
  printf 'Terminal display checks passed\n'
  exit 0
fi

# 9. Ctrl-C, typed into a terminal. Half-way through a command line it
#    starts a fresh line, at an empty prompt it does nothing, and it never
#    ends GrADS. During an animation it stops the script, and nothing more
#    is shown until the next command draws. (The script leaves double
#    buffering on, so that command must turn it off, as it would with X.)
cat > "$test_root/long.gs" <<'GRADS_SCRIPT'
'set dbuff on'
i=1
while (i<=400)
  'set t '%(math_mod(i-1,5)+1)
  'd ts'
  'swap'
  i=i+1
endwhile
say 'loop finished'
GRADS_SCRIPT
cat > "$test_root/ctrlc.py" <<'PYTHON'
import os, pty, sys, time, select
launcher, ctl, root = sys.argv[1:4]
pid, fd = pty.fork()
if pid == 0:
    os.execv(launcher, [launcher, '-l', '-d', 'Term', '-g', '400x300'])
out = bytearray()
def pump(t):
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try: out.extend(os.read(fd, 65536))
            except OSError: return
def send(b, t=0.5):
    os.write(fd, b); pump(t)
pump(2)
send(('open ' + ctl + '\r').encode(), 1)
send(b'd ts')
send(b'\x03')
send(b'q dims\r', 1)
send(b'\x03'); send(b'\x03')
send(('run ' + root + '/long.gs\r').encode(), 2)
send(b'\x03', 1.5)
send(('!cp ' + root + '/ctrlc_pics/seq ' + root + '/seq_a\r').encode(), 1.5)
send(('!cp ' + root + '/ctrlc_pics/seq ' + root + '/seq_b\r').encode(), 1)
send(b'set dbuff off\r', 1)      # the interrupted script left it on
send(('!cp ' + root + '/ctrlc_pics/seq ' + root + '/seq_b2\r').encode(), 1)
send(b'd ps\r', 1)
send(('!cp ' + root + '/ctrlc_pics/seq ' + root + '/seq_c\r').encode(), 1)
alive = os.waitpid(pid, os.WNOHANG) == (0, 0)
if alive: send(b'quit\r', 2)
text = out.decode(errors='replace')
print('alive' if alive else 'exited')
print('typed-line-ran' if 'Contouring' in text.split('q dims')[0] else 'typed-line-dropped')
print('dims' if 'Default file number' in text else 'no-dims')
print('loop-finished' if 'loop finished' in text else 'loop-stopped')
PYTHON
mkdir "$test_root/ctrlc_pics"
result="$(
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 GA_TERM_VIEWER="$viewer" \
  GA_TERM_MODE=file GA_TERM_DIR="$test_root/ctrlc_pics" GA_TERM_SYNC=1 \
    python3 "$test_root/ctrlc.py" "$launcher" "$model_ctl" "$test_root"
)"
[[ "$result" == *alive* ]] || fail 'Ctrl-C ended GrADS' "$result"
[[ "$result" == *typed-line-dropped* ]] ||
  fail 'Ctrl-C did not drop the half-typed command' "$result"
[[ "$result" == *dims* ]] || fail 'the command typed after Ctrl-C did not run' "$result"
[[ "$result" == *loop-stopped* ]] || fail 'Ctrl-C did not stop the animation' "$result"
a="$(seq_number "$test_root/seq_a")"
b="$(seq_number "$test_root/seq_b")"
b2="$(seq_number "$test_root/seq_b2")"
c="$(seq_number "$test_root/seq_c")"
(( a == b )) || fail "pictures were still shown after Ctrl-C ($a, then $b)"
(( c == b2 + 1 )) || fail "the next command after Ctrl-C showed $((c - b2)) pictures, expected 1"

# 10. Inside tmux, with a client attached on a terminal read at 1 MB/s,
#     like an ssh link: the pane says it is waiting until the first picture,
#     every frame arrives whole, each is placed at the viewer pane, and the
#     pane closes with GrADS. Three ways: plain tmux; plain tmux handed the
#     picture a few KB at a time, in parts, as for tmux before 3.3, which
#     drops output the terminal cannot take fast enough; and iTerm2's tmux
#     integration (tmux -CC), where the sequences go into the pane unwrapped
#     and placed at its own top left corner. TEST_TMUX names another tmux to
#     run, such as a build of 3.2a.
tmux_bin="${TEST_TMUX:-tmux}"
if ! command -v "$tmux_bin" > /dev/null 2>&1; then
  printf 'SKIP: tmux checks need tmux\n'
  printf 'Terminal display checks passed\n'
  exit 0
fi
cat > "$test_root/intmux.py" <<'PYTHON'
import os, pty, fcntl, termios, struct, subprocess, time, select, re, sys, base64
launcher, ctl, root, viewer, build, tmux, how = sys.argv[1:8]
control = how == 'control'
sock = root + '/tmux-' + how + '.sock'
env = dict(os.environ, TERM='xterm-256color', OPENGRADS_COLOR='0',
           OPENGRADS_BUILD_ROOT=build, GA_TERM_VIEWER=viewer)
if os.sep in tmux:
    env['PATH'] = os.path.dirname(os.path.abspath(tmux)) + os.pathsep + env['PATH']
for k in ('TMUX', 'TMUX_PANE', 'LC_TERMINAL', 'GA_TERM_TMUX_STEP'):
    env.pop(k, None)
if how == 'steps':
    env.update(GA_TERM_TMUX_STEP='4096', LC_TERMINAL='iTerm2')
COLS, ROWS = 160, 45
tm = [tmux, '-S', sock, '-f', '/dev/null']
subprocess.run(tm + ['new-session', '-d', '-s', 't', '-x', str(COLS), '-y', str(ROWS),
               launcher + ' -l -d Term; sleep 30'], env=env, check=True)
master, slave = pty.openpty()
fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', ROWS, COLS, 0, 0))
client = subprocess.Popen(tm + (['-CC'] if control else []) + ['attach', '-t', 't'],
                          stdin=slave, stdout=slave, stderr=slave, env=env,
                          start_new_session=True)
if control:
    os.write(master, b'refresh-client -C %d,%d\n' % (COLS, ROWS))
out = bytearray()
def pump(most, idle=1.5):
    # read at about 1 MB/s until nothing has come for idle seconds
    end = time.time() + most
    last = time.time()
    while time.time() < end and time.time() - last < idle:
        r, _, _ = select.select([master], [], [], 0.05)
        if r:
            chunk = os.read(master, 4096)
            out.extend(chunk)
            last = time.time()
            time.sleep(len(chunk) / 1e6)
def keys(*k):
    subprocess.run(tm + ['send-keys', '-t', 't:0.0'] + list(k), check=True, env=env)
pump(3, 3)
panes = subprocess.run(tm + ['list-panes', '-t', 't', '-F', '#{pane_id} #{pane_left} #{pane_top}'],
                       capture_output=True, text=True, env=env).stdout.split()
keys('open ' + ctl, 'Enter'); pump(2, 1)
keys('run ' + root + '/dbuffloop.gs', 'Enter'); pump(30)
keys('quit', 'Enter'); pump(3, 1)
after = subprocess.run(tm + ['list-panes', '-t', 't'], capture_output=True, text=True,
                       env=env).stdout
client.terminate()
subprocess.run(tm + ['kill-server'], capture_output=True, env=env)
d = bytes(out)
print('panes', len(panes) // 3)
if len(panes) != 6:
    sys.exit()
if control:
    # the picture pane's own output, as tmux hands it to iTerm2
    pane = bytearray()
    for line in d.split(b'\n'):
        m = re.match(rb'%output (%\d+) (.*)', line.rstrip(b'\r'))
        if m and m.group(1) == panes[3].encode():
            pane.extend(re.sub(rb'\\([0-7]{3})', lambda x: bytes([int(x.group(1), 8)]),
                               m.group(2)))
    d = bytes(pane)
    at = rb'\x1b\[H'
else:
    at = rb'\x1b7\x1b\[%d;%dH' % (int(panes[5]) + 1, int(panes[4]) + 1)
pics = whole = placed = 0
for m in re.finditer(rb'\x1b\]1337;(File=|MultipartFile=)([^:\x07]*)', d):
    pics += 1
    size = int(re.search(rb'size=(\d+)', m.group(2)).group(1))
    if m.group(1) == b'File=':
        e = re.match(rb'[^:]*:([A-Za-z0-9+/=]*)\x07', d[m.end():])
        b64 = e.group(1) if e else b''
        ends = []
    else:
        rest = d[m.end():]
        end = rest.find(b'\x1b]1337;FileEnd\x07')
        b64 = b''.join(re.findall(rb'\x1b\]1337;FilePart=([A-Za-z0-9+/=]*)\x07', rest[:end]))
        if end < 0 or re.search(rb'FilePart=[A-Za-z0-9+/=]*[^A-Za-z0-9+/=\x07]', rest[:end]):
            b64 = b''
        ends = [m.end() + end]
    try:
        png = base64.b64decode(b64, validate=True)
    except ValueError:
        png = b''
    whole += len(png) == size and png.startswith(b'\x89PNG') and b'IEND' in png[-12:]
    placed += all(re.search(at + rb'$', d[max(0, i - 32):i]) for i in [m.start()] + ends)
first = d.find(b'\x1b]1337;')
print('waiting', int(0 <= d.find(b'Waiting for a GrADS picture') < first))
print('pictures', pics)
print('whole', whole)
print('placed', placed)
print('wrapped', d.count(b'Ptmux;'))
print('panes-after', len([l for l in after.splitlines() if l.strip()]))
PYTHON
for how in plain steps control; do
  result="$(python3 "$test_root/intmux.py" "$launcher" "$model_ctl" "$test_root" \
    "$viewer" "$build_root" "$tmux_bin" "$how" 2>&1)"
  grep -qx 'panes 2' <<< "$result" ||
    fail "GrADS did not split a pane in tmux ($how)" "$result"
  grep -qx 'waiting 1' <<< "$result" ||
    fail "the pane did not say it was waiting before the first picture ($how)" "$result"
  grep -qx 'pictures 5' <<< "$result" ||
    fail "five frames, and only those, did not arrive through tmux ($how)" "$result"
  grep -qx 'whole 5' <<< "$result" ||
    fail "pictures arrived incomplete through tmux ($how)" "$result"
  grep -qx 'placed 5' <<< "$result" ||
    fail "pictures were not placed at the viewer pane ($how)" "$result"
  grep -qx 'wrapped 0' <<< "$result" ||
    fail "passthrough wrapping reached the terminal ($how)" "$result"
  grep -qx 'panes-after 0' <<< "$result" || grep -qx 'panes-after 1' <<< "$result" ||
    fail "the viewer pane outlived GrADS ($how)" "$result"
done

printf 'Terminal display checks passed\n'
