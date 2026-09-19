#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
src="$root/tests/features"
ndk=${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}
android_cc="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang"
mkdir -p "$root/build/feature-tests"
for shader in descriptor.comp bda.comp push.comp robust.comp draw.vert draw.frag texture.comp; do
  glslangValidator --target-env vulkan1.2 -V "$src/$shader" -o "$root/build/feature-tests/$shader.spv"
  xxd -i -n "${shader%.*}_${shader##*.}_spv" "$root/build/feature-tests/$shader.spv" > "$root/build/feature-tests/$shader.spv.h"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
  "$root/tests/features/sync-features.c" -ldl -o "$root/build/feature-tests/sync-features-host"
"$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
  "$root/tests/features/sync-features.c" -ldl -o "$root/build/feature-tests/sync-features-android"
for test in compute-features graphics-features texture-features; do
  cc -std=c11 -O2 -Wall -Wextra -Werror -I"$root/work/mesa/include" \
    -I"$root/build/feature-tests" "$src/$test.c" -ldl -lm \
    -o "$root/build/feature-tests/$test-host"
  "$android_cc" -std=c11 -O2 -Wall -Wextra -Werror \
    -I"$root/build/feature-tests" "$src/$test.c" -ldl -lm \
    -o "$root/build/feature-tests/$test-android"
done
file "$root/build/feature-tests/sync-features-host" "$root/build/feature-tests/sync-features-android"
file "$root/build/feature-tests/"*-features-host "$root/build/feature-tests/"*-features-android
