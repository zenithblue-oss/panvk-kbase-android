# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9, MT8755), build 115c PanPlay

- Device `HNY07QHT`, `gpu_id 0x90930010 arch v9 model Mali-G57`. APK `panplay-115c.apk` (1.3.0 vc44), `adb install -r` OK, bundled driver `Mesa 26.3.0-devel (git-30c29c0175)`.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario N --es gameloop_config /sdcard/Download/gameloop-override.json` (warm-up 10 s, measure 30 s, shot at 20 s; `gameloop-override.json` here is the 2026-10-28 one). Scenarios run: 3, 5, 6, 7, 8, 9, one after another. 1 and 2 are the two halves of 3; 4 (A/B knobs) not run.
- Raw (no video): `raw/<run>-s<N>/` with results.json, harness.log, per-pass screenshot, session logs.

## Result: no regression, no DEVICE_LOST, no kbase fault

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube | PASS 153.7 fps |
| 3 | D3D9 cube | PASS 90.0 fps (vsync) |
| 5 | D3D12 SquidRoom | FAIL `DX12: FAIL std::exception hr=0x80004005` after 5 s. Expected: APK built without SquidRoom, not a driver bug |
| 6 | D3D12 gears | PASS 254.9 fps |
| 7 | D3D12 FL 11_0 | PASS (tess gs compute mrt msaa4x) 403.4 fps |
| 7 | D3D12 FL 11_1 | PASS (uav_vs, logic_op INVERT 550ff000 ok, tir REPORTED) 429.9 fps |
| 7 | D3D12 FL 12_0 | UNSUPPORTED (typed_uav_load, binding_tier2 pass; TiledResourcesTier=0) |
| 7 | D3D12 FL 12_1, 12_2 | UNSUPPORTED (ROV 0, conservative tier 0; mesh/dxr/vrs 0) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 | PASS 206.6 / 211.1 / 213.1 fps |
| 8 | D3D11 FL 11_1 | FAIL `tir`: 0 PS invocations with ForcedSampleCount=4 (known open); `uav_vs` and `logic_op` PASS; 210.9 fps |
| 8 | D3D10 cube / D3D9 cube | PASS 160.5 / 90.0 fps |
| 9 | fl-quick d3d12 | DONE 11_0=PASS 11_1=PASS 12_0/12_1/12_2=UNSUPPORTED highest=11_1, 428.4 fps |
| 9 | fl-quick d3d11 | 10_0, 10_1, 11_0 PASS; 11_1 FAIL (`tir` only), 207.0 fps |

- Grep for DEVICE_LOST, device lost, `kbase: JM atom` failures and timeouts over all session logs: none (only the normal `MediaTek JM atom layout` info line).
- Compared with 115b oriole v9 (different SoC and GPU, rough): same pattern, and the 115b D3D12 FL 11_1 `logic_op` FAIL is now PASS (patch 0138). Gears fps higher than the single-run 232.7 of build 115 oriole.
