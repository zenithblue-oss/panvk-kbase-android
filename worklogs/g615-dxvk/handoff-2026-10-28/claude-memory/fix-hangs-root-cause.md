---
name: fix-hangs-root-cause
description: "GPU hangs / VK_ERROR_DEVICE_LOST are top priority: fix at the root, no workarounds, no release while any arch hangs"
metadata:
  type: feedback
---

User (2026-10-27), after build 115 hung the GPU queue on panther (v10) in DXVK cubes and vkd3d gears: "gpu hangs and device lost should be fixed as this issue is persistent do not regress on this".

**Why:** hangs keep recurring across builds; the user sees them as release blockers.

**How to apply:** treat any hang/DEVICE_LOST/CS fault in a run as a regression to fix at the root before moving on; check recurring signatures across worklogs; never ship a build that hangs on any tested arch (see [[rc5-release-plan]]).
