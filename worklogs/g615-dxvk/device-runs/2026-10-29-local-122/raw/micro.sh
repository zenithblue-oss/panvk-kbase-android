#!/bin/bash
# usage: micro.sh <serial> <lock> <apk|-> <test> <outfile> [env]: one PanProbe test (rc5 slot), result lines appended to outfile.
S=$1; L=$2; APK=$3; T=$4; OUT=$5; ENV=$6
P=dev.zenithblue.panvktest; F=/sdcard/Android/data/$P/files
a() { timeout 60 adb -s $S "$@"; }
exec 9>$L; flock 9
[ "$APK" != "-" ] && timeout 300 adb -s $S install -r $APK > /dev/null 2>&1
a shell rm -f $F/autorun.txt $F/gpu_stats
a shell am force-stop $P
a shell am start -n $P/.MainActivity --es autorun $T --es driver rc5 ${ENV:+--es env "$ENV"} > /dev/null 2>&1
for i in $(seq 1 120); do
  sleep 3
  a shell input keyevent KEYCODE_WAKEUP > /dev/null 2>&1
  [ -n "$ENV" ] && a shell cat $F/gpu_stats 2>/dev/null | tr '\n' ' ' | sed 's/^/STATS /' >> $OUT.stats && echo >> $OUT.stats
  a shell cat $F/autorun.txt 2>/dev/null | grep -q 'AUTORUN DONE' && break
done
echo "== $(basename ${APK}) $T env=$ENV" >> $OUT
a shell cat $F/autorun.txt | grep 'RESULT' >> $OUT
a shell am force-stop $P
