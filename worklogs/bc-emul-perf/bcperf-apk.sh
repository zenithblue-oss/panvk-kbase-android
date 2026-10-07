#!/bin/sh
# bcperf-apk.sh <apk> <test>...: side-by-side panvk-test (dev.zenithblue.panvktest.bcperf),
# runs each test via autorun with the bundled driver, prints the test logs.
APK="$1"; shift
export ANDROID_SERIAL=192.168.1.34:35419
P=dev.zenithblue.panvktest.bcperf
O=/var/tmp/panvk/res-apk; mkdir -p $O
exec 9>>/var/tmp/panvk/device.lock
flock -w 1800 9 || { echo LOCK_TIMEOUT; exit 1; }
adb install -r "$APK" | tail -1
for t in "$@"; do
  adb shell run-as $P sh -c "'rm -f files/logs/*-$t.log'"
  adb shell am start -S -n $P/dev.zenithblue.panvktest.MainActivity --es driver bundled --es autorun $t --ez uploadDryRun true >/dev/null
  i=0
  while [ $i -lt 120 ]; do
    sleep 5; i=$((i+1))
    L=$(adb shell run-as $P sh -c "'ls files/logs/ | grep -- -$t.log | tail -1'")
    [ -n "$L" ] && adb shell run-as $P cat files/logs/$L > $O/$t.log 2>/dev/null
    grep -qE 'RESULT DONE|BC_DEVICE_FAILS|^FAIL|Abort' $O/$t.log 2>/dev/null && break
  done
  sleep 3; adb shell run-as $P cat files/logs/$L > $O/$t.log 2>/dev/null
  echo "== $t ($L)"
  if [ "$t" = bc_decode ]; then
    rm -rf $O/dump; mkdir -p $O/dump
    adb exec-out run-as $P tar cf - -C cache . | tar xf - -C $O/dump
  fi
done
adb shell am force-stop $P
