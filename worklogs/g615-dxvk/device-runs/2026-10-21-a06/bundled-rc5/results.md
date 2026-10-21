# A06 (SM-A065F, Mali-G52 MC2, v7) — PanProbe, bundled RC5-dev-v7

- Date: 2026-10-21, Samsung RTL via RDB.
- Driver: RC5-dev-v7 (main series, 191 patches, jm-v7 001-005), sha256 a1e68b5912f60888049fc6348ed46a843a17568eecf6ef17953667cbe9923f06.
- Result: observed by the user on the device screen. The run was not captured by the host, because the host-side run was stopped to save RTL credits.
- User reports: released RC4 = 38/38 on A06; RC5-dev-v7 (bundled in the new PanProbe) = same as the older recorded result (release notes said 14/38). The exact RC5 count is to be captured next session.
- Conclusion: this contradicts the binary analysis (RC5 ~= RC4 functionally, worklogs/g615-dxvk/rc4-binary-analysis.md). The difference must come from the build (NDK r29 vs r30, meson options), the test app/APK, or how the driver was loaded. Investigation in progress.
- Next: capture per-test results in the next full device session (no run-as; logcat/external-storage results), and diff the RC4 release build against the RC5 bundled build inputs.
