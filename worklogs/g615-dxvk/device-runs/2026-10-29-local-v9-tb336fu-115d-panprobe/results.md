# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9 JM, MT8755), build 115d PanProbe

- Device `HNY07QHT`, `gpu_id=0x90930010`, arch v9. APK `/var/tmp/panvk/build-out/apks/panprobe-115d.apk`
  (sha256 `b56a3caa...`, rc5 slot = 115d .so `da6f9a0f`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
- Raw: `raw/runs_20261029-032239/`, `raw/autorun.txt`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115d) | Mesa 26.3.0-devel (git-b04b8ee36d) | **40/40 PASS** | 205.3 s |

- Same as 115c (40/40, 206.7 s). Longest: `tess_perf` 77.3 s, `bc_perf` 46.1 s, `submit_stress` 30.1 s,
  `large_draw` 24.9 s. `bc_decode` 0.59 s (v9 does not use the lowmem transcoder).
- No DEVICE_LOST, no `kbase: JM atom` failure in the autorun output.
