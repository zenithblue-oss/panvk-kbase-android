#!/bin/bash
# RTL v13 cube A/B. Holds rtl.lock whole run.
exec 9>/var/tmp/panvk/rtl.lock; flock -n 9 || { echo LOCKED; exit 1; }
S=localhost:35395
O=/var/tmp/panvk/rtl-cubeab/p2; mkdir -p $O/raw $O/shots
A="adb -s $S"
a() { timeout 30 $A shell "$@"; }
log() { echo "$(date +%T) $*" | tee -a $O/progress.txt; }
PL=dev.zenithblue.panvklauncher
APK=/var/tmp/panvk/build-out/apks
D=/sdcard/Android/data/$PL/files/gameloop
G='"PANVK_GPU_STATS": "/sdcard/Android/data/dev.zenithblue.panvklauncher/files/gpustats.txt"'
timeout 20 $A get-state | grep -q device || { log OFFLINE; exit 2; }

inst() { log "install $1"; timeout 600 $A install -r $APK/panplay-$1.apk > $O/install-$1-$2.txt 2>&1; log "install $1: $(tail -1 $O/install-$1-$2.txt)"; }

one() { # tag cfg scen
  tag=$1; cfg=$2; n=$3
  timeout 30 $A push $O/cfg-$cfg.json /sdcard/Download/gameloop-cubeab-$cfg.json >/dev/null 2>&1
  before=$(a "ls $D" | tr -d '\r')
  a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  { echo "== $tag s$n $(date +%T)"; a "dumpsys thermalservice 2>&1 | grep -iE 'Temperature\{|Thermal Status|mStatus' | head -8"; } >> $O/thermal.txt
  a "am start -a com.google.intent.action.TEST_LOOP -n $PL/.MainActivity --ei scenario $n --es gameloop_config /sdcard/Download/gameloop-cubeab-$cfg.json" >/dev/null 2>&1
  run=""; shot=0
  for i in $(seq 1 90); do
    sleep 5
    a input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
    if [ $shot = 0 ] && [ $i -ge 7 ] && [ ! -f $O/shots/$cfg.png ]; then timeout 30 $A exec-out screencap -p > $O/shots/$cfg.png 2>/dev/null; shot=1; fi
    cur=$(a "ls -t $D | head -1" | tr -d '\r')
    [ -n "$cur" ] && ! printf '%s\n' "$before" | grep -qx "$cur" && run=$cur
    [ -n "$run" ] && a "grep -q 'done:' $D/$run/harness.log" 2>/dev/null && break
  done
  { echo "== $tag s$n $(date +%T) post"; a "dumpsys thermalservice 2>&1 | grep -iE 'Temperature\{|Thermal Status|mStatus' | head -8"; } >> $O/thermal.txt
  [ -n "$run" ] && timeout 120 $A pull $D/$run $O/raw/$tag-s$n >/dev/null 2>&1
  res=$(grep -E '^01 ' $O/raw/$tag-s$n/results.txt 2>/dev/null | head -1 | awk '{print $5, $10}')
  log "$tag s$n run=$run fps/1%low: $res"
  echo "$tag s$n $res" >> $O/summary.txt
}

#printf '{"warmupSec":10,"measureSec":30,"screenshotAtSec":20,"env":{%s}}\n' "$G" > $O/cfg-a.json
printf '{"warmupSec":10,"measureSec":30,"screenshotAtSec":20,"env":{%s,"PANVK_WSI_SW_CACHED":"1"}}\n' "$G" > $O/cfg-d.json
cp $O/cfg-a.json $O/cfg-e.json
timeout 20 $A get-state | grep -q device || { log OFFLINE; exit 2; }
for n in 1 3; do one r1-d d $n; one r1-a a $n; done
one r2-d d 1; one r2-a a 1
inst 118 1
one r1-e e 1
inst 121b 3
log ALLDONE
