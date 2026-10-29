# RTL: Galaxy Tab S12+ SM-X846B (Mali-G1-Ultra, v14 CSF, MT6993), build 115e PanProbe

- Samsung Remote Test Lab, adb `localhost:35395` (RDB), Android API 37. APK `panprobe-115e.apk`, `adb install -r` OK
  (rc5 slot driverInfo `Mesa 26.3.0-devel (git-e7d836d5a6)`, .so `8dea43cf`). Launch: `TEST_LOOP --ei scenario 2` (rc5 only).
- `gpu_id=0xe8800010 arch=v14 va_bits=48`, CSF uAPI 1.38 (from `sparse_kbase` setup line).
- Raw: `raw/autorun.txt`, `raw/runs/20261029-014021/` (device clock).

## Result: 38/40 PASS, 2 FAIL (tess_perf, tessellation), test time ~125 s (sum of test times)

| test | result |
|---|---|
| all others (38) incl. `d3d_fl`, `bc_decode`, `bc_perf`, 4 COMPLIANCE lines (0 missingHard) | PASS |
| `tess_perf` | FAIL, `Wait r=-4` (DEVICE_LOST) line 253, 30.9 s |
| `tessellation` | FAIL, `Wait r=-4` line 432, after all CASE lines before it PASS (quad/isolines/point/winding) |

Driver log (first lines of the failure, both tests are CSF timeouts, the device recovers and later tests pass):
- `tess_perf`: `kbase: CSF group 0 tiler heap OOM notification`, then `timeout on subqueue 0: seqno 632 target 633 ... insert 121984, extract 1`,
  `timeout snapshot subqueue 2 stream progress 0x0`, `FAIL Wait r=-4`. Happens during the 2nd pass (`PANVK_TESS_FUSE=0,...` opt)
  after the default-opt pass completed (gs=0/1, L=1/4/16/mix all produced hashes).
- `tessellation`: `CSF group 0/1/2 fatal error: status 0x001003c1 (exception 0xc1), sideband 0x0000005f0005d040`, then
  `timeout on subqueue 0: seqno 4855 target 4856 ... insert 938624, extract 1`, `FAIL Wait r=-4`.
- Full lines in `raw/runs/20261029-014021/*-tess_perf.log` and `*-tessellation.log`.

## sparse_kbase (informational): PASS pass=12 fail=0 skip=2

- `VERDICT v14 eager-only -- MEM_ALLOC_EX FIXED refused; SAME_VA arena, grow-from-end ok`.
- T10 `MEM_ALLOC_EX FIXED at 0x800400000000 unavailable on this kernel (Inappropriate ioctl for device)` = the ENOTTY, still present
  on this kernel; T10f skipped. Others (T1-T9, T11 alias) PASS. `sparse_arena_commit=7.5us uncommit=2.0us`.

## Notes
- The old "v14 blocked by MEM_ALLOC_EX ENOTTY" no longer blocks anything: driver, probe and 38 tests run on v14; only the
  fixed-VA sparse path is unavailable (driver falls back to eager).
- Not investigated (task: report only). Tiler heap OOM on CSF v14 and exception 0xc1 in tessellation look like real v14 bugs.
