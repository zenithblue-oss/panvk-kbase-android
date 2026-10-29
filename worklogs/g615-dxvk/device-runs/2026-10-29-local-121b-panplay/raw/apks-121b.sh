#!/bin/bash
# Build 119 final APKs: panprobe-121b.apk (rc4 + 119) and panplay-121b.apk (119, components incl. MangoHud, SquidRoom).
mkdir -p /var/tmp/panvk/tmp-121b
export TMPDIR=/var/tmp/panvk/tmp-121b ANDROID_HOME=/opt/android-sdk ANDROID_NDK_HOME=/opt/android-sdk/ndk/30.0.14904198
A=/var/tmp/panvk/build-out/apks; R=/home/abhaybyte/repos/panvk/apps
SO=/var/tmp/panvk/build-out/2026-10-29-121b/libvulkan_panfrost.so
RC4=/var/tmp/panvk/dl-rc4/libvulkan_panfrost-android-aarch64.so
exec 8>/var/tmp/panvk/panvk-test-gradle.lock; flock 8
cd $R/panvk-launcher && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSo=$SO -Pdx12AssetsDir=/var/tmp/panvk/dl-squidroom > $A/panplay-121b.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panplay-121b.apk && echo PLAY-OK || echo PLAY-FAIL
sha256sum $A/panplay-121b.apk
echo DONE
