# X6 Pro (Mali-G615 MC6, v11, MediaTek kbase CSF 1.21) - kbase tiler heap OOM, jm-v7-wip 0161 (2026-10-29)

Serial Y5WWBMJVOZSK4HU8, rooted (dmesg readable), shared under `/var/tmp/panvk/x6pro.lock`. PanProbe autorun,
scenario 2 (rc5 slot), env through `--es env`. Scripts: `raw/scripts/oom2-run.sh` (one run, installs the APK every
time because other agents reinstall in between), `oom2-batch.sh`, `oom2-play.sh`. Per-run summary lines:
`<n> <driverInfo git> <AUTORUN DONE line> <dmesg count of "Queue group to be terminated"/"Trigger GPU reset">`.

Note: `r3-oom2a-heap4-32` actually ran build 119 (`git-0146a719f6`): another agent had reinstalled PanProbe 119.
It counts as a build-119 baseline run.

## Builds

| tag | tree | driverInfo | .so |
|---|---|---|---|
| 119 (baseline) | `mesa-119-final` | `git-0146a719f6, p0159` | `build-out/2026-10-29-119/` |
| oom2a | dev, one group + OOM flag only | `git-a28af6b305, p0159` (uncommitted dev) | `build-out/2026-10-29-oom2a/` |
| oom2b | dev, = 0161 on p0159 | `git-a28af6b305, p0159` (uncommitted dev) | `build-out/2026-10-29-oom2b/` |
| **oom2final** | strict, `prepare-rc5-tree.sh --wip-max 0161` on fresh 5a07217f (`mesa-oom2-final`) | `git-84704e1bb7, p0161` | `build-out/2026-10-29-oom2final/libvulkan_panfrost.so` |

oom2final: `BASE-OK applied=228 csf-v11<=233`, `OK base=228 jm-v7-wip=156 head=84704e1bb7a`, tree equal to the dev
tree. .so sha256 `d0d4a971f470d6d2...`, BuildID `0cf6b04d5cfdacb4...`. APKs `build-out/apks/panprobe-oom2final.apk`,
`build-out/apks/panplay-oom2final.apk` (all under `/var/tmp/panvk/`).

## Reproduction (build 119)

| env | runs | result |
|---|---|---|
| `PANVK_KBASE_HEAP_CHUNKS=1,8` | 1 | FAIL at gs=0 L=16: `CSF group 0 tiler heap OOM notification`, dmesg `Queue group to be terminated, couldn't handle the OoM event` |
| `PANVK_KBASE_HEAP_CHUNKS=4,32` | 3 (r3 + old119 x2) | 3 FAIL, same shape as the v13 G925 hang |

So the v13 symptom reproduces locally with a small heap: kbase `kbase_queue_oom_event()` terminates the group.

## 0161

| run | env | tess_perf | notes |
|---|---|---|---|
| oom2a (group+flag, no recycle) | 1,8 | 0/1 | got much further (15 BENCH lines), then firmware hang (CSG STATUS_UPDATE timeout, GPU reset) |
| oom2a | 4,32 | 1/1 | |
| oom2b (= 0161) | 1,8 | 6/10 (r5 + 5/9 batch) | 4 firmware hangs + GPU reset, no OOM termination; mostly at gs=1 |
| oom2b | 1,16 | 4/4 | |
| oom2b | **4,32** | **10/10** | |
| oom2b | 1,8 `large_draw` | 2/2 | |
| oom2b | default heap, full PanProbe | **41/41** | |
| oom2b | 4,32, full PanProbe | **41/41** | IR exercised on every test with geometry |
| oom2final | 4,32 | **2/3** | run 1: firmware hang at gs=1 L=4 (FUSE=0 opt), GPU reset, no OOM termination |
| oom2final | default heap, full PanProbe | **41/41** | |
| oom2final | PanPlay s3/s5/s7 (warmup 10, measure 30) | 3/3 ok | s3 D3D11+D3D9 ok (115.9 fps last pass), s5 D3D12 55.9 fps, s7 FL 11_0..12_2 ok; no reset in dmesg |

The kbase OOM termination is gone in every 0161 run. What is left is a different failure: with a small heap
(8 MiB often, 32 MiB 1 of 13 runs) the firmware stops answering (all CSG slots, `Timeout on CSG_REQ:STATUS_UPDATE`,
kbase GPU reset, group fatal `0x72` = kbase SW fault after reset). Not seen at the default heap (200-400 chunks).

## Perf (default heap, same oom2b binary, `PANVK_KBASE_TILER_IR=0` = old layout and no recycling), ms per frame, 2 runs each

| tess_perf case | 0161 | old |
|---|---|---|
| default gs=0 L=1 | 49 / 37 | 94 / 95 |
| default gs=0 L=4 | 63 / 60 | 71 / 73 |
| default gs=0 L=16 | 283 / 272 | 287 / 288 |
| default gs=1 L=4 | 111 / 105 | 162 / 185 |
| default gs=1 L=16 | 625 / 572 | 637 / 626 |
| FUSE=0 gs=0 L=4 | 74 / 74 | 153 / 135 |
| FUSE=0 gs=1 L=4 | 115 / 112 | 228 / 221 |
| L=ref (tiny) | 0.93-1.21 | 0.62-0.88 |

`large_draw`: 8.9-9.1 s both ways. 0161 is faster on heavy geometry (chunks reused instead of growing the heap and
draining at renewal) and ~0.2-0.3 ms slower per frame on the trivial `L=ref` frame (per-pass FINISH_FRAGMENT).

## Not done here

- v13 G925 / v14: the coordinator runs oom2final on RTL (`2026-10-29-rtl-v13-sm-x930-oom2final/`).
- TB336FU (v9 JM, not affected by the CSF change) not run.
- Firmware hang under a tiny heap: see the jm-v7-wip README section for 0161, open problems.
