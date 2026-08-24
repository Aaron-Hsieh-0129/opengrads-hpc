#!/usr/bin/env bash
# One-command self-contained opengrads-hpc release build.

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
# RHEL-family packages put the UDUNITS headers in a subdirectory, unlike
# Debian which installs them straight into /usr/include.
if [[ -r /usr/include/udunits2/udunits.h ]]; then
  export CPPFLAGS="$CPPFLAGS -I/usr/include/udunits2"
fi
# Debian hides HDF5 under a serial/parallel multiarch subdirectory, so the
# configure probe misses it and dtype hdf5_grid is silently unavailable.
if [[ ! -r /usr/include/hdf5.h && -r /usr/include/hdf5/serial/hdf5.h ]]; then
  export CPPFLAGS="$CPPFLAGS -I/usr/include/hdf5/serial"
  hdf5_libdir="/usr/lib/$(uname -m)-linux-gnu/hdf5/serial"
  if [[ -d "$hdf5_libdir" ]]; then
    export LDFLAGS="${LDFLAGS:+$LDFLAGS }-L$hdf5_libdir -Wl,-rpath,$hdf5_libdir"
  fi
fi
export LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib"
export PKG_CONFIG_PATH="$deps_root/lib/pkgconfig:$deps_root/share/pkgconfig"
export LD_LIBRARY_PATH="$adios2_root/lib:$adios2_root/lib64:$deps_root/lib:$deps_root/lib64"

cd "$build_root"
"$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --enable-openmp \
  --enable-sdfopen \
  --with-opengrads \
  --without-gadap \
  --with-adios2="$adios2_root"

make -C src --jobs "$jobs" \
  grads libgxdummy.la libgxdX11.la libgxdCairo.la libgxpCairo.la

OPENGRADS_BUILD_ROOT="$build_root" \
OPENGRADS_ADIOS2_ROOT="$adios2_root" \
  "$repo_root/pytests/TestBP5.sh"
OPENGRADS_BUILD_ROOT="$build_root" "$repo_root/pytests/TestSDFOpen.sh"
OPENGRADS_BUILD_ROOT="$build_root" "$repo_root/pytests/TestOpenMP.sh"

"$repo_root/release/package-linux.sh" \
  "$build_root" "$deps_root" "$adios2_root" "$work_root" "$output_root"
