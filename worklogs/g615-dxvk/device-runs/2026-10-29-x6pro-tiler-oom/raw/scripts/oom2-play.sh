#!/bin/bash
# PanPlay fast pass on X6 Pro under x6pro.lock. usage: oom2-play.sh <apk> <outdir> "<scenarios>"
exec 9>/var/tmp/panvk/x6pro.lock; flock -w 3600 9 || { echo LOCKED; exit 1; }
S=Y5WWBMJVOZSK4HU8
P=dev.zenithblue.panvklauncher
D=/sdcard/Android/data/$P/files/gameloop
O=$2; mkdir -p $O
a() { timeout 20 adb -s $S shell "$@"; }
log() { echo "$(date +%T) $*" >> $O/progress.txt; }
timeout 900 adb -s $S install -r $1 > $O/install.txt 2>&1 || { log INSTALL-FAIL; exit 1; }
for n in $3; do
  before=$(a "ls $D 2>/dev/null" | tr -d '\r')
  a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  log "start s$n"
  timeout 30 adb -s $S shell "am start -a com.google.intent.action.TEST_LOOP -n $P/.MainActivity --ei scenario $n --ei warmup_sec 10 --ei measure_sec 30" >> $O/progress.txt 2>&1
  dir=""; t=0
  while [ $t -lt 900 ]; do
    sleep 10; t=$((t+10))
    a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
    now=$(a "ls $D 2>/dev/null" | tr -d '\r')
    dir=$(printf '%s\n' "$now" | grep -E -- "-s$n\$" | while read x; do printf '%s\n' "$before" | grep -qx "$x" || echo $x; done | tail -1)
    if [ -n "$dir" ] && a "grep -q 'done:' $D/$dir/harness.log" 2>/dev/null; then break; fi
  done
  log "s$n dir=$dir t=$t"
  mkdir -p $O/s$n
  [ -n "$dir" ] && for f in $(a "ls $D/$dir" | tr -d '\r'); do
    sz=$(a "stat -c %s $D/$dir/$f" | tr -d '\r')
    [ "${sz:-9999999}" -lt 400000 ] && timeout 30 adb -s $S pull $D/$dir/$f $O/s$n/ >/dev/null 2>&1
  done
done
a 'dmesg | grep -iE "mali.*(oom|terminate|reset)" | tail -5' > $O/dmesg.txt
log ALLDONE
