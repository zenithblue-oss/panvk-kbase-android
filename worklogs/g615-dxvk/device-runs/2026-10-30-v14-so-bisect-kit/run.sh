#!/bin/bash
# usage: run.sh <serial> <lock> [builds...]: pushes the .so files (each ~29 MB) to PanPlay's external files dir,
# then runs gameloop scenario 3 with bisect.json (one 40 s D3D11 cube pass per build, about 1.5 min each).
# Needs the PanPlay debug APK already installed (122c or later; no reinstall). Results: <out>/raw/*/results.txt.
S=$1; L=$2; shift 2; B=${@:-116 117 118 119 120 121b 122c}
K=$(dirname $(readlink -f $0)); O=$K/out-$(date +%H%M); mkdir -p $O
X=/sdcard/Android/data/dev.zenithblue.panvklauncher/files
exec 9>$L; flock 9
for b in $B; do timeout 300 adb -s $S push /var/tmp/panvk/build-out/2026-10-29-$b/libvulkan_panfrost.so $X/bisect/$b.so; done
flock -u 9
SER=$S LOCK=$L bash /var/tmp/panvk/fps119/play-cfg.sh $K/bisect.json $O bisect 3
grep -E '^ *0[1-9] ' $O/raw/*/results.txt
