#!/bin/sh
# Install a PanProbe APK, run autorun with a bundled driver slot, pull results. No run-as, no .so push.
# Usage: rtl-panprobe-apk.sh [-d rc4|rc5] [-p package] [-t timeout_s] [-o outdir] [-n] SERIAL APK
#   -d slot    bundled driver slot (default rc5)
#   -p package default dev.zenithblue.panvktest; per-variant APKs use dev.zenithblue.panvktest.<suffix lowercase> (e.g. .v7a)
#   -n         skip install
# Results: <outdir>/<package>-<slot>/ (default /home/flux/test-results/<date>), pulled from the app's external files dir.
PKG=dev.zenithblue.panvktest
SLOT=rc5
TIMEOUT=900
OUT=/home/flux/test-results/$(date +%Y-%m-%d)
NOINSTALL=
usage() { sed -n 2,7p "$0"; exit 2; }
while getopts d:p:t:o:nh o; do
  case $o in d) SLOT=$OPTARG ;; p) PKG=$OPTARG ;; t) TIMEOUT=$OPTARG ;; o) OUT=$OPTARG ;; n) NOINSTALL=1 ;; *) usage ;; esac
done
shift $((OPTIND - 1))
[ $# -ge 2 ] || usage
SERIAL=$1; APK=$2
A="adb -s $SERIAL"
EXT=/sdcard/Android/data/$PKG/files
$A get-state >/dev/null 2>&1 || { echo "device $SERIAL not reachable"; exit 1; }
RD=$OUT/$PKG-$SLOT; mkdir -p "$RD"
if [ -z "$NOINSTALL" ]; then
  echo "installing $APK"
  $A install -r -t "$APK" >/dev/null || { echo "install failed"; exit 1; }
fi
$A logcat -c
$A shell am start -n $PKG/dev.zenithblue.panvktest.MainActivity --es driver "$SLOT" --es autorun all --ez uploadDryRun true >/dev/null
t=0; ok=0
while [ $t -lt "$TIMEOUT" ]; do
  if $A shell "cat $EXT/autorun.txt 2>/dev/null" | grep -q 'EXTERNAL READY'; then ok=1; break; fi
  if $A logcat -d -s PanProbe | grep -q PANPROBE_RESULT; then sleep 5; ok=1; break; fi
  sleep 30; t=$((t + 30))
done
[ $ok -eq 1 ] || echo "timeout after ${TIMEOUT}s" > "$RD/TIMEOUT"
$A logcat -d > "$RD/logcat.txt" 2>&1
$A pull "$EXT/." "$RD/" >/dev/null 2>&1
grep -h 'PANPROBE_' "$RD/logcat.txt" | tr -d '\r' > "$RD/panprobe-lines.txt"
grep -h 'PANPROBE_RESULT' "$RD/panprobe-lines.txt" | tail -1
echo "results in $RD"
