# FTL a31 (Galaxy A31, Mali-G52 r1 MC2, v7 JM) build 115b: shader dump diag

- Matrix `matrix-218jjz1t3ksez` (project `panvk-device-test`); outcome `Passed a31-31-en-portrait`.
- APK `panplay-115b.apk` (vc 43, bundled driver 115b, driverInfo `Mesa 26.3.0-devel (git-ad5cd69141)`).
- `gameloop-override.json`: scenario 1 = D3D12 FL 12_0, D3D12 FL 11_1, D3D11 FL 11_1, D3D12 gears with
  `BIFROST_MESA_DEBUG=shaders,internal` and `MESA_SHADER_CACHE_DISABLE=true` (Bifrost NIR, IR and disassembly of
  every shader in `wine-run.log`, 4 MiB cap). `raw/` has everything but `video.mp4`.
- Goal: find the shader behind the D3D12 FL 12_0 `typed_uav_load` device lost (`JM atom 3 failed: event=0x51`,
  INSTR_INVALID_ENC) and look at the logic op and gears failures.

## Results

| pass | result |
|---|---|
| D3D12 FL 12_0 | `typed_uav_load` FAIL (submit/readback failed), `kbase: JM atom 3 failed: event=0x51 core_req=0x16`, device lost |
| D3D12 FL 11_1 | `uav_vs` PASS, `logic_op` FAIL `ffffffff` (want `550ff000`), `tir` REPORTED (vkd3d-proton ignores ForcedSampleCount) |
| D3D11 FL 11_1 | `uav_vs` PASS, `logic_op` PASS, `tir` FAIL (0 PS invocations with ForcedSampleCount=4) |
| D3D12 gears | PASS 101.1 fps, red gear drawn twice |

## Findings

- **event 0x51**: the only invalid encoding in the dumps is in the typed UAV compute shader (`cs_img[id.xy] =
  cs_img[id.xy] * 2.0 + ...`, register format `auto`, the conversion descriptor computed at run time):

  ```
  *LSHIFT_OR.i32 t0, t0, 0x01000000, #0
  +ST_CVT.v4 t1, r0, r1, t(INVALID), @r7
  ```

  The ST_CVT descriptor comes from the FMA result of the same tuple (`t`, value 3), which ISA.xml masks out
  (`mask="0xf7"`). `bi_reads_t()` excludes src 2 for ST_CVT, ST_TILE and TEXC, but these read a staging register
  as IR src 0, so their descriptor is src 3. Fixed in jm-v7-wip 0139 (`pan/bi: a message descriptor behind a
  staging source is src 3`).
- **logic_op**: both APIs compile the same blend shader (`pan_blend(rt=0,fmt=PIPE_FORMAT_R8G8B8A8_UINT,
  logicop=invert)`). DXVK's pixel shader has `uses_discard: true` (cull distance `demote_if`), so forward pixel kill
  is off; vkd3d-proton's has no discard, the logic op draw forward-kills the plain draw under it and INVERT reads the
  clear value (`~0 = ffffffff`). Fixed in 0138 (`panvk: a logic op reads the destination, no forward pixel kill`).
- **gears**: one red gear drawn twice (screenshot). vkd3d-proton creates pipelines with vertex binding stride 0
  and sets the strides dynamically; the JM per-instance attribute offset used the pipeline stride. Fixed in 0137.

## Screenshots

- `screenshots/02-d3d12fl_11_1-shaders.png`, `03-d3d11fl_11_1-shaders.png`: the feature level tests.
- `screenshots/04-d3d12gears-shaders.png`: gears, red gear twice.
- The 12_0 pass lost the device before its screenshot.
