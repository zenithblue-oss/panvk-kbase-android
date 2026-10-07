#!/bin/sh
# Build bc-perf + bc_decode test executables for the device. Out: /var/tmp/panvk/bcperf-bin
set -eu
WT=/home/abhaybyte/repos/panvk/.claude/worktrees/agent-a3c08a5bb85d0d417
CC=/opt/android-sdk/ndk/27.2.12479018/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android35-clang
INC=/var/tmp/panvk/wt-bcperf/include
OUT=/var/tmp/panvk/bcperf-bin
mkdir -p $OUT
$CC -O2 -w -I$INC -I$WT/device -o $OUT/bc-perf $WT/device/bc-perf.c -ldl -lm
$CC -O2 -w -I$INC -I$WT/tests/dxvk/vulkan/bc -I$WT/apps/panvk-test/app/src/main/cpp/compat \
   -o $OUT/bc_decode $WT/tests/dxvk/vulkan/bc/bc_decode.c -ldl -lm
echo BUILD-OK
