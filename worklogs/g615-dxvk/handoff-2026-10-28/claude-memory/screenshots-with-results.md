---
name: screenshots-with-results
description: Device test runs must capture screenshots and push them with the results
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-22T04:58:51.401Z
---

User (2026-10-22): "pls tell to take screenshots ans push those as well with results".

Every device runner captures screenshots, `adb -s <serial> exec-out screencap -p > x.png`: one during each run and one of the end-of-run PanProbe screen. They go in `<results dir>/screenshots/`, linked from results.md, and are committed and pushed with the results. For Firebase, download screenshots or video from the results bucket, or extract frames from video.mp4.

**Why:** the user wants visual evidence alongside the logs.

**How to apply:** put this in every device-runner prompt. On RTL devices screencap is read-only, so it is allowed, but keep it to a couple of shots per run (see [[device-session-loop-to-full-pass]]).
