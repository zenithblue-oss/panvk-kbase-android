# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9, MT8755), build 115d PanPlay (full bundle)

- Device `HNY07QHT`, `gpu_id 0x90930010 arch v9 model Mali-G57`. APK `/var/tmp/panvk/build-out/apks/panplay-115d.apk`
  (1.3.0 vc44, sha256 `41b6ea9c...`, 415 MB, full bundle: `imagefs_bionic.txz`, all component wcps, `SquidRoom.bin`),
  `adb install -r` OK. Bundled driver `Mesa 26.3.0-devel (git-b04b8ee36d)`, sha256 `da6f9a0f`.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario N --es gameloop_config /sdcard/Download/gameloop-override-115d.json`
  (`gameloop-override.json` here: warm-up 10 s, measure 30 s, shot at 20 s, as 115c).
- Raw (no video): `raw/<run>-s<N>/`.

## Result: D3D11 FL 11_1 `tir` PASS, SquidRoom PASS, no DEVICE_LOST

| scenario | pass | result |
|---|---|---|
| 3 | D3D11 cube / D3D9 cube | PASS 153.3 / 89.9 fps |
| 5 | D3D12 SquidRoom (d3d12-multithreading) | **PASS** 24.8 fps (115c: FAIL, not bundled) |
| 6 | D3D12 gears | PASS 255.5 fps |
| 7 | D3D12 FL 11_0 / 11_1 | PASS 402.7 / 427.2 fps (`logic_op` PASS, `tir` REPORTED) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED (as 115c) |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 | PASS 207.8 / 209.2 / 203.5 fps |
| 8 | D3D11 FL 11_1 | **PASS** (`uav_vs`, `logic_op`, `tir` PASS), 209.3 fps |
| 8 | D3D10 cube / D3D9 cube | PASS 155.5 / 89.9 fps |
| 9 | fl-quick d3d12 | DONE 11_0, 11_1 PASS, 12_x UNSUPPORTED, highest 11_1 |
| 9 | fl-quick d3d11 | DONE 10_0, 10_1, 11_0, 11_1 PASS |

- `tir` line: 104 PS invocations with ForcedSampleCount=4 through `CreateRasterizerState2` (DESC2, want >= 32), 0
  with 0; DESC1 ForcedSampleCount=4 gives 0 (DXVK `PromoteDesc` drops it). The test was changed in d3d11fl.c
  (commit 57f81f67); the driver needed no change.
- Grep for DEVICE_LOST, device lost and `JM atom ... failed` over all pulled logs: none.
- Versus 115c: same fps within noise; SquidRoom now runs because the APK bundles it.
