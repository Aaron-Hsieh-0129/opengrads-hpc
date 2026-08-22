#!/usr/bin/env bash
# Assemble a relocatable macOS archive from a completed native build.

set -euo pipefail

if [[ "$#" -ne 2 ]]; then
  printf 'Usage: %s BUILD_ROOT OUTPUT_ROOT\n' "$0" >&2
  exit 2
fi
if [[ "$(uname -s)" != Darwin ]]; then
  printf 'This packager must run on macOS.\n' >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$1"
output_root="$2"
version="$(<"$repo_root/cola/src/VERSION")"
machine="$(uname -m)"
archive_base="opengrads-$version-macos-$machine"
bundle_root="$output_root/$archive_base"
lib_root="$bundle_root/lib"
plugin_root="$bundle_root/plugins"

rm -rf -- "$bundle_root"
mkdir -p "$bundle_root/bin" "$lib_root" "$plugin_root" "$bundle_root/etc" \
  "$bundle_root/cola/data" "$bundle_root/lib/scripts" "$bundle_root/docs"

install -m 0755 "$build_root/src/grads" "$bundle_root/bin/grads"

# The display side is headless: macOS release archives do not depend on
# XQuartz. Cairo still provides the full hardcopy path (printim, print).
plugin_sources=()
plugin_stems=()

install_plugin()
{
  local stem="$1"
  local source_file
  source_file="$(find "$build_root/src/.libs" -maxdepth 1 -type f \
    -name "$stem*.dylib" -print -quit)"
  if [[ -z "$source_file" ]]; then
    printf 'Required macOS graphics plug-in was not found: %s\n' "$stem" >&2
    exit 1
  fi
  install -m 0755 "$source_file" "$plugin_root/$stem.dylib"
  plugin_sources+=("$source_file")
  plugin_stems+=("$stem")
}

install_plugin libgxdummy
install_plugin libgxpCairo

cat > "$bundle_root/etc/udpt" <<'UDPT'
# OpenGrADS macOS release plug-in table.
# GA_ROOT is set by the bundled launcher.
gxdisplay  gxdummy  $GA_ROOT/libgxdummy.dylib
*
gxprint    Cairo    $GA_ROOT/libgxpCairo.dylib
gxprint    gxdummy  $GA_ROOT/libgxdummy.dylib
UDPT

cp -a "$repo_root/cola/data/." "$bundle_root/cola/data/"
cp -a "$repo_root/lib/scripts/." "$bundle_root/lib/scripts/"
cp -a "$repo_root/docs/." "$bundle_root/docs/"
install -m 0644 "$repo_root/README.md" "$repo_root/COPYING" \
  "$repo_root/COPYRIGHT" "$repo_root/THIRD_PARTY_NOTICES.md" "$bundle_root/"
install -m 0755 "$repo_root/release/opengrads-macos" "$bundle_root/opengrads"

executable_dir="$bundle_root/bin"

# Mach-O records dependencies as absolute paths, @rpath, @loader_path, or
# @executable_path. Resolve every form so the archive can be assembled from a
# Homebrew prefix that will not exist on the user's machine.
list_rpaths()
{
  otool -l "$1" \
    | awk '/^ *cmd LC_RPATH$/ { want = 1; next }
           want && /^ *path / { print $2; want = 0 }'
}

