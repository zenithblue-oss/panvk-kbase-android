# FTL shiba (Pixel 8, Mali-G715, v11 CSF) build 115e: PanProbe (scenario 2, rc5 only)

- Matrix `matrix-1xdqinvdtu9nz`, project `phone-finder-hub`, results
  `gs://test-lab-7jybm598jy36m-hdr2td6as0u1q/panvk-115d-shiba-probe/` (dir name says 115d; APK is `panprobe-115e.apk`);
  FTL outcome `Passed shiba-34-en-portrait`, test time 183 s. rc5 slot `driverInfo Mesa 26.3.0-devel (git-e7d836d5a6)`.
  `raw/` has everything but `video.mp4`.

## Result

- **40 PASS of 40**, 0 fail, 0 skip (`AUTORUN DONE pass=40 fail=0 skip=0 total=40`).
- `d3d_fl` PASS. `sparse_kbase` PASS 18/18 (informational): `VERDICT v11 lazy-tile possible -- 64 KiB FIXED tiles commit
  16->0->16 and re-alloc at the same VA (CSF 1.24)`; `sparse_arena_commit=3.7us sparse_arena_uncommit=2.7us`.
- No DEVICE_LOST, no `timeout on subqueue` in logcat or logs.

## Screenshots

- `screenshots/video-*.png`: FTL video frames.
