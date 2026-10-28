---
name: kill-stopped-subagents
description: "Always TaskStop subagents once they've reported/stopped, so leftover background loops (adb polling, sleeps) die"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-21T05:55:45.457Z
---

As soon as a subagent has delivered its final report (or stops "waiting on its own background work"), kill it with TaskStop. Never leave finished agents with lingering background loops.

**Why:** leftover agent loops (adb polling, sleep/until waiters) spammed notifications and helped knock the RTL A06 adb connection offline (2026-10-21).

**How to apply:** after each hand-back, TaskStop that agent id unless more work for it is planned. Also tell subagents to kill their own background commands before finishing. See [[orchestrator-and-continuous-push]].
