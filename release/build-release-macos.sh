#!/usr/bin/env bash
# Build, test, and package a native macOS OpenGrADS release archive.

set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
  printf 'This release builder must run on macOS.\n' >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
work_root="${OPENGRADS_RELEASE_WORK_ROOT:-$repo_root/.release-work-macos}"
output_root="${OPENGRADS_RELEASE_OUTPUT_ROOT:-$repo_root/release-dist}"
build_root="$work_root/opengrads-build"
jobs="${OPENGRADS_BUILD_JOBS:-$(sysctl -n hw.ncpu)}"
udunits_root="$work_root/udunits1"

for formula in adios2 autoconf automake cairo coreutils gcc geotiff hdf5 libomp \
               libtool netcdf pkgconf; do
  if ! brew list --versions "$formula" >/dev/null 2>&1; then
    printf 'Required Homebrew formula is not installed: %s\n' "$formula" >&2
    exit 1
  fi
done

gcc_bin="$(find "$(brew --prefix gcc)/bin" -maxdepth 1 -name 'gcc-[0-9]*' -type f \
  | sort -V | tail -n 1)"
if [[ -z "$gcc_bin" ]]; then
  printf 'Homebrew GCC compiler was not found.\n' >&2
  exit 1
fi
gcc_suffix="${gcc_bin##*-}"
export CC="$gcc_bin"
export CXX="$(dirname -- "$gcc_bin")/g++-$gcc_suffix"
export PATH="$(brew --prefix coreutils)/libexec/gnubin:$PATH"
export PKG_CONFIG_PATH="$(brew --prefix adios2)/lib/pkgconfig:$(brew --prefix cairo)/lib/pkgconfig:$(brew --prefix geotiff)/lib/pkgconfig:$(brew --prefix hdf5)/lib/pkgconfig:$(brew --prefix netcdf)/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

"$repo_root/release/build-udunits1.sh" "$work_root"

rm -rf -- "$build_root"
mkdir -p "$build_root/src" "$build_root/lib" "$output_root"
cd "$build_root"
"$repo_root/cola/configure" \
  --enable-dyn-supplibs \
  --enable-openmp \
  --enable-sdfopen \
  --with-opengrads \
  --without-gadap \
  --with-udunits="$udunits_root" \
  --with-adios2="$(brew --prefix adios2)"
make -C src --jobs "$jobs" grads libgxdummy.la libgxpCairo.la

runtime_libraries="$(brew --prefix adios2)/lib:$(brew --prefix gcc)/lib/gcc/current:$(brew --prefix libomp)/lib:$(brew --prefix netcdf)/lib"
export OPENGRADS_BUILD_ROOT="$build_root"
export OPENGRADS_ADIOS2_ROOT="$(brew --prefix adios2)"
export OPENGRADS_LAUNCHER="$repo_root/release/launch-local.sh"
export OPENGRADS_RELEASE_PLATFORM=Darwin
export OPENGRADS_RUNTIME_LIBRARY_PATH="$runtime_libraries"
"$repo_root/pytests/TestBP5.sh"
"$repo_root/pytests/TestSDFOpen.sh"
"$repo_root/pytests/TestOpenMP.sh"

"$repo_root/release/package-macos.sh" "$build_root" "$output_root"
