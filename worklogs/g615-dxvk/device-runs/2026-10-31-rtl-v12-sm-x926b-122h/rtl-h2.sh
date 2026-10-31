#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-122h
O=/var/tmp/panvk/rtl-h; mkdir -p $O; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
p start
timeout 420 bash /var/tmp/panvk/b122/probe.sh $S $L $A/panprobe-122h.apk rc5 $O/probe $W/probe-122h.png
cp $O/probe/raw/autorun.txt $W/probe-full-122h-autorun.txt; cp $O/probe/mem.txt $W/probe-122h-mem.txt
p "probe full 122h $(head -1 $O/probe/summary.txt) tess=$(grep -o 'RESULT tess_perf [A-Z]*' $W/probe-full-122h-autorun.txt)"
cd $K
CFG=ab-v12-122h.json SC=3 TO=240 bash run.sh $S $L 116 122h > $W/s3-122h-116.txt 2>&1; p "s3 122h vs 116 rc=$?"
CFG=ab-v12-122h.json SC=5 TO=200 bash run.sh $S $L 116 122h > $W/s5-122h-116.txt 2>&1; p "s5 122h vs 116 rc=$?"
CFG=ab-v12-s8-122h.json SC=8 TO=780 bash run.sh $S $L 122h > $W/s8-122h.txt 2>&1; p "s8 122h rc=$?"
p DONE
