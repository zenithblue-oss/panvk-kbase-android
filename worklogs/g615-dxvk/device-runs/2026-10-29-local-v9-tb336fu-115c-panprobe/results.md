# Local USB: Lenovo TB336FU (Mali-G57 MC2, v9 JM, MT8755), build 115c PanProbe

- Device `HNY07QHT`, `ro.soc.model=MT8755` (board property says mt6835), Android 16. kbase JM uAPI 11.38, `gpu_id=0x90930010`, arch v9, Mali-G57 MC2.
- APK `panprobe-115c.apk` (1.3.0 vc19), `adb install -r` OK. Launched with `am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvktest/.MainActivity --ei scenario 1` (rc5 then rc4).
- Raw: `raw/runs_20261029-021338` (rc5), `raw/runs_20261029-021901` (rc4), `raw/autorun.txt`.

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115c) | Mesa 26.3.0-devel (git-30c29c0175) | **40/40 PASS** | 206.7 s |
| rc4 | PanVK-kbase RC4 (git-5a07217f03) | 38/40, FAIL `tess_perf`, `large_draw` | 304.5 s |

- 115c: no failures. Longest: `tess_perf` 78.4 s, `bc_perf` 46.3 s, `submit_stress` 30.1 s, `large_draw` 24.7 s.
- rc4 failures are the known old-driver bugs, not 115c: `tess_perf` has `MESA: error: kbase: JM atom 175/176/177 failed: event=0x60` then `FAIL Wait r=-4`; `large_draw` fan/indirect and base-instance cases give wrong primitive counts. Both pass on 115c.
- rc4 adds about 4 s per test (JM warm-up), which is why its total is higher despite `tess_perf` failing early.
- Reference (build 115, oriole v9, different SoC): 39/39, 214.0 s. 115c is 40/40, 206.7 s.
