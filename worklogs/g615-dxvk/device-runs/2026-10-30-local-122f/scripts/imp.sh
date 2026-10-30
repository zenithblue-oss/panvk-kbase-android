#!/bin/bash
# usage: imp.sh <serial> <lock> <so-on-device> <test> <outfile> [env] : one PanProbe test with the IMPORTED driver slot
# (files/imported/libimported.so, copied from <so-on-device> with run-as). Result lines appended to outfile.
# CLEAR=1 removes PanProbe's mesa_shader_cache first (cold).
S=$1; L=$2; SO=$3; T=$4; OUT=$5; ENV=$6
P=dev.zenithblue.panvktest; F=/sdcard/Android/data/$P/files
a() { timeout 60 adb -s $S "$@"; }
exec 9>$L; flock 9
a shell "run-as $P sh -c 'cp $SO files/imported/libimported.so'"
[ "$CLEAR" = 1 ] && a shell "run-as $P rm -rf cache/mesa_shader_cache"
a shell rm -f $F/autorun.txt
a shell am force-stop $P
a shell am start -n $P/.MainActivity --es autorun $T --es driver imported ${ENV:+--es env "$ENV"} > /dev/null 2>&1
for i in $(seq 1 200); do
  sleep 3
  a shell input keyevent KEYCODE_WAKEUP > /dev/null 2>&1
  a shell cat $F/autorun.txt 2>/dev/null | grep -q 'AUTORUN DONE' && break
done
echo "== $(basename $SO) $T env=$ENV clear=$CLEAR $(a shell dumpsys battery | grep temperature | tr -d ' ')" >> $OUT
a shell cat $F/autorun.txt | grep -E 'RESULT|BENCH' >> $OUT
a shell am force-stop $P
