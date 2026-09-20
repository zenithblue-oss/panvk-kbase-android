#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/dxvk-vkd3d"
out="$root/build/descriptor-indexing-stress"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$out"
for kind in 0 1 2 3 4 5; do
  glslangValidator --target-env vulkan1.2 -V --define-macro KIND="$kind" "$src/descriptor-indexing-stress.comp" -o "$out/kind-$kind.spv"
  spirv-val --target-env vulkan1.2 "$out/kind-$kind.spv"
  xxd -i -n "descriptor_kind_${kind}_spv" "$out/kind-$kind.spv" > "$out/kind-$kind.spv.h"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" -I"$out" \
  "$src/descriptor-indexing-stress.c" -ldl -o "$out/descriptor-indexing-stress-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror -I"$out" \
  "$src/descriptor-indexing-stress.c" -ldl -o "$out/descriptor-indexing-stress-android"
file "$out/descriptor-indexing-stress-host" "$out/descriptor-indexing-stress-android"
