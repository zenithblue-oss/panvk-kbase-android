#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/p15-transform-feedback"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$src/p15-transform-feedback.c" -ldl -o "$out/p15-transform-feedback-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
  "$src/p15-transform-feedback.c" -ldl -o "$out/p15-transform-feedback-android"
file "$out/p15-transform-feedback-host" "$out/p15-transform-feedback-android"
