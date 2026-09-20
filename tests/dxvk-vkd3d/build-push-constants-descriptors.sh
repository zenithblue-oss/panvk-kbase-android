#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/push-constants-descriptors"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
for stage in comp vert frag; do
  glslangValidator --target-env vulkan1.2 -V "$src/push-constants-descriptors.$stage" -o "$out/push.$stage.spv"
  spirv-val --target-env vulkan1.2 "$out/push.$stage.spv"
  xxd -i -n "push_${stage}_spv" "$out/push.$stage.spv" > "$out/push.$stage.spv.h"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" -I"$out" \
  "$src/push-constants-descriptors.c" -ldl -o "$out/push-constants-descriptors-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$out" \
  "$src/push-constants-descriptors.c" -ldl -o "$out/push-constants-descriptors-android"
file "$out/push-constants-descriptors-host" "$out/push-constants-descriptors-android"
