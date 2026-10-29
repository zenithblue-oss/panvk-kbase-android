#!/bin/bash
# usage: dev-run.sh <name> <serial> <lock>: 120 cold, 121 cold, 121 warm (PanProbe full, cache cleared before cold runs).
N=$1; S=$2; L=$3; O=/var/tmp/panvk/r121; A=/var/tmp/panvk/build-out/apks; M=/var/tmp/panvk/b121/probe.sh
mkdir -p $O
CLEAR=1 bash $M $S $L $A/panprobe-120.apk rc5 $O/$N-120-cold $O/$N-120-panprobe-cold.png
CLEAR=1 bash $M $S $L $A/panprobe-121.apk rc5 $O/$N-121-cold $O/$N-121-panprobe-cold.png
bash $M $S $L $A/panprobe-121.apk rc5 $O/$N-121-warm $O/$N-121-panprobe-warm.png
echo "$(date +%T) $N-DONE" >> $O/progress.txt
