# Perf review 2026-10-26: consolidated plan (build 115+)

Four read-only reviews of rc5g-114 (HEAD d324a269):

| Part | Area | Report | Commit |
|---|---|---|---|
| A | CSF CPU: recording and submit (v10/v11) | [a-csf-cpu.md](a-csf-cpu.md) | 3ed76dc |
| B | Memory footprint (CSF focus, JM notes) | [b-memory.md](b-memory.md) | 0e02582 |
| C | GPU side: bandwidth, stalls, present (CSF focus) | [c-gpu.md](c-gpu.md) | cc0ec45 |
| D | JM path (v6 G72, v7 G52, v9 G78) | [d-jm.md](d-jm.md) | 67f5dcd |

Measurement limits. FO4 in game on G615 is bound by swap (memory), not GPU or CPU, so GPU and
CPU fixes show up there only after the memory work. The DXVK cube is bound by the present path
(GPU about half idle). Firebase PanProbe timings and the PanPlay cube harness (with the
`PANVK_FPS_LOG` patch, csf-v11 233) are the yardsticks; FO4 needs the X6 Pro, which is not
planned for now.

## Ranked list

Gain and risk are the reviewers' estimates (see the reports for code locations and reasoning).

### Quick, safe wins

| Id | Finding | Gain | Risk | Package |
|---|---|---|---|---|
| C2 | X11 software present: blit buffer in uncached memory, CPU memcpy from it, X round trip per present. Cached memory + invalidate, defer the reply | cube 47 to 60+ fps (est. 3.5 ms/present at 1280x705, 10 ms at 2400x1080) | low | P-C |
| A1 | Since csf-v11 092 every depth-only draw re-emits all FS state (stand-in depth-bounds shader trips the re-dirty rule) | med-high in shadow/prepass | low | P-A |
| C1 | Depth-bounds emulation lowers every DXVK fast-linked FS: no FPK, late ZS, FS runs with colour masked. Port jm-v7 076 to v9+ | 0.5-3 ms per full-screen pass | low-med | P-A |
| A2 | Any descriptor bind or push dirties both VS and FS tables on CSF (and v9 JM); jm-v7 097 fixed v6/v7 only | med-high | low-med | P-A |
| A3 | CS chunks flushed whole (>= 192 KiB `dc cvac` per primary cmdbuf) | med for small cmdbufs | low | P-B |
| A4, A5, M1 | Vertex buffer dirty compare; prerast eligibility checked 3x per draw; register-write shadowing | low each | low-med | P-A |
| C6 | v9/v10 still preload depth/stencil on every tile (061 fixed v11) | up to 13 MB reads per pass | low | P-C |
| Q2 | Destroy finished retired tiler heaps at every graphics submit | 15-50 MB | low | P-B |
| Q3 | Tess arena VS region 8 MiB instead of 32 MiB | 24 MiB | low | P-B |
| Q4 | 041 memset zeroes every BO; zero only what needs it | load-time CPU | low-med | P-B |
| D3 | JM: descriptor-copy job built for every FS draw (and uninitialised `ptr` read on depth-only draws; UB) | 1-3 us CPU + 1 GPU job per draw | low | P-D |
| D5 | JM resubmit flushes the whole descriptor pool once per batch | 10-50 ms per tess_perf frame | very low | P-D |
| D1 | v6/v7: each tessellated draw charged its level-64 worst case, so each gets its own batch | 2-4 ms per extra batch at 1080p | low-med | P-D |
| D2a | JM TLS/WLS per cmdbuf: skip the CPU clear; enable cmd-pool auto-trim on JM | hundreds of MiB with several cmdbufs | low | P-D (TLS), P-B (auto-trim) |
| D7 | v9: every VkDevice commits and clears 160 MiB of arenas up front | 160 MiB, 50-100 ms per device | low-med | P-D |
| D4 | JM 2 s warm-up sleep in every process (reason unknown) | 21-26 % of each FTL run, 2 s per app start | low-med | P-D |

### Larger work

