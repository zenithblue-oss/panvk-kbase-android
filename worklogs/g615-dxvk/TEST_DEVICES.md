# Mali test devices (Samsung RTL, Firebase Test Lab, own)

Snapshot: 2026-10-16. Fix letters A-L are the error groups in PROGRESS.md "RC3 and RC3-lowmem cloud log findings"; "v7 bring-up" and "v6" refer to "Mali v7 (Bifrost) support" and BIFROST_V6_V7_RESEARCH.md.

How each part was checked:

- **Firebase Test Lab:** the model list is from `gcloud firebase test android models list --filter=form=PHYSICAL` (project `panvk-device-test`, 2026-10-16, 200 physical models). Model ids and API levels are exact. The catalog does not give the SoC or GPU, so those come from public spec sheets. Only the rows marked "seen" have a gpu_id from our own uploads; the rest still need a gpu_id from a first run.
- **Samsung RTL:** confirmed 2026-10-16 from the signed-in device list (`/remotetestlab/rtl/api/v1/devices`, read-only, 1324 devices in all locations including watches and TVs). See "RTL devices (confirmed 2026-10-16)" below. SoC and GPU come from public spec sheets and my own knowledge, not from RTL; rows marked "uncertain" should be checked with the gpu_id from a first run.
- No Firebase or RTL script exists in the repo yet (grep for gcloud/testlab/RTL found nothing).

## Table 1: Devices we have

