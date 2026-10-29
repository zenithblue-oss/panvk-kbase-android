# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9 JM, MT8755), build 115f PanProbe

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panprobe-115f.apk` (sha256 `4d6baf87...`, rc4 slot = RC4 .so
  `977c0c4c`, rc5 slot = 115f .so `62186713`), `adb install -r` OK.
- Launch: `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 2` (rc5 only).
- Raw: `raw/autorun.txt`, `raw/20261029-060829/`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115f) | Mesa 26.3.0-devel (git-0f038f9e96) | **40/40 PASS** | 202.9 s (115e 206.0 s) |

No DEVICE_LOST, no `kbase: JM atom` failure. tess_perf readback hashes equal to 115e.

## tess_perf, us per draw (opt=default, 256 draws x 16 quad patches)

| config | rc4 | 115e | 115f | 115f vs 115e |
|---|---|---|---|---|
| gs=0 L=ref | 14.6 | 13.3 | 8.0 | (no tess) |
| gs=0 L=1 | 213.8 | 217.5 | **155.6** | -28 % |
| gs=0 L=4 | 679.3 | 610.3 | 584.9 | -4 % |
| gs=0 L=16 | 3042.4 | 3013.7 | 2899.8 | -4 % |
| gs=0 L=mix | 2864.3 | 2728.6 | 2724.9 | 0 % |
| gs=1 L=1 | 562.2 | 532.3 | **368.3** | -31 % |
| gs=1 L=4 | 1984.9 | 2027.0 | **1717.0** | -15 % |
| gs=1 L=16 | FAIL (rc4 stops here) | 12526.1 | 12016.7 | -4 % |
| gs=1 L=mix | - | 6593.7 | 6355.2 | -4 % |

- rc4 column: `PanProbe` single test `--es autorun tess_perf --es driver rc4` on 115e's APK, same day.
- In-process A/B on a dev build of the same patch (imported driver): `PANVK_V9_TESS_EXACT=0` gs=0 L=1 194.8,
  gs=1 L=1 568.6; default 148.8 / 360.6.
- L>=4 is bound by the serial tessellator (one thread per patch, about 2 KiB of stack per thread), not the job chain.
- Other emulation tests: tessellation 3.50 -> 3.53 s, geometry 1.36 -> 1.35 s, xfb 0.34 -> 0.30 s, all PASS.
