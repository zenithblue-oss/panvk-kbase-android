#!/bin/sh
# build-android.sh — Android/Bionic arm64-v8a PanVK (hot-loadable ICD + Android WSI + AHB + Kbase)
# Usage: ./scripts/build-android.sh --profile g615-v11-csf [--api 35] [--ndk $ANDROID_NDK_ROOT]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE=""; API="${ANDROID_API:-35}"; NDK="${ANDROID_NDK_ROOT:-${ANDROID_HOME:-/opt/android-sdk}/ndk}"
while [ $# -gt 0 ]; do case "$1" in
  --profile) PROFILE="$2"; shift 2;; --api) API="$2"; shift 2;; --ndk) NDK="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
[ -n "$PROFILE" ] || { echo "--profile required" >&2; exit 2; }
NDK_BIN="$(ls -d "$NDK"/*/toolchains/llvm/prebuilt/linux-x86_64/bin 2>/dev/null | sort -V | tail -n1)"
if [ -z "$NDK_BIN" ]; then NDK_BIN="$(ls -d "$NDK"/toolchains/llvm/prebuilt/linux-x86_64/bin 2>/dev/null | head -n1)"; fi
[ -n "$NDK_BIN" ] || { echo "NDK toolchain not found under $NDK" >&2; exit 1; }
export PATH="$ROOT/build/host-tools/bin:$NDK_BIN:$PATH"
MESA="$ROOT/work/mesa"; BDIR="$ROOT/build/android-bionic"; DDIR="$ROOT/dist/android-$PROFILE"
CC_TRIPLE="aarch64-linux-android$API-clang"
mkdir -p "$BDIR"
sed "s/aarch64-linux-android[0-9]*-clang/$CC_TRIPLE/g" "$ROOT/meson/android-aarch64.ini" > "$BDIR.cross.ini"
# Pin target pkg-config to our NDK-built deps prefix so host /usr/lib .pc
# files (zlib, libudev, ...) can never leak host paths into the link.
DEPS_PCDIR="$ROOT/work/android-deps/lib/pkgconfig"
if [ -d "$DEPS_PCDIR" ]; then
  python3 - "$BDIR.cross.ini" "$DEPS_PCDIR" <<'EOF'
import sys
p, d = sys.argv[1], sys.argv[2]
t = open(p).read()
line = f"pkg_config_libdir = ['{d}']\n"
assert '[properties]' in t
t = t.replace('[properties]', '[properties]\n' + line, 1)
open(p, 'w').write(t)
EOF
fi
export PKG_CONFIG_PATH="$DEPS_PCDIR:${PKG_CONFIG_PATH:-}"
mkdir -p "$DDIR"
# Host codegen tools (mesa_clc, vtn_bindgen2) must come from a native build:
# export PATH with build/host-tools/bin (see bootstrap-host-tools.sh) or pass
# --native-file with [binaries] mesa_clc/vtn_bindgen2 paths.
meson setup "$BDIR" "$MESA" --cross-file "$BDIR.cross.ini" \
  -Dbuildtype=release -Dplatforms=android -Dandroid-stub=true \
  -Dgallium-drivers= -Dvulkan-drivers=panfrost -Dpanfrost-kmds=kbase \
  -Dmesa-clc=system -Dprecomp-compiler=system \
  -Degl=disabled -Dgles1=disabled -Dgles2=disabled -Dopengl=false \
  -Dglx=disabled -Dgbm=disabled -Dlibunwind=disabled -Dzstd=disabled \
  -Dcpp_link_args=-static-libstdc++ 2>&1 | tail -n 5
ninja -j"$(nproc)" -C "$BDIR" 2>&1 | tail -n 5
SO="$(find "$BDIR" -name libvulkan_panfrost.so | head -n1)"
[ -n "$SO" ] || { echo "BUILD-FAIL: libvulkan_panfrost.so not produced" >&2; exit 1; }
cp "$SO" "$DDIR/"
echo "OK profile=$PROFILE so=$DDIR/libvulkan_panfrost.so"
