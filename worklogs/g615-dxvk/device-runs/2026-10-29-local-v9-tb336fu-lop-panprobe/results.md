# Local USB: TB336FU (Mali-G57 MC2, v9 JM), logic_op CSF fix (jm-v7-wip 0144) PanProbe

- Device `HNY07QHT`. APK `/var/tmp/panvk/build-out/apks/panprobe-lop.apk` (rc5 slot = `p115f-final` + 0144, .so
  sha256 `b0d63069...`), `adb install -r`, scenario 2 (rc5 only), `panprobe-115f.apk` reinstalled afterwards.

| slot | result | sum of test times |
|---|---|---|
| rc5 (115f + 0144) | **40/40 PASS** | 218.0 s (115f 202.9 s; 0144 does not touch v9 JM) |

No DEVICE_LOST. Raw `raw/autorun.txt`. `tests/logic-op` LOP-PASS 4/4 on both 115f and 115f + 0144.
