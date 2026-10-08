#!/usr/bin/env bash
# Added in 2026 for the undo feature. GPLv2; see COPYING.
#
# Undo rewinds the graphics buffer and replays what is left, so the check that
# matters is that the rendered output after an undo is identical to the output
# of the shorter command sequence. That part needs a real printing plug-in; it
# is skipped when the build has none, and the state checks always run.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-/tmp/opengrads-build-cpu}"
grads_binary="$build_root/src/grads"
launcher="${OPENGRADS_LAUNCHER:-$repo_root/opengrads}"
model_ctl="$repo_root/pytests/data/model.ctl"

if [[ ! -x "$grads_binary" && -x "$grads_binary.exe" ]]; then
  grads_binary="$grads_binary.exe"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'GrADS binary not found: %s\n' "$grads_binary" >&2
  exit 1
fi

# The printing plug-in carries a platform-specific extension, so look for any
# of them rather than assuming .so.
have_plugin()
{
  find "$build_root/src/.libs" -maxdepth 1 \
    \( -name "$1.so" -o -name "$1.dylib" -o -name "$1.dll" \) \
    -print -quit 2>/dev/null | grep -q .
}

hardcopy=gxdummy
if have_plugin libgxpCairo; then
  hardcopy=Cairo
elif have_plugin libgxpGD; then
  hardcopy=GD
fi

test_root="$(mktemp -d /tmp/opengrads-undo-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

cat > "$test_root/three_lines.gs" <<'GRADS_SCRIPT'
'draw line 1 1 2 2'
'draw line 2 2 3 3'
'draw line 3 3 4 4'
GRADS_SCRIPT

run_grads()
{
  OPENGRADS_BUILD_ROOT="$build_root" \
  OPENGRADS_COLOR=0 \
    "$launcher" -bl -d gxdummy -h "$1" 2>&1
}

output="$(
  run_grads gxdummy <<GRADS_COMMANDS
q undo
undo
set undo 2
q undo
draw line 1 1 5 5
draw line 2 2 6 6
draw line 3 3 7 7
q undo
undo 4
q undo
set undo 4
set gxout shaded
q undo
run $test_root/three_lines.gs
q undo
undo
q undo
draw line 1 1 2 2
clear
q undo
undo
q undo
set undo 3
draw line 1 1 2 2
set dbuff on
q undo
set dbuff off
undo
set undo off
q undo
undo
quit
GRADS_COMMANDS
)"

check_text()
{
  local expected="$1"
  if ! grep -Fq -- "$expected" <<< "$output"; then
    printf 'Undo test did not find expected text: %s\n' "$expected" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

check_count()
{
  local expected="$1" wanted="$2" found
  found="$(grep -Fc -- "$expected" <<< "$output" || true)"
  if (( found != wanted )); then
    printf 'Undo test expected %s occurrences of: %s (found %s)\n' \
      "$wanted" "$expected" "$found" >&2
    printf '%s\n' "$output" >&2
    exit 1
  fi
}

# Off by default, and the error says how to turn it on.
check_text 'Undo is off, 7 meta buffer words used by the current plot'
check_text "UNDO error:  undo is off.  Turn it on with 'set undo <steps>'"

# Turning it on reports the step count; three draws against a two-step stack
# keep only the newest two, so 'undo 4' rewinds two and stops.
check_text 'Undo is on, keeping up to 2 steps'
check_text 'Undo is on, 0 of 2 steps available, 7 meta buffer words used by the current plot'
check_text 'Undo is on, 2 of 2 steps available, 46 meta buffer words used by the current plot'
check_text 'Undid 2 steps, 0 of 2 still available'
check_text 'Undo is on, 0 of 2 steps available, 20 meta buffer words used by the current plot'

# A command that draws nothing costs no step, and a script costs exactly one
# however much it draws: undoing it removes all three of its lines.
check_text 'Undo is on, keeping up to 4 steps'
check_text 'Undo is on, 0 of 4 steps available, 20 meta buffer words used by the current plot'
check_text 'Undo is on, 1 of 4 steps available, 59 meta buffer words used by the current plot'
check_text 'Undid 1 step, 0 of 4 still available'

# A clear is a step of its own, and undoing it brings back the frame it
# cleared, with the steps taken in it.
check_text 'Undo is on, 2 of 4 steps available, 0 meta buffer words used by the current plot'
check_text 'Undid 1 step, 1 of 4 still available'
check_text 'Undo is on, 1 of 4 steps available, 33 meta buffer words used by the current plot'

# Double buffering drops the stored steps.
check_text 'Undo is on, keeping up to 3 steps'
check_text 'Undo is on, 0 of 3 steps available, 0 meta buffer words used by the current plot'
check_text 'Undo is off'

# Exactly one 'undo' ran with the feature on and nothing to rewind: the one
# after double buffering. The other two report the 'undo is off' error.
check_count 'Nothing to undo' 1
check_count "UNDO error:  undo is off.  Turn it on with 'set undo <steps>'" 2

# Undo puts back what GrADS knows about the picture along with it, as the
# shorter command sequence would have left it, so queries and the next plot
# see the picture that is on screen. The answers to each query are compared
# in the order they were asked.
state_output="$(
  run_grads gxdummy <<GRADS_COMMANDS
set undo 10
open $model_ctl
set gxout shaded
d ts
q gxinfo
q xy2w 5 4
q shades
set lon 100 150
set gxout contour
d ps
undo
q gxinfo
q xy2w 5 4
q shades
q contours
c
set x 36
set y 23
set t 1 5
d ts
q xy2w 5 4
c
d ts*10-2727
undo
d ts
q xy2w 5 4
c
d ts
set vrange 250 350
undo
d ts
q xy2w 5 4
c
set vrange 250 350
d ts
q xy2w 5 4
c
set vrange 250 350
d ts
c
undo
d ts
q xy2w 5 4
c
set vrange 250 350
d ts
c
set vrange 200 400
undo
d ts
q xy2w 5 4
c
set vrange 200 400
d ts
q xy2w 5 4
c
set x 1 72
set y 1 46
set t 1
set gxout contour
set cint 20
d ts+0
undo
d ts+0
c
q gxinfo
undo
q gxinfo
quit
GRADS_COMMANDS
)"

