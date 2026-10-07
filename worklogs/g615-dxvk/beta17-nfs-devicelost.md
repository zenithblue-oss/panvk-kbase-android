# beta.17: VK_ERROR_DEVICE_LOST in NFS: Most Wanted (attempt 1)

Date: 2026-10-07. Device: Poco X6 Pro (Mali-G615 MC6, CSF v11, 7.5 GB RAM). Driver: PanVK-kbase beta.17,
BuildID `313addcb0b88110895cf59e29874f666887efe6c` (= `dist-beta17d`, built from the `wt-beta17b`
tree). App: PanPlay 1.2.3, DXVK D3D9, Proton 11 arm64ec.

## Summary

The driver-side cause cannot be proven from the saved logs. PanVK's own error message, which names
the exact failing path, was lost: no `MESA_LOG` is set, so on Android `mesa_loge` and the
`vk_queue_set_lost` report only go to logcat. The logcat main buffer had already rotated when
PanPlay saved the session. Its copy of main starts at 23:55:01, which is 52 s before the game was
killed.

Best-supported hypothesis, medium confidence (about 60%): system-wide memory pressure set off the
loss. NFS was loading the race while FluxLinux was restarting, and scrcpy was running. lmkd killed a
cached app (PanProbe) at 23:53:14 and then a foreground-service app (FluxLinux) at 23:53:46. Killing a
foreground-service app means the pressure was severe. The DEVICE_LOST falls in the same window.
Under that pressure, the two most likely kbase paths are:

- the kernel cannot allocate a tiler-heap chunk, which sends a `TILER_HEAP_OOM` group error;
- the 10 s software wait watchdog fires while the system is stalled.

Thermal throttling (GPU capped at 740 MHz) only slows the GPU. On its own it does not explain a loss.

## Timeline (wall clock, 2026-10-06)

The wine and DXVK logs have no timestamps. Times come from three sources:

- the app-scoped logcat in the session folder (audit lines);
- the X server log's millisecond counter, anchored at its last line: the window teardown at
  23:55:53.5, which gives a counter base of about 23:41:08;
- `dumpsys activity exit-info`, which persists across logcat rotation.

| Time | Event | Source |
|---|---|---|
| 23:41:05 | NFS launched (session start) | session.json `startMs` |
| 23:51:10 | FluxLinux force-stopped by another process, then restarted (new pid, foreground service, importance 125) | exit-info |
| 23:52:23.9 | Last Enter key press (starting the race) | xserver.log counter 943930 |
| 23:52:24 to 23:52:47 | Burst of `speed.exe` executable-mapping audits (race load); the last one is at 23:52:47.8 | logcat (auditd) |
| 23:53:13 | The `winedevice.exe` 3 s `/dev/input` poll stops for 54 s, the longest gap in the run (earlier gaps were 15 to 39 s) | logcat (auditd) |
| 23:53:14.356 | PanProbe (cached, importance 400) killed: `reason=3 (LOW_MEMORY)` | exit-info |
| 23:53:46.298 | FluxLinux (foreground service, importance 125) killed: `reason=3 (LOW_MEMORY)` | exit-info |
| 23:55:05 | `run-as` shell from adb (tester cleanup) | logcat (auditd) |
| 23:55:16.8 | Overlay touch at (1130,50) | xserver.log |
| 23:55:53 | Wine killed by SIGKILL (exit 137), `userStopped=false`; PanPlay itself (pid 3130) survives | session.json, logcat |

The DEVICE_LOST lines come after audio init and are the only output after it. The game produced
nothing else for about 12 minutes before them. The loss therefore happened between race start
(23:52:24) and the cleanup kill (23:55:53). It cannot be ordered against the 23:53:14 LMK kill at
sub-minute resolution. The memory pressure itself started before 23:53:14, because lmkd kills in
response to pressure. The SIGKILL is the tester's cleanup of the hung game, not part of the cause.

## Evidence

- `err:   DxvkSubmissionQueue: Command submission failed: VK_ERROR_DEVICE_LOST` (50 times) and
  `err:   Failed to query semaphore value: VK_ERROR_DEVICE_LOST` (18 times). These are the first
  errors after start-up, with nothing in between.
- There is no DXVK `Memory allocation failed` line. User-space BO allocation (`MEM_ALLOC`, which
  would return `OUT_OF_DEVICE_MEMORY`) did not fail.
- `ApplicationExitInfo ... process=dev.zenithblue.panvktest reason=3 (LOW_MEMORY) ... timestamp=2026-10-06 23:53:14.356`
- `ApplicationExitInfo ... process=com.ivarna.fluxlinux reason=3 (LOW_MEMORY) importance=125 ... timestamp=2026-10-06 23:53:46.298`
- Session `logcat.txt`: `buffers main,system,crash,events`, scope own uid. The main section starts at
  `10-06 23:55:01`, and no `MESA`, `kbase:` or `panvk` line survives.
- `mesa-panvk.txt` and `wine-run.log` contain no `kbase:` line. Every driver diagnostic uses
  `mesa_loge`, and with no `MESA_LOG` set it goes to the Android logger only (`src/util/log.c`
  default for Android).
- Device logcat (`-b all`) and dmesg no longer cover 23:5x: main starts at 06:23, events at 05:01,
  and dmesg at uptime 76131 s (about 05:12).

## Where PanVK returns VK_ERROR_DEVICE_LOST on kbase (beta.17 tree)

`src/panfrost/vulkan/csf/panvk_vX_gpu_queue.c`:

