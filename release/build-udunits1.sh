#!/usr/bin/env bash
# Build the legacy UDUNITS 1 API required by the GrADS SDF reader.

set -euo pipefail

if [[ "$#" -ne 1 ]]; then
  printf 'Usage: %s WORK_ROOT\n' "$0" >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=versions.env
source "$repo_root/release/versions.env"
work_root="$1"
download_root="$work_root/downloads"
source_root="$work_root/sources"
build_root="$work_root/dependency-build"
prefix="$work_root/udunits1"
jobs="${OPENGRADS_BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf 4)}"
archive="$download_root/udunits-$UDUNITS1_VERSION.tar.gz"
source_dir="$source_root/udunits-$UDUNITS1_VERSION/src"

if [[ -r "$prefix/include/udunits.h" && -r "$prefix/lib/libudunits.a" ]]; then
  printf 'UDUNITS 1 prefix: %s\n' "$prefix"
  exit 0
fi

mkdir -p "$download_root" "$source_root" "$build_root" "$prefix"
if [[ ! -f "$archive" ]]; then
  curl -fL --retry 3 --retry-delay 2 "$UDUNITS1_URL" -o "$archive"
fi
printf '%s  %s\n' "$UDUNITS1_SHA256" "$archive" | sha256sum --check -
if [[ ! -d "$source_dir" ]]; then
  tar -xzf "$archive" -C "$source_root"
fi

cd "$source_dir"
find . -type d -exec touch {}/depend \;
./configure --prefix="$prefix"
CPPFLAGS="${CPPFLAGS:-} -Df2cFortran" ./configure --prefix="$prefix"
make install

if [[ ! -r "$prefix/include/udunits.h" || ! -r "$prefix/lib/libudunits.a" ]]; then
  printf 'UDUNITS 1 installation is incomplete: %s\n' "$prefix" >&2
  exit 1
fi
printf 'UDUNITS 1 prefix: %s\n' "$prefix"
