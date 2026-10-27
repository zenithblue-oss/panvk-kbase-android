# FTL panther (Pixel 7, Mali-G710, v10) build 115b diag4: D3D11 cube hang, wait trace with 8-byte aligned cells

- Matrix `matrix-1e9wvq2ishpx0` (project `phone-finder-hub`); outcome `Passed panther-33-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra driver `d4` = `/home/flux/build-out/2026-10-27-115b-diag4/libvulkan_panfrost.so`
  (sha256 `691459164666734d...`, `git-3d82a178`): 115 + wait trace with one 32-bit value per 8-byte cell (diag3's
  `SYNC_SET32` at +4/+12 was CS fatal 0x49), `PANVK_KBASE_V10_ITER_SYNC` (FRAG ring wrapper selects the endpoint
  iterator the subqueue context names), JM IDVS local dependency, gralloc dma-buf fallback.
- `gameloop-override.json`: scenario 1 = D3D11 cube under 5 env sets, scenario 2 = D3D12 gears under 2.
  `raw/` has everything but `video.mp4`.

## Result: every D3D11 pass hangs; the trace shows a three-way wait cycle

| # | env set | frames before the hang | queue timeout |
|---|---|---|---|
| 1 | d4-plain (no trace) | 2778 | VT seqno 15137, target 15138 |
| 2 | d4-iter (`PANVK_KBASE_V10_ITER_SYNC=1`) | first frames | VT seqno 82 |
| 3 | d4 (wait trace) | 450 | VT seqno 2607 |
| 4 | d4-norn (`PANVK_KBASE_HEAP_RENEW_INTERVAL=0`) | 1123 | VT seqno 6472 |
| 5 | d4-iter2 (repeat of 2) | first frames | VT seqno 1407 |

D3D12 gears (scenario 2) ran its full pass under d4-plain and d4-iter.

Pass 3, wait trace at the 10 s queue timeout (the trace is valid now: no CS fatal):

```
subqueue 0 (VT):      in sync64 wait at site 1023: syncobjs+0x10 > 6260, memory 6259
subqueue 1 (FRAG):    in sync64 wait at site 7118: syncobjs+0x0  > 4690, memory 4690
subqueue 2 (COMPUTE): in sync64 wait at site 1023: syncobjs+0x10 > 6260, memory 6259
```

Site 1023 is the barrier's cross-subqueue wait (`panvk_vX_cmd_buffer.c`): VT and COMPUTE wait for FRAG's syncobj
(+0x10). Site 7118 is `wait_finish_tiling` (`panvk_vX_cmd_draw.c`): FRAG waits for VT's syncobj (+0x0) to pass the
tiling of the render pass it is about to run. VT emitted that tiling-done signal as a deferred `SYNC_ADD64` (on its
`DEFERRED_SYNC` scoreboard slot, waiting for the tiler iterator) before it entered the barrier wait, and the signal
never landed. No CS error, 0-1 CSF notifications at the timeout; the `CS_EVENT_SIGNAL` nudge does not help.

- Heap renewal off (pass 4) still hangs, and the heaps were fresh at every hang: not a tiler heap OOM.
- `PANVK_KBASE_V10_ITER_SYNC` hangs within the first frames: it does not fix the cycle.
- Candidate tested in diag5: before a stream blocks in a cross-subqueue `SYNC_WAIT`, wait for its own
  `DEFERRED_SYNC` slot (`../2026-10-27-ftl-v10-panther-115b-diag5-panplay/`).

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/02-d3d12gears-d4-iter-harness.png`: gears under d4-iter (renders).
