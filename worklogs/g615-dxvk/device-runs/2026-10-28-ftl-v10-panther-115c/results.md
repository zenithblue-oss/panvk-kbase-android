# FTL panther (Pixel 7, Mali-G710, v10 CSF) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-2m2neehy8ybnd` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v10-panther-115c-probe/`; outcome `Passed panther-33-en-portrait --`.
- APK `/home/flux/build-out/apks/panprobe-115c.apk` (sha256 `9c9989eb...`), `dev.zenithblue.panvktest` 1.3.0 (vc 19); rc5 slot = build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`). Scenario 2 = rc5 only, the whole suite once, the sparse_kbase S0 probe first. `raw/` has everything but `video.mp4`.

## Result

panther (v10): 40/40 PASS (115b: 40/40), no device lost.

```text
GAMELOOP scenario=2
INFO devices=1 exts=185 formats=195
DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-15fd70c7c4)
SPARSE_KBASE VERDICT v10 lazy-tile possible -- 64 KiB FIXED tiles commit 16->0->16 and re-alloc at the same VA (CSF 1.14)
RESULT {'PASS': 40} of 40 test time 164.2 s
```

## Screenshots

- `screenshots/video-end.png`
- `screenshots/video-mid.png`
- `screenshots/video-start.png`
