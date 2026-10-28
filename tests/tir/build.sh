#!/bin/sh
# Build tir for Android arm64. Usage: build.sh <mesa-tree> <out-binary>
set -e
D="$(cd "$(dirname "$0")" && pwd)"
NDK="${ANDROID_NDK_ROOT:-/opt/android-sdk/ndk/30.0.14904198}"
CC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang"
O="$(dirname "$2")"
glslc -mfmt=c "$D/tir.vert" -o "$O/tir.vert.inc"
glslc -mfmt=c "$D/tir.frag" -o "$O/tir.frag.inc"
"$CC" -O1 -I"$1/include" -I"$O" "$D/tir.c" -ldl -o "$2"
