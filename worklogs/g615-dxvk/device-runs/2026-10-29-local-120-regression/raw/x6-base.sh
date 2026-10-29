#!/bin/bash
# X6 Pro PanProbe baselines with memory: 116 x2, 118 x2, rc4 x1 (warm, after 116 runs).
O=/var/tmp/panvk/r120; mkdir -p $O; P=/var/tmp/panvk/perf119; S=Y5WWBMJVOZSK4HU8; L=/var/tmp/panvk/x6pro.lock; M=/var/tmp/panvk/b120/probe-mem.sh
bash $M $S $L $P/panprobe-wp116.apk rc5 $O/x6-116-1 $O/x6pro-116-panprobe-s2-1.png
bash $M $S $L - rc5 $O/x6-116-2 $O/x6pro-116-panprobe-s2-2.png
bash $M $S $L - rc4 $O/x6-rc4-1 $O/x6pro-rc4-panprobe-s2-1.png
bash $M $S $L $P/panprobe-wp118.apk rc5 $O/x6-118-1 $O/x6pro-118-panprobe-s2-1.png
bash $M $S $L - rc5 $O/x6-118-2 $O/x6pro-118-panprobe-s2-2.png
echo "$(date +%T) X6-BASE-DONE" >> $O/progress.txt
