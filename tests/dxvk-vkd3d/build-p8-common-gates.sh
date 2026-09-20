#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/p8-common-gates"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
for shader in p8-int.comp p8-sampler.comp p8.vert p8.frag; do
  glslangValidator --target-env vulkan1.2 -V "$src/$shader" -o "$out/$shader.spv"
  spirv-val --target-env vulkan1.2 "$out/$shader.spv"
  name=$(printf '%s' "$shader" | tr '.-' '__')
  xxd -i -n "${name}_spv" "$out/$shader.spv" > "$out/$shader.spv.h"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" -I"$out" \
  "$src/p8-common-gates.c" -ldl -o "$out/p8-common-gates-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$out" \
  "$src/p8-common-gates.c" -ldl -o "$out/p8-common-gates-android"
file "$out/p8-common-gates-host" "$out/p8-common-gates-android"
