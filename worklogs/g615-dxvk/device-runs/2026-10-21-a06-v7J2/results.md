# A06 v7J2 (harness fix) - INCOMPLETE, device lost

APK /home/flux/build-out/apks/panprobe-v7J2.apk, package dev.zenithblue.panvktest.v7j, rc5 .so sha c8ea8459... (verified inside APK, unchanged driver).

## Harness changes (MainActivity.kt, pushed to main)
1. executeSwapchainTest: select Tests tab (selectedTabState=2) before showing the surface card; surface wait 5s -> 20s.
2. Autorun: FLAG_KEEP_SCREEN_ON.
3. (second APK, sha256 c9840047...) setShowWhenLocked(true) + setTurnScreenOn(true), since the first fix still SKIPped (suspect display off).

## Device results
- First fix APK (sha b703625c...): swapchain_lifecycle still SKIP after 20 s (run 20261021-201113, files in swapchain/).
- Second APK (wake display): install failed ("failed to read copy response"), then localhost:54520 vanished from adb. Reconnect is forbidden, so NOT tested.
- pipeline_stats x3, thermals, autorun all: NOT run.
