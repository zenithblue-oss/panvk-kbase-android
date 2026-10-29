#!/bin/bash
# usage: apk-play.sh <tag> <so>: panplay-<tag>.apk from the clean worktree /var/tmp/panvk/repo-122 (no other agent's WIP).
T=$1; SO=$2
mkdir -p /var/tmp/panvk/tmp-122
export TMPDIR=/var/tmp/panvk/tmp-122 ANDROID_HOME=/opt/android-sdk ANDROID_NDK_HOME=/opt/android-sdk/ndk/30.0.14904198
A=/var/tmp/panvk/build-out/apks; R=/var/tmp/panvk/repo-122/apps
exec 8>/var/tmp/panvk/panvk-test-gradle.lock; flock 8
cd $R/panvk-launcher && ./gradlew --offline --no-daemon -q assembleDebug --max-workers=4 -PpanvkSo=$SO -Pdx12AssetsDir=/var/tmp/panvk/dl-squidroom > $A/panplay-$T.log 2>&1 \
  && cp app/build/outputs/apk/debug/app-debug.apk $A/panplay-$T.apk && echo PLAY-OK || echo PLAY-FAIL
