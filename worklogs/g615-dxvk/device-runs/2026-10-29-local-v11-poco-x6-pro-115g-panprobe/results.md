# Local USB: Poco X6 Pro (Mali-G615 MC6, v11 CSF, MT6897), build 115g PanProbe

- Device `Y5WWBMJVOZSK4HU8`. APK `/var/tmp/panvk/build-out/apks/panprobe-115g.apk` (sha256 `4ac640a7...`, rc4 slot =
  RC4 .so `977c0c4c`, rc5 slot = 115g .so `ce6635b2`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
  Screen kept awake with `KEYCODE_WAKEUP` every 5 s (no system setting changed).
- Raw: `raw/autorun.txt`, `raw/20261029-073141/`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115g) | Mesa 26.3.0-devel (git-2f2a20a728) | **40/40 PASS** | 122.0 s (115f 148.2 s) |

No DEVICE_LOST, no kbase error. tess_perf readback hashes equal to 115f.

First attempt (07:26) stopped after 29 tests: SIGSEGV (write) in the `panvk present` thread, `memcpy` in
`panvk_android_swapchain_post()` (panvk_wsi.c CPU-copy present path, `win_buf.stride * 4` copy), during
`swapchain_lifecycle` (resize). Not GPU, no DEVICE_LOST; panvk_wsi.c is unchanged since 115f (115g only changes
tessellator.h and the CSF logic-op dirty bits). The rerun passed 40/40. Open WSI issue, not fixed here.

## tess_perf, us per draw (opt=default)

| config | rc4 | 115f | 115g | 115g vs 115f |
|---|---|---|---|---|
| gs=0 L=1 | 544.3 | 497.4 | 383.1 | -23 % |
| gs=0 L=4 | 577.7 | 500.0 | **278.6** | -44 % |
| gs=0 L=16 | 2832.9 | 2316.1 | **1141.7** | -51 % |
| gs=0 L=mix | 2734.5 | 2328.7 | **1078.3** | -54 % |
| gs=1 L=1 | 688.3 | 686.7 | 571.7 | -17 % |
| gs=1 L=4 | 873.4 | 795.8 | 559.7 | -30 % |
| gs=1 L=16 | 4647.7 | 4127.5 | **2798.6** | -32 % |
| gs=1 L=mix | 3757.9 | 3267.1 | **1936.5** | -41 % |

- v11 CSF runs `panlib_tess_count_draw` (quad COUNT variant, TLS 2208 -> 0 B) and `panlib_tess_gen` (quad TLS
  2112 -> 64 B).
- Other emulation tests: tessellation 2.85 -> 2.32 s, geometry 1.90 -> 1.60 s, xfb 0.42 -> 0.31 s, all PASS.
