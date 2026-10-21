# A06 (SM-A065F, Mali-G52 MC2, v7) — PanProbe, bundled RC5-dev-v7

- Date: 2026-10-21, Samsung RTL via RDB.
- Driver: RC5-dev-v7 (main series, 191 patches, jm-v7 001-005), sha256 a1e68b5912f60888049fc6348ed46a843a17568eecf6ef17953667cbe9923f06.
- Result: observed by the user on the device screen. The run was not captured by the host, because the host-side run was stopped to save RTL credits. User's words: "its had same result as recorded old non merge lost code rc4 result", i.e. the same as the RC4 result recorded in the release notes (14/38 on A06).
- Conclusion: consistent with worklogs/g615-dxvk/rc4-binary-analysis.md (RC5 base = RC4 functionally). v7 improvements must come from jm-v7-wip stages A-D and v7E+.
- Next: capture per-test results in the next full device session (no run-as; logcat/external-storage results).
