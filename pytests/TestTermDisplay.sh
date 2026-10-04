#!/usr/bin/env bash
# Added in 2026 for the terminal display. GPLv2; see COPYING.
#
# The terminal display (-d Term) draws without an X server and writes the
# picture to plot.png whenever GrADS waits for the user. These checks cover
# what can be seen without a real iTerm2: the PNG and its size, one picture
# per prompt rather than per command, the inline image sequence, cleanup of
# the temporary directory, and the viewer's tmux passthrough wrapping.
# Skipped when the build has no Cairo, which the terminal display needs.

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
trap 'rm -rf -- "$test_root"' EXIT

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

run_grads()
{
  OPENGRADS_BUILD_ROOT="$build_root" \
  OPENGRADS_COLOR=0 \
  GA_TERM_VIEWER="$viewer" \
    "$launcher" -l -d Term "$@" 2>&1
}

# 1. File mode: the picture, its size, and one picture per prompt.
cat > "$test_root/loop.gs" <<'GRADS_SCRIPT'
i=1
while (i<=5)
  'clear'
  'd ts'
  i=i+1
endwhile
GRADS_SCRIPT

pictures="$test_root/pictures"
output="$(
  GA_TERM_MODE=file GA_TERM_DIR="$pictures" GA_TERM_SCALE=1 \
    run_grads -g 400x300 <<GRADS_COMMANDS
open $model_ctl
d ts
!cat $pictures/seq > $test_root/seq_after_display
run $test_root/loop.gs
!cat $pictures/seq > $test_root/seq_after_script
q dims
!cat $pictures/seq > $test_root/seq_after_query
quit
GRADS_COMMANDS
)"

[[ -s "$pictures/plot.png" ]] || fail 'no plot.png was written' "$output"
[[ "$(head -c 4 "$pictures/plot.png" | od -An -c | tr -d ' ')" == '211PNG' ]] ||
  fail 'plot.png is not a PNG'
size="$(png_size "$pictures/plot.png")"
[[ "$size" == 400x300 ]] || fail "picture is $size, expected 400x300 from -g"

after_display="$(<"$test_root/seq_after_display")"
after_script="$(<"$test_root/seq_after_script")"
after_query="$(<"$test_root/seq_after_query")"
(( after_script == after_display + 1 )) ||
  fail "a script drawing five times wrote $((after_script - after_display)) pictures, expected 1"
(( after_query == after_script )) ||
  fail 'a command that draws nothing wrote a new picture'
grep -Fq 'pictures are written to' <<< "$output" ||
  fail 'file mode did not say where the pictures go' "$output"

# 2. Pixel density: GA_TERM_SCALE=2 doubles the pixels, not the page.
GA_TERM_MODE=file GA_TERM_DIR="$test_root/scaled" GA_TERM_SCALE=2 \
  run_grads -g 400x300 > /dev/null <<GRADS_COMMANDS
open $model_ctl
d ts
quit
GRADS_COMMANDS
size="$(png_size "$test_root/scaled/plot.png")"
[[ "$size" == 800x600 ]] || fail "scaled picture is $size, expected 800x600"

# 3. Inline mode: one image per drawn picture, none for a cleared page, and
#    the temporary directory is gone afterwards. Without a terminal the image
#    goes to standard output.
mkdir "$test_root/tmp"
output="$(
  TMPDIR="$test_root/tmp" GA_TERM_MODE=inline GA_TERM_SCALE=1 \
    run_grads -g 200x150 <<GRADS_COMMANDS
open $model_ctl
d ts
clear
quit
GRADS_COMMANDS
)"
images="$(grep -o $'\033\\]1337;File=inline=1;size=[0-9]*' <<< "$output" | wc -l)"
(( images == 1 )) || fail "inline mode printed $images images, expected 1"
if compgen -G "$test_root/tmp/grads-term-*" > /dev/null; then
  fail 'the temporary picture directory was left behind'
fi

# 4. Outside tmux, asking for tmux falls back to inline.
output="$(
  unset TMUX
  TMPDIR="$test_root/tmp" GA_TERM_MODE=tmux run_grads <<'GRADS_COMMANDS'
quit
GRADS_COMMANDS
)"
grep -Fq 'not running inside tmux' <<< "$output" ||
  fail 'GA_TERM_MODE=tmux outside tmux gave no warning' "$output"

# 5. The viewer wraps the image for tmux, sizes it to the pane, and exits
#    with the GrADS process it follows.
sleep 1 &
follow_pid=$!
TMUX=/tmp/fake,1,0 timeout 10 "$viewer" "$pictures" "$follow_pid" \
  > "$test_root/viewer.out" 2>/dev/null < /dev/null ||
  fail 'the viewer did not exit after the GrADS process it follows'
wait "$follow_pid" 2>/dev/null || true
grep -Fq $'\033Ptmux;\033\033]1337;File=inline=1;' "$test_root/viewer.out" ||
  fail 'the viewer did not wrap the image for tmux'
grep -Eq 'width=[0-9]+;height=[0-9]+;preserveAspectRatio=1:' \
  "$test_root/viewer.out" || fail 'the viewer did not size the image in cells'

printf 'Terminal display checks passed\n'