| Platform | Model | Model code / FTL id | SoC | GPU | Arch | Android / API | Tests (fix items) |
|---|---|---|---|---|---|---|---|
| Own | Poco X6 Pro (dev device) | 2311DRK48G | Dimensity 8300 Ultra | Mali-G615 MC6, `0xb8a31030` (seen) | v11 | not recorded here | B (Dark Souls MMU fault, same GPU), J, all regression gates |
| Own (not attached since 2026-10-11) | Lenovo tablet | TB336FU | MediaTek (G57 MC2) | Mali-G57 MC2, `0x90930010` (seen) | v9 | vendor API 33 | C, F, v9 JM |
| RTL | Galaxy Tab S11 Ultra | SM-X930 / SM-X936B | Dimensity 9400+ | Immortalis-G925 MC12 | v13 | to confirm after login | A (G925 seen in A), E, v13 VPSA (item 23), D3D8/9 frame rate (item 29) |
| RTL | Galaxy Tab S11 | SM-X730 / SM-X736B | Dimensity 9400+ | Immortalis-G925 MC12 | v13 | to confirm after login | same as Tab S11 Ultra |
| RTL | Galaxy Tab S10 Ultra | SM-X920 / SM-X926B | Dimensity 9300+ | Immortalis-G720 MC12 | v12 | to confirm after login | A, E, L |
| RTL | Galaxy Tab S10+ | SM-X820 / SM-X826B | Dimensity 9300+ | Immortalis-G720 MC12 | v12 | to confirm after login | A, E, L |
| Firebase | Samsung Galaxy A10 | `a10` | Exynos 7884 | Mali-G71 MP2 | v6 | API 29 | v6 bring-up, G (Android 10), gpu_id check (G71 never seen) |
| Firebase | Samsung Galaxy S9 | `starlte` | Exynos 9810 | Mali-G72 MP18 | v6 | API 29 | v6 bring-up, G (Android 10), gpu_id check (G72 MP18) |
| Firebase | Samsung Galaxy XCover Pro | `xcoverpro` | Exynos 9611 | Mali-G72 MP3 | v6 | API 33 | v6 bring-up on Android 13 (no `call_once` problem) |
| Firebase | Samsung Galaxy A04s | `a04s` | Exynos 850 | Mali-G52 MC1 | v7 | API 34 | v7 bring-up, H (G52 `0x7212` row) |
| Firebase | Samsung Galaxy A13 | `a13` | Exynos 850 | Mali-G52 MC1 | v7 | API 34 | v7 bring-up, H |
| Firebase | Samsung Galaxy A14 | `a14m` | Helio G80 or Exynos 850 (depends on region) | Mali-G52 MC2 or MC1 | v7 | API 34 | v7 bring-up, H |
| Firebase | Samsung Galaxy A06 | `a06` | Helio G85 | Mali-G52 MC2 (r1) | v7 | API 35 | v7 bring-up, H (`0x7402` variant bug, `MEM_JIT_INIT`) |
| Firebase | Samsung Galaxy A31 | `a31` | Helio P65 | Mali-G52 MC2 | v7 | API 31 | v7 bring-up, H |
| Firebase | Samsung Galaxy A32 | `a32` | Helio G80 | Mali-G52 MC2 (r1) | v7 | API 33 | v7 bring-up, H (`0x7402`, `MEM_JIT_INIT`) |
| Firebase | OPPO A38 | `OP5759L1` | Helio G85 | Mali-G52 MC2 (r1) | v7 | API 34 | v7 bring-up, H (`0x7402`) |
| Firebase | Motorola moto g24 | `fogorow` | Helio G85 | Mali-G52 MC2 (r1) | v7 | API 34 | v7 bring-up, H (`0x7402`) |
| Firebase | Motorola moto g stylus (2023) | `gnevan` | Helio G85 | Mali-G52 MC2 (r1) | v7 | API 33 | v7 bring-up, H (`0x7402`) |
| Firebase | Motorola moto g05 | `lamul` | Helio G81 | Mali-G52 MC2 (r1) | v7 | API 35 | v7 bring-up, H (`0x7402`) |
| Firebase | Motorola G20 | `java` | Unisoc T700 | Mali-G52 MP2 | v7 | API 30 | v7 bring-up, gpu_id check (Unisoc G52) |
| Firebase | Samsung Galaxy Tab A8 | `gta8wifi` | Unisoc T618 | Mali-G52 MP2 | v7 | API 34 | v7 bring-up, gpu_id check (Unisoc G52) |
| Firebase | Google Pixel 6 | `oriole` | Tensor G1 | Mali-G78 MP20 | v9 | API 31, 32, 33 | C, E, v9 regression |
| Firebase | Google Pixel 6a | `bluejay` | Tensor G1 | Mali-G78 MP20 | v9 | API 32 | C, E, v9 regression |
| Firebase | Samsung Galaxy S21 5G | `o1s` | Exynos 2100 | Mali-G78 MP14 | v9 | API 35 | C (G78 MC14 on SM-S901B seen in C), E |
| Firebase | Samsung Galaxy S21 Ultra 5G | `p3s` | Exynos 2100 | Mali-G78 MP14 | v9 | API 35 | C, E |
| Firebase | Samsung Galaxy Note20 Ultra 5G | `c2s` | Exynos 990 | Mali-G77 MP11 | v9 | API 33 | C, gpu_id check (G77 MP11; only MC9 seen) |
| Firebase | Motorola edge 40 | `lyriq` | Dimensity 8020 | Mali-G77 MC9 | v9 | API 34 | C (G77 MC9 seen in C) |
| Firebase | Samsung Galaxy A13 5G | `a13x` | Dimensity 700 | Mali-G57 MC2 | v9 | API 33 | C, F |
| Firebase | Samsung Galaxy A14 5G | `a14xm` | Dimensity 700 | Mali-G57 MC2 | v9 | API 34 | C, F |
| Firebase | Samsung Galaxy A14 5G | `a14xmsq` | Dimensity 700 | Mali-G57 MC2 | v9 | API 35 | C, F |
| Firebase | Samsung Galaxy A14 5G | `a14xmtfn` | Dimensity 700 | Mali-G57 MC2 | v9 | API 35 | C, F |
| Firebase | Motorola moto g 5G (2022) | `austin` | Dimensity 700 | Mali-G57 MC2 | v9 | API 33 | C, F |
| Firebase | Samsung Galaxy A15 | `a15` | Helio G99 | Mali-G57 MC2 | v9 | API 34 | C, F |
| Firebase | Samsung Galaxy A16 | `a16` | Helio G99 | Mali-G57 MC2 | v9 | API 35 | C, F |
| Firebase | Samsung Galaxy A15 5G | `a15x` | Dimensity 6100+ | Mali-G57 MC2 | v9 | API 34, 36 | C, F |
| Firebase | Samsung Galaxy A15 5G | `a15xtfn` | Dimensity 6100+ | Mali-G57 MC2 | v9 | API 36 | C, F |
| Firebase | Samsung Galaxy XCover7 | `xcover7` | Dimensity 6100+ | Mali-G57 MC2 | v9 | API 34 | C, F |
| Firebase | Motorola moto g (2025) | `kansas` | Dimensity 6300 | Mali-G57 MC2 | v9 | API 35 | C, F |
| Firebase | OPPO A79 5G | `OP573DL1` | Dimensity 6020 | Mali-G57 MC2 | v9 | API 34 | C, F |
| Firebase | Samsung Galaxy A16 5G | `a16x` | Exynos 1330 or Dimensity 6300 (depends on region) | Mali-G68 MP2 or G57 MC2 | v9 | API 34, 35, 36 | C, E |
| Firebase | Samsung Galaxy A16 5G (EEA) | `a16xeea` | Exynos 1330 | Mali-G68 MP2 | v9 | API 35 | C, E, gpu_id check (G68 MP2) |
| Firebase | Samsung Galaxy A25 5G | `a25x` | Exynos 1280 | Mali-G68 MP4 | v9 | API 34 | C, E (G68 MC4 seen in E) |
| Firebase | Samsung Galaxy A53 5G (docomo) | `SC-53C` | Exynos 1280 | Mali-G68 MP4 | v9 | API 36 | C, E |
| Firebase | Samsung Galaxy A26 5G | `a26x` | Exynos 1380 | Mali-G68 MP5 | v9 | API 36 | C, E |
| Firebase | Samsung Galaxy A35 5G | `a35x` | Exynos 1380 | Mali-G68 MP5 | v9 | API 34, 35, 36 | C, E |
| Firebase | Samsung Galaxy A54 5G | `a54x` | Exynos 1380 | Mali-G68 MP5 | v9 | API 34 | C, E |
| Firebase | Samsung Galaxy A54 5G | `a54xue` | Exynos 1380 | Mali-G68 MP5 | v9 | API 36 | C, E |
| Firebase | Samsung Galaxy Tab S9 FE 5G | `gts9fe` | Exynos 1380 | Mali-G68 MP5 | v9 | API 34 | C, E |
| Firebase | Samsung Galaxy A34 5G | `a34x` | Dimensity 1080 | Mali-G68 MC4 | v9 | API 36 | C, E |
| Firebase | Lenovo Tab P12 | `TB370FU` | Dimensity 7050 | Mali-G68 MC4 | v9 | API 35 | C, E |
| Firebase | Infinix SMART 8 | `Infinix-X6525` | Unisoc T606 | Mali-G57 MP1 | v9 | API 33 | C, F, gpu_id check (Unisoc G57 MP1) |
| Firebase | TECNO SPARK Go 2024 | `TECNO-BG6` | Unisoc T606 | Mali-G57 MP1 | v9 | API 33 | C, F, gpu_id check |
| Firebase | TECNO SPARK 10C | `TECNO-KI5k` | Unisoc T606 | Mali-G57 MP1 | v9 | API 31 | C, F, gpu_id check |
| Firebase | Motorola moto e20 | `aruba` | Unisoc T606 | Mali-G57 MP1 | v9 | API 30 | C, F, gpu_id check |
| Firebase | Motorola moto g04 | `lion` | Unisoc T606 | Mali-G57 MP1 | v9 | API 34 | C, F, gpu_id check |
| Firebase | Motorola moto g14 | `cancun` | Unisoc T616 | Mali-G57 MP1 | v9 | API 34 | C, F, gpu_id check |
| Firebase | realme C53 | `RE58C2` | Unisoc T612 | Mali-G57 MP1 | v9 | API 35 | C, F, gpu_id check |
| Firebase | Google Pixel 7 | `panther` | Tensor G2 | Mali-G710 MP7 | v10 | API 33 | A, E, K, v10 regression |
| Firebase | Google Pixel 7 Pro | `cheetah` | Tensor G2 | Mali-G710 MP7 | v10 | API 33 | A, E, K |
| Firebase | Google Pixel 7a | `lynx` | Tensor G2 | Mali-G710 MP7 | v10 | API 33 | A, E, K |
| Firebase | Google Pixel Fold | `felix` (also `felix_camera`, API 33) | Tensor G2 | Mali-G710 MP7 | v10 | API 33, 34, 36 | A, E, K |
| Firebase | Google Pixel Tablet | `tangorpro` | Tensor G2 | Mali-G710 MP7 | v10 | API 33, 36 | A, E, K |
| Firebase | Motorola edge 40 neo | `manaus` | Dimensity 7030 | Mali-G610 MC3 (seen, 37/37 on rc1) | v10 | API 34 | A (G610 seen in A), E, v10 regression |
| Firebase | Google Pixel 8 | `shiba` | Tensor G3 | Mali-G715 MC7, `0xb8a21020` (seen) | v11 | API 34, 35, 36 | A (Pixel 8 in A), v11 regression |
| Firebase | Google Pixel 8 Pro | `husky` | Tensor G3 | Mali-G715 MC7 (seen) | v11 | API 34, 35 | A (Pixel 8 Pro in A) |
| Firebase | Google Pixel 8a | `akita` | Tensor G3 | Mali-G715 MC7 | v11 | API 34, 35 | A |
| Firebase | Google Pixel 9 | `tokay` | Tensor G4 | Mali-G715 MC7 | v11 | API 34, 35, 36 | A |
| Firebase | Google Pixel 9 Pro | `caiman` | Tensor G4 | Mali-G715 MC7 | v11 | API 34, 35 | A |
| Firebase | Google Pixel 9 Pro XL | `komodo` | Tensor G4 | Mali-G715 MC7 | v11 | API 34, 35 | A |
| Firebase | Google Pixel 9 Pro Fold | `comet` | Tensor G4 | Mali-G715 MC7 | v11 | API 34, 35, 36 | A |
| Firebase | Google Pixel 9a | `tegu` | Tensor G4 | Mali-G715 MC7 | v11 | API 35 | A |
| Firebase | Google Pixel 10a | `stallion` | Tensor G4 | Mali-G715 MC7 (one spec source) | v11 | API 36 | A |
| Firebase | Samsung Galaxy Tab S10+ | `gts10pwifi` | Dimensity 9300+ | Immortalis-G720 MC12 | v12 | API 36 | A (G720 in A), E, L |

