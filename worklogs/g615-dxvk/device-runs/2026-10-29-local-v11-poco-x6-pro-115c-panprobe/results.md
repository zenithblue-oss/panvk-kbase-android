# Local USB: Poco X6 Pro (duchamp, MT6897, Mali-G615 MC6, v11 CSF), build 115c PanProbe

- Device `Y5WWBMJVOZSK4HU8`, Android 16, sysfs `Mali-G615 6 cores r1p3 0xB8A3`. kbase CSF uAPI 1.21, `gpu_id=0xb8a31030`, arch v11. User allowed testing on this device this time.
- APK `panprobe-115c.apk` (1.3.0 vc19), `adb install -r` OK. Launched with `--ei scenario 1` (rc5 then rc4).
- logcat is not readable on this device (`logcat: failed to get the readable ... log sizes`), so results come from the app's `autorun.txt` and run dirs only.
- Raw: `raw/runs_20261029-021519` (rc5), `raw/runs_20261029-021751` (rc4), `raw/autorun.txt`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115c) | Mesa 26.3.0-devel (git-30c29c0175) | **40/40 PASS** | 168.3 s |
| rc4 | PanVK-kbase RC4 (git-5a07217f03) | 40/40 PASS | 145.1 s |

- `sparse_kbase` (informational): PASS, 18/18, `VERDICT v11 lazy-tile possible` (CSF 1.21).
- Compliance (dxvk, bachata_s4, vkd3d, d3d_fl): all pass, no missing hard items. Soft gaps as before: tiled resources tier 2, ROV/fragment shader interlock, shaderFloat64.
- 115c vs rc4: `bc_decode` 15537 ms vs 687 ms (known bc_decode lowmem regression, see commit be143edc), `bc_perf` 43.3 s vs 28.7 s (slower on 115c), `tess_perf` 44.6 s vs 48.5 s. All still PASS. This is why 115c total is 23 s above rc4.
- Reference (build 115, shiba v11, different SoC): 39/39, 187.4 s. Here 40/40, 168.3 s.
- PanPlay could not be installed on this device (see `../2026-10-29-local-115c-summary.md`).
