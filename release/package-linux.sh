#!/usr/bin/env bash
# Assemble a relocatable Linux archive from a completed release build.

set -euo pipefail

if [[ "$#" -ne 5 ]]; then
  printf 'Usage: %s BUILD_ROOT DEPS_ROOT ADIOS2_ROOT WORK_ROOT OUTPUT_ROOT\n' "$0" >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=versions.env
source "$repo_root/release/versions.env"
build_root="$1"
deps_root="$2"
adios2_root="$3"
work_root="$4"
output_root="$5"
grads_version="$(<"$repo_root/cola/src/VERSION")"
dist_version="$(<"$repo_root/release/VERSION")"
machine="$(uname -m)"
archive_base="opengrads-hpc-$dist_version-linux-$machine"
bundle_root="$output_root/$archive_base"
runtime_lib_root="$bundle_root/adios2/lib"
glibc_root="$bundle_root/glibc"
plugin_root="$bundle_root/build/src/.libs"

case "$(uname -s)" in
  Linux) ;;
  *) printf 'This packager currently supports Linux only.\n' >&2; exit 2 ;;
esac

rm -rf -- "$bundle_root"
mkdir -p "$bundle_root/build/src" "$plugin_root" "$runtime_lib_root" \
  "$bundle_root/deps/lib" "$bundle_root/cola/data" "$bundle_root/lib/scripts" \
  "$bundle_root/etc" "$bundle_root/docs" "$bundle_root/licenses/system" \
  "$bundle_root/glibc"

install -m 0755 "$build_root/src/grads" "$bundle_root/build/src/grads"
install -m 0755 "$repo_root/opengrads" "$bundle_root/opengrads"
install -D -m 0755 "$repo_root/libexec/grads-termview" \
  "$bundle_root/libexec/grads-termview"
install -m 0644 "$repo_root/etc/udpt-local" "$bundle_root/etc/udpt-local"
cp -a "$repo_root/cola/data/." "$bundle_root/cola/data/"
cp -a "$repo_root/lib/scripts/." "$bundle_root/lib/scripts/"
cp -a "$repo_root/docs/." "$bundle_root/docs/"

# UDUNITS-2 reads its unit database from a compiled-in path at runtime, so a
# machine without udunits2 installed fails sdfopen with "UDUNITS package
# initialization failure". Bundle the database and point the launcher at it.
udunits_xml=""
for candidate in /usr/share/udunits /usr/local/share/udunits; do
  if [[ -r "$candidate/udunits2.xml" ]]; then
    udunits_xml="$candidate"
    break
  fi
done
if [[ -n "$udunits_xml" ]]; then
  mkdir -p "$bundle_root/share/udunits"
  cp -a "$udunits_xml/." "$bundle_root/share/udunits/"
else
  printf 'UDUNITS-2 database not found; sdfopen will need it on the host.\n' >&2
fi
install -m 0644 "$repo_root/README.md" "$repo_root/COPYING" \
  "$repo_root/COPYRIGHT" "$repo_root/THIRD_PARTY_NOTICES.md" "$bundle_root/"

# Record both identities so an unpacked archive can say what it is.
cat > "$bundle_root/VERSION" <<VERSIONFILE
opengrads-hpc $dist_version
GrADS base $grads_version
VERSIONFILE

"$repo_root/release/write-source-offer.sh" "$bundle_root" "$dist_version" \
  "$grads_version"

for plugin in libgxdummy.so libgxdX11.so libgxdCairo.so libgxdTerm.so \
              libgxpCairo.so; do
  if [[ ! -r "$build_root/src/.libs/$plugin" ]]; then
    printf 'Required release plug-in is missing: %s\n' "$plugin" >&2
    exit 1
  fi
  install -m 0755 "$build_root/src/.libs/$plugin" "$plugin_root/$plugin"
done

copy_notice()
{
  local source_file="$1"
  local target_file="$2"
  if [[ -r "$source_file" ]]; then
    install -m 0644 "$source_file" "$target_file"
  fi
}

copy_notice "$work_root/sources/ADIOS2-$ADIOS2_VERSION/LICENSE" \
  "$bundle_root/licenses/ADIOS2-LICENSE"
copy_notice "$work_root/sources/ADIOS2-$ADIOS2_VERSION/Copyright.txt" \
  "$bundle_root/licenses/ADIOS2-Copyright.txt"
copy_notice "$work_root/sources/readline-$READLINE_VERSION/COPYING" \
  "$bundle_root/licenses/Readline-COPYING"
