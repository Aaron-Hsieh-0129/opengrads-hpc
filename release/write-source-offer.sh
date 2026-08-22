#!/usr/bin/env bash
# Expand the GPLv2 section 3 written offer into a finished bundle.

set -euo pipefail

if [[ "$#" -ne 3 ]]; then
  printf 'Usage: %s BUNDLE_ROOT DIST_VERSION GRADS_VERSION\n' "$0" >&2
  exit 2
fi

repo_root="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
bundle_root="$1"
dist_version="$2"
grads_version="$3"

repo_url="${OPENGRADS_SOURCE_REPO:-https://github.com/Aaron-Hsieh-0129/opengrads-hpc}"
contact="${OPENGRADS_SOURCE_CONTACT:-$repo_url/issues}"

# The commit is what makes the offer answerable, so fall back through every
# source that might know it before giving up.
commit="${GITHUB_SHA:-}"
if [[ -z "$commit" ]]; then
  commit="$(git -C "$repo_root" rev-parse HEAD 2>/dev/null || true)"
fi
if [[ -z "$commit" ]]; then
  printf 'Cannot determine the source commit for the written offer.\n' >&2
  printf 'Set GITHUB_SHA or build from a git checkout.\n' >&2
  exit 1
fi
if [[ -n "$(git -C "$repo_root" status --porcelain 2>/dev/null || true)" ]]; then
  commit="$commit (plus uncommitted local changes)"
fi

build_id="$(uname -s) $(uname -m) on ${SOURCE_DATE_EPOCH:+epoch $SOURCE_DATE_EPOCH}"
if [[ -z "${SOURCE_DATE_EPOCH:-}" ]]; then
  build_id="$(uname -s) $(uname -m) on $(date -u +%Y-%m-%dT%H:%M:%SZ)"
fi

sed -e "s|@DIST_VERSION@|$dist_version|g" \
    -e "s|@GRADS_VERSION@|$grads_version|g" \
    -e "s|@REPO_URL@|$repo_url|g" \
    -e "s|@COMMIT@|$commit|g" \
    -e "s|@BUILD_ID@|$build_id|g" \
    -e "s|@CONTACT@|$contact|g" \
    "$repo_root/release/SOURCE_OFFER.in" > "$bundle_root/SOURCE_OFFER"
chmod 0644 "$bundle_root/SOURCE_OFFER"
