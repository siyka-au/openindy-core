#!/usr/bin/env bash
# Plugin ABI check: compares the public ABI of libOpenIndyCore against the
# committed baseline (abi/baseline.abi) with libabigail's abidw/abidiff.
#
#   scripts/abi-check.sh [--build-dir DIR | --lib FILE] [--update] [--save-current FILE]
#
# The library must carry DWARF debug info (abidw reads types from it). A normal
# Release build does not; configure the build used for this check with
#   -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-g
# (-g on top of Release keeps the Conan packages ABI-compatible, unlike Debug).
#
# Rule enforced here (see include/plugin/plugin_abi.h):
#   * abidiff reports an incompatible change (exit bit 8) and
#     OI_PLUGIN_ABI_VERSION equals abi/baseline.version  -> FAIL: bump the
#     version and refresh the baseline in the same commit.
#   * OI_PLUGIN_ABI_VERSION differs from abi/baseline.version -> FAIL until the
#     baseline is refreshed (--update) in the same commit as the bump, whether
#     or not abidiff sees a difference, so that the baseline always documents
#     the ABI of the current version. The diff is printed.
#   * only compatible changes (exit bit 4, e.g. added functions) -> pass with a
#     note; refresh the baseline with --update whenever convenient.
#   * no baseline -> FAIL with the one-time generation command (never a
#     silent pass).
#
# Exit codes: 0 ok, 1 ABI rule violated, 2 usage/setup problem or missing
# baseline, 77 abidw/abidiff not installed (ctest "skipped").
#
# Environment: ABIDW, ABIDIFF override the tool names.
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
abidw=${ABIDW:-abidw}
abidiff=${ABIDIFF:-abidiff}
build_dir=$root/build
lib=
update=0
save=

while [ $# -gt 0 ]; do
  case $1 in
    --update) update=1 ;;
    --build-dir) build_dir=$2; shift ;;
    --lib) lib=$2; shift ;;
    --save-current) save=$2; shift ;;
    -h|--help) sed -n '2,/^set -euo/p' "$0" | sed '$d;s/^# \{0,1\}//'; exit 0 ;;
    *) echo "abi-check: unknown argument $1" >&2; exit 2 ;;
  esac
  shift
done

for tool in "$abidw" "$abidiff"; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "abi-check: $tool not found (install libabigail: apt install abigail-tools / libabigail-tools, pacman -S libabigail)" >&2
    exit 77
  fi
done

[ -n "$lib" ] || lib=$build_dir/bin/libOpenIndyCore.so
if [ ! -f "$lib" ]; then
  echo "abi-check: $lib not found; build first (see header of this script)" >&2
  exit 2
fi
if ! readelf -S "$lib" 2>/dev/null | grep -q '\.debug_info'; then
  echo "abi-check: $lib has no DWARF debug info; rebuild with -DCMAKE_CXX_FLAGS=-g" >&2
  exit 2
fi

version=$(sed -n 's/^#define[[:space:]]\{1,\}OI_PLUGIN_ABI_VERSION[[:space:]]\{1,\}\([0-9]\{1,\}\).*/\1/p' \
          "$root/include/plugin/plugin_abi.h")
[ -n "$version" ] || { echo "abi-check: cannot read OI_PLUGIN_ABI_VERSION" >&2; exit 2; }

baseline=$root/abi/baseline.abi
baseline_version_file=$root/abi/baseline.version
suppr=$root/abi/suppressions.abignore
suppr_diff=$root/abi/suppressions-diff.abignore

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
current=$tmp/current.abi

# Run from the repo root so recorded paths are relative and reproducible.
( cd "$root" && "$abidw" --headers-dir include --drop-private-types \
    --force-early-suppression --no-show-locs --no-comp-dir-path \
    --suppressions "$suppr" --no-corpus-path --out-file "$current" "$lib" )
# make the abixml independent of where the repository is checked out
sed -i "s#path='$root/#path='#" "$current"

[ -z "$save" ] || cp "$current" "$save"

if [ "$update" -eq 1 ]; then
  cp "$current" "$baseline"
  echo "$version" > "$baseline_version_file"
  echo "abi-check: baseline refreshed for OI_PLUGIN_ABI_VERSION $version; commit abi/baseline.abi and abi/baseline.version"
  exit 0
fi

if [ ! -f "$baseline" ] || [ ! -f "$baseline_version_file" ]; then
  cat >&2 <<MSG
abi-check: FAIL - no baseline (abi/baseline.abi, abi/baseline.version).
Generate it once from a build with debug info and commit it:
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-g \\
        -DCMAKE_TOOLCHAIN_FILE=build/conan/conan_toolchain.cmake
  cmake --build build --target OpenIndyCore
  scripts/abi-check.sh --update
MSG
  exit 2
fi

base_version=$(tr -d '[:space:]' < "$baseline_version_file")

set +e
"$abidiff" --suppressions "$suppr" --suppressions "$suppr_diff" --drop-private-types "$baseline" "$current" > "$tmp/diff.txt" 2>&1
rc=$?
set -e

if [ "$rc" -ne 0 ]; then cat "$tmp/diff.txt"; fi

if [ $((rc & 3)) -ne 0 ]; then
  echo "abi-check: FAIL - abidiff error (exit $rc)" >&2
  exit 2
fi

if [ "$version" != "$base_version" ]; then
  echo "abi-check: FAIL - OI_PLUGIN_ABI_VERSION is $version but the baseline is for $base_version." >&2
  echo "  Refresh it in the same commit as the bump: scripts/abi-check.sh --update" >&2
  exit 1
fi

if [ $((rc & 8)) -ne 0 ]; then
  echo "abi-check: FAIL - incompatible ABI change with OI_PLUGIN_ABI_VERSION unchanged ($version)." >&2
  echo "  Bump OI_PLUGIN_ABI_VERSION in include/plugin/plugin_abi.h and refresh the baseline" >&2
  echo "  (scripts/abi-check.sh --update) in the same commit." >&2
  exit 1
fi

if [ $((rc & 4)) -ne 0 ]; then
  echo "abi-check: ok - compatible ABI changes only (see above); consider refreshing the baseline with --update"
else
  echo "abi-check: ok - no ABI change (version $version)"
fi
