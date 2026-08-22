#!/usr/bin/env bash
# Launch an out-of-tree GrADS build on Linux, macOS, or MSYS2/MinGW.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${OPENGRADS_BUILD_ROOT:-$repo_root/build}"
platform="${OPENGRADS_RELEASE_PLATFORM:-$(uname -s)}"
plugin_dir="$build_root/src/.libs"
grads_binary="$build_root/src/grads"

case "$platform" in
  Darwin*) plugin_ext=dylib; library_variable=DYLD_LIBRARY_PATH ;;
  MINGW*|MSYS*|CYGWIN*) plugin_ext=dll; library_variable=PATH ;;
  *) plugin_ext=so; library_variable=LD_LIBRARY_PATH ;;
esac

if [[ ! -x "$grads_binary" && -x "$grads_binary.exe" ]]; then
  grads_binary="$grads_binary.exe"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'GrADS executable was not found: %s\n' "$grads_binary" >&2
  exit 1
fi

dummy_plugin="$plugin_dir/libgxdummy.$plugin_ext"
if [[ ! -r "$dummy_plugin" ]]; then
  candidate="$(find "$plugin_dir" -maxdepth 1 -type f -name "libgxdummy*.$plugin_ext" -print -quit)"
  if [[ -z "$candidate" ]]; then
    printf 'Headless graphics plug-in was not found in: %s\n' "$plugin_dir" >&2
    exit 1
  fi
  ln -sf "$(basename -- "$candidate")" "$dummy_plugin"
fi

plugin_table="$build_root/udpt-local.$plugin_ext"
sed "s/\\.so/.$plugin_ext/g" "$repo_root/etc/udpt-local" > "$plugin_table"

export GA_ROOT="$plugin_dir"
export GAUDPT="$plugin_table"
export GADDIR="$repo_root/cola/data"
if [[ -n "${GASCRP:-}" ]]; then
  export GASCRP="$GASCRP:$repo_root/lib/scripts"
else
  export GASCRP="$repo_root/lib/scripts"
fi

runtime_path="$plugin_dir"
if [[ -n "${OPENGRADS_RUNTIME_LIBRARY_PATH:-}" ]]; then
  runtime_path="$OPENGRADS_RUNTIME_LIBRARY_PATH:$runtime_path"
fi
case "$library_variable" in
  PATH) export PATH="$runtime_path:$PATH" ;;
  DYLD_LIBRARY_PATH)
    export DYLD_LIBRARY_PATH="$runtime_path${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" ;;
  *) export LD_LIBRARY_PATH="$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ;;
esac

exec "$grads_binary" "$@"
