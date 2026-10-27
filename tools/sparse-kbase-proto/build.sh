#!/bin/sh
# Build the standalone kbase sparse probe for aarch64 Android (API 29) with a
# bare NDK clang.  No Gradle, no Mesa.  Output: tools/sparse-kbase-proto/out/.
#
#   tools/sparse-kbase-proto/build.sh
#
# Honours NDK at /opt/android-sdk/ndk/29.0.14206865 (override with NDK=...).
# Runs under the shared build lock so it does not contend with Mesa/Gradle.
set -eu

D="${0%/*}"
case "$D" in /*) ;; *) D="$PWD/$D" ;; esac

NDK="${NDK:-/opt/android-sdk/ndk/29.0.14206865}"
API="${API:-29}"
LOCK="${BUILD_LOCK:-/home/flux/build-cache/build.lock}"

# Pick a prebuilt toolchain: native aarch64 host first, else x86_64.
TC="$NDK/toolchains/llvm/prebuilt/linux-aarch64"
[ -d "$TC" ] || TC="$NDK/toolchains/llvm/prebuilt/linux-x86_64"
CC="$TC/bin/aarch64-linux-android${API}-clang"

if [ ! -x "$CC" ]; then
   echo "build.sh: NDK clang not found at $CC" >&2
   echo "build.sh: set NDK=/path/to/ndk (looked under $NDK)" >&2
   exit 1
fi

OUT="$D/out"
mkdir -p "$OUT"

set -- "$CC" -O2 -Wall -Wextra -Werror -std=c11 -D_GNU_SOURCE \
   "$D/sparse_kbase_proto.c" -o "$OUT/sparse_kbase_proto"

if [ -x /usr/bin/flock ]; then
   flock "$LOCK" "$@"
else
   "$@"
fi

echo "build.sh: built $OUT/sparse_kbase_proto"
"$OUT/sparse_kbase_proto" --selftest
