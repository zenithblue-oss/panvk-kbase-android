# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7) build 115: PanProbe driver env A/B (scenario 3)

- Matrix `matrix-14tod5u9d24gr`; outcome `Passed a31-31-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v7-a31-115-probe/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16); rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).
- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every default-on 115 change) in the same session.
- Baselines: `2026-10-26-ftl-v7-a31-115pre-d`.

| pass | result | total time |
|---|---|---|
| `defaults` | 39 PASS of 39 | 323.1 s |
| `114-knobs` | 39 PASS of 39 | 431.9 s |

## Result

Both passes 39/39 (`AUTORUN DONE pass=39 fail=0 skip=0` twice), FTL outcome Passed. logcat has no DEVICE_LOST, `event=0x`,
GPU/MMU fault, CS_FATAL, Fatal signal, ANR, FATAL EXCEPTION or lowmemorykiller kill. Every driver string is
`Mesa 26.3.0-devel (git-9dde9e7a18)`; `deviceName` `Mali-G52 r1 MC2`. `tess_perf` image hashes equal 115pre-d (18 of 18).

Both passes are slower than 115pre-d: `defaults` 323.1 s vs 286.9 s (+12.6 %), `114-knobs` 431.9 s vs 362.9 s (+19.0 %).
**This is the FTL a31, not build 115:**

- The `114-knobs` pass (every default-on 115 change at its 114 value, including `PANVK_KBASE_UNCACHED_HEAP=0` for the
  0128 heap change; the rest of 0128-0131 is X11 WSI or opt-in) slowed down more than `defaults`. A default-on 115
  change would slow `defaults`, not the 114 control.
- Only the GPU-bound tests moved, in both passes: `tess_perf` 153.1 / 179.7 s (115pre-d 131.1 / 133.3), `large_draw`
  27.5 / 29.2 s (21.0 / 23.4), `tessellation`, `bc_perf`, `robust_image_access2`. The CPU-bound tests are within a few
  percent.
- Earlier a31 runs switch between the same two speeds: rc5u-113 `tess_perf` 133.8 s, `large_draw` 22.9 s (total 364.3 s);
  rc5u-112 179.9 s, 29.4 s (total 433.3 s). This run's `114-knobs` pass is rc5u-112 to the tenth (179.7 s, 29.2 s, 431.9 s).
- Within the session, `defaults` is 25.2 % faster than `114-knobs` (115pre-d: 20.9 %): the 115 changes still pay off.

Knob lines per pass (device opens, the JSON echoes left out) equal 115pre-d: `JM warm-up off` 92 in `defaults`, 0 in
`114-knobs`; `JM warm-up 2000 ms` 0 / 47 and `skipped` 0 / 45; `114 behaviour restored` 0 / 46; `kbase: coherency mode` 92 / 92. **No regression from build 115.**

## Summary (`an115f.py`)

```text
autorun raw/a31-31-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvktest/files/autorun.txt
   GAMELOOP envab passes=2 source=assets/env-ab.json sha256=bbb3c2eddccd35c5a65595e065c6877e9d2458e1c88b8140495aedf2b5756abf
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
pass defaults: {'PASS': 39} tests=39 total=323.1s nonpass=[]
pass 114-knobs: {'PASS': 39} tests=39 total=431.9s nonpass=[]
logcat faults: none
logcat counts: {'JM warm-up off': 94, 'kbase: coherency mode': 187, 'AUTORUN DONE': 2, 'PANPROBE_RESULT': 2, '114 behaviour restored': 47, 'JM warm-up 2000 ms': 48}
driver strings: {'Mesa 26.3.0-devel (git-9dde9e7a18)': 10}
summary 20261027-005924-defaults envPass defaults driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G52 r1 MC2
summary 20261027-010652-114-knobs envPass 114-knobs driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G52 r1 MC2
device_info driverName panvk
device_info driverInfo Mesa 26.3.0-devel (git-9dde9e7a18)
device_info deviceName Mali-G52 r1 MC2
device_info apiVersion 1.3.363
== vs 2026-10-26-ftl-v7-a31-115pre-d passes ['defaults', '114-knobs']
 defaults: total 323.1s vs 286.9s (+12.6%) over 39 tests; tess_perf hashes equal (18)
    tessellation: 10.9s vs 8.9s (+23%)
    large_draw: 27.5s vs 21.0s (+31%)
    robust_image_access2: 5.0s vs 3.7s (+37%)
 114-knobs: total 431.9s vs 362.9s (+19.0%) over 39 tests; tess_perf hashes equal (18)
    tess_perf: 179.7s vs 133.3s (+35%)
    large_draw: 29.2s vs 23.4s (+25%)
    robust_image_access2: 6.9s vs 5.5s (+24%)
```

## Screenshots

- video-start/mid/end.png
- video-start.png: 30 s in, pass 1 (defaults) running; video-mid.png: middle of the video; video-end.png: last frame, after pass 2 (114-knobs)