# The answers to one query, in order, each on one line ('|' between lines)
answers()
{
  awk -v q="ga-> $1" '
    index($0, "ga-> ") == 1 { if (inq) print buf; inq = ($0 == q); buf = ""; next }
    inq { buf = buf (buf == "" ? "" : "|") $0 }
    END { if (inq) print buf }
  ' <<< "$state_output"
}

state_fail()
{
  printf 'Undo state test: %s\n' "$1" >&2
  printf '%s\n' "$state_output" >&2
  exit 1
}

# Gather the answers to a query into an array; the bash 3.2 of macOS has no
# mapfile.
collect()
{
  local line
  eval "$1=()"
  while IFS= read -r line; do
    eval "$1+=(\"\$line\")"
  done < <(answers "$2")
}

collect gxinfo 'q gxinfo'
collect xy 'q xy2w 5 4'
collect shades 'q shades'
collect contours 'q contours'
collect plotted 'd ts+0'
(( ${#gxinfo[@]} == 4 && ${#xy[@]} == 9 && ${#shades[@]} == 2 && ${#contours[@]} == 1 &&
   ${#plotted[@]} == 2 )) || state_fail 'a query went unanswered'

# Undoing a zoomed contour plot over a shaded one: the plot area, the
# mapping from page to world, and the shading levels are the shaded plot's
# again, and there are no contour levels.
[[ "${gxinfo[1]}" == "${gxinfo[0]}" ]] || state_fail 'q gxinfo after undo is not that of the remaining plot'
[[ "${xy[1]}" == "${xy[0]}" ]] || state_fail 'q xy2w after undo still maps through the undone plot'
[[ "${shades[1]}" == "${shades[0]}" ]] || state_fail 'q shades after undo is not that of the remaining plot'
[[ "${contours[0]}" == None ]] || state_fail 'q contours after undo still reports the undone contours'

# A 1-D plot fixes the axis range for the plots over it. After undoing one,
# the next plot gets its own range, as on a cleared page.
[[ "${xy[3]}" == "${xy[2]}" ]] || state_fail 'a 1-D plot after an undo kept the undone plot'"'"'s axis range'
# But a range the user set after the step stays.
[[ "${xy[4]}" == "${xy[5]}" && "${xy[4]}" != "${xy[2]}" ]] ||
  state_fail 'undo dropped a vrange set after the undone step'

# A clear can be undone: the frame comes back, with the options the clear
# reset, here a vrange that the plot drawn over it then follows, unless
# the user has set them since.
[[ "${xy[6]}" == "${xy[5]}" ]] || state_fail 'undoing a clear did not bring back the vrange it reset'
[[ "${xy[7]}" == "${xy[8]}" && "${xy[7]}" != "${xy[5]}" ]] ||
  state_fail 'undoing a clear overrode a vrange set after it'
[[ "${gxinfo[2]}" == 'Last Graphic = Clear|'* ]] || state_fail 'q gxinfo after clear'
[[ "${gxinfo[3]}" == 'Last Graphic = Contour|'* ]] || state_fail 'undo did not bring the cleared plot back'

# A display uses up options such as cint; undoing it gives them back, so the
# display can be issued again as it was.
[[ "${plotted[0]}" == *'interval 20'* && "${plotted[1]}" == "${plotted[0]}" ]] ||
  state_fail 'undoing a display did not give back the cint it used'

if [[ "$hardcopy" == gxdummy ]]; then
  printf 'Undo state test passed: default off, step accounting, script as one step, clear as a step, frame resets, plot state.\n'
  printf 'Rendered-output comparison skipped: this build has no printing plug-in.\n'
  exit 0
fi

# The rendered output after an undo must match the shorter sequence exactly.
# The data plot is the demanding case: it fills more than one buffer in the
# chain, so the rewind has to cross a buffer boundary.
run_grads "$hardcopy" > "$test_root/rewound.log" <<GRADS_COMMANDS
set undo 5
open $model_ctl
set gxout shaded
d ts
set gxout contour
d ps
draw title undone
undo 2
printim $test_root/rewound.png
quit
GRADS_COMMANDS

run_grads "$hardcopy" > "$test_root/direct.log" <<GRADS_COMMANDS
open $model_ctl
set gxout shaded
d ts
printim $test_root/direct.png
quit
GRADS_COMMANDS

# Undoing a clear brings back the cleared picture exactly, here after a plot
# drawn on the cleared page; so does undoing a script that clears.
cat > "$test_root/clearplot.gs" <<'GRADS_SCRIPT'
'c'
'set gxout contour'
'd ps'
'draw title replaced'
GRADS_SCRIPT

run_grads "$hardcopy" > "$test_root/unclear.log" <<GRADS_COMMANDS
set undo 5
open $model_ctl
set gxout shaded
d ts
c
set gxout contour
d ps
undo 2
printim $test_root/unclear.png
quit
GRADS_COMMANDS

run_grads "$hardcopy" > "$test_root/unscript.log" <<GRADS_COMMANDS
set undo 5
open $model_ctl
set gxout shaded
d ts
run $test_root/clearplot.gs
undo
printim $test_root/unscript.png
quit
GRADS_COMMANDS

# Contour labels keep clear of each other through a mask, which an undo
# rewinds as well: a plot drawn again over the remaining one places its
# labels as when drawn directly.
run_grads "$hardcopy" > "$test_root/unmask.log" <<GRADS_COMMANDS
set undo 5
open $model_ctl
set gxout contour
set clab masked
d ts
d ps
undo
set clab masked
d ps
printim $test_root/unmask.png
quit
GRADS_COMMANDS

run_grads "$hardcopy" > "$test_root/mask.log" <<GRADS_COMMANDS
open $model_ctl
set gxout contour
set clab masked
d ts
set clab masked
d ps
printim $test_root/mask.png
quit
GRADS_COMMANDS

for image in rewound direct unclear unscript unmask mask; do
  if [[ ! -s "$test_root/$image.png" ]]; then
    printf 'Undo test could not render %s.png\n' "$image" >&2
    cat "$test_root/$image.log" >&2
    exit 1
  fi
done

for pair in rewound:direct unclear:direct unscript:direct unmask:mask; do
  image="${pair%%:*}"
  want="${pair#*:}"
  if ! cmp -s "$test_root/$image.png" "$test_root/$want.png"; then
    printf 'Undo test: the %s plot does not match the directly drawn plot\n' "$image" >&2
    ls -l "$test_root/$image.png" "$test_root/$want.png" >&2
    cat "$test_root/$image.log" >&2
    exit 1
  fi
done

printf 'Undo test passed: default off, step accounting, script as one step, clear as a step, frame resets, plot state, and byte-identical plots after undoing draws, a clear, a script that clears, and with masked labels (%s).\n' \
  "$hardcopy"
