#!/bin/bash
S=localhost:50909; L=/var/tmp/panvk/rtl.lock; A=/var/tmp/panvk/build-out/apks
W=/home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-31-rtl-v12-sm-x926b-wl
O=/var/tmp/panvk/rtl-wl; mkdir -p $O
p(){ echo "$(date +%T) $*" | tee -a $W/progress.txt; }
RUNS=${RUNS:-"1:122g 2:116c 3:122g 4:116c 5:122g 6:116c"}
for r in $RUNS; do n=${r%%:*}; t=${r#*:}
  AR=workload_perf ENVX=$ENVX timeout 420 bash /var/tmp/panvk/b122/probe-wl.sh $S $L $A/panprobe-$t.apk rc5 $O/r$n-$t /dev/null
  cp $O/r$n-$t/raw/autorun.txt $W/wl-r$n-$t${TAG}.txt 2>/dev/null
  p "run $n $t ${ENVX:-noenv} $(head -1 $O/r$n-$t/summary.txt 2>/dev/null)"
done
p DONE ${TAG}
