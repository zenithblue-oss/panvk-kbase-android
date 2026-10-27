# FTL panther (Pixel 7, Mali-G710, v10) build 115: PanProbe driver env A/B (scenario 3)

- Matrix `matrix-wht2r6ud1mz6a`; outcome `Passed panther-33-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v10-panther-115-probe/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16); rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).
- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every default-on 115 change) in the same session.
- Baselines: `2026-10-25-ftl-v10-panther-rc5u-114`.

| pass | result | total time |
|---|---|---|
| `defaults` | 39 PASS of 39 | 167.5 s |
| `114-knobs` | 39 PASS of 39 | 136.6 s |

## Result

Both passes 39/39 (`AUTORUN DONE pass=39 fail=0 skip=0` twice), FTL outcome Passed. logcat has no DEVICE_LOST, `event=0x`,
GPU/MMU fault, CS_FATAL, Fatal signal, ANR, FATAL EXCEPTION or lowmemorykiller kill. Every driver string is
`Mesa 26.3.0-devel (git-9dde9e7a18)`; `deviceName` `Mali-G710 MC7`. `tess_perf` image hashes equal 114 (18 of 18).

Baseline: the 114 run (`2026-10-25-ftl-v10-panther-rc5u-114`, one pass, cold shader cache). `defaults` 167.5 s vs
175.7 s (-4.6 %): `bc_decode` 16.7 vs 18.7 s, `bc_perf` 47.4 vs 53.5 s (both -11 %), `tess_perf` 48.4 vs 48.2 s,
`large_draw` 6.3 vs 6.2 s, every other test within 3 %. `114-knobs` (136.6 s) is faster than `defaults` only because it
runs second with the Mesa disk cache that pass 1 filled (`bc_decode` 0.1 s, `bc_perf` 33.5 s; same effect as in the
shiba 115pre run); the tests that do not compile much (`tess_perf` 48.7 s, `large_draw` 6.0 s, `submit_stress`,
`swapchain_lifecycle`) match `defaults`.

Knob lines per pass: `kbase: GPU not IO-coherent: HOST_COHERENT dma-heap BOs from /dev/dma_heap/system-uncached` 92 in
`defaults`, 0 in `114-knobs` (`PANVK_KBASE_UNCACHED_HEAP=0`); `kbase: coherency mode 31 (none)` 92 / 92 (shaderDeviceClock
off / on); `114 behaviour restored` 0 / 46. **No regression.**

## Summary (`an115f.py`)

```text
autorun raw/panther-33-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvktest/files/autorun.txt
   GAMELOOP envab passes=2 source=assets/env-ab.json sha256=bbb3c2eddccd35c5a65595e065c6877e9d2458e1c88b8140495aedf2b5756abf
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
pass defaults: {'PASS': 39} tests=39 total=167.5s nonpass=[]
pass 114-knobs: {'PASS': 39} tests=39 total=136.6s nonpass=[]
logcat faults: none
logcat counts: {'kbase: coherency mode': 187, 'system-uncached (PANVK_KBASE_UNCACHED_HEAP=0 = 115)': 94, 'AUTORUN DONE': 2, 'PANPROBE_RESULT': 2, '114 behaviour restored': 47}
driver strings: {'Mesa 26.3.0-devel (git-9dde9e7a18)': 10}
summary 20261027-012553-defaults envPass defaults driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G710 MC7
summary 20261027-012814-114-knobs envPass 114-knobs driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G710 MC7
device_info driverName panvk
device_info driverInfo Mesa 26.3.0-devel (git-9dde9e7a18)
device_info deviceName Mali-G710 MC7
device_info apiVersion 1.4.363
== vs 2026-10-25-ftl-v10-panther-rc5u-114 passes ['single']
 defaults: total 167.5s vs 175.7s (-4.6%) over 39 tests; tess_perf hashes equal (18)
 114-knobs: total 136.6s vs 175.7s (-22.2%) over 39 tests; tess_perf hashes equal (18)
    bc_decode: 0.1s vs 18.7s (-99%)
    bc_perf: 33.5s vs 53.5s (-37%)
```

## Screenshots

- video-start/mid/end.png
- video-start.png: 30 s in, pass 1 (defaults) running; video-mid.png: middle of the video; video-end.png: last frame, after pass 2 (114-knobs)

