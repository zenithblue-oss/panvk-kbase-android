#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-wl
O=/var/tmp/panvk/rtl-wl; p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
while pgrep -f 'run.sh localhost' >/dev/null; do sleep 2; done
timeout 360 bash /var/tmp/panvk/b122/probe-wl.sh $S $L $A/panprobe-122h.apk rc5 $O/probe-122h $W/probe-122h.png
cp $O/probe-122h/raw/autorun.txt $W/probe-full-122h-autorun.txt; p "probe full 122h $(head -1 $O/probe-122h/summary.txt)"
cd $K; CFG=ab-v12-122h.json SC=3 TO=200 bash run.sh $S $L 116 122h > $W/s3-122h.txt 2>&1; p "s3 122h done rc=$?"
[ $(date +%s) -lt $(date -d 09:09 +%s) ] && { CFG=ab-v12-122h.json SC=5 TO=150 bash run.sh $S $L 116 122h > $W/s5-122h.txt 2>&1; p "s5 122h done rc=$?"; }
p DONE-h
