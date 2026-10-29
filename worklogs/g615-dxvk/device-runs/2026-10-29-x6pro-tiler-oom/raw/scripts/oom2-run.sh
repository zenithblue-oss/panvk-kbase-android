#!/bin/bash
# usage: oom2-run.sh <tests> <env> <outdir> [apk]   X6 Pro PanProbe autorun under x6pro.lock
T=$1; E=$2; O=$3; APK=$4
exec 9>/var/tmp/panvk/x6pro.lock; flock -w 3600 9 || { echo LOCKED; exit 1; }
S=Y5WWBMJVOZSK4HU8; A="adb -s $S"
F=/sdcard/Android/data/dev.zenithblue.panvktest/files
mkdir -p $O
if [ -n "$APK" ]; then timeout 300 $A install -r -t $APK > $O/install.txt 2>&1 || { echo INSTALL-FAIL; exit 1; }; fi
timeout 20 $A shell rm -f $F/autorun.txt
timeout 20 $A logcat -c
timeout 20 $A shell 'dmesg | tail -1' > $O/dmesg-before.txt 2>&1
if [ -n "$E" ]; then
  timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2 --es autorun "$T" --es env "'$E'" --ez uploadDryRun true > $O/start.txt 2>&1
else
  timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2 --es autorun "$T" --ez uploadDryRun true > $O/start.txt 2>&1
fi
for i in $(seq 1 480); do
  sleep 5
  timeout 20 $A shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  timeout 20 $A shell cat $F/autorun.txt > $O/autorun.txt 2>/dev/null
  grep -q 'AUTORUN DONE' $O/autorun.txt && break
done
R=$(timeout 20 $A shell ls $F/runs | tail -1 | tr -d '\r')
timeout 120 $A pull $F/runs/$R $O/ > /dev/null 2>&1
timeout 30 $A logcat -d -s MESA PanProbe PanVKTest > $O/logcat.txt 2>&1
timeout 20 $A shell 'dmesg | grep -iE "mali|kbase|csf|heap|oom|tiler" | tail -80' > $O/dmesg.txt 2>&1
echo FINISHED > $O/done.txt
