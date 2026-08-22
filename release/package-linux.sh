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
plugin_root="$bundle_root/build/src/.libs"

case "$(uname -s)" in
  Linux) ;;
  *) printf 'This packager currently supports Linux only.\n' >&2; exit 2 ;;
esac

rm -rf -- "$bundle_root"
mkdir -p "$bundle_root/build/src" "$plugin_root" "$runtime_lib_root" \
  "$bundle_root/deps/lib" "$bundle_root/cola/data" "$bundle_root/lib/scripts" \
  "$bundle_root/etc" "$bundle_root/docs" "$bundle_root/licenses/system"

install -m 0755 "$build_root/src/grads" "$bundle_root/build/src/grads"
install -m 0755 "$repo_root/opengrads" "$bundle_root/opengrads"
install -m 0644 "$repo_root/etc/udpt-local" "$bundle_root/etc/udpt-local"
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

is_system_abi()
{
  case "$1" in
    linux-vdso.so.*|ld-linux*.so.*|libc.so.*|libm.so.*|libdl.so.*|\
    libpthread.so.*|librt.so.*|libutil.so.*|libresolv.so.*)
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
    install -m 0755 "$resolved" "$runtime_lib_root/$soname"
    printf '%s\t%s\n' "$soname" "$resolved" >> "$bundle_root/runtime-libraries.txt"
    record_system_notice "$resolved"
    queue+=("$resolved")
  done <<< "$ldd_output"
done

sort -o "$bundle_root/runtime-libraries.txt" "$bundle_root/runtime-libraries.txt"

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
