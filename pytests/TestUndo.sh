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
set undo 3
draw line 1 1 2 2
set dbuff on
q undo
set dbuff off
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

# Resetting the frame drops the stored steps: clear, then double buffering.
check_text 'Undo is on, 0 of 4 steps available, 0 meta buffer words used by the current plot'
check_text 'Undo is on, keeping up to 3 steps'
check_text 'Undo is on, 0 of 3 steps available, 0 meta buffer words used by the current plot'
check_text 'Undo is off'

# Exactly one 'undo' ran with the feature on and nothing to rewind: the one
# after 'clear'. The other two report the 'undo is off' error instead.
check_count 'Nothing to undo' 1
check_count "UNDO error:  undo is off.  Turn it on with 'set undo <steps>'" 2

if [[ "$hardcopy" == gxdummy ]]; then
  printf 'Undo state test passed: default off, step accounting, script as one step, frame resets.\n'
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

for image in rewound direct; do
  if [[ ! -s "$test_root/$image.png" ]]; then
    printf 'Undo test could not render %s.png\n' "$image" >&2
    cat "$test_root/$image.log" >&2
    exit 1
  fi
done

if ! cmp -s "$test_root/rewound.png" "$test_root/direct.png"; then
  printf 'Undo test: the rewound plot does not match the directly drawn plot\n' >&2
  ls -l "$test_root/rewound.png" "$test_root/direct.png" >&2
  cat "$test_root/rewound.log" >&2
  exit 1
fi

printf 'Undo test passed: default off, step accounting, script as one step, frame resets, and a byte-identical rewound plot (%s).\n' \
  "$hardcopy"
