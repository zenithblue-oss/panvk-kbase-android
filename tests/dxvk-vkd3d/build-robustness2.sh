#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/robustness2"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
glslangValidator --target-env vulkan1.2 -V "$src/robustness2.comp" -o "$out/robustness2.spv"
spirv-val --target-env vulkan1.2 "$out/robustness2.spv"
xxd -i -n robustness2_spv "$out/robustness2.spv" > "$out/robustness2.spv.h"
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" -I"$out" \
  "$src/robustness2.c" -ldl -o "$out/robustness2-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$out" \
  "$src/robustness2.c" -ldl -o "$out/robustness2-android"
file "$out/robustness2-host" "$out/robustness2-android"