copy_notice "$work_root/sources/ncurses-$NCURSES_VERSION/COPYING" \
  "$bundle_root/licenses/ncurses-COPYING"

library_path="$adios2_root/lib:$adios2_root/lib64:$deps_root/lib:$deps_root/lib64"
queue=("$bundle_root/build/src/grads" "$plugin_root/libgxdummy.so" \
       "$plugin_root/libgxdX11.so" "$plugin_root/libgxdCairo.so" \
       "$plugin_root/libgxdTerm.so" \
       "$plugin_root/libgxpCairo.so")
declare -A seen=()
: > "$bundle_root/runtime-libraries.txt"

# Only the kernel-provided vDSO and the dynamic loader are left out of the
# bundle. The loader is copied separately because the archive is launched
# through it; everything else, glibc included, is bundled so the archive does
# not depend on the host's C library version.
is_system_abi()
{
  case "$1" in
    linux-vdso.so.*|ld-linux*.so.*|ld64.so.*)
      return 0 ;;
    *) return 1 ;;
  esac
}

# The glibc family goes into its own directory rather than alongside the other
# bundled libraries. It must never appear on LD_LIBRARY_PATH: a host loader
# newer than the bundled libc will load it and die with SIGILL. It is used
# only via the bundled loader's --library-path, where loader and libc match.
is_glibc_lib()
{
  case "$1" in
    libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|\
    libresolv.so.*|libutil.so.*|libnsl.so.*|libanl.so.*|libcrypt.so.*)
      return 0 ;;
    *) return 1 ;;
  esac
}

record_system_notice()
{
  local library="$1"
  local owner package copyright_file
  owner="$(dpkg-query -S "$library" 2>/dev/null | head -n 1 || true)"
  package="${owner%%:*}"
  copyright_file="/usr/share/doc/$package/copyright"
  if [[ -n "$owner" && -r "$copyright_file" && \
        ! -r "$bundle_root/licenses/system/$package.copyright" ]]; then
    install -m 0644 "$copyright_file" \
      "$bundle_root/licenses/system/$package.copyright"
  fi
}

