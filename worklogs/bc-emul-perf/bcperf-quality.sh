#!/bin/sh
# bcperf-quality.sh <tag> <src-name> : dumps LOD0 of BC1/BC3/BC7 decoded via
# default path and PANVK_BC_AFRC=32/24/16, pulls to res-q-<tag>-<src>/.
T="$1"; S="$2"
export ANDROID_SERIAL=192.168.1.34:35419
B=/var/tmp/panvk/bcperf-bin; O=/var/tmp/panvk/res-q-$T-$S; D=/data/local/tmp/bcperf
mkdir -p $O
exec 9>>/var/tmp/panvk/device.lock
flock -w 1800 9 || { echo LOCK_TIMEOUT; exit 1; }
adb shell mkdir -p $D/$T
adb push /var/tmp/panvk/dist-$T/libvulkan_panfrost.so $D/$T/ >/dev/null
adb push $B/bc-perf $B/src-$S.rgba $D/ >/dev/null
for cu in 0 32 24 16; do
  adb shell "cd $D && rm -rf q$cu && mkdir q$cu && chmod 755 bc-perf && PANVK_BC_AFRC=$cu BCPERF_SIZE=1024 BCPERF_SRC=src-$S.rgba BCPERF_DUMP=q$cu ./bc-perf $D/$T/libvulkan_panfrost.so BC" > $O/perf-$cu.txt 2>&1
  rm -rf $O/q$cu; adb pull $D/q$cu $O/q$cu >/dev/null
done
adb shell "rm -rf $D/q0 $D/q32 $D/q24 $D/q16"
echo DONE
