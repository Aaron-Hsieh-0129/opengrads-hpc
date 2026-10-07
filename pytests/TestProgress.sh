#!/usr/bin/env bash
# Added in 2026 for calculation progress. GPLv2; see COPYING.
#
# Long calculations (ave and its kin, tloop, eloop, gint, define) show a
# progress line on the terminal: what runs, how far along, how long it has
# taken and about how long is left. It appears only on a terminal and only
# once a calculation has run for a while (GA_PROGRESS sets the delay; it is
# set tiny here so the short calculations below show it), and it is taken
# away before anything else is printed, so output and logs read as before.

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

# 1. Without a terminal (a pipe, a log file) nothing is added to the output.
output="$(
  OPENGRADS_BUILD_ROOT="$build_root" OPENGRADS_COLOR=0 GA_PROGRESS=0.000001 \
    "$launcher" -bl -d gxdummy -h gxdummy 2>&1 <<GRADS_COMMANDS
open $model_ctl
set t 1
d ave(ts,t=1,t=5)
quit
GRADS_COMMANDS
)"
grep -Fq 'Averaging.' <<< "$output" || fail 'ave did not run' "$output"
if grep -aq $'\033\\[K\\|ave \\[' <<< "$output"; then
  fail 'a progress line went to a pipe' "$(od -c <<< "$output" | head -20)"
fi

if ! command -v python3 > /dev/null 2>&1; then
  printf 'SKIP: progress on a terminal needs python3\n'
  printf 'Progress checks passed (no terminal)\n'
  exit 0
fi

test_root="$(mktemp -d /tmp/opengrads-progress-test.XXXXXX)"
trap 'rm -rf -- "$test_root"' EXIT

# Run GrADS on a terminal, type the commands, and print what the terminal
# got: each progress frame on a line of its own ("frame ..."), each erase
# ("erase"), and the rest of the output with them taken out ("text ...").
cat > "$test_root/onterm.py" <<'PYTHON'
import os, pty, re, select, sys, time
launcher, settings, commands = sys.argv[1], sys.argv[2], sys.argv[3:]
env = dict(os.environ, OPENGRADS_COLOR='0')
# Python sets LC_CTYPE for what it starts; the settings name the locale
for k in ('GA_PROGRESS', 'LC_TERMINAL', 'TERM_PROGRAM', 'TMUX', 'LC_ALL', 'LC_CTYPE', 'LANG'):
    env.pop(k, None)
env.update(dict(kv.split('=', 1) for kv in settings.split() if '=' in kv))
pid, fd = pty.fork()
if pid == 0:
    os.execve(launcher, [launcher, '-bl', '-d', 'gxdummy', '-h', 'gxdummy'], env)
out = bytearray()
def pump(most):
    end = time.time() + most
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                out.extend(os.read(fd, 65536))
            except OSError:
                return
pump(2)
for c in commands + ['quit']:
    n = len(out)
    os.write(fd, (c + '\r').encode())
    end = time.time() + 30
    while time.time() < end and b'ga->' not in out[n + len(c):]:
        pump(0.1)
pump(1)
d = out.decode('utf-8', 'replace')
for m in re.finditer(r'\r([^\r\n]*?)\x1b\[K|\x1b\]9;4;(\d+)(?:;(\d+))?\x07', d):
    if m.group(1) is not None:
        print('frame ' + m.group(1) if m.group(1) else 'erase')
    else:
        print('iterm %s %s' % (m.group(2), m.group(3) or ''))
rest = re.sub(r'\r[^\r\n]*?\x1b\[K|\x1b\]9;4;[^\x07]*\x07', '', d)
for line in rest.splitlines():
    print('text ' + line)
PYTHON

onterm()
{
  local settings="$1"
  shift
  OPENGRADS_BUILD_ROOT="$build_root" \
    python3 "$test_root/onterm.py" "$launcher" "$settings" "$@" 2>&1
}

# 2. On a terminal: the line names the calculation and how far it is, and
#    is taken away before the next output, the prompt included.
result="$(onterm 'GA_PROGRESS=0.000001 LANG=C' "open $model_ctl" 'set t 1' 'set threads 3' \
  'd ave(ts,t=1,t=5)' 'set lon 0' 'set lat 0' 'set t 1 5' 'd tloop(ts+1)')"
# 3 with OpenMP; a build without it says 1
threads="$(sed -n 's/^text Calculation threads set to \([0-9]*\).*/\1/p' <<< "$result")"
[[ "$threads" == 1 ]] && word=thread || word=threads
grep -Eq "^frame ave \[[#.]+\] +[0-9]+%  [0-9]/5  ${threads:-x} $word  " <<< "$result" ||
  fail "ave showed no progress line, or not its ${threads:-?} $word" "$result"
grep -Eq '^frame tloop \[[#.]+\] +[0-9]+%  [0-9]/5  ' <<< "$result" ||
  fail 'tloop showed no progress line' "$result"
awk '/^frame /{shown=1} /^erase$/{shown=0} /^text .*ga->/{if (shown) bad=1}
     END{exit bad}' <<< "$result" ||
  fail 'a progress line was left up at the prompt' "$result"
if grep -Eq '^text .*(ave|tloop) \[' <<< "$result"; then
  fail 'a progress line got mixed into the output' "$result"
fi
grep -q '^text Averaging\.  dim = 3, start = 1, end = 5' <<< "$result" ||
  fail 'the output around the progress line changed' "$result"

# 3. A calculation inside another counts as part of its step.
result="$(onterm 'GA_PROGRESS=0.000001 LANG=en_US.UTF-8' "open $model_ctl" \
  'set t 2 4' 'define x = ave(ts,t-1,t+1)')"
grep -Eq '^frame define > ave \[(█|░)+\] +[0-9]+%  [0-9]/3  ' <<< "$result" ||
  fail 'define of an ave showed no combined progress line' "$result"
grep -q '^text Define memory allocation size' <<< "$result" ||
  fail 'define did not finish' "$result"

# 4. Quick calculations show nothing (the line waits a second), and
#    GA_PROGRESS=off turns it off.
for settings in '' 'GA_PROGRESS=off'; do
  result="$(onterm "$settings" "open $model_ctl" 'set t 1' 'd ave(ts,t=1,t=5)')"
  grep -q '^text Averaging\.' <<< "$result" || fail 'ave did not run' "$result"
  if grep -q '^frame \|^iterm ' <<< "$result"; then
    fail "a progress line showed (${settings:-default delay})" "$result"
  fi
done

# 5. In iTerm2 its own progress bar follows, and is put away at the end.
result="$(onterm 'GA_PROGRESS=0.000001 LC_TERMINAL=iTerm2' "open $model_ctl" 'set t 1' \
  'd ave(ts,t=1,t=5)')"
grep -Eq '^iterm 1 [0-9]+$' <<< "$result" || fail 'iTerm2 was not told the progress' "$result"
[[ "$(grep '^iterm ' <<< "$result" | tail -n 1)" == 'iterm 0 ' ]] ||
  fail "iTerm2's progress bar was left up" "$result"

printf 'Progress checks passed: ave with its threads, tloop, define of an ave, delay, off, iTerm2, no terminal\n'
