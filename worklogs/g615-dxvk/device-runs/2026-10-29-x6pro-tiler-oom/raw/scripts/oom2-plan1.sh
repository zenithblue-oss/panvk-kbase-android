#!/bin/bash
# sequential local runs for jm-v7-wip 0160 (X6 Pro)
B=/var/tmp/panvk/oom2
N=/var/tmp/panvk/build-out/apks/panprobe-oom2b.apk
O=/var/tmp/panvk/build-out/apks/panprobe-119.apk
bash /var/tmp/panvk/oom2-batch.sh $O $B/old119-tess432 2 tess_perf 'PANVK_KBASE_HEAP_CHUNKS=4,32'
bash /var/tmp/panvk/oom2-batch.sh $N $B/b-all 1 all ''
bash /var/tmp/panvk/oom2-batch.sh $N $B/b-all432 1 all 'PANVK_KBASE_HEAP_CHUNKS=4,32'
for k in 1 2; do
  for t in tess_perf large_draw; do
    bash /var/tmp/panvk/oom2-batch.sh $N $B/perf-new-$t-$k 1 $t ''
    bash /var/tmp/panvk/oom2-batch.sh $N $B/perf-off-$t-$k 1 $t 'PANVK_KBASE_TILER_IR=0'
  done
done
echo PLAN1-DONE > $B/plan1.done
