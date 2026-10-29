#!/bin/bash
# oom2final gate on RTL SM-X930 (v13). Holds rtl.lock whole run.
exec 9>/var/tmp/panvk/rtl.lock; flock -n 9 || { echo LOCKED; exit 1; }
S=localhost:35395
O=/var/tmp/panvk/rtl-oom2final; mkdir -p $O
A="adb -s $S"
a() { timeout 20 $A shell "$@"; }
log() { echo "$(date +%T) $*" >> $O/progress.txt; }
PT=dev.zenithblue.panvktest
PL=dev.zenithblue.panvklauncher
APK=/var/tmp/panvk/build-out/apks

# health
{ timeout 20 $A get-state; a getprop ro.product.model; a getprop ro.build.version.release; a date; a "dumpsys battery | head -12"; a "pm list packages | grep zenithblue"; } > $O/health.txt 2>&1
grep -q '^device' $O/health.txt || { log OFFLINE; exit 2; }
log health-ok

# logcat stream (only process we start)
timeout 20 $A logcat -c >/dev/null 2>&1
timeout 4000 $A logcat -b all -v threadtime > $O/logcat-stream.txt 2>&1 &
LP=$!
echo $LP > $O/logcat.pid
sleep 2

# PanProbe
log install-probe
timeout 600 $A install -r -t $APK/panprobe-oom2final.apk > $O/install-probe.txt 2>&1 || { log INSTALL-PROBE-FAIL; }
F=/sdcard/Android/data/$PT/files
a rm -f $F/autorun.txt
L0=$(wc -l < $O/logcat-stream.txt)
a "am start -a com.google.intent.action.TEST_LOOP -n $PT/.MainActivity --ei scenario 2 --es autorun all --ez uploadDryRun true" > $O/probe-start.txt 2>&1
log probe-start
mkdir -p $O/probe-all; : > $O/probe-pss.txt
for i in $(seq 1 120); do
  sleep 5
  a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  a cat $F/autorun.txt > $O/probe-all/autorun.txt 2>/dev/null
  if [ $((i%3)) -eq 0 ]; then echo "$(date +%T) $(a "dumpsys meminfo $PT" | grep -E 'TOTAL PSS|TOTAL:' | head -1 | tr -s ' ')" >> $O/probe-pss.txt; fi
  grep -q 'AUTORUN DONE' $O/probe-all/autorun.txt && break
done
timeout 30 $A exec-out screencap -p > $O/probe-results.png 2>/dev/null
L1=$(wc -l < $O/logcat-stream.txt)
sed -n "$((L0+1)),${L1}p" $O/logcat-stream.txt > $O/probe-all/logcat-full.txt
R=$(a ls $F/runs | tail -1 | tr -d '\r')
timeout 120 $A pull $F/runs/$R $O/probe-all/ >/dev/null 2>&1
log probe-done $R

# PanPlay
log install-play
timeout 900 $A install -r $APK/panplay-oom2final.apk > $O/install-play.txt 2>&1 || { log INSTALL-PLAY-FAIL; kill $LP; exit 1; }
log install-play-ok
D=/sdcard/Android/data/$PL/files/gameloop
run() {
  n=$1; max=$2
  before=$(a "ls $D 2>/dev/null" | tr -d '\r')
  a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  L0=$(wc -l < $O/logcat-stream.txt)
  log "start s$n"
  timeout 30 $A shell "am start -a com.google.intent.action.TEST_LOOP -n $PL/.MainActivity --ei scenario $n --ei warmup_sec 10 --ei measure_sec 30" >> $O/progress.txt 2>&1
  dir=""; t=0
  while [ $t -lt $max ]; do
    sleep 10; t=$((t+10))
    a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
    if [ $t -eq 30 ]; then timeout 20 $A exec-out screencap -p > $O/sm-x930-oom2final-panplay-s$n.png 2>/dev/null; a dumpsys meminfo $PL > $O/meminfo-s$n.txt 2>&1; fi
    now=$(a "ls $D 2>/dev/null" | tr -d '\r')
    dir=$(printf '%s\n' "$now" | grep -E -- "-s$n\$" | while read x; do printf '%s\n' "$before" | grep -qx "$x" || echo $x; done | tail -1)
    if [ -n "$dir" ] && a "grep -q 'done:' $D/$dir/harness.log" 2>/dev/null; then break; fi
  done
  log "s$n dir=$dir t=$t"
  mkdir -p $O/s$n
  L1=$(wc -l < $O/logcat-stream.txt)
  sed -n "$((L0+1)),${L1}p" $O/logcat-stream.txt | grep -iE "kbase|mali|panvk|csf|fault|DEVICE_LOST|timeout snapshot|tiler|OOM|incremental|vkWaitForPresent|SubmissionQueue" | tail -400 > $O/s$n/logcat-grep.txt
  [ -n "$dir" ] && for f in $(a "ls $D/$dir" | tr -d '\r'); do
    sz=$(a "stat -c %s $D/$dir/$f" | tr -d '\r')
    [ "${sz:-9999999}" -lt 400000 ] && timeout 30 $A pull $D/$dir/$f $O/s$n/ >/dev/null 2>&1
  done
  log "s$n pulled"
}
for n in 3 8 5 9 1 2 4 6 7; do
  case $n in 7|8|4|9) m=900;; *) m=600;; esac
  run $n $m
done
kill $LP 2>/dev/null
log ALLDONE
