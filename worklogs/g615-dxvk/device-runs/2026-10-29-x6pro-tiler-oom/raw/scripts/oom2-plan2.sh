#!/bin/bash
# final strict build (oom2final) on X6 Pro
B=/var/tmp/panvk/oom2
N=/var/tmp/panvk/build-out/apks/panprobe-oom2final.apk
bash /var/tmp/panvk/oom2-batch.sh $N $B/f-tess432 3 tess_perf 'PANVK_KBASE_HEAP_CHUNKS=4,32'
bash /var/tmp/panvk/oom2-batch.sh $N $B/f-all 1 all ''
bash /var/tmp/panvk/oom2-play.sh /var/tmp/panvk/build-out/apks/panplay-oom2final.apk $B/f-play "3 5 7"
echo DONE > $B/plan2.done
