#!/bin/bash
# X6 Pro warm PanProbe reruns, installing our APK each time (another agent installs its own PanProbe between runs).
O=/var/tmp/panvk/r120; A=/var/tmp/panvk/build-out/apks; P=/var/tmp/panvk/perf119; S=Y5WWBMJVOZSK4HU8; L=/var/tmp/panvk/x6pro.lock; M=/var/tmp/panvk/b120/probe-mem.sh
until grep -q X6-PLAY-120-DONE $O/progress.txt 2>/dev/null; do sleep 20; done
bash $M $S $L $P/panprobe-wp116.apk rc5 $O/x6-116-w $O/x6pro-116-panprobe-s2-w.png
bash $M $S $L $P/panprobe-wp118.apk rc5 $O/x6-118-w $O/x6pro-118-panprobe-s2-w.png
bash $M $S $L $A/panprobe-120.apk rc5 $O/x6-120-w $O/x6pro-120-panprobe-s2-w.png
bash $M $S $L $A/panprobe-120.apk rc4 $O/x6-rc4-w $O/x6pro-rc4-panprobe-s2-w.png
echo "$(date +%T) X6-WARM-DONE" >> $O/progress.txt