Not Mali, left out of the table: Xclipse (AMD RDNA) Exynos models, which are Galaxy S24/S24+ (`e1s`, `e2s`, `e2sksx`, Exynos 2400), S23 FE (`r11s`, Exynos 2200), S24 FE (`r12s`, Exynos 2400e), A55 (`a55xzh`, Exynos 1480) and A56 (`a56x`, `a56xnaeea`, Exynos 1580). Also left out are Pixel 10 / 10 Pro / 10 Pro XL / 10 Pro Fold (Tensor G5, PowerVR) and Pixel 11 (`cubs`, `grizzly`, `yogi`, Tensor G6, GPU not confirmed). PowerVR MediaTek and Unisoc models (A03s, A12, A21, Tab A7 Lite, moto g54, moto g play 2023, realme RMX3231, TECNO POP 7) and every Snapdragon model are also out.

## Table 2: Per-arch coverage

| Arch | GPUs | Best RTL device | Best Firebase device | Covered? |
|---|---|---|---|---|
| v6 Bifrost 1st gen | G71, G72 | **none on RTL** (oldest Mali Exynos is S20 era) | `xcoverpro` G72 MP3 (API 33); `starlte` G72 MP18 and `a10` G71 MP2 are API 29 only | Yes (3 FTL). Only `xcoverpro` can load the ICD until the Android 10 build issue (G) is fixed |
| v7 Bifrost 2nd gen | G31, G51, G52, G76 | G52 MC2: Galaxy A14 `SM-A145F-RU1..3` (Helio G80, Android 14, Russia) or A06 `SM-A065F-RU/VN` (Helio G85, Android 14); Exynos 850 G52 MC1: Galaxy A13 `SM-A135F-IN2/IN3/IN4/VN1` (Android 13/14) or M12 `SM-M127G-IN5` (Android 13); G76: none | `a06` / `fogorow` / `OP5759L1` (Helio G85, G52 MC2 r1 `0x7402`); `a04s` / `a13` (Exynos 850, G52 MC1) | G52 yes (12 FTL). **G76: no device** |
| v9 Valhall JM | G57, G68, G77, G78 | G77 MP11: Galaxy S20 FE `SM-G780G-IN2` (Android 12) / `S20+ SM-G985F-RU3/RU4`, `S20 Ultra SM-G988B-RU1..3`, `Note20 Ultra SM-N985F-RU1/RU2` (Android 13); G78 MP14: S21 `SM-G991B` (S21 FE `SM-G990B` is Snapdragon, excluded) / S21+ `SM-G996B` / S21 Ultra `SM-G998B` (Android 12-15, India/Russia) | `o1s` (G78 MP14), `lyriq` (G77 MC9), `a15` (G57 MC2), `a54x` (G68 MP5), `oriole` (G78 MP20) | Yes (36 FTL + own G57 tablet) |
| v10 Valhall CSF | G310, G510, G610, G710 | none on RTL | `panther` (G710 MP7), `manaus` (G610 MC3) | G610/G710 yes (7 FTL). G310, G510 no |
| v11 | G615, G715 | none on RTL | `shiba` / `tokay` (G715 MC7) | G715 yes (9 FTL). G615 only on the own dev phone |
| v12 | G720, G620 | Tab S10 Ultra `SM-X920-IN1/IN2/VN2`, `SM-X926B-RU1/RU2` (Android 15) / S10+ `SM-X826B-RU1` (Android 15) (Immortalis-G720 MC12) | `gts10pwifi` (Tab S10+, G720 MC12) | G720 MC12 yes (2 RTL + 1 FTL). G720 MC7/MC8 and G620 no |
| v13 | G925, G725, G625 | Tab S11 Ultra `SM-X930_KR1..4`, `SM-X936B-RU1..3/VN1/VN2` (Android 16) / S11 `SM-X730_KR1..4`, `SM-X736B-RU1/RU2` (Android 16) (Immortalis-G925 MC12) | none | G925 yes (2 RTL). G725, G625 no |
| v14 | G1-Ultra, G1-Premium, G1-Pro | none | none | **No** |

