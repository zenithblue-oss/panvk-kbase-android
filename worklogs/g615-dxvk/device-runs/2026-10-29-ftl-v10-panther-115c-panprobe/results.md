# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-oj83hi07pja6a` (project `panvk-device-test`), results
  `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v10-panther-115c30-probe/`; outcome `Passed panther-33-en-portrait`.
- APK `/var/tmp/panvk/build-out/apks/panprobe-115c.apk` (desktop build), `dev.zenithblue.panvktest` 1.3.0 (vc 19);
  rc5 slot = build 115c, `driverInfo Mesa 26.3.0-devel (git-30c29c0175)`. Scenario 2 = rc5 only. `raw/` has everything
  but `video.mp4`.

## Result

| | 115c | 115b (`../2026-10-28-ftl-v10-panther-115b/`) |
|---|---|---|
| suite | **40 PASS of 40**, 0 fail, 0 skip, 162.1 s of test time | 40 of 40, 173.7 s |
| d3d_fl | PASS | PASS |
| sparse_kbase | PASS 18/18 (informational): `VERDICT v10 lazy-tile possible -- 64 KiB FIXED tiles commit 16->0->16 and re-alloc at the same VA (CSF 1.14)` | same verdict |

sparse_kbase bench: `sparse_arena_commit=5.1us sparse_arena_uncommit=11.0us | sparse_tile_bind=3.1us
sparse_tile_unbind=2.0us sparse_tile_reserve=2.2us sparse_tile_free=1.0us | sparse_alias_rebuild24=5.2us`.

No regression vs 115b. No DEVICE_LOST, no `timeout on subqueue`.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
