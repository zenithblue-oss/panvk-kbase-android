# RTL SM-X930 (Immortalis-G925 MC12, v13, gpu_id 0xd8300015) - tess_perf A/B, build 118 (2026-10-29)

Driver git-a0412457ab (build 118), PanProbe 118 already installed. Device work 13:33-13:45 IST under `rtl.lock`,
device still online at 13:45 (not watched until the RTL drop). Script `raw/run-x930-tessperf.sh`
(`--es autorun tess_perf --es env '<ENV>' --ez uploadDryRun true`, one pass ~20 s), progress `raw/progress.txt`.
Per run `raw/<run>/autorun.txt` and (from run e on) the PanProbe `tess_perf.log`; runs a-d have no run log (not pulled).

Device facts (PanProbe log, no sysfs): `device TLS ... 24 core IDs (12 cores, mask 0xee0077)` (non-contiguous),
`kbase: tiler heaps 4 initial / 400 max chunks of 1024 KiB` (default).

## A/B (tess_perf only)
| env | runs | PASS | FAIL / CRASH | tiler heap OOM in log |
|---|---|---|---|---|
| default | 4 | 1 | 3 FAIL (`Wait r=-4`) | every FAIL with a log (2/2) |
| `PANVK_TLS_SPARSE=0` | 1 | 0 | 1 | no log |
| `PANVK_PRERAST_MERGE_WG=0` (0151 off) | 1 | 0 | 1 | no log |
| `PANVK_TLS_SPARSE=0 PANVK_PRERAST_MERGE_WG=0` | 1 | 0 | 1 | no log |
| `PANVK_KBASE_HEAP_CHUNKS=4,1024` | 3 | 1 | 2 | 2/2 FAIL |
| `PANVK_KBASE_HEAP_CHUNKS=4,600` | 1 | 0 | 1 | 1/1 |
| `PANVK_KBASE_HEAP_CHUNKS=64,4096` | 1 | 0 | 1 | 1/1 |
| `PANVK_PRERAST_CHUNK_FLUSH=0` | 1 | 0 | 1 | 1/1 |
| `PANVK_KBASE_GPU_SEMAPHORE_WAITS=0` | 5 | 3 | 2 | 2/2 FAIL |
| `PANVK_DEBUG=sync` | 8 | 3 | 5 CRASH (SIGSEGV addr 0x20 after the OOM) | 5/5 CRASH |
| `PANVK_OVERLAP=0` (JM-only knob, no-op on CSF = placebo) | 2 | 2 | 0 | 0 |

Fail point is random (gs=0 L=4/L=16, gs=1 L=1/L=4/L=16), not only `gs=1 L=4`.
Every one of the 14 failures with a run log has `kbase: CSF group 0 tiler heap OOM notification`; no pass has it.
No knob fixes it: each knob that passed sometimes also failed, and the placebo / default pass too (default 1/4,
placebo 2/2). No patch written.

## Hang shape (all failures)
`timeout on subqueue 0`, snapshots of subqueues 0/1/2 all at the same seqno, target = seqno + 1, `active 0`,
`error 0x0`, `stream progress 0x0`, no CS fault/exception. Subqueues 0/1 have 48 bytes of ring left, subqueue 2
(fragment) 96 bytes. Example g-default2: seqno 1235 target 1236, insert 238656, extract 238608/238608/238560.

## Root cause (best hypothesis)
Same as v14 (jm-v7-wip README "CSF tiler heap OOM"): the tiler raises heap OOM, kbase sends the OOM event, and with
no `BASE_CSF_TILER_OOM_EXCEPTION_FLAG` / incremental-render handler (one kbase group per subqueue) the render pass
never completes, so all three subqueues stall and the wait times out (DEVICE_LOST). The max-chunk count is not the
limit: 1024 or 4096 max chunks fail the same way, so the OOM is hit while the heap can still grow (growth denied or
too late, e.g. no fragment work in flight to free chunks, or an in-flight render-pass limit in kbase). The fix is the
kbase OOM / incremental-rendering path, not a v13 env gate; TLS sparse (0148) and 0151 are not involved.
