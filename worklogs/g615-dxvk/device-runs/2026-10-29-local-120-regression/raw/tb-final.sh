#!/bin/bash
# TB336FU final: wait for rc4 PanPlay, PanProbe 120 x2 + rc4 (memory), PanPlay full 120 (short window).
O=/var/tmp/panvk/r120; A=/var/tmp/panvk/build-out/apks; S=HNY07QHT; L=/var/tmp/panvk/tb336fu.lock; M=/var/tmp/panvk/b120/probe-mem.sh
until grep -q TB-PLAY-RC4-DONE $O/progress.txt 2>/dev/null; do sleep 20; done
bash $M $S $L $A/panprobe-120.apk rc5 $O/tb-120-1 $O/tb336fu-120-panprobe-s2-1.png
bash $M $S $L - rc5 $O/tb-120-2 $O/tb336fu-120-panprobe-s2-2.png
bash $M $S $L - rc4 $O/tb-rc4-1 $O/tb336fu-rc4-panprobe-s2-1.png
echo "$(date +%T) TB-PROBE-DONE" >> $O/progress.txt
export SER=$S LOCK=$L
flock $L timeout 900 adb -s $S install -r $A/panplay-120.apk > $O/tb-120-install.txt 2>&1
bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b120/suite.json $O/tb-play-120 tb336fu-120 1 2 3 4 5 6 7 8 9
echo "$(date +%T) TB-PLAY-120-DONE" >> $O/progress.txt
