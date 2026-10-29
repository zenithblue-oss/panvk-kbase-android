#!/bin/bash
# X6 Pro final: wait for baselines, PanProbe 120 x2 + rc4 (memory), PanPlay full rc4 then 120 (short window).
O=/var/tmp/panvk/r120; A=/var/tmp/panvk/build-out/apks; S=Y5WWBMJVOZSK4HU8; L=/var/tmp/panvk/x6pro.lock; M=/var/tmp/panvk/b120/probe-mem.sh
until grep -q X6-BASE-DONE $O/progress.txt 2>/dev/null; do sleep 20; done
bash $M $S $L $A/panprobe-120.apk rc5 $O/x6-120-1 $O/x6pro-120-panprobe-s2-1.png
bash $M $S $L - rc5 $O/x6-120-2 $O/x6pro-120-panprobe-s2-2.png
bash $M $S $L - rc4 $O/x6-rc4-2 $O/x6pro-rc4-panprobe-s2-2.png
echo "$(date +%T) X6-PROBE-DONE" >> $O/progress.txt
flock $L timeout 900 adb -s $S install -r $A/panplay-rc4-b119.apk > $O/x6-rc4-install.txt 2>&1
bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b120/suite.json $O/x6-play-rc4 x6pro-rc4 1 2 3 4 5 6 7 8 9
echo "$(date +%T) X6-PLAY-RC4-DONE" >> $O/progress.txt
flock $L timeout 900 adb -s $S install -r $A/panplay-120.apk > $O/x6-120-install.txt 2>&1
bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b120/suite.json $O/x6-play-120 x6pro-120 1 2 3 4 5 6 7 8 9
echo "$(date +%T) X6-PLAY-120-DONE" >> $O/progress.txt
