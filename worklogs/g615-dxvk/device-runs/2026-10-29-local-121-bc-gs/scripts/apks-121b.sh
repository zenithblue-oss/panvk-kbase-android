#!/bin/bash
# panprobe-121b.apk: rc4 slot + build 121 slot.
mkdir -p /var/tmp/panvk/tmp-121
export TMPDIR=/var/tmp/panvk/tmp-121 ANDROID_HOME=/opt/android-sdk ANDROID_NDK_HOME=/opt/android-sdk/ndk/30.0.14904198
A=/var/tmp/panvk/build-out/apks; R=/home/abhaybyte/repos/panvk/apps
SO=/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so
RC4=/var/tmp/panvk/dl-rc4/libvulkan_panfrost-android-aarch64.so
exec 8>/var/tmp/panvk/panvk-test-gradle.lock; flock 8
cd $R/panvk-test && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSoRc4=$RC4 -PpanvkSoRc5=$SO > $A/panprobe-121b.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panprobe-121b.apk && echo PROBE-OK || echo PROBE-FAIL
sha256sum $A/panprobe-121b.apk
