# FTL a31 (Galaxy A31, Mali-G52 MC2, v7 JM, API 31) build 117: PanProbe (scenario 2, rc5 only)

- Matrix `matrix-cmsemj9fqf3ca`, project `panvk-device-test`; FTL outcome `Passed a31-31-en-portrait`, ~5 min.
- APK `panprobe-117.apk` (PanProbe 1.3.0 vc 19), rc5 slot `driverInfo Mesa 26.3.0-devel (git-1a0a81aa00)`
  (Mesa 5a07217f + jm-v7-wip 0006..0148, .so sha256 `758ae1e9860f...`). `raw/` has everything but `video.mp4`.

## Result

- **40 PASS of 40**, 0 fail, 0 skip (`AUTORUN DONE pass=40 fail=0 skip=0 total=40`); `d3d_fl` PASS.
- No DEVICE_LOST in logcat or logs.
- `d3d_fl` capability view (PanProbe, reachability, not a created device): D3D12 FL11_0 HARD ok, FL11_1 ok; FL12_0 missing
  (TiledResourcesTier >= 2), FL12_1 missing (ROV, conservative raster), FL12_2 missing. D3D11 up to FL11_1 ok.
  The created-device result is in `../2026-10-29-ftl-v7-a31-117-panplay/` (D3D12 highest 11_1, D3D11 highest 11_1).
- `sparse_arena_commit=5.4us sparse_arena_uncommit=8.6us`.

## Compared with 115e (`../2026-10-29-ftl-v7-a31-115e-panprobe/`)

40/40 -> 40/40, no regressions; driver git-e7d836d5a6 -> git-1a0a81aa00.
