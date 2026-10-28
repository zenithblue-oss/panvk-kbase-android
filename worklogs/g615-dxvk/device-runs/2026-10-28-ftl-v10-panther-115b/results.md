# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115b: PanProbe confirm (scenario 2)

- Matrix `matrix-2llzss0p3zrgu` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v10-panther-115b-probe/`;
  outcome `Passed panther-33-en-portrait`.
- APK `/home/flux/build-out/apks/panprobe-115b.apk` (sha256 `526fde1c...`), `dev.zenithblue.panvktest` 1.3.0 (vc 18);
  rc5 slot = build 115b (`d72e3059...`, driverInfo `Mesa 26.3.0-devel (git-ad5cd69141)`). Scenario 2 = rc5 only, the
  whole suite once, the sparse_kbase S0 probe first. `raw/` has everything but `video.mp4`.

## Result

| | result |
|---|---|
| suite | **40 PASS of 40** (build 115: 39 of 39; the new test is `d3d_fl`), 173.7 s of test time (115 `defaults`: 167.5 s) |
| sparse_kbase | PASS (informational): `VERDICT v10 lazy-tile possible -- 64 KiB FIXED tiles commit 16->0->16 and re-alloc at the same VA (CSF 1.14)` |

sparse_kbase bench: `sparse_arena_commit=3.3us sparse_arena_uncommit=6.1us (per 64KiB tile, n=63) |
sparse_tile_bind=2.3us sparse_tile_unbind=1.5us sparse_tile_reserve=1.2us sparse_tile_free=1.2us (64KiB FIXED tiles,
n=256 x 8 cycles) | sparse_alias_rebuild24=5.0us (n=256)`. Log: `raw/.../runs/20261027-202315/20261027-202315-sparse_kbase.log`.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