## RTL devices (confirmed 2026-10-16)

Source: the signed-in RTL device list, all locations (India, Korea, Russia, UK, USA, Vietnam, Brazil, Poland). Oldest Android on RTL is 12, so **there is no Android 10 (API 29) device**; the API 29 test must stay on Firebase (`a10`, `starlte`). Availability is a snapshot ("A" = available, "busy" = in use or queued at the time). Some devices appear in several Android versions because RTL updates them; the table gives the range seen. Android versions of the exact pick are in the Table 2 column.

| Model | Model code (variant) | Android | Locations | SoC (GPU) | Arch | Notes |
|---|---|---|---|---|---|---|
| Galaxy Tab S11 Ultra | SM-X930 (KR), SM-X936B (RU, VN) | 16 | Korea, Russia, Vietnam | Dimensity 9400+ (Immortalis-G925 MC12) | v13 | 9 units, A |
| Galaxy Tab S11 | SM-X730 (KR), SM-X736B (RU) | 16 | Korea, Russia | Dimensity 9400+ (Immortalis-G925 MC12) | v13 | 6 units, A |
| Galaxy Tab S10 Ultra | SM-X920 (IN, VN), SM-X920B (RU), SM-X926B | 14, 15 | India, Russia, Vietnam | Dimensity 9300+ (Immortalis-G720 MC12) | v12 | 10 units, A (one VN busy) |
| Galaxy Tab S10+ | SM-X820 (IN, RU), SM-X826B | 14, 15 | India, Russia | Dimensity 9300+ (Immortalis-G720 MC12) | v12 | 5 units, A |
| Galaxy S20 FE | SM-G780G | 12, 13 | India | Exynos 990 (Mali-G77 MP11) | v9 | 2 units; the SM-G780G is the 4G Exynos variant. Only Android 12 on IN2 |
| Galaxy S20+ | SM-G985F | 13 | Russia | Exynos 990 (Mali-G77 MP11) | v9 | 2 units, A |
| Galaxy S20 Ultra | SM-G988B | 13 | Russia | Exynos 990 (Mali-G77 MP11) | v9 | 3 units, A |
| Galaxy Note20 Ultra | SM-N985F | 13 | Russia | Exynos 990 (Mali-G77 MP11) | v9 | 2 units (one busy) |
| Galaxy S21 FE | SM-G990B | 14 | India, Russia | **Snapdragon 888 (Adreno), excluded** (`/dev/kgsl-3d0`, seen 2026-10-16) | not Mali | 3 units; India session wasted |
| Galaxy S21 / S21+ / S21 Ultra (suggested G78 pick: S21 `SM-G991B`, check it is available) | SM-G991B / G996B / G998B (also G991U1, G998U1 = Snapdragon, excluded) | 12 to 15 | India, Russia | Exynos 2100 (Mali-G78 MP14) | v9 | A |
| Galaxy A32 5G | SM-A326B | 13 | Vietnam | Dimensity 720 (Mali-G57 MC3) | v9 | 2 units |
| Galaxy A24 | SM-A245F | 13, 16 | India, Poland | Helio G99 (Mali-G57 MC2) | v9 | uncertain: recalled from spec sheets |
| Galaxy A15 | SM-A155F | 14 | Russia, Vietnam | Helio G99 (Mali-G57 MC2) | v9 | A |
| Galaxy A16 (4G) | SM-A165F, SM-A165N | 14, 15, 16 | Korea, Russia, Vietnam | Helio G99 (Mali-G57 MC2) | v9 | A |
| Galaxy A17 | SM-A175F / A175N (Helio G99, uncertain), SM-A176B4 (Exynos 1330, G68 MP2, uncertain) | 15, 16 | Brazil, Korea, Russia, Vietnam | G57 MC2 or G68 MP2 | v9 | uncertain SoC |
| Galaxy A07 | SM-A075F, SM-A076M | 16 | Brazil, Russia | Helio G99 (Mali-G57 MC2), uncertain | v9 | uncertain SoC |
| Galaxy M15 | SM-M156B | 14 | Russia | Dimensity 6100+ (Mali-G57 MC2) | v9 | 3 units |
| Galaxy Tab A11 | SM-X135F | 16 | Russia | Helio G99 (Mali-G57 MC2), uncertain | v9 | 3 units |
| Galaxy A25 | SM-A256E / A256N / A256B | 14 to 16 | India, Korea, Poland, Russia, Vietnam | Exynos 1280 (Mali-G68 MP4) | v9 | A |
| Galaxy A26 | SM-A266M / A266B / A266E | 15, 16 | Brazil, India, Poland, Russia | Exynos 1380 (Mali-G68 MP5) | v9 | A |
| Galaxy A34 | SM-A346E / A346B | 13, 14, 16 | India, Poland, Russia, Vietnam | Dimensity 1080 (Mali-G68 MC4) | v9 | A |
| Galaxy A35 | SM-A356E / A356N | 14 to 16 | India, Korea, Russia, Vietnam | Exynos 1380 (Mali-G68 MP5) | v9 | A |
| Galaxy A53 | SM-A536E / A536B (A536V = Snapdragon, excluded) | 12 to 14 | India, Russia, Vietnam | Exynos 1280 (Mali-G68 MP4) | v9 | A |
| Galaxy A54 | SM-A546E / A546B | 15, 16 | Russia, UK, Vietnam | Exynos 1380 (Mali-G68 MP5) | v9 | A |
| Galaxy A33 | SM-A336E / A336B | 13, 14 | India, Russia | Exynos 1280 (Mali-G68 MP4) | v9 | A |
| Galaxy M34 / M33 | SM-M346B / SM-M336B | 14 | Russia | Exynos 1280 (Mali-G68 MP4) | v9 | M34 may be Exynos 1280; uncertain |
| Galaxy M53 | SM-M536B | 12 to 15 | Russia, UK | Dimensity 900 (Mali-G68 MC4) | v9 | A |
| Galaxy M14 | SM-M146B | 14 | Russia | Exynos 1330 (Mali-G68 MP2) | v9 | A |
| Galaxy Tab S9 FE / S9 FE+ | SM-X516B/N, SM-X616B/N | 14 to 16 | Korea, Russia, Vietnam | Exynos 1380 (Mali-G68 MP5) | v9 | A |
| Galaxy A14 | SM-A145F | 14 | Russia | Helio G80 (Mali-G52 MC2) | v7 | 3 units (one busy) |
| Galaxy A06 | SM-A065F | 14 | Russia, Vietnam | Helio G85 (Mali-G52 MC2) | v7 | 6 units, A |
| Galaxy A13 | SM-A135F (Exynos 850), SM-A137F (Dimensity 700 or Helio, uncertain) | 13, 14 | India, Russia, Vietnam | Exynos 850 (Mali-G52 MC1) for A135F | v7 | 7 units, A |
| Galaxy M12 | SM-M127G | 13 | India | Exynos 850 (Mali-G52 MC1) | v7 | 1 unit, A |

