#!/bin/sh
# Build 122 dev: /var/tmp/panvk/mesa-122 (p122-dev branch) incremental into bionic-122.
mkdir -p /var/tmp/panvk/tmp-122
export TMPDIR=/var/tmp/panvk/tmp-122 LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export MESA=${MESA:-/var/tmp/panvk/mesa-122} BDIR=${BDIR:-/var/tmp/panvk/bionic-122} DDIR=${DDIR:-/var/tmp/panvk/bionic-122-dist} PANVK_JOBS=4
sh /home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > /var/tmp/panvk/b122/build.out 2>&1
echo "BUILD-EXIT $?" | tee /var/tmp/panvk/b122/build.exit
