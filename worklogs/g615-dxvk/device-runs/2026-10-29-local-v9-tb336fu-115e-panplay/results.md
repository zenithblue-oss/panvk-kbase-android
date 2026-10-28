# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9, MT8755), build 115e PanPlay (full bundle)

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panplay-115e.apk` (1.3.0 vc44, sha256 `597c9240...`, 417 MB,
  full bundle: `imagefs_bionic.txz`, all wcps incl. `dxvk-3.1.1-5-arm64ec.wcp`, `SquidRoom.bin`), `adb install -r` OK.
  Bundled driver `Mesa 26.3.0-devel (git-e7d836d5a6)`, sha256 `8dea43cf`.
- Launch: `am start -S -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario N --es gameloop_config /sdcard/Download/gameloop-override-115e.json`
  (`gameloop-override.json` here: warm-up 10 s, measure 30 s, shot at 20 s, as 115d). Each scenario run to its
  `done:` line in `harness.log` before the next (a first pass that moved on at the first `results.json` cut s5-s9 short;
  those runs were discarded and redone).
- Raw (no video): `raw/<run>-s<N>/`.

## Result: all PASS, no DEVICE_LOST (`deviceLost: false` in every session.json)

| scenario | pass | result (115e vs 115d) |
|---|---|---|
| 3 | D3D11 cube / D3D9 cube | PASS 154.5 / 90.0 fps (153.3 / 89.9) |
| 5 | D3D12 SquidRoom (d3d12-multithreading) | PASS 23.1 fps (24.8) |
| 6 | D3D12 gears | PASS 245.7 fps (255.5) |
| 7 | D3D12 FL 11_0 / 11_1 | PASS 396.9 / 419.3 fps (402.7 / 427.2), `logic_op` PASS, `tir` REPORTED |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED, as 115d |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | PASS 182.1 / 207.3 / 206.3 / 202.6 fps (207.8 / 209.2 / 203.5 / 209.3), 11_1 `uav_vs`, `logic_op`, `tir` PASS |
| 8 | D3D10 cube / D3D9 cube | PASS 160.7 / 89.9 fps (155.5 / 89.9) |
| 9 | fl-quick d3d12 | DONE 11_0, 11_1 PASS, 12_x UNSUPPORTED, highest 11_1 |
| 9 | fl-quick d3d11 | DONE 10_0, 10_1, 11_0, 11_1 PASS |

- Same as 115d within noise (single 30 s runs, thermal state differs). 115e only changes v6/v7 (jm-v7-wip 0141), so no
  v9 change expected.
