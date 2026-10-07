#!/bin/sh
# bc-emul-perf android build. $1 = tag (default bcperf)
T="${1:-bcperf}"
export TMPDIR=/var/tmp/panvk/tmpdir
export LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export MESA=${2:-/var/tmp/panvk/wt-bcperf} BDIR=/var/tmp/panvk/build-${3:-$T}-android DDIR=/var/tmp/panvk/dist-$T
/home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > /var/tmp/panvk/$T-android.out 2>&1
echo "EXIT $?" >> /var/tmp/panvk/$T-android.out
