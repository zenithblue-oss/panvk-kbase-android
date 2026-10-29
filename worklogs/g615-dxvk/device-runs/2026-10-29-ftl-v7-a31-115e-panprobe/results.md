# FTL a31 (Galaxy A31, Mali-G52 MC2, v7 JM) build 115e: PanProbe (scenario 2, rc5 only)

- Matrix `matrix-hbhbr8ymfpxga`, project `phone-finder-hub`, results
  `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-115d-a31-probe/` (dir name says 115d; APK is `panprobe-115e.apk`);
  FTL outcome `Passed a31-31-en-portrait`, test time 297 s. rc5 slot `driverInfo Mesa 26.3.0-devel (git-e7d836d5a6)`.
  `raw/` has everything but `video.mp4`.

## Result

- **40 PASS of 40**, 0 fail, 0 skip (`AUTORUN DONE pass=40 fail=0 skip=0 total=40`); `d3d_fl` PASS; DXVK, Bachata and
  vkd3d compliance PASS with no missing hard items.
- `sparse_kbase`: JM, `T7`/`T10`/`T10f` SKIP as expected (CSF-only), `sparse_arena_commit=3.9us sparse_arena_uncommit=8.2us`.
- No DEVICE_LOST in logcat or logs.

## Screenshots

- `screenshots/video-*.png`: FTL video frames.
