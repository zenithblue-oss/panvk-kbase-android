# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115c: PanPlay scenarios 1-9

- Matrix `matrix-3s3d321l9grvr` (project `panvk-device-test`), results
  `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v10-panther-115c30-panplay/`; FTL outcome `Passed panther-33-en-portrait`
  (FTL "Passed" only means the loop finished; the harness results below are all errors).
- APK `/var/tmp/panvk/build-out/apks/panplay-115c.apk` (desktop build, debug-signed), `dev.zenithblue.panvklauncher` 1.3.0
  (vc 44). `libvulkan_panfrost.so` sha256 `9877f62e...`, `Mesa 26.3.0-devel (git-30c29c0175)`. Note: the APK's
  `assets/bundled-driver.json` still says `git-15fd70c7c4` (stale metadata only; the .so is 30c29c0175).
- `gameloop-override.json`: warm-up 10 s, measure 30 s (pushed with `--other-files /data/local/tmp/panplay-gameloop.json=`;
  `results.json` `configSource` shows it was used, sha256 `4683a64a...`). `raw/` has everything but `video.mp4`.

## Result: no scenario ran, the APK has no Wine rootfs

Every scenario (1-9) ended at once with the same harness error, before any driver code ran:

```
ERROR: java.lang.IllegalStateException: container setup failed: imagefs_bionic rootfs is not installed
```

| scenario | tasks | result |
|---|---|---|
| 1 | d3d11 | ERROR (rootfs) |
| 2 | d3d9 | ERROR (rootfs) |
| 3 | d3d11 + d3d9 cubes | ERROR (rootfs) |
| 4 | cubes A/B envs | ERROR (rootfs) |
| 5 | D3D12 SquidRoom | ERROR (rootfs; also not bundled) |
| 6 | D3D12 gears | ERROR (rootfs) |
| 7 | D3D12 FL 11_0..12_2 | ERROR (rootfs) |
| 8 | D3D11 FL 10_0..11_1, D3D10, D3D9 | ERROR (rootfs) |
| 9 | FL quick (d3d12fl_all, d3d11fl_all) | ERROR (rootfs) |

This is a build problem, not a driver result: the desktop APK (88.7 MB) contains no `imagefs_bionic` (the 183 MB
`/var/tmp/panvk/components/imagefs_bionic.txz` is not bundled) and no Zink or component bundles. No fps, no DEVICE_LOST,
no `timeout on subqueue`, no logcat evidence about the driver. There is therefore no 115c vs 115b comparison for PanPlay
here (115b baseline: `../2026-10-28-ftl-v10-panther-115b-panplay/`). Rebuild PanPlay with the rootfs bundled to get the
115c numbers (the FTL runs cost one Spark run each).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames (the app's error screen).
