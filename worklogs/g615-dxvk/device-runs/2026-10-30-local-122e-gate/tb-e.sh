#!/bin/bash
S=192.168.1.39:45553; L=/var/tmp/panvk/tb336fu.lock; A=/var/tmp/panvk/build-out/apks; O=/var/tmp/panvk/r122e/tb336fu; mkdir -p $O; P=$O/progress.txt
temp() { echo "$(date +%T) $1 $(timeout 20 adb -s $S shell dumpsys battery | grep temperature)" >> $P; }
M=/var/tmp/panvk/b122/micro.sh; W=$O/bc.txt
temp bc; bash $M $S $L $A/panprobe-122e.apk workload_perf $W
for e in "PANVK_BC_PREWARM=0" "PANVK_BC_LM_BATCH=0" "PANVK_BC_PREWARM=0;PANVK_BC_LM_BATCH=0" ""; do temp "bc $e"; bash $M $S $L - workload_perf $W "$e"; done
temp bc116; bash $M $S $L $A/panprobe-116c.apk workload_perf $W
bash $M $S $L $A/panprobe-122e.apk workload_perf $W "PANVK_BC_PREWARM=0"
bash $M $S $L - workload_perf $W
for t in 122e rc4c 116c 122e; do
  temp play-$t; bash /var/tmp/panvk/b122/play-run.sh $S $L $t $O/play-$t-$(date +%H%M) $A/panplay-$t.apk
done
temp DONE
