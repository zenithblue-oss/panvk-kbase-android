#!/bin/sh
# Build MangoHud (Vulkan overlay layer) from the pinned upstream tag for Wine's unix side: aarch64 Android bionic,
# the ABI winevulkan.so (Proton arm64ec aarch64-unix) and the imagefs Vulkan loader (usr/lib/libvulkan.so.1) run in.
#   MangoHud-$MANGOHUD_VERSION.wcp : lib/libMangoHud.so + profile.json. The launcher writes the layer manifest at launch
#   (Containers.env, VK_ADD_IMPLICIT_LAYER_PATH, enable env MANGOHUD=1) and the MangoHud.conf (MangoHudSettings).
# X11 is headers only (MangoHud dlopens libX11.so.6 from the imagefs for keybinds); no Wayland, D-Bus, NVML, XNVCtrl.
# Pins in scripts/components.env. Output: $OUT/MangoHud-$MANGOHUD_VERSION.wcp (+ .sha256).
# Needs: NDK $AUDIO_NDK_VER, meson, ninja, python3 + mako, glslangValidator, zstd, git, host X11 + xkbcommon headers.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/components.env"
WORK=${WORK:-/var/tmp/panvk/mangohud}
OUT=${OUT:-/var/tmp/panvk/components}
JOBS=${JOBS:-4}
NDK=${NDK:-/opt/android-sdk/ndk/$AUDIO_NDK_VER}
TC=$NDK/toolchains/llvm/prebuilt/linux-x86_64
TRIPLE=aarch64-linux-android
API=29
[ -x "$TC/bin/$TRIPLE$API-clang" ] || { echo "NDK $AUDIO_NDK_VER not found (set NDK=)" >&2; exit 1; }
mkdir -p "$WORK" "$OUT"

SRC=$WORK/src
[ -d "$SRC/.git" ] || git clone -q --depth 1 --branch "$MANGOHUD_TAG" https://github.com/flightlessmango/MangoHud.git "$SRC"
test "$(git -C "$SRC" rev-parse HEAD)" = "$MANGOHUD_COMMIT" || { echo "MangoHud commit mismatch" >&2; exit 1; }
git -C "$SRC" checkout -q -- .
for p in "$HERE"/../runtimes/mangohud/*.patch; do [ -f "$p" ] && git -C "$SRC" apply "$p"; done

# pkg-config shims: X11 headers only; xkbcommon links against the imagefs libxkbcommon.so.0 (keybind names).
IMAGEFS_LIBS=$WORK/imagefs-lib
if [ ! -f "$IMAGEFS_LIBS/libxkbcommon.so" ]; then
  [ -f "$OUT/imagefs_bionic.txz" ] || { echo "no $OUT/imagefs_bionic.txz (scripts/fetch-launcher-components.sh)" >&2; exit 1; }
  rm -rf "$WORK/imagefs-x"; mkdir -p "$WORK/imagefs-x" "$IMAGEFS_LIBS"
  xz -dc "$OUT/imagefs_bionic.txz" | tar -C "$WORK/imagefs-x" -xf - --wildcards 'usr/lib/libxkbcommon.so*'
  cp -a "$WORK/imagefs-x/usr/lib/." "$IMAGEFS_LIBS/"
fi
PC=$WORK/pc; rm -rf "$PC"; mkdir -p "$PC"
X11_INC=${X11_INCLUDE:-/usr/include}
rm -rf "$WORK/inc"; mkdir -p "$WORK/inc"; cp -r "$X11_INC/X11" "$X11_INC/xkbcommon" "$WORK/inc/"
printf 'Name: x11\nDescription: shim\nVersion: 9.9.9\nCflags: -I%s\n' "$WORK/inc" > "$PC/x11.pc"
printf 'Name: xkbcommon\nDescription: shim\nVersion: 9.9.9\nCflags: -I%s\nLibs: -L%s -lxkbcommon\n' "$WORK/inc" "$IMAGEFS_LIBS" > "$PC/xkbcommon.pc"
cat > "$WORK/cross.ini" <<EOF
[binaries]
c = '$TC/bin/$TRIPLE$API-clang'
cpp = '$TC/bin/$TRIPLE$API-clang++'
ar = '$TC/bin/llvm-ar'
strip = '$TC/bin/llvm-strip'
pkg-config = 'pkg-config'
[built-in options]
cpp_link_args = ['-Wl,-z,max-page-size=16384', '-static-libstdc++', '-Wl,--undefined-version']
c_link_args = ['-Wl,-z,max-page-size=16384']
[properties]
pkg_config_libdir = ['$PC']
[host_machine]
system = 'linux'
cpu_family = 'aarch64'
cpu = 'armv8-a'
endian = 'little'
EOF
B=$WORK/build; rm -rf "$B"
meson setup "$B" "$SRC" --cross-file "$WORK/cross.ini" --buildtype release --strip -Db_ndebug=true \
  -Dwith_x11=enabled -Dwith_wayland=disabled -Dwith_dbus=disabled -Dwith_nvml=disabled -Dwith_xnvctrl=disabled \
  -Dinclude_doc=false -Dtests=disabled -Dmangoplot=disabled -Dmangoapp=false -Dmangohudctl=false \
  -Dappend_libdir_mangohud=false -Ddynamic_string_tokens=false >"$WORK/setup.log" 2>&1 || { tail -n 30 "$WORK/setup.log"; exit 1; }
ninja -C "$B" -j"$JOBS" src/libMangoHud.so >"$WORK/ninja.log" 2>&1 || { grep -E "error|FAILED" "$WORK/ninja.log" | cut -c1-250 | head -20; exit 1; }

P=$WORK/pkg; rm -rf "$P"; mkdir -p "$P/lib"
cp -L "$B/src/libMangoHud.so" "$P/lib/"
"$TC/bin/llvm-strip" "$P/lib/libMangoHud.so"
cat > "$P/profile.json" <<EOF
{
  "type": "MangoHud",
  "versionName": "$MANGOHUD_VERSION",
  "versionCode": 1,
  "description": "MangoHud $MANGOHUD_TAG ($MANGOHUD_COMMIT) Vulkan overlay layer, aarch64 bionic, built from https://github.com/flightlessmango/MangoHud (scripts/build-mangohud.sh)",
  "files": []
}
EOF
wcp=$OUT/MangoHud-$MANGOHUD_VERSION.wcp
tar -C "$P" --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -cf - . | zstd -19 -T0 -q -f -o "$wcp"
sha256sum "$wcp" | tee "$wcp.sha256"