1. **CS error in the seqno wait** (about line 849). The firmware sets `cell->error`, and the CS
   output page has a non-zero exception type at `CS_FAULT` (0x80) or fault info at 0x88. Logs
   `kbase: CS error 0x.. on subqueue N fault 0x.. info 0x..`. This is a real GPU exception
   (for example 0x41/0x72, or a page fault).
2. **Software watchdog** (about line 956). `KBASE_WAIT_TIMEOUT_NS` is 10 s per wait. When the seqno
   has not advanced by then, it logs `kbase: timeout on subqueue N: seqno ...` and sets the queue
   lost. Nothing checks whether the GPU or the system is simply starved: no group state is read and
   progress in `extract` does not extend the deadline.
3. **Ring emission failure** (about line 2966): `kbase: ring emission failed`.
4. **Tiler heap renewal failure** (about line 3037). `kbase_renew_tiler_heap` returns < 0, for
   example when `KBASE_IOCTL_CS_TILER_HEAP_INIT` fails with ENOMEM. Logs `kbase: tiler heap renewal
   failed`.
5. **Submit ioctl failure** (about line 3089): `GROUP_SUBMIT: %m`. The same code is in
   `panvk_async_bind.c:423`.
6. **CSF error latch** (`gpu_queue_check_status`, about line 3431). This is reached through
   `vk_device_check_status` from fence/semaphore waits, `QueueWaitIdle` and
   `GetSemaphoreCounterValue` (`vk_semaphore.c:366`, which is DXVK's "Failed to query semaphore
   value"). `kbase_kmod_csf_has_error()` reads `csf_error`, which
   `src/panfrost/lib/kmod/kbase_kmod.c:737` sets for **every**
   `BASE_CSF_NOTIFICATION_GPU_QUEUE_GROUP_ERROR`:
   - `ERROR_FATAL`
   - `QUEUE_ERROR_FATAL`
   - `ERROR_TIMEOUT`: the kernel CSG progress timer
   - `ERROR_TILER_HEAP_OOM`: the kernel could not grow the heap, and the group is terminated
   - `QUEUE_ERROR_FAULT`: a *recoverable* fault

   The latch is device-wide, is never cleared, and ignores the group handle.
7. After the first loss, `gpu_queue_submit` returns `VK_ERROR_DEVICE_LOST` early through
   `vk_queue_is_lost` (about line 3221). That produces the repeated DXVK submit errors.

`_vk_queue_set_lost` (`src/vulkan/runtime/vk_queue.c:118`) only stores the message. The message is
printed later by `vk_device_check_status`, again through the Android logger only.

Under memory pressure without a user-space allocation failure, the plausible paths are 6
(TILER_HEAP_OOM from the kernel's chunk allocation, or the CSG progress timeout), 2 (a 10 s stall)
and 4 (TILER_HEAP_INIT ENOMEM at renewal). Path 1 with a real fault (0x41/0x72) is the known
`gs_restart_*` GS bug. NFS (D3D9) does not use GS, so path 1 is unlikely.

## Repro

Skipped. When checked, the phone was in active use by someone else: the foreground app changed
from FluxLinux to PanProbe between 06:23 and 06:30, and I launched neither. FluxLinux was also
reinstalled at 06:00 and 06:13.

## Proposed fix (not implemented)

1. **Keep the driver's reason in the session (needed to close this).**
   - Set `MESA_LOG=android,file` for the wine process, so `mesa_loge` and the
     DEVICE_LOST report also go to stderr and `wine-run.log`. File:
     `apps/panvk-launcher/app/src/main/java/dev/zenithblue/panvklauncher/Containers.kt`, next to
     `envMap["DXVK_LOG_LEVEL"]` (about line 369).
   - Alternatively, stream own-uid logcat to a file for the whole run instead of a one-shot
     `logcat -d` at exit. Files: `SessionLogs.kt` and `UploadLogs.kt` in the same package.
   - Also record the first DEVICE_LOST wall time, plus `exit-info` for the other apps in the run
     window, so LMK kills land in `session.json`.
2. **Driver: latch only fatal group errors, per group.** In `kbase_log_csf_notification`, do not
   set `csf_error` for `QUEUE_ERROR_FAULT`. Store the group handle, and have `gpu_queue_check_status`
   compare it with `queue->group_handle`. File: `src/panfrost/lib/kmod/kbase_kmod.c` (`kbase_kmod.h`
   for the accessor), as a new `patches/csf-v11/` patch.
3. **Driver: watchdog that tolerates stalls.** Before declaring the queue lost after
   `KBASE_WAIT_TIMEOUT_NS`, check whether `extract` or the seqno advanced since the last deadline,
   and re-arm the deadline if so. Declare it lost only after a full window with no progress and no
   group error. File: `src/panfrost/vulkan/csf/panvk_vX_gpu_queue.c` (`kbase_subqueue_wait_seqno`).
4. **If 1 shows TILER_HEAP_OOM:** pre-allocate more initial tiler-heap chunks at heap creation and
   renewal, so that growth in the kernel's OOM path, which runs in a worker and fails fast under
   reclaim, is rarer. File: `panvk_vX_gpu_queue.c` (heap create and renew parameters).

Validation for any of these: rerun NFS City Perimeter with FluxLinux and scrcpy running, to
reproduce the memory pressure. Then confirm that the saved `wine-run.log` carries a `kbase:` line
and the DEVICE_LOST reason.
