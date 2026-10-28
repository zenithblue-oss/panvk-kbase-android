---
name: notify-when-device-needed
description: Only ping the user (nativecode-agent push notification) once everything for a device test is ready
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-21T04:12:01.049Z
---

When a device test is needed (e.g. RTL Galaxy A06 via rdb), send a push notification with `nativecode-agent notify` only once ALL artifacts are ready: driver .so, APKs, test plan. Don't ask the user for the device piecemeal. Never start device sessions piecemeal: RTL time costs the user credits. Build EVERY variant (all drivers + all APKs + script) first, then notify once.

**Why:** the user doesn't want partial interruptions; RTL reservations are time-limited.

**How to apply:** gate the device request on driver + apps built and verified, then notify once with what to reserve/click. See [[orchestrator-and-continuous-push]].
