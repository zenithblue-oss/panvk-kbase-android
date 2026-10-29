# Local USB: Poco X6 Pro (Mali-G615 MC6, v11 CSF), logic_op CSF fix (jm-v7-wip 0144) PanProbe

- Device `Y5WWBMJVOZSK4HU8`. APK `/var/tmp/panvk/build-out/apks/panprobe-lop.apk` (`build-lop.sh`, sha256
  `cc1721df...`; rc5 slot = `p115f-final` + 0144, .so sha256 `b0d63069...`, BuildID `8e82ce80...`), `adb install -r`.
- Launch `am start -a com.google.intent.action.TEST_LOOP ... --ei scenario 2` (rc5 only), `KEYCODE_WAKEUP` every 5 s.
  Afterwards `panprobe-115f.apk` reinstalled (`install -r`).

| slot | driverInfo | result | sum of test times |
|---|---|---|---|
| rc5 (115f + 0144) | Mesa 26.3.0-devel (git-0f038f9e96) (build dir configured before the commit) | **40/40 PASS** | 156.2 s (115f 148.2 s) |

No DEVICE_LOST. Raw `raw/autorun.txt`.

Logic op repro `tests/logic-op` (`lop <so>`, 4 cases x 20 iterations), same device:
- 115f: `loadop-clear` and `clearimage-load` FAIL 20/20, center ffffffff (want 550ff000); LOP-FAIL 2/4.
- 115f + 0144: LOP-PASS 4/4, center 550ff000.