Mali devices per arch on RTL (models, not units): v13 2; v12 2; v11 0; v10 0; v9 about 27 (G77 4, G78 5, G57 about 8, G68 about 10); v7 4 (G52 only, no G76/G51/G31); v6 0; v14 0.

Excluded, not Mali (RTL has them but they do not help): Xclipse (AMD) Exynos: S22 / S22+ / S22 Ultra, S23 FE, S24 and S24+ (Exynos 2400, some regions), S24 FE, S25 FE, A55, A56, Tab S10 FE / FE+, Tab S10 Lite, and probably A57 / A37 / A27 (new Exynos with Xclipse; not verified). Snapdragon (Adreno): S23, S23+, S23 Ultra, S24 Ultra (US and some others), S25 family, S26 family, Z Flip3 to Z Flip7, Z Fold2 to Z Fold8, Tab S8 family, Tab S9 family, Tab S7+ / S7 FE, A36, A05, A23, A52s, A72, A73, M23, Z Flip. PowerVR: A04, Tab A7 Lite. Watches and TVs ignored. Variants such as S21 `G991U1` / `G998U1` are Snapdragon; the B/F/N/E suffix variants listed above are Exynos or MediaTek.

Missing on RTL: G71, G72 (v6), G76 (v7), G51/G31, any v10 (Pixel is not on RTL), G615 (v11), G620/G720 MC7-8, G725/G625, v14 Dimensity 9500, and any Android 10 or 11 device.

