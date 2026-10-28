---
name: orchestrator-and-continuous-push
description: "User wants Opus as orchestrator only (subagents do all work, sonnet/haiku mostly), continuous commit+push to main, simple light commands under PRoot"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-20T17:26:37.292Z
---

Opus main session only orchestrates; all hands-on work goes to subagents (haiku for mechanical, sonnet for normal, opus only for hard). Commit and push to main continuously; release source must be on main before building. Under PRoot use one simple command per call, no heavy metadata ops.

**Why:** RC4 v7 (G52 working) and RC4 lowmem BC source were never committed and got lost (2026-10-20). PRoot makes heavy commands time out.

**How to apply:** Delegate every task; tell subagents to commit+push and to follow the PRoot command rules. Rules also live in ~/.claude/CLAUDE.md.
