#!/bin/bash
set -eu
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

SDK_BUILD_TOOLS="/opt/android-sdk/build-tools/35.0.0"
ANDROID_JAR="/opt/android-sdk/platforms/android-35/android.jar"
NDK_BIN="/opt/android-sdk/ndk/30.0.14904198/toolchains/llvm/prebuilt/linux-x86_64/bin"

export PATH="$NDK_BIN:$SDK_BUILD_TOOLS:$PATH"

echo "=== 1. Building native shared library ==="
mkdir -p lib/arm64-v8a
aarch64-linux-android35-clang -O2 -shared -fPIC -w \
  -o lib/arm64-v8a/libpanvk_loader_test.so \
  jni/panvk_loader_test.c \
  -landroid -llog

echo "=== 2. Compiling resources ==="
rm -rf build-temp
mkdir -p build-temp/compiled
aapt2 compile res/layout/activity_main.xml -o build-temp/compiled/
aapt2 compile res/values/strings.xml -o build-temp/compiled/

aapt2 link -I "$ANDROID_JAR" \
  --manifest AndroidManifest.xml \
  -o build-temp/base.apk \
  --java src \
  --auto-add-overlay \
  build-temp/compiled/layout_activity_main.xml.flat \
  build-temp/compiled/values_strings.arsc.flat

echo "=== 3. Compiling Java ==="
mkdir -p build-temp/obj
javac -d build-temp/obj \
  -cp "$ANDROID_JAR" \
  src/org/panvk/loadertest/MainActivity.java \
  src/org/panvk/loadertest/R.java

echo "=== 4. Dexing ==="
d8 --output build-temp/ build-temp/obj/org/panvk/loadertest/*.class

echo "=== 5. Packaging APK ==="
(cd build-temp && zip -u base.apk classes.dex)
zip -u -r build-temp/base.apk lib/arm64-v8a/libpanvk_loader_test.so

echo "=== 6. Signing APK ==="
apksigner sign --ks ~/.android/debug.keystore --ks-pass pass:android \
  --out panvk-loader-test.apk build-temp/base.apk

echo "SUCCESS: panvk-loader-test.apk created"
