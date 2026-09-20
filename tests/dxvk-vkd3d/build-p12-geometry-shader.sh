#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/p12-geometry-shader"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$src/p12-geometry-shader.c" -ldl -o "$out/p12-geometry-shader-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
  "$src/p12-geometry-shader.c" -ldl -o "$out/p12-geometry-shader-android"
file "$out/p12-geometry-shader-host" "$out/p12-geometry-shader-android"
