#!/bin/bash
# usage: run-x930-tessperf.sh  (tess_perf A/B on RTL SM-X930, build 118 installed)
exec 9>/var/tmp/panvk/rtl.lock; flock -w 60 9 || { echo LOCKED; exit 1; }
S=localhost:35395; A="adb -s $S"
O=/var/tmp/panvk/rtlx930-tessperf; mkdir -p $O
F=/sdcard/Android/data/dev.zenithblue.panvktest/files
log() { echo "$(date +%T) $*" >> $O/progress.txt; }
late() { [ "$(date +%H%M)" -ge 1349 ]; }
probe() { # name env
  N=$1; E=$2; D=$O/$N; mkdir -p $D
  late && { log "$N SKIP late"; return; }
  timeout 20 $A get-state > $D/state.txt 2>&1 || { log "$N OFFLINE"; exit 2; }
  T0=$(timeout 20 $A shell date '+%m-%d %H:%M:%S.000' | tr -d '\r')
  timeout 20 $A shell rm -f $F/autorun.txt
  if [ -n "$E" ]; then
    timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2 --es autorun tess_perf --es env "'$E'" --ez uploadDryRun true > $D/start.txt 2>&1
  else
    timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2 --es autorun tess_perf --ez uploadDryRun true > $D/start.txt 2>&1
  fi
  log "$N start env=[$E]"
  for i in $(seq 1 40); do
    sleep 5
    timeout 20 $A shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
    timeout 20 $A shell cat $F/autorun.txt > $D/autorun.txt 2>/dev/null
    grep -q 'AUTORUN DONE' $D/autorun.txt && break
    late && break
  done
  timeout 30 $A logcat -d -b all -t 4000 2>/dev/null | grep -iE "kbase|mali|panvk|csf|fault|DEVICE_LOST|timeout snapshot|core" | tail -200 > $D/logcat-grep.txt
  log "$N done $(grep -cE 'PASS|FAIL' $D/autorun.txt) lines"
}
pullrun() { R=$(timeout 20 $A shell ls $F/runs | tail -1 | tr -d '\r'); timeout 60 $A pull $F/runs/$R $O/$1/ >/dev/null 2>&1; }
log begin7
for k in 4 5 6 7 8; do probe s$k-sync "PANVK_DEBUG=sync"; pullrun s$k-sync; done
log ALLDONE7
