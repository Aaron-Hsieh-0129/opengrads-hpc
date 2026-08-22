#!/usr/bin/env bash
# Build pinned release dependencies into a private prefix.

set -euo pipefail

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=versions.env
source "$repo_root/release/versions.env"
work_root="${1:-$repo_root/.release-work}"
download_root="$work_root/downloads"
source_root="$work_root/sources"
build_root="$work_root/dependency-build"
deps_root="$work_root/deps"
adios2_root="$work_root/adios2"
jobs="${OPENGRADS_BUILD_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf 4)}"

mkdir -p "$download_root" "$source_root" "$build_root" "$deps_root" "$adios2_root"

fetch()
{
  local url="$1"
  local sha256="$2"
  local target="$3"

  if [[ ! -f "$target" ]]; then
    curl -fL --retry 3 --retry-delay 2 "$url" -o "$target"
  fi
  printf '%s  %s\n' "$sha256" "$target" | sha256sum --check -
}

extract()
{
  local archive="$1"
  local directory="$2"

  if [[ ! -d "$directory" ]]; then
    tar -xzf "$archive" -C "$source_root"
  fi
}

ncurses_archive="$download_root/ncurses-$NCURSES_VERSION.tar.gz"
libedit_archive="$download_root/libedit-$LIBEDIT_VERSION.tar.gz"
adios2_archive="$download_root/adios2-$ADIOS2_VERSION.tar.gz"

fetch "$NCURSES_URL" "$NCURSES_SHA256" "$ncurses_archive"
fetch "$LIBEDIT_URL" "$LIBEDIT_SHA256" "$libedit_archive"
fetch "$ADIOS2_URL" "$ADIOS2_SHA256" "$adios2_archive"
extract "$ncurses_archive" "$source_root/ncurses-$NCURSES_VERSION"
extract "$libedit_archive" "$source_root/libedit-$LIBEDIT_VERSION"
extract "$adios2_archive" "$source_root/ADIOS2-$ADIOS2_VERSION"

if [[ ! -f "$deps_root/lib/libncurses.so" ]]; then
  ncurses_build="$build_root/ncurses-$NCURSES_VERSION"
  mkdir -p "$ncurses_build"
  cd "$ncurses_build"
  "$source_root/ncurses-$NCURSES_VERSION/configure" \
    --prefix="$deps_root" \
    --with-shared \
    --without-debug \
    --without-ada \
    --without-tests \
    --disable-widec \
    --enable-overwrite
  make --jobs "$jobs"
  make install
fi

if [[ ! -f "$deps_root/lib/libedit.so" ]]; then
  libedit_build="$build_root/libedit-$LIBEDIT_VERSION"
  mkdir -p "$libedit_build"
  cd "$libedit_build"
  CPPFLAGS="-I$deps_root/include" \
  LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib" \
    "$source_root/libedit-$LIBEDIT_VERSION/configure" \
      --prefix="$deps_root" --enable-shared --disable-static
  make --jobs "$jobs"
  make install
fi

if [[ ! -x "$adios2_root/bin/adios2-config" ]]; then
  unset CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH LIBRARY_PATH
  cmake -S "$source_root/ADIOS2-$ADIOS2_VERSION" \
    -B "$build_root/adios2-$ADIOS2_VERSION" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$adios2_root" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DADIOS2_BUILD_EXAMPLES=OFF \
    -DADIOS2_USE_MPI=OFF \
    -DADIOS2_USE_CUDA=OFF \
    -DADIOS2_USE_Kokkos=OFF \
    -DADIOS2_USE_Fortran=OFF \
    -DADIOS2_USE_Python=OFF \
    -DADIOS2_USE_HDF5=OFF \
    -DADIOS2_USE_HDF5_VOL=OFF \
    -DADIOS2_USE_SST=OFF \
    -DADIOS2_USE_DataMan=OFF \
    -DADIOS2_USE_DataSpaces=OFF \
    -DADIOS2_USE_MHS=OFF \
    -DADIOS2_USE_ZeroMQ=OFF \
    -DADIOS2_USE_UCX=OFF \
    -DADIOS2_USE_BigWhoop=OFF \
    -DADIOS2_USE_Blosc2=OFF \
    -DADIOS2_USE_BZip2=OFF \
    -DADIOS2_USE_Caliper=OFF \
    -DADIOS2_USE_ZFP=OFF \
    -DADIOS2_USE_SZ=OFF \
    -DADIOS2_USE_LIBPRESSIO=OFF \
    -DADIOS2_USE_MGARD=OFF \
    -DADIOS2_USE_PNG=OFF \
    -DADIOS2_USE_DAOS=OFF \
    -DADIOS2_USE_IME=OFF \
    -DADIOS2_USE_Sodium=OFF \
    -DADIOS2_USE_Catalyst=OFF \
    -DADIOS2_USE_Campaign=OFF \
    -DADIOS2_USE_OpenSSL=OFF \
    -DADIOS2_USE_AWSSDK=OFF \
    -DADIOS2_USE_XRootD=OFF \
    -DADIOS2_USE_Profiling=OFF
  cmake --build "$build_root/adios2-$ADIOS2_VERSION" --parallel "$jobs"
  cmake --install "$build_root/adios2-$ADIOS2_VERSION"
fi

printf 'Dependency prefix: %s\n' "$deps_root"
printf 'ADIOS2 prefix: %s\n' "$adios2_root"
"$adios2_root/bin/adios2-config" --version
