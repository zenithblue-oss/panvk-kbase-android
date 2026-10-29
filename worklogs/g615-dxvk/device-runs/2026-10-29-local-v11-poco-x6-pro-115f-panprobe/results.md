# Local USB: Poco X6 Pro (Mali-G615 MC6, v11 CSF, MT6897), build 115f PanProbe

- Device `Y5WWBMJVOZSK4HU8`. APK `/var/tmp/panvk/build-out/apks/panprobe-115f.apk` (sha256 `4d6baf87...`, rc4 slot =
  RC4 .so `977c0c4c`, rc5 slot = 115f .so `62186713`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
  Screen kept awake with `KEYCODE_WAKEUP` every 5 s (no system setting changed).
- Raw: `raw/autorun.txt`, `raw/20261029-060735/`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115f) | Mesa 26.3.0-devel (git-0f038f9e96) | **40/40 PASS** | 148.2 s (115e 149.8 s) |

No DEVICE_LOST, no kbase error. 115f only changes v9 JM (jm-v7-wip 0142), so v11 is unchanged within noise.

## tess_perf, us per draw (opt=default)

| config | rc4 | 115e | 115f |
|---|---|---|---|
| gs=0 L=1 | 544.3 | 504.1 | 497.4 |
| gs=0 L=4 | 577.7 | 499.4 | 500.0 |
| gs=0 L=16 | 2832.9 | 2323.6 | 2316.1 |
| gs=0 L=mix | 2734.5 | 2316.0 | 2328.7 |
| gs=1 L=1 | 688.3 | 700.5 | 686.7 |
| gs=1 L=4 | 873.4 | 792.8 | 795.8 |
| gs=1 L=16 | 4647.7 | 4096.8 | 4127.5 |
| gs=1 L=mix | 3757.9 | 3249.8 | 3267.1 |

rc4 column: single test `--es autorun tess_perf --es driver rc4` on 115e's APK, same day.

Experiment, not shipped: polling the cross-subqueue tess syncs from the CS before the blocking SYNC_WAIT
(`PANVK_CSF_SPIN=2000`, dev build) made every tess draw about 4.7 ms (L=1), so the poll never saw the other CSG's
signal in time; dropped.
