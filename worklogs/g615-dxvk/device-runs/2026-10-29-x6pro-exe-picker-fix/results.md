# X6 Pro (Y5WWBMJVOZSK4HU8) exe picker fix, 2026-10-29

APK: panplay-121b-pickfix.apk (sha256 14353a81514d290a930d3e95dd8760431c3f59e7b5a65f85b66699ece19837c3), driver 121b, installed with `adb install -r`.
Fix: ContainerManager.resolveUriToPath resolves downloads.documents `msf:<id>` docIds via MediaStore.Files `_data`.

| Root | Pick | Result |
|---|---|---|
| Downloads (downloads.documents) | Download/1DM/Compressed/Fallout4.exe | PASS: Executable=/storage/emulated/0/Download/1DM/Compressed/Fallout4.exe, Detected: x86_64 |
| Internal storage (POCO X6 Pro 5G root) | Download/Megabonk.v1.0.69.RexaGames.com/Megabonk.exe | PASS: Executable=/storage/emulated/0/Download/Megabonk.v1.0.69.RexaGames.com/Megabonk.exe, Detected: x86_64 |

Dialog cancelled, no game saved. Screenshots: 1-add-before.png (empty field, before pick), 2-downloads-after.png, 3-internal-root.png.
Pre-fix state (empty field) is the user-reported bug; not re-captured with old APK.
