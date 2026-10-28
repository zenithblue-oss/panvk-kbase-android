---
name: git-email-global
description: "Commit with plain git commit; never pass -c user.email, the global email is always set"
metadata:
  node_type: memory
  type: feedback
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-28T19:55:10.745Z
---

Use plain `git commit`; never `git -c user.email=...`.

**Why:** user (2026-10-28) rejected a commit with `-c user.email=zenithblue.dev@gmail.com`: "email is always set use global".

**How to apply:** all commits, including subagent briefs. Replaces the old `-c user.email` rule. See [[orchestrator-and-continuous-push]].
