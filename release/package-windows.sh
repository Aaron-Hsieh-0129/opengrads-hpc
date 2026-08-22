#!/usr/bin/env bash
# Assemble a native Windows ZIP from an MSYS2/MinGW build.

set -euo pipefail

if [[ "$#" -ne 2 ]]; then
  printf 'Usage: %s BUILD_ROOT OUTPUT_ROOT\n' "$0" >&2
  exit 2
fi
case "$(uname -s)" in
  MINGW*|MSYS*) ;;
  *) printf 'This packager must run in MSYS2/MinGW.\n' >&2; exit 2 ;;
esac

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="$1"
output_root="$2"
grads_version="$(<"$repo_root/cola/src/VERSION")"
dist_version="$(<"$repo_root/release/VERSION")"
archive_base="opengrads-hpc-$dist_version-windows-x86_64"
bundle_root="$output_root/$archive_base"
lib_root="$bundle_root/lib"
plugin_root="$bundle_root/plugins"
grads_binary="$build_root/src/grads.exe"

if [[ ! -x "$grads_binary" ]]; then
  grads_binary="$build_root/src/grads"
fi
if [[ ! -x "$grads_binary" ]]; then
  printf 'Windows GrADS executable was not found in %s/src.\n' "$build_root" >&2
  exit 1
fi

rm -rf -- "$bundle_root"
mkdir -p "$bundle_root/bin" "$lib_root" "$plugin_root" "$bundle_root/etc" \
  "$bundle_root/cola/data" "$bundle_root/lib/scripts" "$bundle_root/docs"
install -m 0755 "$grads_binary" "$bundle_root/bin/grads.exe"

dummy_source="$(find "$build_root/src/.libs" -maxdepth 1 -type f \
  -iname 'libgxdummy*.dll' -print -quit)"
if [[ -z "$dummy_source" ]]; then
  printf 'Required Windows gxdummy plug-in was not found in %s/src/.libs.\n' \
    "$build_root" >&2
  printf 'libtool builds a DLL only when the plug-in links with -no-undefined.\n' >&2
  exit 1
fi
install -m 0755 "$dummy_source" "$plugin_root/libgxdummy.dll"

# Windows DLLs must resolve every symbol at link time, and the Cairo plug-ins
# call back into symbols that live in the GrADS executable. Only the
# self-contained gxdummy driver links natively, so this archive is headless.
cat > "$bundle_root/etc/udpt" <<'UDPT'
# OpenGrADS Windows release plug-in table.
# GA_ROOT is set by opengrads.cmd.
gxdisplay  gxdummy  $GA_ROOT/libgxdummy.dll
*
gxprint    gxdummy  $GA_ROOT/libgxdummy.dll
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

# MSYS2 installs every package's license under share/licenses/<package>, so
# the notices for bundled DLLs can be copied wholesale.
mkdir -p "$bundle_root/licenses"
: > "$bundle_root/licenses/BUNDLED-LIBRARIES.txt"

for package in adios2 cairo geotiff gcc-libs hdf5 netcdf dlfcn zlib libwinpthread; do
  license_dir="$MINGW_PREFIX/share/licenses/$package"
  if [[ -d "$license_dir" ]]; then
    cp -a "$license_dir" "$bundle_root/licenses/$package"
  fi
done
install -m 0644 "$repo_root/release/opengrads.cmd" "$bundle_root/opengrads.cmd"

# Breadth-first copy of every non-system DLL the binaries need. MSYS2 reports
# system DLLs under paths such as /c/WINDOWS/SYSTEM32, whose case varies.
is_system_dll()
{
  local path
  path="$(tr '[:upper:]' '[:lower:]' <<< "$1")"
  case "$path" in
    /c/windows/*|/c/program\ files/*) return 0 ;;
    *) return 1 ;;
  esac
}

queue=("$bundle_root/bin/grads.exe" "$plugin_root/libgxdummy.dll")
declare -A bundled=()

while (( ${#queue[@]} )); do
  binary="${queue[0]}"
  queue=("${queue[@]:1}")

  while IFS= read -r resolved; do
    [[ -n "$resolved" ]] || continue
    is_system_dll "$resolved" && continue
    if [[ ! -r "$resolved" ]]; then
      printf 'Unresolved Windows runtime dependency for %s: %s\n' \
        "$binary" "$resolved" >&2
      exit 1
    fi
    soname="$(basename -- "$resolved")"
    [[ -z "${bundled[$soname]:-}" ]] || continue
    bundled[$soname]=1
    cp -L "$resolved" "$lib_root/$soname"
    chmod 0755 "$lib_root/$soname"
    printf '%s\t%s\n' "$soname" "$resolved" \
      >> "$bundle_root/licenses/BUNDLED-LIBRARIES.txt"
    queue+=("$lib_root/$soname")
  done < <(ldd "$binary" | awk '$2 == "=>" && $3 ~ /^\// { print $3 }')
done

smoke_output="$(PATH="$lib_root:$plugin_root:$PATH" \
  GA_ROOT="$plugin_root" GAUDPT="$bundle_root/etc/udpt" \
  GADDIR="$bundle_root/cola/data" GASCRP="$bundle_root/lib/scripts" \
  "$bundle_root/bin/grads.exe" -bl -d gxdummy -h gxdummy <<'GRADS'
q config
q threads
quit
GRADS
)"
grep -Fq 'adios2-bp5' <<< "$smoke_output"
grep -Fq 'openmp' <<< "$smoke_output"
grep -Fq 'netcdf' <<< "$smoke_output"
grep -Fq 'Calculation threads = 4' <<< "$smoke_output"

mkdir -p "$output_root"
(
  cd "$output_root"
  rm -f "$archive_base.zip" "$archive_base.zip.sha256"
  zip -qr "$archive_base.zip" "$archive_base"
  sha256sum "$archive_base.zip" > "$archive_base.zip.sha256"
)
printf 'Release archive: %s\n' "$output_root/$archive_base.zip"
