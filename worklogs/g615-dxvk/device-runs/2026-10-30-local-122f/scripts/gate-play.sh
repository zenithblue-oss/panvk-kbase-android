#!/bin/bash
# usage: gate-play.sh <name> <serial> <lock> [tags...]: PanPlay 1-9 (b120/suite.json) on rc4c, 116c, 122f
# (default order) after install -r of panplay-<tag>.apk, with launcher and game process (EXE) meminfo per pass.
# Waits for battery <= 38.0 C before each tag.
N=$1; S=$2; L=$3; shift 3; [ $# -gt 0 ] || set -- rc4c 116c 122f
O=/var/tmp/panvk/r122f/$N; A=/var/tmp/panvk/build-out/apks; mkdir -p $O; P=$O/progress.txt
temp() {
  local c
  for i in $(seq 1 60); do
    c=$(timeout 20 adb -s $S shell dumpsys battery | grep temperature | tr -dc 0-9)
    [ -n "$c" ] && [ "$c" -le 380 ] && break
    sleep 30
  done
  echo "$(date +%T) $1 temperature:$c" >> $P; }
for tag in "$@"; do
  temp play-$tag; D=$O/play-$tag; mkdir -p $D
  flock $L timeout 900 adb -s $S install -r $A/panplay-$tag.apk > $D/install.txt 2>&1
  SER=$S LOCK=$L bash /var/tmp/panvk/b122f/play-cfg.sh /var/tmp/panvk/b120/suite.json $D $tag 1 2 3 4 5 6 7 8 9 > /dev/null 2>&1
  echo DONE > $D/done.txt
done
temp DONE
