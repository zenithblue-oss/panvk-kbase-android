#!/bin/bash
# usage: oom2-apk.sh <so> <tag> [play]   PanProbe (rc5 slot = <so>) and optionally PanPlay APKs
SO=$1; T=$2
mkdir -p /var/tmp/panvk/tmp-oom2apk
export TMPDIR=/var/tmp/panvk/tmp-oom2apk ANDROID_HOME=/opt/android-sdk ANDROID_NDK_HOME=/opt/android-sdk/ndk/30.0.14904198
A=/var/tmp/panvk/build-out/apks
R=/home/abhaybyte/repos/panvk/apps
cd $R/panvk-test && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSoRc4=/var/tmp/panvk/dl-rc4/libvulkan_panfrost-android-aarch64.so -PpanvkSoRc5=$SO > $A/panprobe-$T.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panprobe-$T.apk && echo PROBE-OK || echo PROBE-FAIL
if [ "$3" = play ]; then
cd $R/panvk-launcher && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSo=$SO -Pdx12AssetsDir=/var/tmp/panvk/dl-squidroom > $A/panplay-$T.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panplay-$T.apk && echo PLAY-OK || echo PLAY-FAIL
fi