resolve_reference()
{
  local binary="$1"
  local reference="$2"
  local loader_dir candidate rpath
  loader_dir="$(dirname -- "$binary")"

  case "$reference" in
    @rpath/*)
      while IFS= read -r rpath; do
        [[ -n "$rpath" ]] || continue
        rpath="${rpath/#@loader_path/$loader_dir}"
        rpath="${rpath/#@executable_path/$executable_dir}"
        candidate="$rpath/${reference#@rpath/}"
        if [[ -r "$candidate" ]]; then
          printf '%s\n' "$candidate"
          return 0
        fi
      done < <(list_rpaths "$binary")
      return 1
      ;;
    @loader_path/*) candidate="$loader_dir/${reference#@loader_path/}" ;;
    @executable_path/*) candidate="$executable_dir/${reference#@executable_path/}" ;;
    *) candidate="$reference" ;;
  esac

  [[ -r "$candidate" ]] || return 1
  printf '%s\n' "$candidate"
}

# Breadth-first walk over the binaries, copying every non-system dependency
# into lib/ and queueing it so its own dependencies are copied too. The walk
# follows the originals rather than the bundle copies: @rpath and
# @loader_path resolve against the location a binary was built in, which the
# bundle layout deliberately does not reproduce.
#
# macOS ships bash 3.2, so this stays clear of associative arrays and array
# slicing: the queue is walked by index and "bundled" is a delimited string.
queue=("$build_root/src/grads" "${plugin_sources[@]}")
bundled_names=()
bundled=""
index=0

while (( index < ${#queue[@]} )); do
  binary="${queue[index]}"
  index=$((index + 1))
  install_name="$(otool -D "$binary" | awk 'NR > 1 { print; exit }')"

  while IFS= read -r reference; do
    case "$reference" in
      /System/*|/usr/lib/*) continue ;;
    esac
    [[ "$reference" != "$install_name" ]] || continue

    if ! resolved="$(resolve_reference "$binary" "$reference")"; then
      printf 'Unresolved macOS runtime dependency for %s: %s\n' \
        "$binary" "$reference" >&2
      exit 1
    fi

    soname="$(basename -- "$resolved")"
    case "$bundled" in
      *"|$soname|"*) continue ;;
    esac
    bundled="$bundled|$soname|"
    bundled_names+=("$soname")
    cp -L "$resolved" "$lib_root/$soname"
    chmod 0755 "$lib_root/$soname"
    queue+=("$resolved")
  done < <(otool -L "$binary" | awk 'NR > 1 { print $1 }')
done

# Repoint every recorded path at the bundle itself, then re-sign: editing a
# Mach-O header invalidates the ad-hoc signature that arm64 macOS requires.
relocate()
{
  local binary="$1"
  local rpath="$2"
  local install_name reference soname

  install_name="$(otool -D "$binary" | awk 'NR > 1 { print; exit }')"
  if [[ -n "$install_name" ]]; then
    install_name_tool -id "@rpath/$(basename -- "$binary")" "$binary"
  fi

  while IFS= read -r reference; do
    [[ "$reference" != "$install_name" ]] || continue
    soname="$(basename -- "$reference")"
    case "$bundled" in
      *"|$soname|"*) ;;
      *) continue ;;
    esac
    if [[ "$reference" != "@rpath/$soname" ]]; then
      install_name_tool -change "$reference" "@rpath/$soname" "$binary"
    fi
  done < <(otool -L "$binary" | awk 'NR > 1 { print $1 }')

  install_name_tool -add_rpath "$rpath" "$binary" 2>/dev/null || true
  codesign --force --sign - "$binary" >/dev/null 2>&1
}

relocate "$bundle_root/bin/grads" "@executable_path/../lib"
for stem in "${plugin_stems[@]}"; do
  relocate "$plugin_root/$stem.dylib" "@loader_path/../lib"
done
for soname in "${bundled_names[@]}"; do
  relocate "$lib_root/$soname" "@loader_path/../lib"
done

# Prove the archive runs with no Homebrew prefix and no X server in the
# environment, and that the Cairo hardcopy path produces a real image.
smoke_root="$(mktemp -d /tmp/opengrads-macos-smoke.XXXXXX)"
trap 'rm -rf -- "$smoke_root"' EXIT

smoke_output="$(env -i HOME="$smoke_root" PATH=/usr/bin:/bin \
  OPENGRADS_COLOR=0 "$bundle_root/opengrads" -bl -d gxdummy -h Cairo <<GRADS
q config
q threads
set vpage 0 11 0 8.5
draw recf 1 1 6 5
printim $smoke_root/smoke.png x800 y600
quit
GRADS
)"
grep -Fq 'adios2-bp5' <<< "$smoke_output"
grep -Fq 'openmp' <<< "$smoke_output"
grep -Fq 'netcdf' <<< "$smoke_output"
grep -Fq 'Calculation threads = 4' <<< "$smoke_output"
if [[ ! -s "$smoke_root/smoke.png" ]]; then
  printf 'Cairo hardcopy output was not produced by the macOS bundle.\n' >&2
  printf '%s\n' "$smoke_output" >&2
  exit 1
fi

mkdir -p "$output_root"
tar -C "$output_root" -czf "$output_root/$archive_base.tar.gz" "$archive_base"
(
  cd "$output_root"
  shasum -a 256 "$archive_base.tar.gz" > "$archive_base.tar.gz.sha256"
)
printf 'Release archive: %s\n' "$output_root/$archive_base.tar.gz"
