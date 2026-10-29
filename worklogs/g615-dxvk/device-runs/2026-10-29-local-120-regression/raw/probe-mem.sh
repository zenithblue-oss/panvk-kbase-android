#!/bin/bash
# Full PanProbe (game loop scenario 2) for one driver slot under the device lock, with dumpsys meminfo every 5 s.
# usage: probe-mem.sh <serial> <lock> <apk|-> <rc4|rc5> <outdir> <shot.png>
S=$1; L=$2; APK=$3; DRV=$4; O=$5; SHOT=$6
A="adb -s $S"; P=dev.zenithblue.panvktest
F=/sdcard/Android/data/$P/files
mkdir -p $O/raw
exec 9>$L
flock 9
[ "$APK" != "-" ] && timeout 300 $A install -r $APK > $O/install.txt 2>&1
timeout 20 $A shell rm -f $F/autorun.txt
timeout 20 $A logcat -c 2>/dev/null
timeout 20 $A shell am start -a com.google.intent.action.TEST_LOOP -n $P/.MainActivity --ei scenario 2 --es driver $DRV --ez uploadDryRun true > /dev/null 2>&1
: > $O/mem.txt
for i in $(seq 1 300); do
  sleep 5
  timeout 20 $A shell input keyevent KEYCODE_WAKEUP >/dev/null 2>&1
  timeout 30 $A shell dumpsys meminfo $P | grep -E 'TOTAL PSS|Graphics:|GL mtrack|EGL mtrack' | tr -s ' ' | tr '\n' '|' >> $O/mem.txt; echo >> $O/mem.txt
  timeout 20 $A shell cat $F/autorun.txt > $O/raw/autorun.txt 2>/dev/null
  grep -q 'AUTORUN DONE' $O/raw/autorun.txt && break
done
sleep 3
timeout 30 $A exec-out screencap -p > $SHOT
timeout 30 $A logcat -d > $O/raw/logcat.txt 2>&1
R=$(timeout 20 $A shell ls -t $F/runs | head -1 | tr -d '\r')
[ -n "$R" ] && timeout 120 $A pull $F/runs/$R $O/raw/ > /dev/null 2>&1
flock -u 9
echo "drv=$(grep -m1 -o 'driverInfo=.*' $O/raw/autorun.txt) pass=$(grep -c 'RESULT [a-z0-9_]* PASS' $O/raw/autorun.txt) fail=$(grep -c 'RESULT [a-z0-9_]* FAIL' $O/raw/autorun.txt) lost=$(grep -ci 'DEVICE_LOST' $O/raw/logcat.txt) done=$(grep -c 'AUTORUN DONE' $O/raw/autorun.txt)" | tee $O/summary.txt
