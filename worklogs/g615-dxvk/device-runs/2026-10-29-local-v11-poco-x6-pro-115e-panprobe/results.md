# Local USB: Poco X6 Pro (Mali-G615 MC6, v11 CSF, MT6897), build 115e PanProbe

- Device `Y5WWBMJVOZSK4HU8`, `gpu_id 0xb8a31030`. APK `/var/tmp/panvk/build-out/apks/panprobe-115e.apk`
  (sha256 `4f651d08...`, rc4 slot = RC4 .so `977c0c4c`, rc5 slot = 115e .so `8dea43cf`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
  Screen kept awake by sending `KEYCODE_WAKEUP` every 10 s during the run (no system setting changed).
- Raw: `raw/autorun.txt`, `raw/20261029-043308/`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115e) | Mesa 26.3.0-devel (git-e7d836d5a6) | **40/40 PASS** in one run (no doze stop) | 149.8 s |

- 115d: 39 tests in the first run + `swapchain_lifecycle` rerun (138.9 s for 39). Same results; `bc_decode` 5.1 s (115d 5.1 s),
  `bc_perf` 34.8 s (33.1 s). `swapchain_lifecycle` PASS 9.2 s.
- SPARSE_KBASE pass=18 fail=0. No DEVICE_LOST, no kbase error.
- 115e adds only jm-v7-wip 0141 (v6/v7 descriptor limits), so no change expected on v11.
