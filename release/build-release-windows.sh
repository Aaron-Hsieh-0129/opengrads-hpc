#!/usr/bin/env bash
# Build, test, and package a native Windows opengrads-hpc release in MSYS2/MinGW.
#
# NOT exercised by CI. Requires these MSYS2 packages -- note libgeotiff, not
# geotiff, which is the name that broke the first CI attempt:
#   autoconf automake libtool make zip mingw-w64-x86_64-{adios2,cairo,dlfcn,
#   libgeotiff,gcc,hdf5,netcdf,pkgconf,udunits}
# See docs/RELEASES.md for remaining known blockers.

set -euo pipefail

case "$(uname -s)" in
  MINGW*|MSYS*) ;;
  *) printf 'This release builder must run in MSYS2/MinGW.\n' >&2; exit 2 ;;
esac
if [[ -z "${MINGW_PREFIX:-}" ]]; then
  printf 'MINGW_PREFIX is required; run this script from the MSYS2 MINGW64 shell.\n' >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work_root="${OPENGRADS_RELEASE_WORK_ROOT:-$repo_root/.release-work-windows}"
output_root="${OPENGRADS_RELEASE_OUTPUT_ROOT:-$repo_root/release-dist}"
build_root="$work_root/opengrads-build"
jobs="${OPENGRADS_BUILD_JOBS:-$(nproc)}"

if [[ ! -x "$MINGW_PREFIX/bin/adios2-config" ]]; then
  printf 'MSYS2 ADIOS2 is not installed in %s.\n' "$MINGW_PREFIX" >&2
  exit 1
fi
# GrADS loads its graphics plug-ins with dlopen(), which MinGW only provides
# through the dlfcn-win32 compatibility package.
if [[ ! -r "$MINGW_PREFIX/include/dlfcn.h" ]]; then
  printf 'MSYS2 dlfcn is not installed in %s; install mingw-w64-x86_64-dlfcn.\n' \
    "$MINGW_PREFIX" >&2
  exit 1
fi

rm -rf -- "$build_root"
mkdir -p "$build_root/src" "$build_root/lib" "$output_root"
export PATH="$MINGW_PREFIX/bin:$PATH"
export PKG_CONFIG_PATH="$MINGW_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
cd "$build_root"
"$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --enable-openmp \
  --enable-sdfopen \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$MINGW_PREFIX"
make -C src --jobs "$jobs" grads libgxdummy.la

export OPENGRADS_BUILD_ROOT="$build_root"
export OPENGRADS_ADIOS2_ROOT="$MINGW_PREFIX"
export OPENGRADS_LAUNCHER="$repo_root/release/launch-local.sh"
export OPENGRADS_RELEASE_PLATFORM=MINGW64
export OPENGRADS_RUNTIME_LIBRARY_PATH="$MINGW_PREFIX/bin"
"$repo_root/pytests/TestBP5.sh"
"$repo_root/pytests/TestSDFOpen.sh"
"$repo_root/pytests/TestOpenMP.sh"

"$repo_root/release/package-windows.sh" "$build_root" "$output_root"
