#!/bin/sh
# release-local.sh — local end-to-end: matrix + build + static validate + package
# Usage: ./scripts/release-local.sh --profile g615-v11-csf [--version v0.1.0-beta.1]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""; VER="v0.0.0-dev"
while [ $# -gt 0 ]; do case "$1" in --profile) PROFILE="$2"; shift 2;; --version) VER="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }
"$ROOT/scripts/fetch-mesa.sh"
"$ROOT/scripts/apply-patches.sh" --profile "$PROFILE"
python3 "$ROOT/scripts/capture-matrix.py" --mesa "$ROOT/work/mesa" --out "$ROOT/dist/matrix-$PROFILE"
"$ROOT/scripts/build-android.sh" --profile "$PROFILE"
"$ROOT/scripts/validate-binary.sh" --abi android "$ROOT/dist/android-$PROFILE/libvulkan_panfrost.so"
"$ROOT/scripts/package-android-adpkg.sh" --profile "$PROFILE" --version "$VER"
# Launcher zips (adrenotools/winlator/bannerlator/cmod/ludashi/gamehub) from the validated .so.
"$ROOT/scripts/package-launcher-zips.sh" --so "$ROOT/dist/android-$PROFILE/libvulkan_panfrost.so" --version "$VER"
# Optional glibc/EMULATOR package. Only valid when built from THIS checkout's patch series.
# To wrap an already released binary instead (no rebuild), use scripts/package-prebuilt.sh;
# that cannot claim a reproducible source series.
if [ "${WITH_GLIBC:-0}" = 1 ]; then
  "$ROOT/scripts/build-glibc.sh" --profile "$PROFILE"
  "$ROOT/scripts/package-glibc.sh" --profile "$PROFILE" --version "$VER"
  cp "$ROOT/dist/glibc-$PROFILE/libvulkan_panfrost.so" "$ROOT/dist/libvulkan_panfrost-glibc-aarch64.so"
fi
echo "LOCAL-RELEASE-ANDROID OK profile=$PROFILE version=$VER"
