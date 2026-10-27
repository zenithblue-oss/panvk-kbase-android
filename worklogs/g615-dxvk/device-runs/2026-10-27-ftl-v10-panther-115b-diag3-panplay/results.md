# FTL panther (Pixel 7, Mali-G710, v10) build 115b diag3: PanPlay D3D11 cube, wait trace with 32-bit SYNC_SETs

- Matrix `matrix-1gg98lykbxe1u` (project `phone-finder-hub`); outcome `Passed panther-33-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra driver `d3` = `/home/flux/build-out/2026-10-27-115b-diag3/libvulkan_panfrost.so`
  (sha256 `ffbbaa7604e0b3cf...`, `git-ab1a4f65d9`): 115 + wait trace written with `SYNC_SET32` only, desc ringbuf wait
  traced, heap state + CSF event drain + `CS_EVENT_SIGNAL` nudge at the 10 s queue timeout.
- `gameloop-override.json`: scenario 1 = D3D11 cube under 6 env sets. `raw/` has everything but `video.mp4`.

## Result: the hang reproduced without the trace; every traced pass hit CS fatal 0x49 again

| # | env set | result |
|---|---|---|
| 1 | d3-plain (no trace) | **hang** after 2827 frames (572 fps): `timeout on subqueue 0: seqno 14697, target 14699 ... active 1, error 0x0` |
| 2-6 | d3, d3-iter, d3-rn16, d3-norn, d3-ch1600 (trace on) | **invalid**: CSF groups 0, 1, 2 `fatal error: status 0x00000049` at the first submit |

- d3-plain dump: all three subqueues sit on the `CALL` of one ring entry (VT stream `0x5ffe7d0000`/0x928, FRAG
  `0x5ffe7d1000`/0x460), no CS error, 0 CSF notifications at the timeout, the nudge does not help (1 notification of
  type 0 after it). Heap: 840 renewals, 0 tiler submits since the last one, 1 of 4 slots retired.
- 0x49 again: diag3 placed its 32-bit cells at +4/+12 of the trace slot. Together with diag (a `SYNC_SET64` at +8)
  this gives the rule: a sync object address must be 8-byte aligned for `SYNC_*32` and 16-byte aligned for
  `SYNC_*64` (the size of `panvk_cs_sync32`/`panvk_cs_sync64`). diag4 uses one 32-bit value per 8-byte cell
  (`../2026-10-27-ftl-v10-panther-115b-diag4-panplay/`).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/0N-d3d11-<set>-harness.png`: harness screenshots of the traced passes (white X window, CS fatal).
