
## PanProbe Game Loop run (v7K FTL APK)

PanProbe answers `com.google.intent.action.TEST_LOOP` (mime `application/javascript`, meta-data `com.google.test.loops=2`).

- Scenario 1: full suite with rc5, then the full suite with rc4 (rc4 may refuse v6; that shows up as FAIL results in phase 2).
- Scenario 2: rc5 only.
- `PAN_I_WANT_A_BROKEN_VULKAN_DRIVER=1` is set by default in game-loop mode.
- Outputs: `autorun.txt` and `runs/` in `/sdcard/Android/data/<pkg>/files`, and the JSON result written to the intent data URI (phase 2 includes `previousPhases`). `finish()` is called after the last phase.
- Package `dev.zenithblue.panvktest.v7kftl`, APK `/home/flux/build-out/apks/panprobe-v7K-ftl.apk`.

```
gcloud firebase test android run --type game-loop --app /home/flux/build-out/apks/panprobe-v7K-ftl.apk --scenario-numbers 1 --device model=<m>,version=<v> --timeout 30m --directories-to-pull /sdcard/Android/data/dev.zenithblue.panvktest.v7kftl/files
```
