# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115c: PanProbe confirm (scenario 2)

- Matrix `matrix-1j51g40e9jmf0` (project `finalbenchmark-665a8`), results `gs://test-lab-rh5tw441b11v6-yb3zb70vx85mm/panvk-v7-a31-115c-probe/`; outcome `Passed a31-31-en-portrait --`.
- APK `/home/flux/build-out/apks/panprobe-115c.apk` (sha256 `9c9989eb...`), `dev.zenithblue.panvktest` 1.3.0 (vc 19); rc5 slot = build 115c (`eb44336a...`, driverInfo `Mesa 26.3.0-devel (git-15fd70c7c4)`). Scenario 2 = rc5 only, the whole suite once, the sparse_kbase S0 probe first. `raw/` has everything but `video.mp4`.

## Result

a31 (v7): 40/40 PASS, no device lost.

```text
GAMELOOP scenario=2
INFO devices=1 exts=166 formats=193
DRIVER slot=RC5 driverInfo=Mesa 26.3.0-devel (git-15fd70c7c4)
SPARSE_KBASE VERDICT v7 eager-only -- JM kbase has no FIXED-VA allocations; SAME_VA arena, grow-from-end ok
RESULT {'PASS': 40} of 40 test time 288.1 s
```

## Screenshots

- `screenshots/video-end.png`
- `screenshots/video-mid.png`
- `screenshots/video-start.png`
