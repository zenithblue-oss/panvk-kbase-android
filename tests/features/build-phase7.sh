#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/features"
out="$root/build/phase7"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
for name in derivatives-quads derivatives-linear untyped relaxed constant-data user-type; do
  spirv-as --target-env spv1.6 "$src/phase7-$name.comp.spvasm" -o "$out/phase7-$name.spv"
  if [ "$name" != constant-data ]; then
    spirv-val --target-env vulkan1.3 "$out/phase7-$name.spv"
  fi
  symbol=$(printf '%s' "$name" | tr '-' '_')
  xxd -i -n "phase7_${symbol}_spv" "$out/phase7-$name.spv" > "$out/phase7-$name.spv.h"
done
glslangValidator --target-env vulkan1.3 -V "$src/phase7-fma.comp" -o "$out/phase7-fma.spv"
spirv-val --target-env vulkan1.3 "$out/phase7-fma.spv"
xxd -i -n phase7_fma_spv "$out/phase7-fma.spv" > "$out/phase7-fma.spv.h"
"$cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" -I"$out" \
  "$src/phase7-compute.c" -ldl -o "$out/phase7-compute-android"
"$cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$src/phase7-api.c" -ldl -pthread -o "$out/phase7-api-android"
