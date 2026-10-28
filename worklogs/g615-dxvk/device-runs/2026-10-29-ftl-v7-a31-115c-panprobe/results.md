# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-17oz7fp5kfv2a` (project `panvk-device-test`), results
  `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v7-a31-115c30-probe/`; outcome `Passed a31-31-en-portrait`.
- APK `/var/tmp/panvk/build-out/apks/panprobe-115c.apk`, `dev.zenithblue.panvktest` 1.3.0 (vc 19); rc5 slot = build 115c,
  `driverInfo Mesa 26.3.0-devel (git-30c29c0175)`. Scenario 2 = rc5 only. `raw/` has everything but `video.mp4`.

## Result

| | 115c |
|---|---|
| suite | **40 PASS of 40**, 0 fail, 0 skip, 341.6 s of test time (build 115 `defaults`: 323.1 s) |
| d3d_fl | PASS (this is the instance-level FL check, not the PanPlay FL 12_0 typed-UAV draw) |
| bc_decode | PASS |
| sparse_kbase | PASS 11, skip 3 (informational): `VERDICT v7 eager-only -- JM kbase has no FIXED-VA allocations; SAME_VA arena, grow-from-end ok` |

No failing test, no DEVICE_LOST. PanPlay D3D12 FL 11_1 `logic_op`, FL 12_0 typed UAV 0x51 and gears were not exercised
(PanPlay had no rootfs, see `../2026-10-29-ftl-v10-panther-115c-panplay/results.md`).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
