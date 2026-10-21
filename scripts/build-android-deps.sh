#!/bin/sh
# build-android-deps.sh — NDK-built target deps for build-android.sh (work/android-deps)
# libdrm: static, headers + pkg-config only (kbase does not use DRM, Mesa util includes xf86drm.h).
# zlib: the NDK sysroot libz, so only a .pc file is written.
# Everything else (expat, spirv-tools, libelf, valgrind) is left out on purpose:
# build-android.sh pins pkg_config_libdir here, so host copies cannot leak in.
# Usage: ./scripts/build-android-deps.sh [--api 29] [--ndk $ANDROID_NDK_ROOT] [--out work/android-deps]
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
API="${ANDROID_API:-29}"; NDK="${ANDROID_NDK_ROOT:-${ANDROID_HOME:-/opt/android-sdk}/ndk}"
OUT="$ROOT/work/android-deps"; LIBDRM_TAG="libdrm-2.4.125"
while [ $# -gt 0 ]; do case "$1" in
  --api) API="$2"; shift 2;; --ndk) NDK="$2"; shift 2;; --out) OUT="$2"; shift 2;; *) echo "unknown $1" >&2; exit 2;; esac; done
NDK_BIN="$(ls -d "$NDK"/*/toolchains/llvm/prebuilt/linux-$(uname -m)/bin "$NDK"/toolchains/llvm/prebuilt/linux-$(uname -m)/bin 2>/dev/null | sort -V | tail -n1)"
[ -n "$NDK_BIN" ] || { echo "NDK toolchain not found under $NDK" >&2; exit 1; }
export PATH="$NDK_BIN:$PATH"
SRC="$ROOT/work/libdrm"; BDIR="$ROOT/build/libdrm-android"
[ -d "$SRC/.git" ] || git clone -q --depth 1 -b "$LIBDRM_TAG" https://gitlab.freedesktop.org/mesa/drm.git "$SRC"
mkdir -p "$ROOT/build"
sed "s/aarch64-linux-android[0-9]*-clang/aarch64-linux-android$API-clang/g" "$ROOT/meson/android-aarch64.ini" > "$BDIR.cross.ini"
RECONF=""; [ -f "$BDIR/build.ninja" ] && RECONF="--reconfigure"
PKG_CONFIG_LIBDIR=/nonexistent meson setup $RECONF "$BDIR" "$SRC" --cross-file "$BDIR.cross.ini" \
  --prefix="$OUT" --libdir=lib -Dbuildtype=release -Ddefault_library=static \
  -Dintel=disabled -Dradeon=disabled -Damdgpu=disabled -Dnouveau=disabled -Dvmwgfx=disabled \
  -Domap=disabled -Dexynos=disabled -Dfreedreno=disabled -Dtegra=disabled -Dvc4=disabled \
  -Detnaviv=disabled -Dcairo-tests=disabled -Dman-pages=disabled -Dvalgrind=disabled \
  -Dtests=false -Dudev=false >"$BDIR.log" 2>&1 || { tail -n 20 "$BDIR.log"; exit 1; }
ninja -C "$BDIR" install >>"$BDIR.log" 2>&1 || { tail -n 20 "$BDIR.log"; exit 1; }
cat > "$OUT/lib/pkgconfig/zlib.pc" <<'EOF'
Name: zlib
Description: zlib from the NDK sysroot
Version: 1.3
Libs: -lz
Cflags:
EOF
echo "OK deps=$OUT ($LIBDRM_TAG static, NDK zlib)"
