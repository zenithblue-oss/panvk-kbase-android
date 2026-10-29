# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9 JM, MT8755), build 115g PanProbe

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panprobe-115g.apk` (sha256 `4ac640a7...`, rc4 slot = RC4 .so
  `977c0c4c`, rc5 slot = 115g .so `ce6635b2`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
- Raw: `raw/autorun.txt`, `raw/20261029-072726/`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115g) | Mesa 26.3.0-devel (git-2f2a20a728) | **40/40 PASS** | 187.0 s (115f 202.9 s) |

No DEVICE_LOST, no `kbase: JM atom` failure. tess_perf readback hashes equal to 115f (ref 4807f3f5, L=1 a03404c5,
L=4 cec55885, L=16 d4463b35, mix b34a251c).

## tess_perf, us per draw (opt=default, 256 draws x 16 quad patches)

| config | rc4 | 115f | 115g | 115g vs 115f |
|---|---|---|---|---|
| gs=0 L=1 | 213.8 | 155.6 | 111.5 | -28 % |
| gs=0 L=4 | 679.3 | 584.9 | **281.2** | -52 % |
| gs=0 L=16 | 3042.4 | 2899.8 | **1483.4** | -49 % |
| gs=0 L=mix | 2864.3 | 2724.9 | **1312.1** | -52 % |
| gs=1 L=1 | 562.2 | 368.3 | 299.4 | -19 % |
| gs=1 L=4 | 1984.9 | 1717.0 | 1465.1 | -15 % |
| gs=1 L=16 | FAIL (rc4 stops here) | 12016.7 | 10627.1 | -12 % |
| gs=1 L=mix | - | 6355.2 | 4904.7 | -23 % |

- 115g = jm-v7-wip 0143 (tessellator state in registers: v9 tri/quad WITH_COUNTS TLS 1824/2112 B -> 0/64 B) + 0144
  (CSF only, not used on v9).
- gs=1 at L>=4 is now bound by the GS emulation, not the tessellator.
- Other emulation tests: tessellation 3.53 -> 3.36 s, geometry 1.35 -> 1.34 s, xfb 0.30 -> 0.34 s, all PASS.
