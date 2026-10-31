#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks; END=$(date -d 11:25 +%s)
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v14-sm-x940-122i
O=/var/tmp/panvk/rtl-v14-122i; mkdir -p $O; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
left(){ echo $(( END - $(date +%s) )); }
p "start model=$(timeout 10 adb -s $S shell getprop ro.product.model | tr -d '\r')"
flock $L timeout 20 adb -s $S shell "rm -rf /sdcard/Android/data/dev.zenithblue.panvklauncher/files/gameloop.json /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect-*.so /sdcard/Android/data/dev.zenithblue.panvklauncher/files/bisect /sdcard/Download/gameloop-*.json"; p "pre-cleanup rc=$?"
for t in 122i 116c; do
  timeout 420 bash /var/tmp/panvk/b122/probe.sh $S $L $A/panprobe-$t.apk rc5 $O/probe-$t $W/probe-$t.png
  cp $O/probe-$t/raw/autorun.txt $W/probe-full-$t-autorun.txt; cp $O/probe-$t/mem.txt $W/probe-$t-mem.txt
  p "probe full $t $(head -1 $O/probe-$t/summary.txt) tess=$(grep -o 'RESULT tess_perf [A-Z]*' $W/probe-full-$t-autorun.txt)"
done
flock $L timeout 600 adb -s $S install -r $A/panplay-122i.apk > $W/play-install.txt 2>&1; p "panplay-122i install $(tail -1 $W/play-install.txt)"
cd $K
run(){ local cfg=$1 sc=$2 to=$3 tag=$4; shift 4
  [ $(left) -lt $to ] && to=$(left); [ $to -lt 60 ] && { p "skip $tag (time)"; return; }
  pre=$(ls -d $K/out-*)
  CFG=$cfg SC=$sc TO=$to bash run.sh $S $L "$@" > $W/$tag.txt 2>&1; rc=$?
  d=$(comm -13 <(echo "$pre") <(ls -d $K/out-*) | tail -1); mkdir -p $W/$tag; cp $d/progress.txt $d/summary.txt $W/$tag/ 2>/dev/null; cp $d/live/results.txt $W/$tag/live-results.txt 2>/dev/null
  p "$tag rc=$rc $(grep -a 'A/B' $d/live/results.txt | head -1)"; }
run ab-v14-cube-122i.json 3 300 s3-cube 116 122i
run ab-v14-all-122i.json 5 200 s5-122i 122i
run ab-v14-all-122i.json 6 200 s6-122i 122i
run ab-v14-all-122i.json 7 600 s7-122i 122i
run ab-v14-all-122i.json 8 700 s8-122i 122i
run ab-v14-all-122i.json 9 300 s9-122i 122i
run ab-v14-all-122i.json 15 200 s5-116 116
run ab-v14-all-122i.json 16 200 s6-116 116
run ab-v14-all-122i.json 18 700 s8-116 116
run ab-v14-all-122i.json 20 500 s8fl100-rep 116 122i
timeout 20 adb -s $S shell am force-stop dev.zenithblue.panvklauncher
p DONE
