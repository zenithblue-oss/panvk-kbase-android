# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9, MT8755), build 115f PanPlay (full bundle)

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panplay-115f.apk` (1.3.0 vc44, sha256 `83cf214d...`, 426 MB,
  full bundle as `build-115e.sh`), `adb install -r` OK. Bundled driver `Mesa 26.3.0-devel (git-0f038f9e96)`, sha256 `62186713`.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario N --es gameloop_config /sdcard/Download/gameloop-override-115e.json`
  (`gameloop-override.json` here: warm-up 10 s, measure 30 s, as 115e), each scenario run to its `done:` line before the next.
- Raw (no video): `raw/<run>-s<N>/`.

## Result: all PASS, no DEVICE_LOST (`deviceLost: false` in all 14 session.json)

| scenario | pass | result (115f vs 115e) |
|---|---|---|
| 3 | D3D11 cube / D3D9 cube | PASS 154.9 / 89.9 fps (154.5 / 90.0) |
| 6 | D3D12 gears | PASS 252.2 fps (245.7) |
| 7 | D3D12 FL 11_0 / 11_1 | PASS 399.1 / 405.3 fps (396.9 / 419.3); 11_0 tess, gs PASS; 11_1 `uav_vs`, `logic_op` PASS, `tir` REPORTED |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED, as 115e |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | PASS 215.5 / 210.3 / 211.0 / 202.8 fps (182.1 / 207.3 / 206.3 / 202.6); 10_0 gs, stream_out PASS; 11_0 tess PASS; 11_1 `tir` PASS |
| 8 | D3D10 cube / D3D9 cube | PASS 161.2 / 89.9 fps (160.7 / 89.9) |

Single 30 s runs; differences are within noise except D3D11 FL 10_0 (GS + stream out), 182.1 -> 215.5 fps.