## Table 3: Devices we still need

| Arch / GPU | Example phones | Why | Priority |
|---|---|---|---|
| v14 G1-Ultra (MT6993, Dimensity 9500) | vivo X300 / X300 Pro (V2514/V2515), OPPO Find X9 (CPH2797, PLG110), Xiaomi/Redmi 2602EPTC0G | Confirmed main gap: neither FTL nor (as far as we know) RTL has a Dimensity 9500 device. Needed for D (`MEM_IMPORT` ENOTTY, uAPI 1.38), the swapchain, and 32-bit games. Today it relies on tester uploads only | **P1** |
| v11 G615 (second device) | Dimensity 7300 phones (G615 MC2, e.g. TECNO CM7, seen in uploads), Dimensity 8300 phones (G615 MC6) | B is G615-only, and we have just the one dev phone. Neither FTL nor the known RTL list has a G615 | P2 |
| v12 G720 MC7/MC8 | Dimensity 8400-class phones; Xiaomi 2511FPC34G (MC8, seen in uploads) | A and L were seen on G720 MC8; the RTL/FTL G720 is MC12 only | P2 |
| v7 G76 | Redmi Note 8 Pro (Helio G90T), Huawei P40 (Kirin 990), Galaxy A51 5G / A71 5G (Exynos 980, G76 MP5) | v7 bring-up on G76 (`0x7211` row). The known G76 testers are on Android 10 (G) | P2 if v7 is in scope |
| v13 G725 / G625, v12 G620, v10 G510 / G310, v14 G1-Premium / G1-Pro | no confirmed phone found; ask testers | gpu_id never seen in any upload, so the model rows are unchecked | P3 |

