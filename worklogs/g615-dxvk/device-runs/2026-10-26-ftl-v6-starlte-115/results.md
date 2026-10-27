# FTL starlte (Galaxy S9, Mali-G72 MC18, v6) build 115: PanProbe driver env A/B (scenario 3)

- Matrix `matrix-2micengn1l7ql`; outcome `Passed starlte-29-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v6-starlte-115-probe/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16); rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).
- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every default-on 115 change) in the same session.
- Baselines: `2026-10-26-ftl-v6-starlte-115pre-d`.

| pass | result | total time |
|---|---|---|
| `defaults` | 39 PASS of 39 | 216.5 s |
| `114-knobs` | 39 PASS of 39 | 292.5 s |

## Result

Both passes 39/39 (`AUTORUN DONE pass=39 fail=0 skip=0` twice), FTL outcome Passed. logcat has no DEVICE_LOST, `event=0x`,
GPU/MMU fault, CS_FATAL, Fatal signal, ANR, FATAL EXCEPTION or lowmemorykiller kill. Every driver string is
`Mesa 26.3.0-devel (git-9dde9e7a18)`; `deviceName` `Mali-G72 MC18`.

Against 115pre-d on the same device (same `env-ab.json` passes): `defaults` 216.5 s vs 215.0 s (+0.7 %),
`114-knobs` 292.5 s vs 292.1 s (+0.1 %); no test moved by more than 1 s and 20 %; `tess_perf` image hashes equal (18 of 18).
Knob lines as in 115pre-d: `JM warm-up off` 93 (115pre-d 92) in `defaults`; `JM warm-up 2000 ms` 48 (47) and
`114 behaviour restored` 47 (46) only in `114-knobs`. **No regression.**

## Summary (`an115f.py`)

```text
autorun raw/starlte-29-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvktest/files/autorun.txt
   GAMELOOP envab passes=2 source=assets/env-ab.json sha256=bbb3c2eddccd35c5a65595e065c6877e9d2458e1c88b8140495aedf2b5756abf
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
pass defaults: {'PASS': 39} tests=39 total=216.5s nonpass=[]
pass 114-knobs: {'PASS': 39} tests=39 total=292.5s nonpass=[]
logcat faults: none
logcat counts: {'JM warm-up off': 93, 'kbase: coherency mode': 186, 'AUTORUN DONE': 2, 'PANPROBE_RESULT': 2, '114 behaviour restored': 47, 'JM warm-up 2000 ms': 48}
driver strings: {'Mesa 26.3.0-devel (git-9dde9e7a18)': 8}
summary 20261027-004334-defaults envPass defaults driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G72 MC18
summary 20261027-004843-114-knobs envPass 114-knobs driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G72 MC18
device_info driverName panvk
device_info driverInfo Mesa 26.3.0-devel (git-9dde9e7a18)
device_info deviceName Mali-G72 MC18
device_info apiVersion 1.3.363
== vs 2026-10-26-ftl-v6-starlte-115pre-d passes ['defaults', '114-knobs']
 defaults: total 216.5s vs 215.0s (+0.7%) over 39 tests; tess_perf hashes equal (18)
 114-knobs: total 292.5s vs 292.1s (+0.1%) over 39 tests; tess_perf hashes equal (18)
```

## Screenshots

- video-start/mid/end.png
- video-start.png: 30 s in, pass 1 (defaults) running; video-mid.png: middle of the video; video-end.png: last frame, after pass 2 (114-knobs)

