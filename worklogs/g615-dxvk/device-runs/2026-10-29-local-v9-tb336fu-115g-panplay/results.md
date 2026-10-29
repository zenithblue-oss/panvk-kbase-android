# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9, MT8755), build 115g PanPlay (full bundle)

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panplay-115g.apk` (sha256 `14c7b5b4...`, full bundle as
  `build-115f.sh`, built from a copy of `apps/panvk-launcher` at HEAD without uncommitted work-tree changes),
  `adb install -r` OK. Bundled driver `Mesa 26.3.0-devel (git-2f2a20a728)`, sha256 `ce6635b2`.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity --ei scenario N --es gameloop_config /sdcard/Download/gameloop-override-115e.json`
  (warm-up 10 s, measure 30 s, as 115f), each scenario run to its `done:` line before the next.
- Raw (no video): `raw/<run>-s<N>/`.

## Result: s3, s6, s8 PASS; s7 PASS except one intermittent D3D12 11_0 `tess` FAIL; no DEVICE_LOST

`deviceLost: false` in all 16 session.json.

| scenario | pass | result (115g vs 115f) |
|---|---|---|
| 3 | D3D11 cube / D3D9 cube | PASS 155.7 / 90.0 fps (154.9 / 89.9) |
| 6 | D3D12 gears | PASS 247.1 fps (252.2) |
| 7 | D3D12 FL 11_0 / 11_1 | run 1 (`073301`): PASS 394.2 / 306.6 fps, then the app crashed (below). Run 2 (`073709`): 11_0 **FAIL** `tess` (388.6 fps), 11_1 PASS 408.9 fps (`uav_vs`, `logic_op` PASS, `tir` REPORTED) |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED, as 115f |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | PASS 211.4 / 212.4 / 213.1 / 233.8 fps (215.5 / 210.3 / 211.0 / 202.8); 10_0 gs, stream_out PASS; 11_0 tess PASS; 11_1 `tir` PASS |
| 8 | D3D10 cube / D3D9 cube | PASS 160.6 / 89.9 fps (161.2 / 89.9) |

### s7 run 1: PanPlay crash (app side)

At 07:35:02, between pass 2 and pass 3, `FATAL EXCEPTION: Thread-24` in PanPlay: `NullPointerException` reading
`GLRenderer.xServerView` in `com.winlator.xserver.DrawableManager.removeDrawable` (X client freeing a pixmap after
"screenshot failed (no display view)"). Java, not the driver, no tombstone. "Close app" was tapped and s7 was rerun.

### s7 run 2: D3D12 11_0 `tess` FAIL, once

`tess FAIL zero-area tri patch, factor 4, interior lifted by the DS: 0 px covered (want >= 20)` (one tri patch, one
draw, readback). This check never failed before in the repo's runs. It was the first pass after the crash above, and
the pass had a 5.0 fps minimum (a stall). Repeats of the same pass on the same APK (short windows, override
`/var/tmp/panvk/perf115g/gameloop-tess-repeat.json`): 8/8 PASS (`074828`) and 2/2 PASS (`075104`, stopped when
another session started PanPlay scenario 3 on this device). Total for 115g: 11 PASS, 1 FAIL. The 0143 kernels are
deterministic (same inputs, same output; tess_perf hashes equal), so a GPU-side sync or first-use race is more
likely than wrong generation, but it is not proven: no A/B against 115f with the same repeat count yet.
