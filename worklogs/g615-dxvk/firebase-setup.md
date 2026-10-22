
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

## Firebase Test Lab CLI setup (flux PRoot, arm64)

Installed 2026-10-22, no login and no project created yet.

- firebase-tools 15.34.1: `/home/flux/.local/bin/firebase` (npm prefix `/home/flux/.local`).
- Google Cloud SDK 590.0.0 (core 2026.10.16, bq 2.1.40, gsutil 5.37): `/home/flux/.local/google-cloud-sdk`, symlinked as `/home/flux/.local/bin/gcloud` and `/home/flux/.local/bin/gsutil`. Uses its bundled Python 3.14, so no system python3 is needed.
- `gcloud firebase test android` is in core (`run`, `models list`, ...).
- Add `/home/flux/.local/bin` to PATH (or call the full paths above).

Login (run by the user):

```
gcloud auth login --no-launch-browser
gcloud config set project <PROJECT_ID>
firebase login --no-localhost
```

Device list (needs the login above first):

```
gcloud firebase test android models list --filter="form=PHYSICAL"
gcloud firebase test android models list --filter="model_id=<MODEL>"
```

Example game-loop run:

```
gcloud firebase test android run --type game-loop --app <apk> --device model=<model>,version=<api> --scenario-numbers 2 --timeout 30m --directories-to-pull /sdcard/Android/data/<pkg>/files
```

Use `--scenario-numbers 2` by default. Scenario 2 runs RC5 only, which is what the user wants. Scenario 1 runs RC5 and then RC4, so use it only when RC4 is needed.
