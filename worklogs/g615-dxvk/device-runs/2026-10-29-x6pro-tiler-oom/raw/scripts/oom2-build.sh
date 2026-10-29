#!/bin/sh
# usage: oom2-build.sh <mesa tree> <tag>   (bionic build, 4 jobs)
mkdir -p /var/tmp/panvk/tmp-oom2
export TMPDIR=/var/tmp/panvk/tmp-oom2 LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export MESA=$1 BDIR=/var/tmp/panvk/bionic-$2 DDIR=/var/tmp/panvk/bionic-$2-dist PANVK_JOBS=4
sh /home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > /var/tmp/panvk/bionic-$2.out 2>&1
echo "EXIT $?" >> /var/tmp/panvk/bionic-$2.out
