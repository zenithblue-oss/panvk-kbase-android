#!/bin/bash
# TB336FU: s6 + s8 A/B, 122d with and without PANVK_GPU_STATS, 116 without; then PanProbe 122d workload_perf BC A/B.
S=HNY07QHT; L=/var/tmp/panvk/tb336fu.lock; O=/var/tmp/panvk/r122c/tbab; A=/var/tmp/panvk/build-out/apks; mkdir -p $O
export SER=$S LOCK=$L
inst() { flock $L timeout 900 adb -s $S install -r $A/$1.apk > $O/install-$1.txt 2>&1; }
pc() { bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b122/$1.json $O/$2 $2 6 8; }
inst panplay-122d; pc nostats d-nostats-1; pc wsi-auto d-stats-1
inst panplay-116c; pc nostats 116-nostats-1
inst panplay-122d; pc nostats d-nostats-2; pc wsi-auto d-stats-2
M=/var/tmp/panvk/b122/micro.sh; W=$O/bc.txt
bash $M $S $L $A/panprobe-122d.apk workload_perf $W
bash $M $S $L - workload_perf $W "PANVK_BC_PREWARM=0"
bash $M $S $L - workload_perf $W "PANVK_BC_LM_BATCH=0"
bash $M $S $L - workload_perf $W "PANVK_BC_PREWARM=0;PANVK_BC_LM_BATCH=0"
bash $M $S $L $A/panprobe-116c.apk workload_perf $W
bash $M $S $L $A/panprobe-122d.apk workload_perf $W
echo DONE > $O/done.txt