RTL check done: the RTL Mali list is above. Tab A9+ on RTL (SM-X216B) is Snapdragon, so it does not count.

## Notes

- **Firebase quota:** the project is on the Spark plan, so it gets 5 physical-device test runs per day. Never use retry loops. Pick one device per arch and per fix item, not all variants. Direct Access (interactive sessions) is listed as supported on many of these models, but it is not verified for the Spark plan.
- **What Firebase can run:** an instrumentation or game-loop test only. PanProbe needs a test entry point that runs the suite and writes the zip to test outputs. Real-game items (A, B, C) need a scripted PanPlay launch, which is harder; RTL (interactive) is better for those.
- **RTL credits:** the account showed 98 credits on 2026-10-16 (see the credit log below); a session costs 2 credits per 30 minutes (memory). Public third-party sources say 1 credit = 15 minutes, 30 minutes is the minimum reservation, and members get a daily credit allowance (older sources say 20 a day). Samsung's own FAQ is behind the login; confirm there. The user starts the session and gives access.
- **Android 10 (API 29):** the current ICD imports `call_once` (bionic API 30+), so `dlopen` fails on Android 10 (group G). On Firebase this affects `a10` (G71) and `starlte` (G72 MP18). Use `xcoverpro` (API 33) for v6 until the API 29 build is done. Every other Mali device in Table 1 is API 30 or newer.
- **RTL baseline picks (2026-10-16):** correction: RTL Galaxy A14 `SM-A145F` is Exynos 850 (G52 MC1, gpu_id `0x72120000`), not Helio G80. For G52 MC2 r1 (`0x74021000`) try the A06 `SM-A065F` (Helio G85). Old pick: G52 MC2 = Galaxy A14 `SM-A145F-RU2` (Android 14, wrong SoC); Exynos 850 = Galaxy A13 `SM-A135F-IN2` (Android 13); G77 = Galaxy S20+ `SM-G985F-RU3` (Android 13); G78 = Galaxy S21 FE `SM-G990B-IN3` is Snapdragon, excluded; suggest S21 `SM-G991B` (Exynos 2100) if available; G720 = Tab S10 Ultra `SM-X926B-RU1` (Android 15); G925 = Tab S11 Ultra `SM-X930_KR1` (Android 16). No RTL device for G72, G76 or Android 10.
- **Variants that depend on the region** (`a14m`, `a16x`) may have either SoC; read the gpu_id from the first run before using them for a specific fix.

## RTL credit log

Start: 98 credits (2026-10-16). Cost: 2 credits per 30 minutes. Update this table after every session.

| date | device | arch | purpose | credits | remaining |
|---|---|---|---|---|---|
| 2026-10-16 | Galaxy S20+ SM-G985F-RU3 (Russia) | v9 G77 | rc3-lowmem PanProbe baseline | 2 | 96 |
| 2026-10-16 | Galaxy A14 SM-A145F-RU2 | v7 G52 MC2 | reservation click, status unknown (browser froze), to confirm | 2? | 94? |
| 2026-10-16 | Galaxy S21 FE SM-G990B (India) | NOT Mali: Snapdragon 888 SM8350 (/dev/kgsl-3d0) | wasted session | 2 | (remaining unknown, user to confirm) |
| 2026-10-16 | Galaxy S20+ SM-G985F (Russia), 2nd session | v9 G77 | v9fix driver test | 2 | (user to confirm) |
| 2026-10-16 | Galaxy A14 SM-A145F (RTL) | v7 G52 (Exynos 850 s5e3830, NOT Helio) | jm-v7 baseline | 2 | (user to confirm) |
| 2026-10-16 | Galaxy A06 SM-A065F (RTL) | v7 G52 MC2 r1 (Helio G85 mt6768) | jm-v7 baseline | 2 | (user to confirm) |

