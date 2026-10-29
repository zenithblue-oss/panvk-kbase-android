#!/bin/bash
# usage: final-local.sh <name> <serial> <lock>: build 122 PanProbe cold + warm, then rc4 warm (same APK).
N=$1; S=$2; L=$3; O=/var/tmp/panvk/r122; M=/var/tmp/panvk/b122/probe.sh
bash /var/tmp/panvk/b122/dev-run.sh $N $S $L 122
bash $M $S $L - rc4 $O/$N-rc4-warm $O/$N-rc4-panprobe-warm.png
echo "$(date +%T) $N-final-DONE" >> $O/progress.txt
