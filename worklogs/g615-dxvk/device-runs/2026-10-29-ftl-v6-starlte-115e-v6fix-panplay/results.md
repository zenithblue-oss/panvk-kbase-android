# FTL starlte (Galaxy S9 SM-G960F, Mali-G72 MP18, v6 JM, Android 10) build 115e + v6 fixes: PanPlay scenarios 1-9

- Project `abhay-byte` (phone-finder-hub gave TEST_QUOTA_EXCEEDED at validation, phonefinder-hub 403), matrix `7516381515231077159`,
  FTL outcome `Passed starlte-29-en-portrait`, test time 1558 s. APK = PanPlay 1.3.0 vc 44 + commits ac1f84d6 and 974fa3dd, driver .so sha256 8dea43cf.
- Fixes, both launcher-side (no driver change). Previous run (2026-10-29-ftl-v6-starlte-115e-panplay) never got a Wine prefix.

## Root causes
1. `wine wineboot -u` hung 5 min then `could not load kernel32.dll, c0000135`: aarch64 wineboot.exe loops
   `f = CNTFRQ_EL0; do { x = f; f <<= 1 } while (x < 1e9)`. Exynos 9810 (kernel 4.9) leaves CNTFRQ_EL0 at 0, so it never ends.
   The SELinux `ioctl 0x7201` denials were noise, not the cause. Fix ac1f84d6: NOP the back-branch in setup (idempotent).
2. After the prefix worked, DXVK Presenter failed: `Failed to query surface formats: VK_ERROR_OUT_OF_HOST_MEMORY`, cube `Present hr=0x80004005`.
   logcat: `wsi/x11: dlopen(libxshmfence.so.1) failed: cannot locate symbol "memfd_create"`. Bionic exports `memfd_create` only from API 30.
   Fix 974fa3dd: `libmemfdshim.so` (syscall wrapper) LD_PRELOADed into Wine on SDK < 30.

## Results (drv-fps; vs a31 v7 115d in brackets)

| # | test | result | fps | a31 v7 |
|---|------|--------|-----|--------|
| 1 | d3d11 cube | ok PASS | 98.9 | 68.1 |
| 2 | d3d9 cube | ok PASS | 60.0 | 36.3 |
| 3 | d3d11 / d3d9 | d3d11 ok 99.0; d3d9 **FAIL** `still running 93 s after the first frame` | 99.0 / - | 67.3 / 36.4 |
| 4 | d3d11 old / default | ok | 107.0 / 100.2 | 54.5 / 67.7 |
| 4 | d3d9 old / default | ok | 60.0 / 60.0 | 36.1 / 36.4 |
| 5 | D3D12 SquidRoom | ok `DX12: PASS` 77 frames / 48 s | 1.5 | 1.3 (harness fail) |
| 6 | D3D12 gears | ok PASS | 311.3 | 110.0 |
| 7 | D3D12 FL 11_0 / 11_1 | ok PASS / PASS (uav_vs, logic_op PASS) | 447.5 / 443.6 | 191.8 / UNSUPPORTED |
| 7 | D3D12 FL 12_0 / 12_1 / 12_2 | UNSUPPORTED (tiled_tier2, rov+conservative, mesh/dxr/vrs) | ~430-450 | UNSUPPORTED |
| 8 | D3D11 FL 10_0 / 10_1 / 11_0 / 11_1 | ok PASS | 100.0 / 99.6 / 100.7 / 100.0 | 96.9-99.4 |
| 8 | D3D10 / D3D9 cube | ok PASS | 100.5 / 60.0 | 66.7 / 36.3 |
| 9 | fl-quick d3d12 (highest 11_1) / d3d11 (11_1) | ok | 419.7 / 98.1 | 192.5 (highest 11_0) / 97.5 |

No DEVICE_LOST. Differences vs a31: d3d12 FL 11_1 is created on v6 (a31 reports highest 11_0); a31 numbers are v7 on a weaker GPU/CPU.

## Open
- Scenario 3 second pass (d3d9 WoW64 cube after d3d11 in the same scenario) hung: d3d9 log ends at DXVK compiler-thread line, harness killed it at 93 s.
  d3d9 passes alone in scenarios 2, 4 and 8 on this device, so likely a pass-to-pass flake (not repeated: one matrix per Firebase quota).
- d3d12 FL 12_x UNSUPPORTED is expected on JM.
- Local regression, Lenovo TB336FU (v9, same APK): scenario 3 d3d11 161.9 fps PASS; scenario 5 d3d12 SquidRoom 23.0 fps PASS. Another agent was using the
  tablet in parallel (my `install -r` and its s7 runs overlapped), so scenario 3's d3d9 pass was not read.

`raw/` has per-scenario results.txt, harness.log and the gcloud log.
