# FTL panther (Pixel 7, Mali-G710, v10) build 115b diag5: drain own deferred syncs before a cross-subqueue wait

- Matrix `matrix-lsi0s2x6yesia` (project `phone-finder-hub`); outcome `Passed panther-33-en-portrait`.
- APK `panplay-115.apk` (vc 41). Extra driver `d5` = `/home/flux/build-out/2026-10-27-115b-diag5/libvulkan_panfrost.so`
  (sha256 `9bc9340c38cd3f38...`, `git-c5cfb070`): diag4 + `PANVK_KBASE_DRAIN_BEFORE_WAIT=1` (`cs_wait_slot(DEFERRED_SYNC)`
  before every instrumented sync wait and the desc ringbuf wait, kbase only) + a dump of the stuck streams at the
  queue timeout.
- `gameloop-override.json`: scenario 1 = D3D11 cube under 5 env sets, scenario 2 = D3D12 gears under d5-drain-iter.
  `raw/` has everything but `video.mp4`.

## Result: draining fixes the hang

| # | env set | result |
|---|---|---|
| 1 | d5-drain (`PANVK_KBASE_DRAIN_BEFORE_WAIT=1`) | **full pass**: 11645 frames in 20.0 s, 583.6 fps, no timeout |
| 2 | d5-drain-iter (+ `PANVK_KBASE_V10_ITER_SYNC=1`) | **full pass**: 11814 frames, 591.1 fps |
| 3 | d5-iter-trace (iterator restore + trace, no drain) | hang after 1459 frames |
| 4 | d5-trace (trace, no drain) | hang after 1481 frames |
| 5 | d5-drain-iter2 (repeat of 2) | **full pass**: 11414 frames, 571.0 fps |
| gears | d5-drain-iter | full pass, exit 0 |

The hang in passes 3 and 4 is the diag4 cycle again (pass 4):

```
subqueue 0 (VT):      in sync64 wait at site 1023: syncobjs+0x10 > 19688, memory 19687
subqueue 1 (FRAG):    in sync64 wait at site 7120: syncobjs+0x0  > 14761, memory 14761
subqueue 2 (COMPUTE): in sync64 wait at site 1023: syncobjs+0x10 > 19688, memory 19687
```

**Root cause.** On kbase each subqueue is its own CSG. VT defers its tiling-done signal (a `SYNC_ADD64` on its
`DEFERRED_SYNC` slot, waiting for the tiler iterator) and then blocks in the barrier's `SYNC_WAIT` on FRAG; that
deferred signal is not sent while the stream sits in the wait, and FRAG's `wait_finish_tiling` needs it. Waiting for
the stream's own `DEFERRED_SYNC` slot before it blocks on another subqueue breaks the cycle, with no fps cost (571-591
fps against 566 for the no-trace pass of diag4 before its hang). The iterator restore is not needed (pass 1 passes
without it, pass 3 hangs with it).

Fix: `panvk/csf/kbase: let a stream's deferred syncs land before it waits on another subqueue` (build 115b), which
also covers the prerast arena / tessellation token waits.

## Screenshots

- `screenshots/video-start.png`, `video-mid.png`, `video-end.png`: FTL video frames.
- `screenshots/0N-d3d11-d5-<set>-harness.png`: the cube at 6 s in the drain passes;
  `screenshots/01-d3d12gears-d5-drain-iter-harness.png`: gears.
