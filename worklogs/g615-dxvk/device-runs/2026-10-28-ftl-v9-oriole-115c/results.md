# FTL oriole (Pixel 6, Mali-G78 MC20, v9 JM) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-x5y6f4x9hss2a` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v9-oriole-115c-probe/`; outcome `Passed oriole-33-en-portrait --`.
- APK `/home/flux/build-out/apks/panprobe-115c.apk` (sha256 `9c9989eb...`), `dev.zenithblue.panvktest` 1.3.0 (vc 19); rc5 slot = build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`). Scenario 2 = rc5 only, the whole suite once, the sparse_kbase S0 probe first. `raw/` has everything but `video.mp4`.

## Result

oriole (v9): 40/40 PASS, no device lost.

```text
GAMELOOP scenario=2
INFO devices=1 exts=175 formats=195
DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-15fd70c7c4)
SPARSE_KBASE VERDICT v9 eager-only -- JM kbase has no FIXED-VA allocations; SAME_VA arena, grow-from-end ok
RESULT {'PASS': 40} of 40 test time 219.0 s
```

## Screenshots

- `screenshots/video-end.png`
- `screenshots/video-mid.png`
- `screenshots/video-start.png`
