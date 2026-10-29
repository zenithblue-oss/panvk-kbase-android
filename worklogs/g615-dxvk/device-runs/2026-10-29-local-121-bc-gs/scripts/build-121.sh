#!/bin/sh
# Build 121b: Mesa 5a07217f + strict prepare --wip-max 0163 (0162 BC batch prewarm only for batchable images, 0163 v9 GS pt skip).
set -e
M=/var/tmp/panvk/mesa-121-final
[ -d $M ] || git -C /var/tmp/panvk/mesa worktree add -q -b p121-final $M 5a07217f
sh /home/abhaybyte/repos/panvk/scripts/prepare-rc5-tree.sh --mesa $M --wip-max 0163 > /var/tmp/panvk/b121/prepare.out 2>&1
tail -1 /var/tmp/panvk/b121/prepare.out
mkdir -p /var/tmp/panvk/tmp-121
export TMPDIR=/var/tmp/panvk/tmp-121 LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export MESA=$M BDIR=/var/tmp/panvk/bionic-121 DDIR=/var/tmp/panvk/bionic-121-dist PANVK_JOBS=4
set +e
sh /home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > /var/tmp/panvk/b121/build.out 2>&1
echo "BUILD-EXIT $?" | tee /var/tmp/panvk/b121/build.exit
