#!/usr/bin/env bash
# One-command self-contained OpenGrADS release build.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work_root="${OPENGRADS_RELEASE_WORK_ROOT:-$repo_root/.release-work}"
output_root="${OPENGRADS_RELEASE_OUTPUT_ROOT:-$repo_root/release-dist}"
build_root="$work_root/opengrads-build"
jobs="${OPENGRADS_BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf 4)}"

if [[ -n "${OPENGRADS_RELEASE_ADIOS2_ROOT:-}" || \
      -n "${OPENGRADS_RELEASE_DEPS_ROOT:-}" ]]; then
  if [[ -z "${OPENGRADS_RELEASE_ADIOS2_ROOT:-}" || \
        -z "${OPENGRADS_RELEASE_DEPS_ROOT:-}" ]]; then
    printf 'Set both OPENGRADS_RELEASE_ADIOS2_ROOT and OPENGRADS_RELEASE_DEPS_ROOT.\n' >&2
    exit 2
  fi
  adios2_root="$OPENGRADS_RELEASE_ADIOS2_ROOT"
  deps_root="$OPENGRADS_RELEASE_DEPS_ROOT"
else
  "$repo_root/release/build-dependencies.sh" "$work_root"
  adios2_root="$work_root/adios2"
  deps_root="$work_root/deps"
fi

if [[ ! -x "$adios2_root/bin/adios2-config" ]]; then
  printf 'ADIOS2 release dependency is missing: %s\n' "$adios2_root" >&2
  exit 1
fi

rm -rf -- "$build_root"
mkdir -p "$build_root/src" "$build_root/lib" "$output_root"
unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH SUPPLIBS \
  CFLAGS CXXFLAGS CPPFLAGS LDFLAGS PKG_CONFIG_PATH
export PATH="$deps_root/bin:$PATH"
export CPPFLAGS="-I$deps_root/include"
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig"
export LD_LIBRARY_PATH="$adios2_root/lib:$adios2_root/lib64:$deps_root/lib:$deps_root/lib64"

cd "$build_root"
"$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --enable-openmp \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$adios2_root"

make -C src --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la

OPENGRADS_BUILD_ROOT="$build_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  "$repo_root/pytests/TestBP5.sh"
OPENGRADS_BUILD_ROOT="$build_root" "$repo_root/pytests/TestOpenMP.sh"

"$repo_root/release/package-linux.sh" \
  "$build_root" "$deps_root" "$adios2_root" "$work_root" "$output_root"
