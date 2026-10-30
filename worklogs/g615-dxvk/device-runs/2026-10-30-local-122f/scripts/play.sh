#!/bin/bash
# usage: play.sh <serial> <lock> <cfg.json> <outdir> <scenario> [so-tags...]: pushes build-out/2026-10-30-<tag>/libvulkan_panfrost.so
# (or 2026-10-29-<tag>) to PanPlay files/bisect/<tag>.so, the config to files/gameloop.json (read by the launcher), runs the
# game loop scenario with fps119/play-cfg.sh, removes gameloop.json. Battery temperature is logged before the run.
S=$1; L=$2; CFG=$3; O=$4; SC=$5; shift 5
X=/sdcard/Android/data/dev.zenithblue.panvklauncher/files
mkdir -p $O
exec 9>$L; flock 9
for b in "$@"; do
  f=/var/tmp/panvk/build-out/2026-10-30-$b/libvulkan_panfrost.so; [ -f $f ] || f=/var/tmp/panvk/build-out/2026-10-29-$b/libvulkan_panfrost.so
  timeout 300 adb -s $S push $f $X/bisect/$b.so > /dev/null
done
timeout 30 adb -s $S push $CFG $X/gameloop.json > /dev/null
echo "$(date +%T) start $(timeout 20 adb -s $S shell dumpsys battery | grep temperature | tr -d ' ')" >> $O/temp.txt
flock -u 9
SER=$S LOCK=$L bash /var/tmp/panvk/b122f/play-cfg.sh $CFG $O p $SC > /dev/null 2>&1
flock $L timeout 20 adb -s $S shell rm -f $X/gameloop.json
echo "$(date +%T) end $(timeout 20 adb -s $S shell dumpsys battery | grep temperature | tr -d ' ')" >> $O/temp.txt
echo DONE > $O/done.txt
