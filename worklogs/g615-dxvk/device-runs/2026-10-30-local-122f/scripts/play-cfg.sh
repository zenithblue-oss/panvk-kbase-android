#!/bin/bash
# PanPlay game loop on the X6 Pro with a given override config; screenshot + dumpsys meminfo per pass.
# usage: play-cfg.sh <config.json> <outdir> <tag> <scenario...>
S=${SER:-Y5WWBMJVOZSK4HU8}; LOCK=${LOCK:-/var/tmp/panvk/x6pro.lock}; CFG=$1; O=$2; TAG=$3; shift 3
A="adb -s $S"; P=dev.zenithblue.panvklauncher
X=/sdcard/Android/data/$P/files; G=$X/gameloop
mkdir -p $O/raw $O/shots $O/mem
exec 9>$LOCK
flock 9
timeout 30 $A push $CFG /sdcard/Download/gameloop-$TAG.json > /dev/null 2>&1
for N in "$@"; do
  before=$(timeout 20 $A shell ls -t $G | head -1 | tr -d '\r')
  timeout 20 $A logcat -c
  timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n $P/.MainActivity --ei scenario $N --es gameloop_config /sdcard/Download/gameloop-$TAG.json > /dev/null 2>&1
  run=""; lastpid=""; first=0; k=0
  for i in $(seq 1 360); do
    sleep 5
    timeout 20 $A shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
    pid=$(timeout 20 $A shell "ps -A -o PID,NAME" | grep -E '\.exe' | grep -viE 'services|explorer|winedevice|rpcss|plugplay|svchost|conhost|start\.exe|tabtip|wineboot|rundll|winemenubuilder' | tail -1 | awk '{print $1}')
    if [ -n "$pid" ] && [ "$pid" != "$lastpid" ]; then lastpid=$pid; first=$i; fi
    if [ -n "$pid" ] && [ $first -gt 0 ] && [ $((i-first)) -ge 4 ]; then
      k=$((k+1)); sfx=""; [ $k -gt 1 ] && sfx="-$k"
      timeout 30 $A exec-out screencap -p > $O/shots/$TAG-panplay-s$N$sfx.png
      { echo "pass $k pid $pid"; timeout 20 $A shell "ps -A -o PID,NAME | grep -w $pid"; timeout 30 $A shell dumpsys meminfo $P | grep -E 'TOTAL PSS|TOTAL RSS|Graphics|GL mtrack|EGL mtrack'; echo "gpustats: $(timeout 20 $A shell cat $X/gpustats.txt 2>/dev/null | tr '\n' ' ')"; echo "EXE $pid"; timeout 30 $A shell dumpsys meminfo $pid | grep -E 'TOTAL PSS|TOTAL RSS|Graphics|GL mtrack|EGL mtrack|Native Heap|Unknown|Other dev|\.so mmap'; } > $O/mem/s$N$sfx.txt 2>&1
      first=0
    fi
    cur=$(timeout 20 $A shell ls -t $G | head -1 | tr -d '\r')
    [ -n "$cur" ] && [ "$cur" != "$before" ] && run=$cur
    [ -n "$run" ] && timeout 20 $A shell grep -q 'done:' $G/$run/harness.log 2>/dev/null && break
  done
  sleep 3
  timeout 60 $A logcat -d > $O/raw/logcat-s$N.txt 2>&1
  [ -n "$run" ] && timeout 300 $A pull $G/$run $O/raw/ > /dev/null 2>&1
  echo "s$N run=$run"; [ -n "$run" ] && grep -E '^[0-9]{2} ' $O/raw/$run/results.txt
  [ -n "$run" ] && grep -h -o '"deviceLost": *[a-z]*' -r $O/raw/$run | sort | uniq -c
done | tee $O/summary.txt
flock -u 9
