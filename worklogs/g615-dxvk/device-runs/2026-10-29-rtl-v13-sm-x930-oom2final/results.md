# RTL SM-X930 (Immortalis-G925 MC12, v13, gpu_id 0xd8300015, mask 0xee0077) - oom2final (build 120 + jm-v7-wip 0161), 2026-10-29

PARTIAL (PanProbe done; PanPlay in progress, updated below when finished).
driverInfo reported by the device: `Mesa 26.3.0-devel (git-84704e1bb7, p0161, 2026-10-29)`.
.so on disk: sha256 d0d4a971f470d6d2863975125d54cd03d48214ba72a9f695fe35e34e046c454c, BuildID 0cf6b04d5cfdacb4cd4943b5d11835998176eb4b (unstripped build-out copy; the APK .so is stripped).
Script `raw/run.sh`, progress `raw/progress.txt`. Device on adb `localhost:35395` via rdb, rtl.lock held.

## PanProbe autorun `all`: 41/41 PASS, 0 fail, 0 skip (build 118: 39/40, tess_perf FAIL)
- **tess_perf PASS** (`TESS_PERF_FAILS=0`, `RESULT tess_perf PASS mismatch=0`), both variants (default and TESS_FUSE=0/SETUP_STEP=0/ARENAS=1/WINDING_INLINE=0) ran all 10 gs x L points to completion.
  Hashes identical to 118's good points (L=1 a03404c5, L=4 cec55885, L=16 d4463b35, mix b34a251c). Decisive: no `Wait r=-4`, no `tiler heap OOM notification`, no `timeout snapshot` anywhere in the logcat stream (0 DEVICE_LOST); 118 had the OOM line in every failure.
- Fps/timing (default, gs0/gs1, ms per 256 draws): L=1 22.1/37.2, L=4 40.5/61.2, L=16 150.3/376.2, mix 148.1/208.7. 118 never finished gs=1 L=4, so no like-for-like past that.
- DEVICE_LOST: none. CSF faults: none (0 lines of fault/OOM/incremental from MESA in the stream, `raw/probe-logcat-grep.txt`).
- Memory (PanProbe `dumpsys meminfo` TOTAL PSS sampled every 15 s over the run): 127.6 - 139.5 MB.
- Full stream (local only, 100+ MB): /var/tmp/panvk/rtl-oom2final/logcat-stream.txt.
- Screenshot: [sm-x930-oom2final-panprobe-results.png](sm-x930-oom2final-panprobe-results.png)
