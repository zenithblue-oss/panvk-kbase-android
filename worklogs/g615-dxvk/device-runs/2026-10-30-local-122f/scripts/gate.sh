#!/bin/bash
# usage: gate.sh <name> <serial> <lock>: rc4 / 116 / 122f in one session (b122/base-e.sh order).
# PanProbe cold+warm pairs: 122f, rc4, 116, then 122f, 116, rc4; then PanPlay 1-9 on rc4c, 116c, 122f
# with launcher and game process (EXE) meminfo per pass. Waits for battery <= 38.0 C before each run.
N=$1; S=$2; L=$3; T=122f; O=/var/tmp/panvk/r$T/$N; A=/var/tmp/panvk/build-out/apks; M=/var/tmp/panvk/b122/probe.sh
mkdir -p $O; P=$O/progress.txt
temp() {
  local c
  for i in $(seq 1 60); do
    c=$(timeout 20 adb -s $S shell dumpsys battery | grep temperature | tr -dc 0-9)
    [ -n "$c" ] && [ "$c" -le 380 ] && break
    sleep 30
  done
  echo "$(date +%T) $1 temperature:$c" >> $P; }
pair() { # <tag> <apk> <slot> <round>
  temp $1-cold$4; CLEAR=1 bash $M $S $L $2 $3 $O/probe-$1-cold$4 $O/probe-$1-cold$4.png
  temp $1-warm$4; bash $M $S $L - $3 $O/probe-$1-warm$4 $O/probe-$1-warm$4.png; }
pair $T $A/panprobe-$T.apk rc5 1; pair rc4 $A/panprobe-$T.apk rc4 1
pair 116 $A/panprobe-116c.apk rc5 1
pair $T $A/panprobe-$T.apk rc5 2; pair 116 $A/panprobe-116c.apk rc5 2; pair rc4 $A/panprobe-$T.apk rc4 2
timeout 900 adb -s $S install -r $A/panprobe-$T.apk > /dev/null 2>&1
for t in rc4c 116c $T; do
  temp play-$t; D=$O/play-$t; mkdir -p $D
  flock $L timeout 900 adb -s $S install -r $A/panplay-$t.apk > $D/install.txt 2>&1
  SER=$S LOCK=$L bash /var/tmp/panvk/b122f/play-cfg.sh /var/tmp/panvk/b120/suite.json $D $t 1 2 3 4 5 6 7 8 9 > /dev/null 2>&1
  echo DONE > $D/done.txt
done
temp DONE
