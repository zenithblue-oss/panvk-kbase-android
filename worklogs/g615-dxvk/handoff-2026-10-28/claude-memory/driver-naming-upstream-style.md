---
name: driver-naming-upstream-style
description: "Driver must be named like an upstream Mesa PanVK devel build everywhere (no \"PanVK-kbase RC5-universal-NNN\"); first official GitHub release"
metadata:
  node_type: memory
  type: project
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-26T09:51:54.152Z
---

User (2026-10-26): "change name to normal driver name, like how its in upstream gitlab panvk devel build okay change like this every where as will first official release in github"

From build 115 on, the driver reports itself like upstream Mesa: driverName "panvk", driverInfo "Mesa 26.3.0-devel (git-<sha>)" (Mesa VERSION is 26.3.0-devel). PanPlay/PanProbe labels, bundled-driver.json, packaging scripts and release docs use the same style (e.g. "Mesa PanVK 26.3.0-devel"). Build numbers stay only in metadata and worklogs; identify builds by git sha and .so sha256.

**Why:** the next release is the project's first official GitHub release.

**How to apply:** never reintroduce "PanVK-kbase", "RC5-universal-NNN" or "PanVK Kbase —" in user-visible strings. Android package ids and the app versionName are separate decisions; ask the user before changing them. No release or tag until the user says go (see [[rc5-release-plan]]).
