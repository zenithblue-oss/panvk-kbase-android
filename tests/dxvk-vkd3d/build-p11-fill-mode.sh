#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/p11-fill-mode"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$src/p11-fill-mode.c" -ldl -o "$out/p11-fill-mode-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
  "$src/p11-fill-mode.c" -ldl -o "$out/p11-fill-mode-android"
file "$out/p11-fill-mode-host" "$out/p11-fill-mode-android"
