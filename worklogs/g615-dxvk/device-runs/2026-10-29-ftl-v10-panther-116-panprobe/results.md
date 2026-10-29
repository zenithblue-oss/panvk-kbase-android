# FTL panther (Pixel 7, Mali-G710 MC7, v10 CSF, API 33) build 116: PanProbe (scenario 2, rc5 only)

- Matrix `matrix-vwxu6lper1ata`, project `abhay-byte` (panvk-device-test and finalbenchmark-665a8 were out of quota);
  FTL outcome `Passed panther-33-en-portrait`.
- APK `panprobe-116.apk`, rc5 slot `driverInfo Mesa 26.3.0-devel (git-f7e78953ac)` (build 116, jm-v7-wip through 0151,
  which includes 0148; the coordinator switched to 116 for runs not yet started). `raw/` has everything but `video.mp4`.
  (The GCS results dir is named `panvk-117-panther-probe`; the APK is 116.)

## Result

- **40 PASS of 40**, 0 fail, 0 skip (`AUTORUN DONE pass=40 fail=0 skip=0 total=40`); no DEVICE_LOST in logs.
- `d3d_fl` capability view: D3D12 FL11_0 HARD ok, FL11_1 ok; FL12_0 missing (TiledResourcesTier >= 2), FL12_1 missing
  (ROV, conservative raster), FL12_2 missing. D3D11 up to FL11_1 ok.
- `sparse_arena_commit=5.8us sparse_arena_uncommit=7.9us`.

## Compared with 115c (`../2026-10-29-ftl-v10-panther-115c-panprobe/`)

40/40 -> 40/40, no regressions (driver git-15fd70c7c4 -> git-f7e78953ac).
