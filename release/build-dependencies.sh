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

# Where a file may be fetched from, best first. GNU's own server is at times
# unreachable (the 1.0.10 build timed out on ftp.gnu.org); its mirrors carry
# the same files. The pinned checksum, not the server, decides whether a
# download is accepted.
sources_for()
{
  local url="$1"
  printf '%s\n' "$url"
  case "$url" in
    https://ftp.gnu.org/gnu/*)
      printf '%s\n' "https://ftpmirror.gnu.org/${url#https://ftp.gnu.org/gnu/}" \
                    "https://mirrors.kernel.org/gnu/${url#https://ftp.gnu.org/gnu/}"
      ;;
  esac
}

# Hosts that could not be reached during this run. They are tried last for
# the files that follow, rather than first: with both ftp.gnu.org and
# ftpmirror.gnu.org down, waiting on each of them for each of the fifteen
# GNU files took longer than the whole build.
unreachable=" "

host_of()
{
  local host="${1#*://}"
  printf '%s\n' "${host%%/*}"
}

fetch()
{
  local url="$1"
  local sha256="$2"
  local target="$3"
  local source host rc
  local first=() last=()

  if [[ -f "$target" ]] &&
     printf '%s  %s\n' "$sha256" "$target" | sha256sum --check --status -; then
    printf '%s: OK\n' "$target"
    return 0
  fi
  while read -r source; do
    if [[ "$unreachable" == *" $(host_of "$source") "* ]]; then
      last+=("$source")
    else
      first+=("$source")
    fi
  done < <(sources_for "$url")
  for source in ${first[@]+"${first[@]}"} ${last[@]+"${last[@]}"}; do
    host="$(host_of "$source")"
    rm -f -- "$target.part"
    if curl -fL --connect-timeout 20 --retry 2 --retry-delay 2 \
         "$source" -o "$target.part"; then
      rc=0
    else
      rc=$?
    fi
    if (( rc == 0 )) &&
       printf '%s  %s\n' "$sha256" "$target.part" | sha256sum --check --status -; then
      mv -- "$target.part" "$target"
      printf '%s: OK, from %s\n' "$target" "$source"
      return 0
    fi
    case "$rc" in
      6|7|28|35)                  # name lookup, connect, timeout, TLS connect
        [[ "$unreachable" == *" $host "* ]] || unreachable+="$host " ;;
    esac
    printf 'Could not get %s from %s, or it did not match its checksum.\n' \
      "${target##*/}" "$source" >&2
  done
  rm -f -- "$target.part"
  printf 'No source had %s with SHA-256 %s.\n' "${target##*/}" "$sha256" >&2
  return 1
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
readline_archive="$download_root/readline-$READLINE_VERSION.tar.gz"
adios2_archive="$download_root/adios2-$ADIOS2_VERSION.tar.gz"

fetch "$NCURSES_URL" "$NCURSES_SHA256" "$ncurses_archive"
fetch "$READLINE_URL" "$READLINE_SHA256" "$readline_archive"
for readline_patch in $READLINE_PATCHES; do
  fetch "$READLINE_PATCH_URL/readline${READLINE_VERSION//./}-${readline_patch%%:*}" \
    "${readline_patch#*:}" \
    "$download_root/readline${READLINE_VERSION//./}-${readline_patch%%:*}"
done
fetch "$ADIOS2_URL" "$ADIOS2_SHA256" "$adios2_archive"
extract "$ncurses_archive" "$source_root/ncurses-$NCURSES_VERSION"
extract "$readline_archive" "$source_root/readline-$READLINE_VERSION"
# Apply the official patches once; the stamp records how far a source tree
# that survived an earlier run has got.
readline_source="$source_root/readline-$READLINE_VERSION"
readline_stamp="$readline_source/.opengrads-patches"
for readline_patch in $READLINE_PATCHES; do
  number="${readline_patch%%:*}"
  if ! grep -qx "$number" "$readline_stamp" 2>/dev/null; then
    (cd "$readline_source" &&
       patch -p0 --forward --batch \
         < "$download_root/readline${READLINE_VERSION//./}-$number")
    printf '%s\n' "$number" >> "$readline_stamp"
    rm -f "$deps_root/lib/libreadline.so"   # rebuild with the patch
  fi
done
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

if [[ ! -f "$deps_root/lib/libreadline.so" ]]; then
  readline_build="$build_root/readline-$READLINE_VERSION"
  mkdir -p "$readline_build"
  cd "$readline_build"
  CPPFLAGS="-I$deps_root/include" \
  LDFLAGS="-L$deps_root/lib -Wl,-rpath,$deps_root/lib" \
    "$source_root/readline-$READLINE_VERSION/configure" \
      --prefix="$deps_root" --enable-shared --disable-static
  make --jobs "$jobs" SHLIB_LIBS=-lncurses
  make install SHLIB_LIBS=-lncurses
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
