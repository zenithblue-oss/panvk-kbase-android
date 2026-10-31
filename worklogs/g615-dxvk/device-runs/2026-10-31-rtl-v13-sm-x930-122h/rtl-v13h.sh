#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks; END=$(date -d 09:54 +%s)
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v13-sm-x930-122h
O=/var/tmp/panvk/rtl-v13h; mkdir -p $O; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
left(){ echo $(( END - $(date +%s) )); }
p "start model=$(timeout 10 adb -s $S shell getprop ro.product.model | tr -d '\r')"
flock $L timeout 20 adb -s $S shell "rm -rf /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect-*.so /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect /sdcard/Download/gameloop-*.json"; p "pre-cleanup rc=$?"
timeout 420 bash /var/tmp/panvk/b122/probe.sh $S $L $A/panprobe-122h.apk rc5 $O/probe $W/probe-122h.png
cp $O/probe/raw/autorun.txt $W/probe-full-122h-autorun.txt; cp $O/probe/mem.txt $W/probe-122h-mem.txt
p "probe full 122h $(head -1 $O/probe/summary.txt) tess=$(grep -o 'RESULT tess_perf [A-Z]*' $W/probe-full-122h-autorun.txt)"
flock $L timeout 300 adb -s $S install -r $A/panplay-122h.apk > $W/play-install.txt 2>&1; p "panplay-122h install $(tail -1 $W/play-install.txt)"
cd $K
run(){ local cfg=$1 sc=$2 to=$3 tag=$4; shift 4
  [ $(left) -lt $to ] && to=$(left); [ $to -lt 60 ] && { p "skip $tag (time)"; return; }
  CFG=$cfg SC=$sc TO=$to bash run.sh $S $L "$@" > $W/$tag.txt 2>&1; rc=$?
  d=$(ls -td $K/out-* | head -1); mkdir -p $W/$tag; cp $d/progress.txt $d/summary.txt $W/$tag/ 2>/dev/null; cp $d/live/results.txt $W/$tag/live-results.txt 2>/dev/null
  p "$tag rc=$rc $(grep -a 'A/B' $d/live/results.txt | head -1)"; }
run ab-v13-cube-122h.json 3 300 s3-cube 116 122h
run ab-v13-s578-122h.json 5 150 s5-122h 122h
run ab-v13-s578-122h.json 7 420 s7-122h 122h
run ab-v13-s578-122h.json 8 480 s8-122h 122h
run ab-v13-s578-122h.json 4 150 s5-116 116
timeout 20 adb -s $S shell am force-stop dev.zenithblue.panvklauncher
p DONE
