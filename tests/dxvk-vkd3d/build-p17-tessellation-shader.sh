#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/p17-tessellation-shader"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$src/p17-tessellation-shader.c" -ldl -o "$out/p17-tessellation-shader-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
  "$src/p17-tessellation-shader.c" -ldl -o "$out/p17-tessellation-shader-android"
file "$out/p17-tessellation-shader-host" "$out/p17-tessellation-shader-android"
