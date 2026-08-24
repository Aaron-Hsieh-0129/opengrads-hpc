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

for plugin in libgxdummy.so libgxdX11.so libgxdCairo.so libgxpCairo.so; do
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

mkdir -p "$output_root"
tar -C "$output_root" -czf "$output_root/$archive_base.tar.gz" "$archive_base"
(
  cd "$output_root"
  sha256sum "$archive_base.tar.gz" > "$archive_base.tar.gz.sha256"
)
printf 'Release archive: %s\n' "$output_root/$archive_base.tar.gz"
