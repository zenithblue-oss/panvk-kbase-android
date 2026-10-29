#!/bin/bash
# usage: dev-run.sh <name> <serial> <lock> <tag>: PanProbe full cold (shader cache cleared) + warm for panprobe-<tag>.apk.
N=$1; S=$2; L=$3; T=$4; O=/var/tmp/panvk/r122; A=/var/tmp/panvk/build-out/apks; M=/var/tmp/panvk/b122/probe.sh
mkdir -p $O
CLEAR=1 bash $M $S $L $A/panprobe-$T.apk rc5 $O/$N-$T-cold $O/$N-$T-panprobe-cold.png
bash $M $S $L - rc5 $O/$N-$T-warm $O/$N-$T-panprobe-warm.png
echo "$(date +%T) $N-$T-DONE" >> $O/progress.txt
