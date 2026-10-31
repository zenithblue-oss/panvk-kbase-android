#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks; END=$(date -d 10:45 +%s)
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v13-sm-x930-rep
O=/var/tmp/panvk/rtl-v13-rep; mkdir -p $O; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
left(){ echo $(( END - $(date +%s) )); }
p "start model=$(timeout 10 adb -s $S shell getprop ro.product.model | tr -d '\r')"
flock $L timeout 20 adb -s $S shell "rm -rf /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect-*.so /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect /sdcard/Download/gameloop-*.json"; p "pre-cleanup rc=$?"
cd $K
run(){ local cfg=$1 sc=$2 to=$3 tag=$4; shift 4
  [ $(left) -lt $to ] && to=$(left); [ $to -lt 60 ] && { p "skip $tag (time)"; return; }
  pre=$(ls -d $K/out-*)
  CFG=$cfg SC=$sc TO=$to bash run.sh $S $L "$@" > $W/$tag.txt 2>&1; rc=$?
  d=$(comm -13 <(echo "$pre") <(ls -d $K/out-*) | tail -1); mkdir -p $W/$tag; cp $d/progress.txt $d/summary.txt $W/$tag/ 2>/dev/null; cp $d/live/results.txt $W/$tag/live-results.txt 2>/dev/null
  p "$tag rc=$rc $(grep -a 'A/B' $d/live/results.txt | head -1)"; }
run ab-v13-rep-122h-116.json 6 360 s6-gears-x2 116 122h
run ab-v13-rep-122h-116.json 8 600 s8-fl-x3 116 122h
timeout 20 adb -s $S shell am force-stop dev.zenithblue.panvklauncher
p DONE
