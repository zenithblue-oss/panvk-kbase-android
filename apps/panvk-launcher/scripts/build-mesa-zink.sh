#!/bin/sh
# Build the OpenGL component for Wine's winex11 EGL backend (UseEGL): Mesa EGL + Zink (OpenGL on Vulkan/PanVK), Android bionic aarch64.
#   Mesa-Zink-$MESAZ_VERSION.wcp : libEGL.so.1, libGLESv2.so.2, libGLESv1_CM.so.1, dri/zink_dri.so + libgallium, libexpat.
#   ABI: aarch64 bionic, because Wine's unix side (win32u/winex11.so in Proton arm64ec, aarch64-unix) dlopens libEGL.so.1;
#   x86_64/i386 game code reaches it through opengl32 thunks, so no x86 Mesa is needed.
# Pins in scripts/components.env (tarball sha256 checked). Output: $OUT/*.wcp (+ .sha256).
# Needs: NDK $AUDIO_NDK_VER, meson, ninja, python3 + mako, flex, bison, glslangValidator, zstd, xz, host X11/xcb headers,
# $OUT/imagefs_bionic.txz (the bundled imagefs: libxcb/libX11/libdrm/android-sysvshm to link against) and libdrm
# headers built for Android (work/android-deps from scripts/build-android.sh, or DRM_PCDIR=).
# Patch: runtimes/mesa-zink/*.patch. Runtime env (RenderApi / Containers.env): LIBGL_KOPPER_DRI2=1 is required, the
# launcher's X server has no DRI3, and without it Mesa EGL drops kopper for a swrast PutImage path that shows nothing.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/components.env"
WORK=${WORK:-/var/tmp/panvk/mesa-zink}
OUT=${OUT:-/var/tmp/panvk/components}
JOBS=${JOBS:-8}
NDK=${NDK:-/opt/android-sdk/ndk/$AUDIO_NDK_VER}
TC=$NDK/toolchains/llvm/prebuilt/linux-x86_64
TRIPLE=aarch64-linux-android
API=29
[ -x "$TC/bin/$TRIPLE$API-clang" ] || { echo "NDK $AUDIO_NDK_VER not found (set NDK=)" >&2; exit 1; }
IMAGEFS_LIBS=${IMAGEFS_LIBS:-$WORK/imagefs-lib}   # dir with libxcb*.so, libxshmfence.so, libdrm.so, libXau, libXdmcp
if [ ! -f "$IMAGEFS_LIBS/libxcb.so" ]; then
  [ -f "$OUT/imagefs_bionic.txz" ] || { echo "no $OUT/imagefs_bionic.txz (scripts/fetch-launcher-components.sh)" >&2; exit 1; }
  rm -rf "$WORK/imagefs-x"; mkdir -p "$WORK/imagefs-x" "$IMAGEFS_LIBS"
  xz -dc "$OUT/imagefs_bionic.txz" | tar -C "$WORK/imagefs-x" -xf - --wildcards 'usr/lib/libX*' 'usr/lib/libxcb*' \
    'usr/lib/libxshmfence*' 'usr/lib/libdrm.so*' 'usr/lib/libexpat*' 'usr/lib/libandroid-sysvshm.so'
  cp -a "$WORK/imagefs-x/usr/lib/." "$IMAGEFS_LIBS/"
