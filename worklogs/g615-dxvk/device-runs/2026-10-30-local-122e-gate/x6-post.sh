#!/bin/zsh
# X6 Pro after the 122e gate: xfb_vs tiler IR A/B (0167), cached FL A/B, FO4 sound smoke.
S=Y5WWBMJVOZSK4HU8; L=/var/tmp/panvk/x6pro.lock; O=/var/tmp/panvk/r122e/x6pro; B=/var/tmp/panvk/b122
P=$O/post-progress.txt
t() { echo "$(date +%T) $1 $(timeout 10 adb -s $S shell dumpsys battery | grep temperature)" >> $P; }
for r in 1 2; do
  t ir-default-$r; bash $B/micro.sh $S $L - workload_perf $O/ir-ab.txt "PANVK_AB_DUMMY=0"
  t ir-0-$r; bash $B/micro.sh $S $L - workload_perf $O/ir-ab.txt "PANVK_KBASE_TILER_IR=0"
done
t wsiab; bash $B/wsiab.sh $S $L - $O/wsiab 8 1
t fo4; sed 's/192.168.1.35:42767/Y5WWBMJVOZSK4HU8/g' $B/fo4smoke.sh > $B/fo4smoke-usb.sh
zsh $B/fo4smoke-usb.sh $O/fo4smoke > $O/fo4smoke.txt 2>&1
t DONE
