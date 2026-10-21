#!/bin/sh
# Run PanProbe "run all" on an adb device once per driver variant (imported driver).
# Usage: rtl-v7-panprobe.sh [-a PanProbe.apk] [-n] [-t timeout_s] [-o outdir] SERIAL [VARIANT_DIR ...]
# Each VARIANT_DIR must hold libvulkan_panfrost.so. Missing dirs are skipped.
# -n: skip APK install (already installed).
# Default variants: /home/flux/build-out/2026-10-21-v7{,A,B,C,D,E,F}
# Results: <outdir>/<variant>/ (default /home/flux/test-results/<date>)
PKG=dev.zenithblue.panvktest
APK=/home/flux/build-out/apks/panprobe-debug.apk
TIMEOUT=900
DATE=$(date +%Y-%m-%d)
OUT=/home/flux/test-results/$DATE
BASE=/home/flux/build-out/2026-10-21-v7

usage() { sed -n 2,6p "$0"; exit 2; }

NOINSTALL=
while getopts a:t:o:nh o; do
  case $o in
    a) APK=$OPTARG ;; n) NOINSTALL=1 ;; t) TIMEOUT=$OPTARG ;; o) OUT=$OPTARG ;; *) usage ;;
  esac
done
shift $((OPTIND - 1))
[ $# -ge 1 ] || usage
SERIAL=$1; shift
if [ $# -eq 0 ]; then set -- "$BASE" "${BASE}A" "${BASE}B" "${BASE}C" "${BASE}D" "${BASE}E" "${BASE}F"; fi

if [ -n "$DRYRUN" ]; then
  echo "serial=$SERIAL apk=$APK timeout=$TIMEOUT out=$OUT"
  for d in "$@"; do
    if [ -f "$d/libvulkan_panfrost.so" ]; then echo "variant $(basename "$d"): ok"; else echo "variant $(basename "$d"): MISSING (skip)"; fi
  done
  exit 0
fi

A="adb -s $SERIAL"
$A get-state >/dev/null 2>&1 || { echo "device $SERIAL not reachable"; exit 1; }
mkdir -p "$OUT"

# Device facts
{
  echo "model=$($A shell getprop ro.product.model | tr -d '\r')"
  echo "hardware=$($A shell getprop ro.hardware | tr -d '\r')"
  echo "platform=$($A shell getprop ro.board.platform | tr -d '\r')"
  echo "kernel=$($A shell uname -a | tr -d '\r')"
  echo "gpu_props=$($A shell getprop | grep -i 'gpu\|vulkan' | tr -d '\r' | tr '\n' ';')"
  echo "--- dumpsys SurfaceFlinger GLES"
  $A shell dumpsys SurfaceFlinger 2>/dev/null | grep -i 'GLES\|vulkan' | head -5
  echo "--- vulkaninfo"
  $A shell 'command -v vulkaninfo >/dev/null && vulkaninfo --summary 2>&1 | head -30 || echo "vulkaninfo not available"'
} > "$OUT/device.txt" 2>&1
cat "$OUT/device.txt" | head -5

if [ -z "$NOINSTALL" ]; then
  echo "installing $APK"
  $A install -r -t "$APK" >/dev/null || { echo "install failed"; exit 1; }
fi

SUMMARY="$OUT/summary.txt"
: > "$SUMMARY"
printf '%-22s %5s %5s %5s  %s\n' VARIANT PASS FAIL SKIP FIRST_ERRORS | tee -a "$SUMMARY"

for d in "$@"; do
  v=$(basename "$d"); so=$d/libvulkan_panfrost.so
  if [ ! -f "$so" ]; then echo "skip $v (missing $so)"; continue; fi
  rd=$OUT/$v; mkdir -p "$rd"
  echo "== $v"
  $A shell am force-stop $PKG
  $A logcat -c
  $A push "$so" /data/local/tmp/libimported.so >/dev/null || continue
  $A shell "run-as $PKG sh -c 'mkdir -p files/imported && rm -rf files/runs files/autorun.txt'"
  $A shell "run-as $PKG cp /data/local/tmp/libimported.so files/imported/libimported.so" || { echo "run-as cp failed"; continue; }
  $A shell "run-as $PKG chmod 755 files/imported/libimported.so"
  $A shell am start -n $PKG/.MainActivity --es driver imported --es autorun all --ez uploadDryRun true >/dev/null
  t=0; done_=0
  while [ $t -lt "$TIMEOUT" ]; do
    if $A shell "run-as $PKG cat files/autorun.txt 2>/dev/null" | grep -q 'AUTORUN DONE'; then done_=1; break; fi
    sleep 10; t=$((t + 10))
  done
  [ $done_ -eq 1 ] || echo "timeout after ${TIMEOUT}s" > "$rd/TIMEOUT"
  $A shell "run-as $PKG cat files/autorun.txt" | tr -d '\r' > "$rd/autorun.txt"
  $A logcat -d > "$rd/logcat.txt" 2>&1
  $A shell "run-as $PKG tar -C files -cf - runs" > "$rd/runs.tar" 2>/dev/null
  (cd "$rd" && tar -xf runs.tar 2>/dev/null)
  line=$(grep 'AUTORUN DONE' "$rd/autorun.txt" | tail -1)
  p=$(echo "$line" | sed -n 's/.*pass=\([0-9]*\).*/\1/p'); f=$(echo "$line" | sed -n 's/.*fail=\([0-9]*\).*/\1/p'); s=$(echo "$line" | sed -n 's/.*skip=\([0-9]*\).*/\1/p')
  errs=$(grep 'RESULT' "$rd/autorun.txt" | grep -v ' PASS \| SKIP ' | head -3 | sed 's/RESULT \([^ ]*\) \([A-Z]*\).*/\1:\2/' | tr '\n' ' ')
  [ $done_ -eq 1 ] || errs="TIMEOUT $errs"
  printf '%-22s %5s %5s %5s  %s\n' "$v" "${p:--}" "${f:--}" "${s:--}" "$errs" | tee -a "$SUMMARY"
done
echo "results in $OUT"