## Baseline results 2026-10-16

| Device | SoC / GPU | OS | Result | Heap / ion | Results path |
|---|---|---|---|---|---|
| Galaxy S20+ SM-G985F | Exynos 990, Mali-G77 MP11 (v9) | Android 13, kernel 4.19.87 | PanProbe 1.2.7 rc3-lowmem 38/38 PASS | no `/dev/dma_heap`; `/dev/ion` present, `crw-rw-rw- u:object_r:ion_device:s0` (C fix path applies) | `/var/tmp/panvk/rtl/g985-rc3lm/` |
| Galaxy S20+ SM-G985F | Exynos 990, Mali-G77 MP11 (v9) | Android 13, kernel 4.19.87 | PanProbe dev v9fix (driver BuildID `de4827a8`, jm-v7 001-005, API 29) 38/38 PASS. PanPlay dev v9fix: built-in cube D3D11 arm64ec PASS 1449 frames; 32-bit `dxcube-i686` D3D9 PASS 1393 frames, zero "placed map" errors | logcat: `kbase: using ION system heap, mask 0x1`; `stock JM atom layout (stride 72 refused), stride 56` | `/var/tmp/panvk/rtl/g985-v9fix/` |
| Galaxy A14 SM-A145F | Exynos 850 (s5e3830), Mali-G52 MC1 `0x72120000` (v7) | Android 14, kernel 5.10.223, `/dev/dma_heap` present | PanProbe dev v9fix 14 pass / 19 fail / 5 skip of 38. Pass: gpu_prerast_slice bc_decode bachata_exec blend descriptor_model shader_arith depth_stencil sampler submit_stress csf_event bachata_storage_fmtless bachata_dynamic_render vkd3d_timeline swapchain_lifecycle. Fails are missing features (geometry, tessellation, xfb, multiViewport, fillMode, clip/cull, robustness2, descriptorIndexing, occlusion/pipeline stats queries, depth_bounds, large_draw); no DEVICE_LOST, no GPU fault | logcat: `v7 JM, KBASE_IOCTL_MEM_JIT_INIT skipped; tiler heap is a GROW_ON_GPF region`; `Bifrost JM atom stride 56 (base_jd_atom_v2)`; `v7 growable BO, GROW_ON_GPF, 256 of 35157 pages committed` | `/var/tmp/panvk/rtl/a145-v9fix/` |
| A06 SM-A065F | v7 G52 r1 gpu_id 0x74021000 variant 0x2 | Android 14, kernel 4.19.191, no `/dev/dma_heap`, `/dev/ion` present, uAPI 11.38 | PanProbe dev v9fix 14 pass / 19 fail / 5 skip of 38 (same passing set as A14) | logcat: `gpu_id 0x74021000 variant 0x2 arch v7 model Mali-G52 r1`, `using ION system heap`, JIT skipped, stride 56; no DEVICE_LOST or GPU fault; 36x `E MESA: u_gralloc_get_buffer_basic_info failed` (fallback gralloc, harmless so far) | `/var/tmp/panvk/rtl/a065-v9fix/` |

## How RTL runs work now

- The user starts the RTL session and connects Remote Debug Bridge. Claude uses adb only: RDB binary `~/rtl-rdb/rdb`, then `adb connect localhost:<port>`.
- PanProbe autorun: `adb shell am start -S -n dev.zenithblue.panvktest/.MainActivity --es driver bundled --es autorun all`. Read `files/autorun.txt` via `run-as`; it ends with an `AUTORUN DONE` line.
- APK install over RDB takes about 22 s for 42 MB.
- Next test APKs: `/var/tmp/panvk/rtl/PanProbe-dev-v9fix.apk` and `/var/tmp/panvk/rtl/PanPlay-dev-v9fix.apk` (driver BuildID `de4827a8`, jm-v7 001-005, API 29).
- PanPlay built-in test: `am start -n dev.zenithblue.panvklauncher/.MainActivity --es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT "Cube · D3D9 · x86"` (needs imagefs/Wine/FEX downloaded first).
- PanPlay note (2026-10-16): the launch extra needs the shortcut id, e.g. `--es dev.zenithblue.panvklauncher.LAUNCH_SHORTCUT builtin-d3d9-i686`. The display name did not match and ran the arm64ec d3d11 cube instead. First launch auto-installs the bundled rootfs. Run logs are in `files/logs/run-*.log` (via `run-as`). PanPlay APK is 393 MB and installs over RDB in about 4.5 min.
- Correction: adb serial `d30a1726` (CPH2691) is a Snapdragon 8 Gen 3 phone, not a Mali/G615 device.
