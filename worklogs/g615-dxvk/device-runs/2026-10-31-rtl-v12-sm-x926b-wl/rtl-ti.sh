#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock
K=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-v14-so-bisect-kit
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-wl
p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
while pgrep -f rtl-wl3.sh >/dev/null; do sleep 3; done
cd $K; CFG=ab-v12-tilerir-122g.json SC=3 TO=300 bash run.sh $S $L 116 122g > $W/s3-tilerir.txt 2>&1; p "s3 cube tilerir done rc=$?"
bash /var/tmp/panvk/b122/bis.sh 2026-10-29-120 2026-10-29-121b
