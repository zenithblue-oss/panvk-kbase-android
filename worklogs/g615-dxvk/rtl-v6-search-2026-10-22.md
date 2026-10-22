# RTL Mali v6 (G71/G72) search, 2026-10-22

Result: no G71/G72 device on Samsung RTL could be confirmed. No public source lists current RTL models.

## Earlier snapshot (repo)
- `worklogs/g615-dxvk/TEST_DEVICES.md`: signed-in RTL device list read on 2026-10-16 (1324 devices, all locations). Oldest Android on RTL is 12. "Missing on RTL: G71, G72 (v6)". The oldest Mali Exynos phones there are S20 era (Exynos 990, G77).
- `worklogs/g615-dxvk/v6-readiness-2026-10-22.md`: Exynos 7885/9610/9611/9810 phones (A8 2018, A7 2018, A50, A51, M30s, M31, M21, S9) were not in the list.
- Note: the requested path `TEST_DEVICES.md` at repo root does not exist; the file is in worklogs/g615-dxvk/.

## Web search (this session, no login, no browser)
- https://developer.samsung.com/remote-test-lab : landing page names only featured new devices (Z Fold8 SM-F971, Z Flip8 SM-F776, S26 Ultra SM-S948, Tab S11 SM-L505). It does not list the full inventory, and does not say whether the list needs login.
- https://developer.samsung.com/remotetestlab/devices : fetch returned only a skip-link; the list is a login-gated JS app.
- https://developer.samsung.com/resources/app-testing : mentions "Samsung A" device categories, no models.
- https://testfairy.com/blog/test-android-apps-on-the-latest-samsung-devices-remotely-for-free/ : 2018-era article, mentions S9/S9+ on RTL (stale; Android 12 is now the oldest).
- https://us.fitgap.com/products/samsung-remote-test-lab : third party, says "S8 to S26 Ultra", unverified and not model-level.
- Samsung news posts (Note20, S20, S21 additions) confirm only new-device additions, nothing for v6 chips.

## Candidates (not confirmed on RTL)
| Model code | Phone | SoC | GPU | Last Android |
|---|---|---|---|---|
| SM-A505F | Galaxy A50 | Exynos 9610 | G72 MP3 | 11 |
| SM-A515F | Galaxy A51 | Exynos 9611 | G72 MP3 | 13 |
| SM-M315F | Galaxy M31 | Exynos 9611 | G72 MP3 | 12 |
| SM-M215F | Galaxy M21 | Exynos 9611 | G72 MP3 | 12 |
| SM-G960F | Galaxy S9 | Exynos 9810 | G72 MP18 | 10 |
| SM-G965F | Galaxy S9+ | Exynos 9810 | G72 MP18 | 10 |
| SM-N960F | Galaxy Note9 | Exynos 9810 | G72 MP18 | 10 |
| SM-G950F / SM-G955F | Galaxy S8 / S8+ | Exynos 8895 | G71 MP20 | 9 |
| SM-A530F / SM-A730F | A8 / A8+ 2018 | Exynos 7885 | G71 MP2 | 10 |
| SM-A305F / SM-A405F | A30 / A40 | Exynos 7904 | G71 MP2 | 11 |
| SM-T505 / SM-T515 etc. | Tab A7 10.4 (2020) | Snapdragon 662, excluded | - | - |
Caveat: RTL's oldest Android is 12 (2026-10-16), so only A50 (11, no), A51, M31, M21 (up to 12/13) could plausibly exist, and the 10-16 list showed none. Phones stuck at Android 9-11 are very unlikely.

## How to check in the RTL web UI (needs the user's login)
1. Sign in at developer.samsung.com/remotetestlab, open the device list (Mobile).
2. Search/filter by model name: "A51", "A50", "M31", "M21", "S9", "Note9", "S8", "A8", then by model code (SM-A515, SM-A505, SM-M315, SM-M215, SM-G960, SM-G965, SM-N960).
3. Also filter Android version 12 and below, and check every location (India, Korea, Russia, UK, USA, Vietnam, Brazil, Poland).
4. After connecting, confirm with `getprop ro.hardware` / `ro.board.platform` (exynos9610 / exynos9611 / exynos9810) or gpu_id from the first run.

Conclusion: stay with Firebase `xcoverpro` (G72 MP3, API 33) for v6.
