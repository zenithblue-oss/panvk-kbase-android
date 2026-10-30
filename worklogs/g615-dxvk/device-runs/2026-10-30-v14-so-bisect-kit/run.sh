#!/bin/bash
# usage: [CFG=bisect-v14t.json SC=3 TO=<s>] run.sh <serial> <lock> [builds...]
# 1. Cleanup first: removes leftover gameloop.json, bisect-*.so, bisect/ and /sdcard/Download/gameloop-*.json.
# 2. Pushes each build's .so to the path the config's installDrivers names (files dir root, bisect-<b>.so, for new configs;
#    on a fresh install the app cannot read a shell-created bisect/ dir) and the config to files/gameloop.json
#    (Android 16 cannot read /sdcard/Download).
# 3. Runs play-cfg.sh under `timeout TO` (default 60 s per pass + 300 s). On timeout: am force-stop of PanPlay only.
# 4. While it runs, copies the device run's results.json/results.txt/harness.log to <out>/live/ every 20 s,
#    so finished passes survive a hang or a lost session.
# 5. Cleanup again at the end. Results: <out>/raw/*/results.txt (full) or <out>/live/ (partial).
S=$1; L=$2; CFG=${CFG:-bisect-v14t.json}; SC=${SC:-3}; shift 2; B=${@:-116 122e}
K=$(dirname $(readlink -f $0)); O=$K/out-$(date +%H%M); mkdir -p $O/live
P=dev.zenithblue.panvklauncher; X=/sdcard/Android/data/$P/files; G=$X/gameloop
a() { timeout ${T:-10} adb -s $S "$@"; }
clean() { flock $L timeout 20 adb -s $S shell "rm -rf $X/gameloop.json $X/bisect-*.so $X/bisect /sdcard/Download/gameloop-*.json"; echo "cleanup rc=$?" >> $O/progress.txt; }
clean
exec 9>$L; flock 9
for b in $B; do
  d=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['installDrivers'].get('bisect-'+sys.argv[2],''))" $K/$CFG $b)
  [ -n "$d" ] && T=300 a push /var/tmp/panvk/build-out/2026-10-29-$b/libvulkan_panfrost.so $X/$d
done
T=30 a push $K/$CFG $X/gameloop.json
before=$(a shell ls -t $G 2>/dev/null | head -1 | tr -d '\r')
flock -u 9
n=$(python3 -c "import json,sys; c=json.load(open(sys.argv[1])); print(sum(len(s['envSets'])*len(s['tasks']) for k,s in c['scenarios'].items() if k==sys.argv[2]))" $K/$CFG $SC)
TO=${TO:-$(( n * 60 + 300 ))}
echo "$(date +%T) start cfg=$CFG sc=$SC passes=$n timeout=${TO}s" >> $O/progress.txt
SER=$S LOCK=$L timeout $TO bash /var/tmp/panvk/fps119/play-cfg.sh $K/$CFG $O bisect $SC > $O/play-cfg.txt 2>&1 &
PC=$!
while kill -0 $PC 2>/dev/null; do
  sleep 20
  run=$(a shell ls -t $G 2>/dev/null | head -1 | tr -d '\r')
  [ -n "$run" ] && [ "$run" != "$before" ] && for f in results.json results.txt harness.log; do a exec-out cat $G/$run/$f > $O/live/$f.tmp 2>/dev/null && [ -s $O/live/$f.tmp ] && mv $O/live/$f.tmp $O/live/$f; done
done
wait $PC; rc=$?
if [ $rc = 124 ]; then
  echo "$(date +%T) TIMEOUT after ${TO}s, force-stop $P" >> $O/progress.txt
  pkill -f "play-cfg.sh $K/$CFG $O" 2>/dev/null
  flock $L timeout 10 adb -s $S shell am force-stop $P
fi
echo "$(date +%T) play-cfg rc=$rc" >> $O/progress.txt
clean
grep -hE '^ *[0-9][0-9] ' $O/raw/*/results.txt 2>/dev/null || grep -hE 'pass [0-9]+:' $O/live/harness.log
