---
name: subagents-one-at-a-time
description: "Run subagents sequentially, one at a time; no parallel fan-out (user 2026-10-26)"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-26T05:51:35.142Z
---

User (2026-10-26, after parallel review/implementation agents hit the usage limit): "pls do one subagent at a time and to sequenciallly all parts then test. donot do parallel for this"

Run implementation/test work as one subagent at a time: resume or spawn the next only after the previous reported and was stopped (see [[kill-stopped-subagents]]). Finish all parts first, then test.

**Why:** parallel Opus agents burned the usage limit and all got cut off mid-work.

**How to apply:** keep the orchestrator role from [[orchestrator-and-continuous-push]], but chain agents serially (P-A, P-B, P-C, P-D, then PanPlay DX12 integration, then the build-115 integrator with Firebase tests). Ask before going back to parallel fan-out.
