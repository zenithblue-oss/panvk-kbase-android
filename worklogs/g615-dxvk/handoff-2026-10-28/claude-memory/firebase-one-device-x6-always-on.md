---
name: firebase-one-device-x6-always-on
description: "Firebase Test Lab runs are serial, one device per run; since 2026-10-25 testing focuses on Firebase devices, not the X6 Pro"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-26T01:02:33.831Z
---

User (2026-10-22):
- "test one device on firebase at a time also x6 pro always connected"
- "only do rc5"
- "use firebase limitely only when required focus on one device"

User (2026-10-25): "focus on firebsse devicr not x6 pro". User (2026-10-26): "test panplay cube as well in firebase. also test on firebase."

- Firebase Test Lab: one `--device` per run, runs strictly serial. Wrap every gcloud run in `flock /home/flux/build-cache/firebase.lock` so parallel agents never overlap. PanProbe uses `--scenario-numbers 2` (rc5 only). PanPlay cubes run via a game-loop harness (added 2026-10-26). The gcloud project is phonefinder-hub. Standard devices: starlte (v6, API 29), a31 (v7, API 31), oriole (v9, API 33), panther (v10, API 33), shiba (v11, API 34).
- Poco X6 Pro (Mali-G615, v11) is the user's own phone on wifi adb (10.11.176.130:<port>, port changes). Since 2026-10-25, do NOT plan tests on it unless the user brings it up again.

**Why:** Firebase quota is limited; the user now wants verification on Firebase devices rather than their own phone.

**How to apply:** test every fix build on Firebase devices, one run at a time. The A06 (RTL) rules from [[device-session-loop-to-full-pass]] and [[notify-when-device-needed]] still apply to RTL devices.
