#!/bin/sh
# build-glibc.sh — ARM64 glibc PanVK ICD (Bachata S4 / NativeCode AI)
# Usage: ./scripts/build-glibc.sh --profile g615-v11-csf
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""
while [ $# -gt 0 ]; do case "$1" in --profile) PROFILE="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }
MESA="$ROOT/work/mesa"; BDIR="$ROOT/build/linux-glibc"; DDIR="$ROOT/dist/glibc-$PROFILE"
mkdir -p "$BDIR" "$DDIR"
meson setup "$BDIR" "$MESA" --native-file "$ROOT/meson/linux-aarch64-native.ini" \
  -Dbuildtype=release -Dplatforms=x11,wayland -Dgallium-drivers= -Dvulkan-drivers=panfrost 2>&1 | tail -n 5
ninja -C "$BDIR" 2>&1 | tail -n 5
SO="$(find "$BDIR" -name libvulkan_panfrost.so | head -n1)"
[ -n "$SO" ] || { echo "BUILD-FAIL" >&2; exit 1; }
cp "$SO" "$DDIR/"
find "$BDIR" -name 'panfrost_icd.*.json' | head -n1 | xargs -r -I{} cp {} "$DDIR/panfrost_icd.aarch64.json"
echo "OK profile=$PROFILE dir=$DDIR"
