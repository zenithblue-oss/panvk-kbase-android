# Local USB: Poco X6 Pro (Mali-G615 MC6, v11 CSF, MT6897), build 115d PanProbe

- Device `Y5WWBMJVOZSK4HU8`, `gpu_id 0xb8a31030`. APK `/var/tmp/panvk/build-out/apks/panprobe-115d.apk`
  (sha256 `b56a3caa...`, rc4 slot = RC4 .so `977c0c4c`, rc5 slot = 115d .so `da6f9a0f`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
- Raw: `raw/autorun-1.txt` (39 tests), `raw/autorun-2-swapchain.txt` (rerun of the 40th test).

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115d) | Mesa 26.3.0-devel (git-b04b8ee36d) | **40/40 PASS** (39 + `swapchain_lifecycle` rerun) | 145.1 s |

- The first run stopped at test 40 (`swapchain_lifecycle`): the phone screen went to doze and the app process
  was gone, no RESULT line, no error. Rerun of that test alone (`--es autorun swapchain_lifecycle`): PASS,
  6.2 s. Not a driver problem: the test needs a visible surface.
- BC lowmem compile time (patch jm-v7-wip 0140):

| test | rc4 | 115c | 115d |
|---|---|---|---|
| `bc_decode` (PanProbe) | 0.7 s | 15.5 s | **5.1 s** |
| `bc_perf` (PanProbe) | 29 s | 43 s | **33.1 s** |
| `bc_decode` CLI, no shader cache, 16/16 PASS | 0.68 s | 15.3 s | **4.93 s** |
| `bc_perf` CLI, warm cache, wall | | 22.48 s | 22.45 s |

- The remaining cost is the transcoder pipeline compile (about 0.25 to 0.6 s per BC mode, flat profile). With a
  warm shader cache `bc_decode` is 0.2 s.
- No DEVICE_LOST, no kbase error in the autorun output.