while (( ${#queue[@]} )); do
  binary="${queue[0]}"
  queue=("${queue[@]:1}")
  ldd_output="$(LD_LIBRARY_PATH="$library_path:$runtime_lib_root" ldd "$binary" 2>&1 || true)"
  if grep -Fq 'not found' <<< "$ldd_output"; then
    printf 'Unresolved runtime dependency for %s:\n%s\n' "$binary" "$ldd_output" >&2
    exit 1
  fi

  while read -r soname arrow resolved remainder; do
    [[ "$arrow" == "=>" && "$resolved" == /* ]] || continue
    is_system_abi "$soname" && continue
    [[ -z "${seen[$soname]:-}" ]] || continue
    seen[$soname]=1
    resolved="$(readlink -f "$resolved")"
    if is_glibc_lib "$soname"; then
      install -m 0755 "$resolved" "$glibc_root/$soname"
    else
      install -m 0755 "$resolved" "$runtime_lib_root/$soname"
    fi
    printf '%s\t%s\n' "$soname" "$resolved" >> "$bundle_root/runtime-libraries.txt"
    record_system_notice "$resolved"
    queue+=("$resolved")
  done <<< "$ldd_output"
done

sort -o "$bundle_root/runtime-libraries.txt" "$bundle_root/runtime-libraries.txt"

# Invariant: no part of glibc may sit in a directory that reaches
# LD_LIBRARY_PATH. If it does, a host loader newer than the bundled libc loads
# the bundled one and the program dies with SIGILL before printing anything.
# The packager's own smoke test cannot catch this, because the build machine's
# glibc matches the bundled copy exactly.
if compgen -G "$runtime_lib_root/libc.so.*" > /dev/null || \
   compgen -G "$runtime_lib_root/ld-linux*" > /dev/null; then
  printf 'glibc leaked into %s; it must live only in glibc/.\n' \
    "$runtime_lib_root" >&2
  exit 1
fi

# The archive is started through its own loader so the bundled glibc is used
# instead of the host's. Take the interpreter path from the executable itself
# rather than guessing per-architecture names.
loader_path="$(readelf -l "$bundle_root/build/src/grads" \
  | sed -n 's/.*interpreter: \(.*\)]/\1/p' | head -n 1)"
if [[ -z "$loader_path" || ! -r "$loader_path" ]]; then
  printf 'Unable to determine the dynamic loader for the bundle.\n' >&2
  exit 1
fi
install -m 0755 "$loader_path" "$glibc_root/$(basename -- "$loader_path")"
printf '%s\n' "$(basename -- "$loader_path")" > "$bundle_root/loader-name.txt"

# Record the highest glibc symbol version anything in the bundle needs. The
# launcher uses the host's glibc when it is at least this new, and falls back
# to the bundled one only when it is not. That matters because glibc's NSS
# dlopens the host's libnss_* modules, which a newer bundled glibc cannot
# load -- breaking network name resolution, and with it OPeNDAP.
required_glibc="$(
  find "$bundle_root" -type f \( -name '*.so*' -o -name grads \) -print0 \
    | xargs -0 -r objdump -T 2>/dev/null \
    | grep -oE 'GLIBC_[0-9]+\.[0-9]+' | sort -uV | tail -n 1
)"
required_glibc="${required_glibc#GLIBC_}"
if [[ -z "$required_glibc" ]]; then required_glibc="0"; fi
printf '%s\n' "$required_glibc" > "$bundle_root/glibc-required.txt"
record_system_notice "$loader_path"

# Embed the library search paths in the binaries themselves, relative to
# $ORIGIN, so the launcher does not have to export LD_LIBRARY_PATH. Exporting
# it leaked the bundled libraries into every subprocess GrADS spawns -- a
# shell escape such as "!ls" ran the host's ls against our libselinux and
# warned about it. A path in the binary is private to it and cannot leak.
#
# It has to be DT_RPATH, which patchelf writes only when forced. The loader
# searches DT_RPATH before LD_LIBRARY_PATH but DT_RUNPATH after it, so with
# RUNPATH (1.0.8) any LD_LIBRARY_PATH the user's shell carries -- set by
# environment modules or conda on most clusters -- won over the bundle: GrADS
# loaded that cluster's own cairo, freetype, HDF5 and so on, mixed them with
# ours, and crashed as the window opened.
if ! command -v patchelf > /dev/null 2>&1; then
  printf 'patchelf is required to make the archive relocatable.\n' >&2
  exit 1
fi

patchelf --force-rpath --set-rpath \
  '$ORIGIN/.libs:$ORIGIN/../../adios2/lib:$ORIGIN/../../deps/lib' \
  "$bundle_root/build/src/grads"

for library in "$runtime_lib_root"/*.so*; do
  [[ -f "$library" ]] || continue
  patchelf --force-rpath --set-rpath '$ORIGIN' "$library" 2>/dev/null || true
done

for plugin in "$plugin_root"/*.so; do
  [[ -f "$plugin" ]] || continue
  patchelf --force-rpath --set-rpath \
    '$ORIGIN:$ORIGIN/../../../adios2/lib:$ORIGIN/../../../deps/lib' \
    "$plugin" 2>/dev/null || true
done

# The executable must now resolve everything without LD_LIBRARY_PATH set.
if env -u LD_LIBRARY_PATH ldd "$bundle_root/build/src/grads" 2>&1 \
     | grep -Fq 'not found'; then
  printf 'Bundle does not resolve without LD_LIBRARY_PATH:\n' >&2
  env -u LD_LIBRARY_PATH ldd "$bundle_root/build/src/grads" >&2
  exit 1
fi

# And it must keep resolving to the bundle when LD_LIBRARY_PATH points
# somewhere else, as it does on most clusters. A decoy directory holds empty
# files named like every bundled library: if the loader ever reached for one,
# loading would fail with "file too short". Only the dynamic loader and grads
# itself see the decoy -- ldd and the launcher are shell scripts whose own
# helpers (coreutils) would trip over an empty libselinux first.
decoy_dir="$(mktemp -d)"
for library in "$runtime_lib_root"/*.so*; do
  [[ -f "$library" ]] || continue
  : > "$decoy_dir/$(basename -- "$library")"
done
for binary in "$bundle_root/build/src/grads" "$plugin_root"/*.so; do
  [[ -f "$binary" ]] || continue
  if ! readelf -d "$binary" | grep -Fq '(RPATH)'; then
    printf '%s carries no DT_RPATH; LD_LIBRARY_PATH would override the bundle.\n' \
      "$binary" >&2
    exit 1
  fi
  decoy_list="$(LD_LIBRARY_PATH="$decoy_dir" "$loader_path" --list "$binary" 2>&1 || true)"
  if grep -Fq -e "$decoy_dir" -e 'not found' -e 'error while loading' \
       <<< "$decoy_list"; then
    printf 'LD_LIBRARY_PATH overrides the bundle for %s:\n%s\n' \
      "$binary" "$decoy_list" >&2
    exit 1
  fi
done
decoy_output="$(GA_ROOT="$plugin_root" GAUDPT="$bundle_root/etc/udpt-local" \
  GADDIR="$bundle_root/cola/data" LD_LIBRARY_PATH="$decoy_dir" \
  "$bundle_root/build/src/grads" -bl -d gxdummy -h gxdummy 2>&1 <<'GRADS' || true
q config
quit
GRADS
)"
rm -rf -- "$decoy_dir"
if ! grep -Fq 'adios2-bp5' <<< "$decoy_output"; then
  printf 'grads does not start with a decoy LD_LIBRARY_PATH:\n%s\n' \
    "$decoy_output" >&2
  exit 1
fi

smoke_output="$(env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin \
  OPENGRADS_COLOR=0 "$bundle_root/opengrads" \
  -bl -d gxdummy -h gxdummy <<'GRADS'
q config
q threads
quit
GRADS
)"
grep -Fq 'adios2-bp5' <<< "$smoke_output"
grep -Fq 'openmp' <<< "$smoke_output"
grep -Fq 'netcdf' <<< "$smoke_output"
grep -Fq 'Calculation threads = 4' <<< "$smoke_output"

# Nor may an old OpenGrADS install unpacked beside the archive. The launcher
# used to adopt a sibling opengrads-2.2.1.oga.1 bundle and load its plug-ins
# (built for libpng15 and the like) in place of ours. The decoy here has a
# plug-in table whose every entry points at a file that is not a library.
neighbour="$output_root/opengrads-2.2.1.oga.1"
if [[ ! -e "$neighbour" ]]; then
  neighbour_gex="$neighbour/Contents/$(uname -s)/Versions/2.2.1.oga.1/$(uname -m)/gex"
  mkdir -p "$neighbour_gex"
  printf '2.2.1.oga.1\n' > "$neighbour/Contents/$(uname -s)/Versions/Current@"
  printf 'not a library\n' > "$neighbour_gex/libgxdummy.so"
  printf 'gxdisplay gxdummy %s\n*\ngxprint gxdummy %s\n' \
    "$neighbour_gex/libgxdummy.so" "$neighbour_gex/libgxdummy.so" \
    > "$neighbour_gex/udpt"
  : > "$neighbour_gex/udxt"
  neighbour_output="$(env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin \
    OPENGRADS_COLOR=0 "$bundle_root/opengrads" \
    -bl -d gxdummy -h gxdummy 2>&1 <<'GRADS' || true
q config
quit
GRADS
)"
  rm -rf -- "$neighbour"
  if ! grep -Fq 'adios2-bp5' <<< "$neighbour_output" ||
     grep -Fq 'GX Package Error' <<< "$neighbour_output"; then
    printf 'An OpenGrADS bundle beside the archive replaces its plug-ins:\n%s\n' \
      "$neighbour_output" >&2
    exit 1
  fi
fi

# A locale the machine does not have must not stop GrADS. Readline 8.2 before
# its official patch 001 crashed on the first prompt when LC_ALL, LC_CTYPE or
# LANG named one -- LC_CTYPE=UTF-8 from a macOS ssh session, say.
locale_output="$(env -i HOME="${HOME:-/tmp}" PATH=/usr/bin:/bin \
  LC_ALL=xx_YY.UTF-8 LC_CTYPE=UTF-8 LANG=xx_YY.UTF-8 OPENGRADS_COLOR=0 \
  "$bundle_root/opengrads" -bl -d gxdummy -h gxdummy 2>&1 <<'GRADS' || true
q config
quit
GRADS
)"
if ! grep -Fq 'adios2-bp5' <<< "$locale_output"; then
  printf 'grads does not start under an uninstalled locale:\n%s\n' \
    "$locale_output" >&2
  exit 1
fi

mkdir -p "$output_root"
tar -C "$output_root" -czf "$output_root/$archive_base.tar.gz" "$archive_base"
(
  cd "$output_root"
  sha256sum "$archive_base.tar.gz" > "$archive_base.tar.gz.sha256"
)
printf 'Release archive: %s\n' "$output_root/$archive_base.tar.gz"
