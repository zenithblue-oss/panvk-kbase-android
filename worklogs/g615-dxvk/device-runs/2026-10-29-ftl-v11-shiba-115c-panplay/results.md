# FTL shiba (Pixel 8, Mali-G715, v11 CSF) build 115c: PanPlay scenarios 1-9

- Matrix `matrix-1e1nlsl1mbrzs` (project `panvk-device-test`), results
  `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v11-shiba-115c30-panplay/`; FTL outcome `Passed shiba-34-en-portrait`
  (the loop finished; the harness results are all errors).
- APK `/var/tmp/panvk/build-out/apks/panplay-115c.apk` (vc 44), driver `Mesa 26.3.0-devel (git-30c29c0175)`.
  `gameloop-override.json`: warm-up 10 s, measure 30 s. `raw/` has everything but `video.mp4`.

## Result: no scenario ran, the APK has no Wine rootfs

Scenarios 1-9 all ended at once with, before any driver code ran:

```
ERROR: java.lang.IllegalStateException: container setup failed: imagefs_bionic rootfs is not installed
```

Same as panther (`../2026-10-29-ftl-v10-panther-115c-panplay/results.md`): the desktop APK bundles no `imagefs_bionic`
rootfs (nor SquidRoom, Zink or component bundles). The goals for this run, D3D12 FL 11_1 `tir` and FL 12_1
`conservative` not DEVICE_LOST (they passed on 2026-10-28 in `../2026-10-28-ftl-v11-shiba-115c-diag3-panplay/` with the
layer budget), are therefore NOT confirmed on 115c-final. Needs a PanPlay APK with the rootfs.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
