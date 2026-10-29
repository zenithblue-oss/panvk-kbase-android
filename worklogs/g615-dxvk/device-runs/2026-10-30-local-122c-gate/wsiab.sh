#!/bin/bash
# usage: wsiab.sh <serial> <lock> <apk|-> <outbase> <scen...>: PanPlay scenarios with PANVK_WSI_SW_CACHED auto / 0 / 1.
S=$1; L=$2; APK=$3; O=$4; shift 4
[ "$APK" != "-" ] && flock $L timeout 1200 adb -s $S install -r $APK > $O-install.txt 2>&1
export SER=$S LOCK=$L
for r in 1 2; do for m in auto 0 1; do
  bash /var/tmp/panvk/fps119/play-cfg.sh /var/tmp/panvk/b122/wsi-$m.json $O-$m-$r wsi$m "$@"
done; done
echo DONE > $O-done.txt
