# RTL SM-X846B (Mali-G1-Ultra, v14) - PanPlay 115g (2026-10-29)

Device: Tab S12+ SM-X846B, SDK 37, MT6993. APK panplay-115g (launcher 1.3.0 (44)), driver Mesa 26.3.0-devel (git-2f2a20a728) = 0143 tess + 0144 CSF logic_op.
Install ok (adb install -r, ~2 min). Override: `gameloop-override.json` (warmup 10 s / measure 30 s; the run itself used 20 s / 60 s defaults per results.txt).
First launch did container setup (~20 s). Runs: s3 03:03:44Z, s7 03:09:12Z.

## s3 (D3D11 + D3D9 cube): FAIL (D3D9)
- pass 1 d3d11 cube: PASS, frames=69793, 810.8 fps.
- pass 2 d3d9 cube (i686 WoW64): FAIL. First Present frame=0 ok, then no further progress; harness "still running 133 s after the first frame".
- **DEVICE_LOST reproduces on 115g** (same as 115e): dxvk log `DxvkSubmissionQueue: Command submission failed: VK_ERROR_DEVICE_LOST` (x4) + `Presenter: vkWaitForPresentKHR failed: VK_ERROR_DEVICE_LOST`.
- Kernel/driver side (logcat, pid 28312, 03:06:17Z, ~11 s after launch): `kbase: timeout snapshot` on all 3 CSF subqueues: seqno 386, target 390, extract 1, stream progress 0x0, 390 jobs queued, error 0x0 - i.e. the CSF queues never made progress (job timeout, no GPU fault reported). Details: `raw/s3/d3d9-devlost-evidence.txt`.
- 0143/0144 do not fix this. Release blocker stands.

## s7 (D3D12 feature levels): PASS 5/5, done ok=true
| level | result | fps |
|---|---|---|
| 11_0 | ok | 362.6 |
| 11_1 | ok (115e: logic_op FAIL) | 359.9 |
| 12_0 | ok | 370.0 |
| 12_1 | ok | 357.7 |
| 12_2 | ok | 354.5 |
0144 CSF logic_op fix verified on v14.

Note: PanProbe (another agent) held rtl.lock 08:31-08:33, so the runs started 08:33. Big logs not committed; raw = small harness logs + DEVICE_LOST evidence.