| Id | Finding | Gain | Risk | Package |
|---|---|---|---|---|
| K1/D13 | Android-native present does DeviceWaitIdle + CPU copy every frame (all arches) | high for native Vulkan apps | med | P-C |
| Q1 | Memory budget that shrinks under pressure, swap-aware reserve (not 229 "honest" as default) | 100-150 MB DXVK slack | low-med | P-B |
| B1(mem) | One device-wide cmd-buffer slab cache instead of per-pool peaks | 40-100 MB | med | P-B |
| B2(mem) | Smaller first CS chunks that grow (cs_builder size hint) | 10-40 MB | med | P-B |
| B1(cpu) | KCPU fence wait: device mutex held across 20 ms poll, one waiter per device | up to 20 ms wake latency | med | P-A |
| B4(cpu) | Defer CPU cache maintenance from the recording thread to submit | med if recording-bound | med | P-A |
| C4 | Preload barrier drains the fragment queue even when nothing wrote the attachment | 0.3-1.5 ms per 30-pass frame | med | P-C |
| C8 | Every vertex-stage barrier also makes compute wait | < 0.5 ms/frame | med | P-C |
| C7 | CRC invalidated on every CLEAR; v10 drains to mark valid | up to 10 MB writeback/frame | med | P-C |
| D6 | JM: each dispatch is its own submission, each barrier splits the batch | med | med | P-D |
| D8 | JM batch split every 5K jobs lands mid render pass | med | med | P-D |
| D11 | JM: two submit ioctls, two events and an L2 clean per batch | low-med | med | P-D |
| D12 | v9 tiler heaps never give memory back | memory | med | P-D |
| D2b | JM device-level sparse TLS (as CSF 230/232) | hundreds of MiB | med | P-D |
| B4(mem) | Mark idle driver caches evictable (`BASE_MEM_DONT_NEED`) | 30-100 MB under pressure | med-high | P-B (opt-in) |
| B2(cpu) | Coherent pools if the GPU is ACE-Lite coherent (needs proof) | potentially high | high | P-A (opt-in + probe log) |

### Phase 2 (after 115 is green)

C3 tessellation emulation steps (CSF), D9 TES once per vertex (v6/v7), D10 rotating tess arena
regions (v6/v7), B3 native kbase timelines, B6 tiler chunk recycling, B3(mem)/C5 FO4 8 KiB stack
shader, C10 blend-shader fallbacks, DX12 (vkd3d-proton) feature gaps.

### No build needed (A/B knobs or game config)

C9 `PANVK_KBASE_HEAP_CHUNKS=10,400`, C7 `PANVK_DEBUG=no_crc`, P1 FO4 profile (godrays off,
`iTexMipMapSkip=1`, `dxvk.maxMemoryBudget`). Correctness items to fix on the way:
`timestamp_device_coherent` looks inverted (P-A); unnamed `wsi_device_options` field in
`panvk_wsi.c` (P-C); flush ID forced to 0 when an entry waits on its own subqueue, v10 ZS/DCD
early-ZS disagreement (P-C).

## Packages and integration

Four implementation packages run in parallel, each in its own Mesa worktree on rc5g-114, and
export their commits to `patches/staging-115/<package>/` with a NOTES.md. Every change has an
env knob that restores the 114 behaviour, read once at init.

| Package | Owns |
|---|---|
| P-A | CSF CPU record/submit, depth-bounds/FS state (A1, C1, A2 incl. v9 JM, A4, A5, M1, B1/B4/B2 cpu, timestamp fix) |
| P-B | Memory, budget, cmd pools and CS chunks (Q1-Q4, B1/B2/B4 mem, A3, JM auto-trim) |
| P-C | Present paths and GPU-side passes/barriers (C2, K1/D13, M8, C4, C6, C7, C8, wsi fix, flush ID) |
| P-D | JM v6/v7/v9 only (D1-D8, D11, D12, D2 TLS) |

Integration: stack 233 (fps log) and the four packages after the current series, strict prepare
with no drift, build RC5-universal-115, then Firebase one device at a time: PanProbe on starlte,
a31, oriole, panther and shiba, and the PanPlay cube harness (each cube once with the 115 knobs
set to the 114 values and once with defaults). Any regression is bisected with the knobs.
