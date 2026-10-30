#!/bin/bash
# usage: base.sh <name> <serial> <lock>: rc4 / 116 / $T baselines in one session.
# PanProbe cold+warm pairs: $T, rc4, 116, then $T, 116, rc4;
# then PanPlay 1-9 on rc4c, 116c, and $T again ($T stays installed).
N=$1; S=$2; L=$3; T=${T:-122e}; O=/var/tmp/panvk/r$T/$N; A=/var/tmp/panvk/build-out/apks; M=/var/tmp/panvk/b122/probe.sh
mkdir -p $O; P=$O/progress.txt
temp() { echo "$(date +%T) $1 $(timeout 20 adb -s $S shell dumpsys battery | grep temperature)" >> $P; }
pair() { # <tag> <apk> <slot> <round>
  temp $1-cold$4; CLEAR=1 bash $M $S $L $2 $3 $O/probe-$1-cold$4 $O/probe-$1-cold$4.png
  temp $1-warm$4; bash $M $S $L - $3 $O/probe-$1-warm$4 $O/probe-$1-warm$4.png; }
pair $T $A/panprobe-$T.apk rc5 1; pair rc4 $A/panprobe-$T.apk rc4 1
pair 116 $A/panprobe-116c.apk rc5 1
pair $T $A/panprobe-$T.apk rc5 2; pair 116 $A/panprobe-116c.apk rc5 2; pair rc4 $A/panprobe-$T.apk rc4 2
for t in rc4c 116c $T; do
  temp play-$t; bash /var/tmp/panvk/b122/play-run.sh $S $L $t $O/play-$t $A/panplay-$t.apk
done
temp DONE
