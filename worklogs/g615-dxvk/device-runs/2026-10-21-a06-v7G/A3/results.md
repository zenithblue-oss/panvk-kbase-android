# A06 v7G-A3 (panprobe-v7gA3.apk, rc5 so sha256 c792eb61...f858, pkg v7ga)

Run 20261021-205018 (`--es driver rc5 --es autorun xfb,large_draw,pipeline_stats,occlusion_query,bc_perf,depth_bounds`): pass 4 fail 2 skip 0.

| test | v7G-A2 | v7G-A3 |
|---|---|---|
| bc_perf | PASS | PASS (65099 ms) |
| pipeline_stats | PASS | PASS (fps 1462.9) |
| depth_bounds | PASS | PASS |
| occlusion_query | PASS | PASS |
| xfb | FAIL mismatch=11 | FAIL mismatch=4 (only vs_clip_cull_capture) |
| large_draw | FAIL | FAIL (5 tess xfb cases) |

Failure lines:
- `RESULT xfb FAIL mismatch=4 fps=0 ms=5976`; `CASE vs_clip_cull_capture mismatch=4 FAIL`, `XFB_FAILS=1`
- `RESULT large_draw FAIL mismatch=0 fps=0 ms=19683`:
  - `FAIL case tess_direct bad=185426 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_indirect bad=185426 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_instanced bad=2 bytes=262144 written=16384 generated=-1`
  - `FAIL case tess_tri_direct bad=247234 bytes=393216 written=8192 generated=-1`
  - `FAIL case tess_tri_indirect bad=247234 bytes=393216 written=8192 generated=-1`
  - `XFB_FAILS=5`

Files: `autorun.txt`, `runs/20261021-205018/`. A2 per-case logs (run 20261021-200935) are in `../A2/runs/`.
