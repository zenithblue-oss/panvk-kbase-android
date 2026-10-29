#!/bin/bash
# usage: play-run.sh <serial> <lock> <tag> <outdir> <apk>: PanPlay suite 1-9 (b120/suite.json) after install -r.
S=$1; L=$2; T=$3; O=$4; APK=$5; mkdir -p $O
flock $L timeout 900 adb -s $S install -r $APK > $O/install.txt 2>&1
export SER=$S LOCK=$L
bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b120/suite.json $O $T 1 2 3 4 5 6 7 8 9
flock $L timeout 60 adb -s $S shell "dmesg 2>&1 | grep -iE 'reset|fault|timeout|panfrost|mali' | tail -40" > $O/dmesg.txt 2>&1
echo DONE > $O/done.txt
