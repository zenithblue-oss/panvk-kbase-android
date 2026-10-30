#!/bin/bash
# Build 122f dev: /var/tmp/panvk/mesa-122f-dev (p122f-dev) incremental into bionic-p122f, 4 jobs.
# usage: build.sh [tag]  -> copies the .so to /var/tmp/panvk/build-out/2026-10-30-<tag>/ when tag given
mkdir -p /var/tmp/panvk/tmp-122f
export TMPDIR=/var/tmp/panvk/tmp-122f LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export MESA=${MESA:-/var/tmp/panvk/mesa-122f-dev} BDIR=${BDIR:-/var/tmp/panvk/bionic-p122f} DDIR=${DDIR:-/var/tmp/panvk/bionic-p122f-dist} PANVK_JOBS=4
O=/var/tmp/panvk/b122f
sh /home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > $O/build.out 2>&1
E=$?
echo "BUILD-EXIT $E $(date +%T)" | tee $O/build.exit
if [ $E = 0 ] && [ -n "$1" ]; then
  D=/var/tmp/panvk/build-out/2026-10-30-$1; mkdir -p $D
  cp $DDIR/libvulkan_panfrost.so $D/ 2>/dev/null || cp $(find $BDIR -name libvulkan_panfrost.so | head -1) $D/
  echo "SO $D $(sha256sum $D/libvulkan_panfrost.so | cut -c1-16)" | tee -a $O/build.exit
fi
