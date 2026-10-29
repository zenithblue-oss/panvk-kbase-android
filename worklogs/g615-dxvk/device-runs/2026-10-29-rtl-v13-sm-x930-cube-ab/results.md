# RTL SM-X930 (v13 G925) cube A/B, 2026-10-29

Device: Galaxy Tab S11 Ultra SM-X930, gpu_id 0xd8300015, via rdb localhost:35395, under rtl.lock. PanPlay game loop, 10 s warm-up + 30 s measure, `gameloop_config` env override (`raw/cfg-*.json`, script `raw/run.sh`). Cell = d3d11 cube row of the scenario: drv-fps / dxvk 1%-low.

Configs: a = 121b (git-07177325ca p0163) default; b = 121b + PANVK_KBASE_TILER_IR=0; c = build 120 (git-94af3802af p0160, no 0161).
Run order: install 121b, (a,b) x2 alternating, install 120, c x2, install 121b again, a x1 (drift check). Only 2 rounds, not 3, to fit the time budget.

| run | s1 fps / 1%low | s3 fps / 1%low |
|---|---|---|
| a r1 | 576.3 / 216.7 | 584.9 / 287.3 |
| b r1 | 606.8 / 288.0 | 581.0 / 294.2 |
| a r2 | 600.3 / 301.6 | 581.9 / 289.6 |
| b r2 | 589.8 / 298.9 | 585.0 / 296.8 |
| c r1 | 606.3 / 295.1 | 575.4 / 277.2 |
| c r2 | 589.0 / 290.1 | 572.5 / 284.2 |
| a r4 (drift) | 618.4 / 308.7 | 587.4 / 289.9 |

Means: a 593 / 584 (s1 / s3, incl. r4: 598 / 585); b 598 / 583; c 598 / 574.

Thermal (dumpsys thermalservice, `raw/thermal.txt`): AP 49.2 C flat for all runs, SKIN 37.7-40.2 C, status 0 (1 right after each s1 only); no drift across the ~40 min. No throttling.

Screenshots: `shots/a.png`, `shots/b.png`, `shots/c.png` (one per config, taken ~35 s into s1).

## Verdict

0161 is NOT the cause. Build 120 (no 0161) gives the same ~590-600 fps as 121b (with 0161) on s1/s3, and TILER_IR=0 changes nothing (as on v11). The ~600 fps is therefore a floor shared by 120 and 121b: the drop from the build 118 numbers (965 / 882) came from something before 0161, i.e. a change between 118 and 120 (patches 0159/0160 or earlier, or an app/DXVK change in the build 119/120 APK). Next step: bisect 118 -> 119 -> 120 on this device. Also note the s1 cube sits near 600 fps both with and without any 0161 code, a suspiciously round cap; check 119 first.

Note: an aborted first attempt (bad loop var; `r1-a` s1 437 fps cold, the first run after install) is not included; the first run after a fresh install can be slower (shader cache cold).
