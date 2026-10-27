# FTL oriole (Pixel 6, Mali-G78 MC20, v9) build 115: PanProbe driver env A/B (scenario 3)

- Matrix `matrix-122b4nxa8x0jw`; outcome `Passed oriole-33-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v9-oriole-115-probe/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16); rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).
- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every default-on 115 change) in the same session.
- Baselines: `2026-10-26-ftl-v9-oriole-115pre-d`.

| pass | result | total time |
|---|---|---|
| `defaults` | 39 PASS of 39 | 214.0 s |
| `114-knobs` | 39 PASS of 39 | 302.1 s |

## Result

Both passes 39/39 (`AUTORUN DONE pass=39 fail=0 skip=0` twice), FTL outcome Passed. logcat has no DEVICE_LOST, `event=0x`,
GPU/MMU fault, CS_FATAL, Fatal signal, ANR, FATAL EXCEPTION or lowmemorykiller kill (its six lines are lmkd start-up
messages, as in 115pre-d). Every driver string is `Mesa 26.3.0-devel (git-9dde9e7a18)`; `deviceName` `Mali-G78 MC20`.

Against 115pre-d on the same device (same `env-ab.json` passes): `defaults` 214.0 s vs 219.5 s (-2.5 %),
`114-knobs` 302.1 s vs 297.8 s (+1.5 %); no test moved by more than 1 s and 20 %; `tess_perf` image hashes equal (18 of 18).
Knob lines per pass (device opens, the JSON echoes left out): `JM warm-up off` 92 in `defaults`, 0 in `114-knobs`;
`JM warm-up 2000 ms` 0 / 46 and `skipped` 0 / 45 (115pre-d 47 / 45); `114 behaviour restored` 0 / 46;
`kbase: coherency mode` 92 / 92. **No regression.**

## Summary (`an115f.py`)

```text
autorun raw/oriole-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvktest/files/autorun.txt
   GAMELOOP envab passes=2 source=assets/env-ab.json sha256=bbb3c2eddccd35c5a65595e065c6877e9d2458e1c88b8140495aedf2b5756abf
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
pass defaults: {'PASS': 39} tests=39 total=214.0s nonpass=[]
pass 114-knobs: {'PASS': 39} tests=39 total=302.1s nonpass=[]
logcat faults: none
logcat counts: {'JM warm-up off': 94, 'kbase: coherency mode': 187, 'AUTORUN DONE': 2, 'PANPROBE_RESULT': 2, '114 behaviour restored': 47, 'JM warm-up 2000 ms': 47}
driver strings: {'Mesa 26.3.0-devel (git-9dde9e7a18)': 10}
summary 20261027-011409-defaults envPass defaults driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G78 MC20
summary 20261027-011924-114-knobs envPass 114-knobs driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G78 MC20
device_info driverName panvk
device_info driverInfo Mesa 26.3.0-devel (git-9dde9e7a18)
device_info deviceName Mali-G78 MC20
device_info apiVersion 1.3.363
== vs 2026-10-26-ftl-v9-oriole-115pre-d passes ['defaults', '114-knobs']
 defaults: total 214.0s vs 219.5s (-2.5%) over 39 tests; tess_perf hashes equal (18)
 114-knobs: total 302.1s vs 297.8s (+1.5%) over 39 tests; tess_perf hashes equal (18)
```

## Screenshots

- video-start/mid/end.png
- video-start.png: 30 s in, pass 1 (defaults) running; video-mid.png: middle of the video; video-end.png: last frame, after pass 2 (114-knobs)

