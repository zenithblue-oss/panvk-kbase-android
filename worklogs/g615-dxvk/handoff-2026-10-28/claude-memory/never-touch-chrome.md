---
name: never-touch-chrome
description: "Never kill, restart, launch or reconfigure the user's Chrome (RTL login lives there)"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-21T05:56:58.227Z
---

Never touch the user's Chrome: no pkill, no relaunch, no flag/wrapper changes, no clicking. Tell every subagent this.
Never kill ANY process (rdb, adb server, Chrome, builds, anything). Only start rdb via rdb-daemon if it is not running.

**Why:** the user explicitly forbade it (2026-10-21); their Samsung RTL session runs in that Chrome.

**How to apply:** if a fix seems to need a Chrome change, propose it to the user and let them do it.
No adb activity at all (not even `adb devices` polling) unless the user has just said a device is connected and asked for a run; never `svc power stayon` (drops RTL devices).
