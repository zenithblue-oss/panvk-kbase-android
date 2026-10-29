#!/bin/bash
# usage: dev-run-b.sh <name> <serial> <lock>: 121b cold + warm PanProbe full.
N=$1; S=$2; L=$3; O=/var/tmp/panvk/r121; A=/var/tmp/panvk/build-out/apks; M=/var/tmp/panvk/b121/probe.sh
CLEAR=1 bash $M $S $L $A/panprobe-121b.apk rc5 $O/$N-121b-cold $O/$N-121b-panprobe-cold.png
bash $M $S $L - rc5 $O/$N-121b-warm $O/$N-121b-panprobe-warm.png
echo "$(date +%T) $N-B-DONE" >> $O/progress.txt
