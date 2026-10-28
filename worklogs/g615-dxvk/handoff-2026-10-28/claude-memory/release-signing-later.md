---
name: release-signing-later
description: "Build 115 APKs are debug-signed; user will sign with the release key (8b10fde8...) later"
metadata:
  type: project
---

2026-10-27: panplay-115.apk / panprobe-115.apk are signed with this machine's debug key (bc517d8c...); shipped 1.2.x apps use 8b10fde8..., so debug builds cannot update existing installs. User: "sign with release key later".

**Why:** release signing is the user's step.

**How to apply:** don't block testing on signing; remind before any GitHub release (see [[rc5-release-plan]]).
