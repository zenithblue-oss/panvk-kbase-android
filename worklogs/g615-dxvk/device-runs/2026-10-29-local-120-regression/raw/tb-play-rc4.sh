#!/bin/bash
# TB336FU PanPlay full suite on rc4 (panplay-rc4-b119.apk), short window.
export SER=HNY07QHT LOCK=/var/tmp/panvk/tb336fu.lock
flock $LOCK timeout 900 adb -s $SER install -r /var/tmp/panvk/build-out/apks/panplay-rc4-b119.apk > /var/tmp/panvk/r120/tb-rc4-install.txt 2>&1
bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b120/suite.json /var/tmp/panvk/r120/tb-play-rc4 tb336fu-rc4 1 2 3 4 5 6 7 8 9
echo "$(date +%T) TB-PLAY-RC4-DONE" >> /var/tmp/panvk/r120/progress.txt
