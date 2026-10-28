---
name: rc5-release-plan
description: "The user's agreed order of steps for releasing RC5 (v6 all-pass, unified build, PanPlay/PanProbe update, cube test, all-arch regression)"
metadata:
  node_type: memory
  type: project
  originSessionId: db381bd6-e816-4c4b-b498-8633cd891ac4
  modified: 2026-10-22T07:31:13.680Z
---

User (2026-10-22): "when v6 passes all test then we will do regression on all arch mali then release rc5, before that will update panplay and panprobe with new rc5 and test cube on x6 pro".

Order:
1. v6 all tests pass. Done on 2026-10-22: v7N got 38/38 on the Firebase S9.
2. Build one unified RC5 .so that has jm-v7-wip (v6/v7, through v7N 0076) plus csf-v11 (v11 BC = bc4, permanent, plus the bc5 fix).
3. Update PanPlay and PanProbe with the new RC5, then test the cube on the X6 Pro.
4. Regression on the final driver: v6 and v7 on Firebase (v6 starlte S9; v7 a G52 model such as Galaxy A31, Helio P65), plus v11 on the Poco X6 Pro. One Firebase device per run. (User 2026-10-22: "test when final driver ready on v6 v7 on firebase and v11 poco x6 pro".)
5. Then release RC5, only when the user confirms.

**Why:** RC5 must ship as one universal driver, verified on every arch.

**How to apply:** don't publish a release before step 4 passes and the user says go. See [[firebase-one-device-x6-always-on]] and [[notify-when-device-needed]].
