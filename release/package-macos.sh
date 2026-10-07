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
grads_version="$(<"$repo_root/cola/src/VERSION")"
dist_version="$(<"$repo_root/release/VERSION")"
machine="$(uname -m)"
archive_base="opengrads-hpc-$dist_version-macos-$machine"
bundle_root="$output_root/$archive_base"
lib_root="$bundle_root/lib"
plugin_root="$bundle_root/plugins"

rm -rf -- "$bundle_root"
mkdir -p "$bundle_root/bin" "$lib_root" "$plugin_root" "$bundle_root/etc" \
  "$bundle_root/cola/data" "$bundle_root/lib/scripts" "$bundle_root/docs" \
  "$bundle_root/libexec"

install -m 0755 "$build_root/src/grads" "$bundle_root/bin/grads"
# The terminal display starts this in the tmux pane it draws into.
install -m 0755 "$repo_root/libexec/grads-termview" "$bundle_root/libexec/grads-termview"

# The X displays (Cairo, the default, and X11) open a window through
# XQuartz. They are plug-ins, loaded only when asked for, so the archive does
# not depend on XQuartz: without it, it runs headless, and Cairo still
# provides the full hardcopy path (printim, print). The terminal display
# (Term) needs no X server: it shows the picture in iTerm2 or WezTerm.
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
install_plugin libgxdCairo
install_plugin libgxdX11
install_plugin libgxdTerm

cat > "$bundle_root/etc/udpt" <<'UDPT'
# opengrads-hpc macOS release plug-in table.
# GA_ROOT is set by the bundled launcher.
gxdisplay  Cairo    $GA_ROOT/libgxdCairo.dylib
gxdisplay  X11      $GA_ROOT/libgxdX11.dylib
gxdisplay  Term     $GA_ROOT/libgxdTerm.dylib
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

# Record both identities so an unpacked archive can say what it is.
cat > "$bundle_root/VERSION" <<VERSIONFILE
opengrads-hpc $dist_version
GrADS base $grads_version
VERSIONFILE

"$repo_root/release/write-source-offer.sh" "$bundle_root" "$dist_version" \
  "$grads_version"

# Homebrew does not install license files to a predictable path for every
# formula, so record what is bundled either way and copy whatever is found.
mkdir -p "$bundle_root/licenses"
: > "$bundle_root/licenses/BUNDLED-LIBRARIES.txt"

copy_formula_notice()
{
  local formula="$1"
  local prefix notice
  prefix="$(brew --prefix "$formula" 2>/dev/null || true)"
  [[ -n "$prefix" && -d "$prefix" ]] || return 0
  while IFS= read -r notice; do
    [[ -n "$notice" ]] || continue
    install -m 0644 "$notice" \
      "$bundle_root/licenses/$formula-$(basename -- "$notice")"
    return 0
  done < <(find "$prefix" -maxdepth 2 \
    \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) \
    -type f 2>/dev/null)
}

for formula in adios2 cairo libgeotiff hdf5 libomp netcdf gcc \
               libx11 libxcb libxau libxdmcp libxext libxrender; do
  copy_formula_notice "$formula"
done
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
    printf '%s\t%s\n' "$soname" "$resolved" \
      >> "$bundle_root/licenses/BUNDLED-LIBRARIES.txt"
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

# The X displays load with the bundled X libraries alone: with no X server
# they get as far as connecting to it.
for display in Cairo X11; do
  smoke_output="$(env -i HOME="$smoke_root" PATH=/usr/bin:/bin \
    OPENGRADS_COLOR=0 "$bundle_root/opengrads" -l -d "$display" 2>&1 <<< quit || true)"
  if ! grep -Fq 'Unable to connect to X server' <<< "$smoke_output"; then
    printf 'The %s display plug-in did not load from the macOS bundle.\n' "$display" >&2
    printf '%s\n' "$smoke_output" >&2
    exit 1
  fi
done

# The terminal display draws without an X server: it writes the picture to a
# directory, or prints it as an iTerm2 image sequence. With OPENGRADS_TERM=1,
# as inside iTerm2 or WezTerm with no X server, the launcher picks it.
for how in file inline picked; do
  case "$how" in
    file) term_args=(-l -d Term -g 400x300); term_env=(GA_TERM_MODE=file) ;;
    inline) term_args=(-l -d Term -g 400x300); term_env=(GA_TERM_MODE=inline) ;;
    picked) term_args=(); term_env=(GA_TERM_MODE=file OPENGRADS_TERM=1) ;;
  esac
  smoke_output="$(env -i HOME="$smoke_root" PATH=/usr/bin:/bin TMPDIR="$smoke_root" \
    OPENGRADS_COLOR=0 GA_TERM_DIR="$smoke_root/term-$how" GA_TERM_SYNC=1 "${term_env[@]}" \
    "$bundle_root/opengrads" ${term_args[@]+"${term_args[@]}"} 2>&1 <<GRADS || true
draw recf 1 1 6 5
q pos
quit
GRADS
)"
  if [[ "$how" == inline ]]; then
    grep -aFq $'\033]1337;File=inline=1;size=' <<< "$smoke_output" &&
      grep -aFq ':iVBORw0KGgo' <<< "$smoke_output" && continue
  elif [[ -s "$smoke_root/term-$how/plot.png" &&
          "$(head -c 4 "$smoke_root/term-$how/plot.png" | od -An -c | tr -d ' ')" == '211PNG' ]]; then
    continue
  fi
  printf 'The Term display did not draw from the macOS bundle (%s).\n' "$how" >&2
  printf '%s\n' "$smoke_output" | head -c 4000 >&2
  exit 1
done
printf 'The Term display drew from the macOS bundle.\n'

# Where XQuartz is installed, draw in a window on its virtual X server and
# print from that session.
xvfb=/opt/X11/bin/Xvfb
if [[ -x "$xvfb" ]]; then
  "$xvfb" :73 -nolisten tcp -screen 0 1280x1024x24 > "$smoke_root/xvfb.log" 2>&1 &
  xvfb_pid=$!
  trap 'kill "$xvfb_pid" 2>/dev/null || true; rm -rf -- "$smoke_root"' EXIT
  for i in $(seq 1 50); do
    [[ -S /tmp/.X11-unix/X73 ]] && break
    sleep 0.2
  done
  for display in Cairo X11; do
    smoke_output="$(env -i HOME="$smoke_root" PATH=/usr/bin:/bin DISPLAY=:73 \
      OPENGRADS_COLOR=0 "$bundle_root/opengrads" -l -d "$display" 2>&1 <<GRADS || true
draw recf 1 1 6 5
draw string 2 6 $display window
printim $smoke_root/window-$display.png x800 y600
quit
GRADS
)"
    if grep -Fq 'Error' <<< "$smoke_output" ||
       [[ ! -s "$smoke_root/window-$display.png" ]]; then
      printf 'The %s display did not draw in an X window from the macOS bundle.\n' "$display" >&2
      printf '%s\n' "$smoke_output" >&2
      exit 1
    fi
  done
  kill "$xvfb_pid" 2>/dev/null || true
  printf 'X displays drew on XQuartz'"'"'s Xvfb.\n'
else
  printf 'XQuartz is not installed here; the X displays were only loaded, not drawn with.\n'
fi

mkdir -p "$output_root"
tar -C "$output_root" -czf "$output_root/$archive_base.tar.gz" "$archive_base"
(
  cd "$output_root"
  shasum -a 256 "$archive_base.tar.gz" > "$archive_base.tar.gz.sha256"
)
printf 'Release archive: %s\n' "$output_root/$archive_base.tar.gz"