fi
[ -f "$IMAGEFS_LIBS/libxcb.so" ] || { echo "IMAGEFS_LIBS=$IMAGEFS_LIBS has no libxcb.so" >&2; exit 1; }
X11_INC=${X11_INCLUDE:-/usr/include}
DRM_PC=${DRM_PCDIR:-$HERE/../../../work/android-deps/lib/pkgconfig}   # libdrm built for Android (headers+.a)
SRC=$WORK/src; mkdir -p "$SRC" "$OUT"
TAR=mesa-$MESAZ_VER.tar.xz
[ -f "$SRC/$TAR" ] || { curl -fL -o "$SRC/$TAR.part" "https://archive.mesa3d.org/$TAR" && mv "$SRC/$TAR.part" "$SRC/$TAR"; }
echo "$MESAZ_SHA256  $SRC/$TAR" | sha256sum -c -
if [ ! -f "$SRC/mesa-$MESAZ_VER/.panplay-patched" ]; then
  rm -rf "$SRC/mesa-$MESAZ_VER"; tar -C "$SRC" -xf "$SRC/$TAR"
  for p in "$HERE"/../runtimes/mesa-zink/*.patch; do patch -d "$SRC/mesa-$MESAZ_VER" -p1 --forward < "$p"; done
  touch "$SRC/mesa-$MESAZ_VER/.panplay-patched"
fi

# pkg-config shims: real libs from the imagefs, headers from the host (X11/xcb headers are arch independent).
PC=$WORK/pc; rm -rf "$PC"; mkdir -p "$PC" "$WORK/inc"
cp -r "$X11_INC/xcb" "$X11_INC/X11" "$WORK/inc/"
ver() { pkg-config --modversion "$1" 2>/dev/null || echo "$2"; }
for pc in xcb xcb-randr xcb-dri3 xcb-present xcb-shm xcb-sync xcb-xfixes xcb-glx xcb-dri2 x11-xcb xshmfence; do
  case $pc in x11-xcb) v=$(ver x11-xcb 1.8.12); l="-lX11-xcb -lX11 -lxcb";; xshmfence) v=$(ver xshmfence 1.3.3); l="-lxshmfence";;
              *) v=$(ver xcb 1.17.0); l="-l$pc -lxcb";; esac
  printf 'Name: %s\nDescription: shim\nVersion: %s\nCflags: -I%s\nLibs: -L%s %s\n' "$pc" "$v" "$WORK/inc" "$IMAGEFS_LIBS" "$l" > "$PC/$pc.pc"
done
for pc in x11:X11 xext:Xext xxf86vm:Xxf86vm xfixes:Xfixes xrandr:Xrandr; do
  printf 'Name: %s\nDescription: shim\nVersion: 9.9.9\nCflags: -I%s\nLibs: -L%s -l%s\n' "${pc%%:*}" "$WORK/inc" "$IMAGEFS_LIBS" "${pc##*:}" > "$PC/${pc%%:*}.pc"
done
# GLX protocol headers (glproto, header only; Mesa libGL = the GL entry points Wine dlsyms from libGL.so.1)
mkdir -p "$WORK/inc/GL"; cp "$X11_INC/GL/glxproto.h" "$X11_INC/GL/glxmd.h" "$X11_INC/GL/glxtokens.h" "$WORK/inc/GL/"
printf 'Name: glproto\nDescription: shim\nVersion: 1.4.17\nCflags: -I%s\n' "$WORK/inc" > "$PC/glproto.pc"
cp "$DRM_PC"/libdrm.pc "$DRM_PC"/zlib.pc "$PC/" 2>/dev/null || true

# <cutils/native_handle.h> stand-in (only the type is used by vk_android_native_buffer.h)
mkdir -p "$WORK/stubinc/cutils"
printf '#pragma once\ntypedef struct native_handle { int version, numFds, numInts; int data[0]; } native_handle_t;\ntypedef const native_handle_t *buffer_handle_t;\n' > "$WORK/stubinc/cutils/native_handle.h"
cat > "$WORK/cross.ini" <<EOF
[binaries]
c = '$TC/bin/$TRIPLE$API-clang'
cpp = '$TC/bin/$TRIPLE$API-clang++'
ar = '$TC/bin/llvm-ar'
strip = '$TC/bin/llvm-strip'
pkg-config = 'pkg-config'
[built-in options]
c_args = ['-I$WORK/stubinc', '-DANDROID_API_LEVEL=$API', '-Dshmget=libandroid_shmget', '-Dshmat=libandroid_shmat', '-Dshmdt=libandroid_shmdt', '-Dshmctl=libandroid_shmctl']
cpp_args = ['-I$WORK/stubinc', '-DANDROID_API_LEVEL=$API', '-Dshmget=libandroid_shmget', '-Dshmat=libandroid_shmat', '-Dshmdt=libandroid_shmdt', '-Dshmctl=libandroid_shmctl']
c_link_args = ['-Wl,-z,max-page-size=16384', '-L$IMAGEFS_LIBS', '-Wl,--allow-shlib-undefined', '-llog', '-lsync', '-landroid-sysvshm']
cpp_link_args = ['-Wl,-z,max-page-size=16384', '-L$IMAGEFS_LIBS', '-static-libstdc++', '-llog', '-lsync', '-landroid-sysvshm']
[properties]
pkg_config_libdir = ['$PC']
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'armv8-a'
endian = 'little'
EOF
B=$WORK/build; [ -f "$B/build.ninja" ] || rm -rf "$B"
[ -f "$B/build.ninja" ] || meson setup "$B" "$SRC/mesa-$MESAZ_VER" --cross-file "$WORK/cross.ini" --buildtype release --strip \
  -Dgallium-drivers=zink -Dvulkan-drivers= -Dplatforms=x11 -Dandroid-libbacktrace=disabled -Degl=enabled -Dgles1=enabled -Dgles2=enabled -Dopengl=true \
  -Dglx=disabled -Dgbm=disabled -Dllvm=disabled -Dglvnd=disabled -Dlibunwind=disabled \
  -Dzstd=disabled -Dvalgrind=disabled -Dlmsensors=disabled -Dxlib-lease=disabled -Dgallium-va=disabled \
  -Dgallium-rusticl=false -Dbuild-tests=false -Dtools= \
  -Dprefix=/mesa-zink -Dlibdir=lib -Ddri-drivers-path=/mesa-zink/lib/dri >"$WORK/setup.log" 2>&1 || { tail -n 30 "$WORK/setup.log"; exit 1; }
ninja -C "$B" -j"$JOBS" >"$WORK/ninja.log" 2>&1 || { grep -E "error|FAILED" "$WORK/ninja.log" | cut -c1-250 | head -15; exit 1; }
rm -rf "$WORK/stage"; DESTDIR=$WORK/stage ninja -C "$B" install >"$WORK/install.log" 2>&1

# --- package: file names are the ones the dlopen'd consumers ask for (Wine: libEGL.so.1; Mesa: zink_dri.so in LIBGL_DRIVERS_PATH).
ST=$WORK/stage/mesa-zink/lib; P=$WORK/pkg; rm -rf "$P"; mkdir -p "$P/lib/dri"
cp -L "$ST/libEGL.so" "$P/lib/libEGL.so.1"; cp -L "$ST/libGLESv2.so" "$P/lib/libGLESv2.so.2"
cp -L "$ST/libGLESv1_CM.so" "$P/lib/libGLESv1_CM.so.1"; cp -L "$ST/libgallium-$MESAZ_VER.so" "$ST/libexpat.so" "$P/lib/"
cp -L "$ST/dri/libdril_dri.so" "$P/lib/dri/zink_dri.so"
cat > "$P/profile.json" <<EOF
{
  "type": "Mesa-Zink",
  "versionName": "$MESAZ_VERSION",
  "versionCode": 1,
  "description": "Mesa $MESAZ_VER EGL + Zink (OpenGL on Vulkan), aarch64 bionic, X11 platform",
  "files": []
}
EOF
wcp=$OUT/Mesa-Zink-$MESAZ_VERSION.wcp
tar -C "$P" --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner -cf - . | zstd -19 -T0 -q -f -o "$wcp"
sha256sum "$wcp" | tee "$wcp.sha256"
