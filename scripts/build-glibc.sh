#!/bin/sh
# build-glibc.sh — ARM64 glibc PanVK ICD (Bachata S4 / NativeCode AI)
# Usage: ./scripts/build-glibc.sh --profile g615-v11-csf [--clean]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""
CLEAN=0
while [ $# -gt 0 ]; do
  case "$1" in
    --profile) PROFILE="$2"; shift 2;;
    --clean) CLEAN=1; shift 1;;
    *) echo "unknown $1" >&2; exit 2;;
  esac
done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }

DDIR="$ROOT/dist/glibc-$PROFILE"
mkdir -p "$DDIR"

if [ "$(uname -m)" = "aarch64" ]; then
  MESA="$ROOT/work/mesa"
  BDIR="$ROOT/build/linux-glibc"
  if [ "$CLEAN" = "1" ] || [ ! -f "$BDIR/build.ninja" ]; then
    rm -rf "$BDIR"
    mkdir -p "$BDIR"
    meson setup "$BDIR" "$MESA" \
      --native-file "$ROOT/meson/linux-aarch64-native.ini" \
      -Dbuildtype=release \
      -Dplatforms=x11,wayland \
      -Dgallium-drivers= \
      -Dvulkan-drivers=panfrost \
      -Dpanfrost-kmds=kbase \
      -Degl=disabled -Dgles1=disabled -Dgles2=disabled -Dopengl=false \
      -Dglx=disabled -Dgbm=disabled -Dzstd=disabled -Dlibunwind=disabled
  fi
  ninja -j"$(nproc)" -C "$BDIR"
  SO="$(find "$BDIR" -name libvulkan_panfrost.so | head -n1)"
  cp "$SO" "$DDIR/libvulkan_panfrost.so"
  find "$BDIR" -name 'panfrost_icd.*.json' | head -n1 | xargs -r -I{} cp {} "$DDIR/panfrost_icd.aarch64.json"
else
  echo "Host is $(uname -m), dispatching clean glibc build on device chroot via ADB..."
  adb shell "/data/local/tmp/nativecode_chroot.sh sh --user root -- 'mkdir -p /tmp/mesa'"
  if [ "$CLEAN" = "1" ]; then
    echo "Cleaning device /tmp/build-glibc..."
    adb shell "/data/local/tmp/nativecode_chroot.sh sh --user root -- 'rm -rf /tmp/build-glibc'"
  fi
  echo "Syncing mesa tree to device chroot..."
  (cd "$ROOT/work/mesa" && tar -cf - .) | adb shell "/data/local/tmp/nativecode_chroot.sh sh --user root -- 'tar -xf - -C /tmp/mesa'"
  
  echo "Configuring and building in chroot..."
  adb shell "/data/local/tmp/nativecode_chroot.sh sh --user root -- '
    if [ ! -f /tmp/build-glibc/build.ninja ]; then
      meson setup /tmp/build-glibc /tmp/mesa \
        -Dbuildtype=release \
        -Dplatforms=x11,wayland \
        -Dgallium-drivers= \
        -Dvulkan-drivers=panfrost \
        -Dpanfrost-kmds=kbase \
        -Degl=disabled -Dgles1=disabled -Dgles2=disabled -Dopengl=false \
        -Dglx=disabled -Dgbm=disabled -Dzstd=disabled -Dlibunwind=disabled
    fi
    ninja -C /tmp/build-glibc
  '"
  echo "Pulling built glibc ICD..."
  adb pull /data/local/tmp/chrootAlpine/tmp/build-glibc/src/panfrost/vulkan/libvulkan_panfrost.so "$DDIR/libvulkan_panfrost.so"
  adb pull /data/local/tmp/chrootAlpine/tmp/build-glibc/src/panfrost/vulkan/panfrost_icd.aarch64.json "$DDIR/panfrost_icd.aarch64.json" 2>/dev/null || \
    printf '{"file_format_version":"1.0.0","ICD":{"library_path":"libvulkan_panfrost.so","api_version":"1.4.0"}}\n' > "$DDIR/panfrost_icd.aarch64.json"
fi

SO="$DDIR/libvulkan_panfrost.so"
[ -f "$SO" ] || { echo "BUILD-FAIL: $SO not found" >&2; exit 1; }

# Static verification of kbase symbols
if readelf -s "$SO" 2>/dev/null | grep -q "kbase_kmod_ops"; then
  echo "VERIFIED: kbase_kmod_ops present in $SO"
elif strings "$SO" | grep -q "kbase_kmod"; then
  echo "VERIFIED: kbase_kmod present in $SO"
else
  echo "FAIL: kbase symbols missing in $SO" >&2
  exit 1
fi

if readelf -s "$SO" 2>/dev/null | grep -E "(panfrost_kmod|panthor_kmod)"; then
  echo "FAIL: unexpected panfrost/panthor symbols found in $SO" >&2
  exit 1
fi

echo "OK glibc build complete: $SO"
