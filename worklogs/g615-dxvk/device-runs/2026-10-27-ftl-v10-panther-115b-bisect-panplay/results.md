# FTL panther (Pixel 7, Mali-G710, v10) build 115b bisect: PanPlay D3D11 cube under 12 env sets

- Matrix `7793327968189859191` (`matrix-2433mmg83ncq3`); outcome `Passed panther-33-en-portrait --`; results `gs://test-lab-r9iqhnhjf96wc-h6in9q007ukk4/panvk-v10-panther-115b-bisect-panplay/` (project `panvk-device-test`).
- APK `panplay-115.apk` (vc 41, bundled driver build 115 `73709644...`, `git-9dde9e7a18`). `gameloop-override.json` (pushed with `--other-files`): 5 s warm-up, 12 s measured, 3 s tail, scenario 1 = D3D11 cube under 12 env sets.
- Extra drivers: `dg` = `/home/flux/build-out/2026-10-27-115b-diag/libvulkan_panfrost.so` (sha256 `ddbb19bc...`, `git-b4a97d74dc`: 115 + wait trace, CSF event drain and CS_EVENT_SIGNAL nudge at the queue timeout, `PANVK_KBASE_V10_ITER_SYNC`); `r114` = build 114 with the fps log (`git-3071c6dfa2`).
- `raw/` has everything but `video.mp4`.

## Result: the hang is in 114 too; the diag driver never got past its first submit

| # | env set | result | frames before the hang |
|---|---|---|---|
| 1 | rel (115, defaults) | **hang**: `timeout on subqueue 0: seqno 8152, target 8153 ... active 1, error 0x0` | 1631 in 2.84 s (574 fps) |
| 2-11 | dg, dg-iter, dg-old, dg-zs, dg-pbt, dg-nocrc, dg-flush, dg-eager, dg-nowb, dg-semw | **invalid run**: CSF groups 0, 1, 2 `fatal error: status 0x00000049` at the first submit | 0 |
| 12 | r114 (114) | **hang**: `timeout on subqueue 0: seqno 6572, target 6573 ... active 1, error 0x0` | 1315 in 11.5 s (114 fps) |

- **114 hangs the same way.** The r114 harness line says `CUBE: PASS frames=1315` only because its last Present blocked
  10 s (`present_ms=193.53/10002.25`) and the cube's 20 s timer ran out; the 10 s queue timeout then fired at exit. Its
  dump matches 115: all three subqueues inside the CALL of one ring entry, VT stream size 0x928 (the DXVK present + HUD
  command buffer), no CS error. So the hang is not a 115 regression and the 115-vs-114 knob bisect is moot.
- **The diag driver was broken on v10.** Exception 0x49 is `CS_INVALID_INSTRUCTION`. The only new instruction in the
  diag's command streams is `SYNC_SET64` (wait trace, written with `PANVK_KBASE_WAIT_TRACE=1`, set in every `dg*` pass);
  nothing else in the tree emits it, and the G710 firmware rejects it. diag3 writes the trace with 32-bit `SYNC_SET`s
  only (`../2026-10-27-ftl-v10-panther-115b-diag3-panplay/`).
- Frames before the hang across all panther runs so far: 867 (D3D11, build 115 run), 536 (gears), 1597 (D3D9 at 90 fps),
  1631 (rel here), 1315 (114 here). SquidRoom ran 7548 frames without one. Not a fixed count or time: a per-frame race
  whose rate depends on the workload.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/02-dg-harness.png`: harness screenshot of a `dg` pass (CS fatal at the first submit).
- `screenshots/12-r114-harness.png`: harness screenshot of the 114 pass.
