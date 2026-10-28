# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9 JM, MT8755), build 115e PanProbe

- Device `HNY07QHT`, `gpu_id=0x90930010`, arch v9. APK `/var/tmp/panvk/build-out/apks/panprobe-115e.apk`
  (sha256 `4f651d08...`, rc4 slot = RC4 .so `977c0c4c`, rc5 slot = 115e .so `8dea43cf`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
- Raw: `raw/20261029-042933/`, `raw/autorun.txt`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115e) | Mesa 26.3.0-devel (git-e7d836d5a6) | **40/40 PASS** | about 205 s |

- Same as 115d (40/40, 205.3 s). Longest: `tess_perf` 77.7 s (77.3), `bc_perf` 46.4 s (46.1), `submit_stress` 30.1 s,
  `large_draw` 24.7 s. `bc_decode` 0.60 s. No DEVICE_LOST, no `kbase: JM atom` failure.
