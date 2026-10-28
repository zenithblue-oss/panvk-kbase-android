# FTL starlte (Galaxy S9, Mali-G72 MC18, v6 JM) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-34emp9mx8byhb` (project `panvk-device-test`), results
  `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v6-starlte-115c30-probe/`; outcome `Passed starlte-29-en-portrait`.
- APK `/var/tmp/panvk/build-out/apks/panprobe-115c.apk`, `dev.zenithblue.panvktest` 1.3.0 (vc 19); rc5 slot = build 115c,
  `driverInfo Mesa 26.3.0-devel (git-30c29c0175)`. Scenario 2 = rc5 only. `raw/` has everything but `video.mp4`.

## Result

| | 115c |
|---|---|
| suite | **40 PASS of 40**, 0 fail, 0 skip, 209.4 s of test time (build 115 `defaults`: 216.5 s) |
| sparse_kbase | PASS 11, skip 3 (informational): `VERDICT v6 eager-only -- JM kbase has no FIXED-VA allocations; SAME_VA arena, grow-from-end ok` |

No failing test, no DEVICE_LOST. The starlte Wine prefix failure (SELinux ioctl 0x7201, kernel32 c0000135) is a PanPlay
matter and was not exercised: PanPlay 115c has no rootfs (`../2026-10-29-ftl-v10-panther-115c-panplay/results.md`).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
