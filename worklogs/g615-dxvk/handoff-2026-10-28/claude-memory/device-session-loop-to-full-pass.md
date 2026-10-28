---
name: device-session-loop-to-full-pass
description: "When the user next connects a device, keep looping fix→build→test until RC5 passes every PanProbe test"
metadata:
  node_type: memory
  type: project
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-21T08:44:22.075Z
---

User (2026-10-21): "when next time device connected loop till rc5 achieve complete test pass on panprobe". During a device session, don't stop after one run: run PanProbe (RC4 vs RC5 slots), fix failing tests via subagents, rebuild (NDK r30 at /opt/android-sdk/ndk/30.0.16248370, fresh BDIR), reinstall, and rerun until RC5 has 0 fail/0 skip that are driver-caused.

**Why:** the goal is RC5 at least matching RC4's claimed 38/38; device time is scarce, so use the session fully.

**How to apply:** only safe adb commands (install --no-streaming, am start/force-stop, logcat, pull); no run-as/setprop/settings/svc/input/reconnect/kill. Settings are passed via PanProbe `--es env`. Commit+push each fix. See [[never-touch-chrome]], [[notify-when-device-needed]].
