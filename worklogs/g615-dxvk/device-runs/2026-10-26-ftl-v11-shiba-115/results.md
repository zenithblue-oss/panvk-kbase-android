# FTL shiba (Pixel 8, Mali-G715 MC7, v11) build 115: PanProbe driver env A/B (scenario 3)

- Matrix `matrix-1haqvpoqqgazt`; outcome `Passed shiba-34-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v11-shiba-115-probe/` (project `panvk-device-test`).
- APK `/home/flux/build-out/apks/panprobe-115.apk` (sha256 `0b774c6a...`), `dev.zenithblue.panvktest` 1.3.0 (vc 16); rc5 slot = build 115 (`73709644...`, driverInfo `Mesa 26.3.0-devel (git-9dde9e7a18)`).
- Passes from `assets/env-ab.json` (`bbb3c2ed...`): `defaults`, then `114-knobs` (27 keys, the 114 value of every default-on 115 change) in the same session.
- Baselines: `2026-10-26-ftl-v11-shiba-115pre`, `2026-10-25-ftl-v11-shiba-rc5u-114`.

| pass | result | total time |
|---|---|---|
| `defaults` | 39 PASS of 39 | 187.4 s |
| `114-knobs` | 39 PASS of 39 | 164.0 s |

## Result

Both passes 39/39 (`AUTORUN DONE pass=39 fail=0 skip=0` twice), FTL outcome Passed. logcat has no DEVICE_LOST, `event=0x`,
GPU/MMU fault, CS_FATAL, Fatal signal, ANR, FATAL EXCEPTION or lowmemorykiller kill. Every driver string is
`Mesa 26.3.0-devel (git-9dde9e7a18)`; `deviceName` `Mali-G715 MC7`. `tess_perf` image hashes equal 115pre and 114 (18 of 18).

- Against 115pre (same `env-ab.json` passes): `defaults` 187.4 s vs 186.0 s (+0.7 %), `114-knobs` 164.0 s vs 164.4 s (-0.3 %).
- Against 114 (one pass, cold shader cache): `defaults` 187.4 s vs 187.8 s (-0.2 %). `114-knobs` is faster (-12.7 %) only
  from the warm Mesa disk cache of pass 1 (`bc_decode` 0.2 s vs 12.3 s), as in 115pre.
- No test moved by more than 1 s and 20 % against 115pre.

Knob lines per pass: `kbase: GPU not IO-coherent: HOST_COHERENT dma-heap BOs from /dev/dma_heap/system-uncached` in
`defaults` only (`PANVK_KBASE_UNCACHED_HEAP=0` in `114-knobs`); `kbase: coherency mode 31 (none)` 92 / 92; `114 behaviour
restored` 0 / 45. **No regression.**

## Summary (`an115f.py`)

```text
autorun raw/shiba-34-en-portrait/artifacts/sdcard/Android/data/dev.zenithblue.panvktest/files/autorun.txt
   GAMELOOP envab passes=2 source=assets/env-ab.json sha256=bbb3c2eddccd35c5a65595e065c6877e9d2458e1c88b8140495aedf2b5756abf
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
   DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-9dde9e7a18)
   AUTORUN DONE pass=39 fail=0 skip=0 total=39 (39 pass / 0 fail / 0 skip)
pass defaults: {'PASS': 39} tests=39 total=187.4s nonpass=[]
pass 114-knobs: {'PASS': 39} tests=39 total=164.0s nonpass=[]
logcat faults: none
logcat counts: {'kbase: coherency mode': 186, 'system-uncached (PANVK_KBASE_UNCACHED_HEAP=0 = 115)': 93, 'AUTORUN DONE': 2, 'PANPROBE_RESULT': 2, '114 behaviour restored': 46}
driver strings: {'Mesa 26.3.0-devel (git-9dde9e7a18)': 8}
summary 20261027-013507-defaults envPass defaults driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G715 MC7
summary 20261027-013800-114-knobs envPass 114-knobs driverInfo Mesa 26.3.0-devel (git-9dde9e7a18) driverName panvk deviceName Mali-G715 MC7
device_info driverName panvk
device_info driverInfo Mesa 26.3.0-devel (git-9dde9e7a18)
device_info deviceName Mali-G715 MC7
device_info apiVersion 1.4.363
== vs 2026-10-26-ftl-v11-shiba-115pre passes ['defaults', '114-knobs']
 defaults: total 187.4s vs 186.0s (+0.7%) over 39 tests; tess_perf hashes equal (18)
 114-knobs: total 164.0s vs 164.4s (-0.3%) over 39 tests; tess_perf hashes equal (18)
== vs 2026-10-25-ftl-v11-shiba-rc5u-114 passes ['single']
 defaults: total 187.4s vs 187.8s (-0.2%) over 39 tests; tess_perf hashes equal (18)
 114-knobs: total 164.0s vs 187.8s (-12.7%) over 39 tests; tess_perf hashes equal (18)
    bc_decode: 0.2s vs 12.3s (-98%)
```

## Screenshots

- video-start/mid/end.png
- video-start.png: 30 s in, pass 1 (defaults) running; video-mid.png: middle of the video; video-end.png: last frame, after pass 2 (114-knobs)

