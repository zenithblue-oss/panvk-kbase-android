#!/bin/bash
# usage: apk-probe.sh <tag> <so>: panprobe-<tag>.apk with the rc4 slot + <so> in the rc5 slot.
T=$1; SO=$2
mkdir -p /var/tmp/panvk/tmp-122
export TMPDIR=/var/tmp/panvk/tmp-122 ANDROID_HOME=/opt/android-sdk ANDROID_NDK_HOME=/opt/android-sdk/ndk/30.0.14904198
A=/var/tmp/panvk/build-out/apks; R=/home/abhaybyte/repos/panvk/apps
RC4=/var/tmp/panvk/dl-rc4/libvulkan_panfrost-android-aarch64.so
exec 8>/var/tmp/panvk/panvk-test-gradle.lock; flock 8
cd $R/panvk-test && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSoRc4=$RC4 -PpanvkSoRc5=$SO > $A/panprobe-$T.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panprobe-$T.apk && echo PROBE-OK || echo PROBE-FAIL
