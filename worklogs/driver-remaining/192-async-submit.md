# 192: asynchronous kbase queue submission (PROGRESS item 11)

Date: 2026-10-16. Patch: `patches/csf-v11/192-destroy-retired-kbase-tiler-heaps-on-the-heap-thread.patch` (not committed).
Build: `g615-v11-csf` OK, BuildID `cf31393d499299938a8411c384ed4f96236605ff` (Mesa 98c8b2f + csf-v11/189-192). No new warnings: the one in `panvk_vX_gpu_queue.c` (`:858`, C23 label) is the same in `build/android-d.log`.
Strict check: `apply-patches.sh --profile g615-v11-csf` on a fresh 5a07217f tree, `OK applied=186`, no PATCH-DRIFT.
**Not run on hardware. Nothing here is a device result.**

## Conclusion

A full submit thread (Mesa `VK_QUEUE_SUBMIT_MODE_THREADED` or a driver thread) is **not worth doing now**:

- vkQueueSubmit on kbase no longer waits for the GPU in steady state (102-107).
- DXVK already calls vkQueueSubmit and vkQueuePresentKHR from its own `dxvk-submit` thread. A driver thread would only move the same work to another thread. The game thread and `dxvk-cs` would not run any sooner.
- Device data says the submit thread is not the limit. 107 cut submit-thread blocking from 345 to 100 ms per run, and fps did not change (CPU bound). 106 cut it from 47 % to 0.2 %. NFS is bound by the game main thread and wineserver. The G925 stutter was frame pacing, fixed with a 60 fps cap (`worklogs/g925-nfs-stutter.md`).
- Mesa's threaded mode costs extra with this sync model (see the plan below), so it could make the present path slower.

One expensive synchronous kernel call was still on the submit path, and it could be moved safely: the tiler heap TERM ioctl. 192 moves it to a worker thread.

## What vkQueueSubmit does synchronously today (from code)

Sync model: kbase has no DRM syncobj. `get_device_sync_types_kbase()` (`panvk_physical_device.c:1147`) exposes the binary `kbase_cpu_sync` and wraps it in `vk_sync_timeline`. That gives `VK_DEVICE_TIMELINE_MODE_EMULATED`, so the submit mode is `VK_QUEUE_SUBMIT_MODE_DEFERRED` (`vk_device.c:232`). Every submit runs on the caller's thread:

Runtime (`vk_queue.c`):
1. Allocates the `vk_queue_submit` and resolves semaphores and fences (temporary payloads, timeline points). This is all CPU work with mutexes.
2. `vk_queue_push_submit`, then `vk_device_flush` → `vk_queue_flush` (`:675`). For every queue, each submit's timeline waits are checked with `VK_SYNC_WAIT_PENDING`, timeout 0. A submit whose waits are ready goes to `vk_queue_submit_final` → `driver_submit` under the queue lock. A wait-before-signal submit stays queued until a later submit or host signal flushes it. This is correct already.

Driver (`csf/panvk_vX_gpu_queue.c`, `panvk_per_arch(gpu_queue_submit)` `:3307` → `panvk_queue_submit_ioctl_kbase` `:2989`):

| step | cost | where |
|---|---|---|
| storage, req-resource, cmdbuf qsubmit list; `panvk_get_flush_id` = one user-reg MMIO read | small CPU | `:2455-2780` |
| wait loop: same-queue kbase syncs become GPU SYNC_WAIT (103) | mutex per wait | `:3020-3032` |
| **other waits** (another queue's sync, imported sync_file, `PANVK_KBASE_GPU_SEMAPHORE_WAITS=0`): `vk_sync_wait(COMPLETE, UINT64_MAX)` | **CPU blocks until the GPU or the fence is done** | `:3034` |
| `pan_kmod_flush_bo_map_syncs`: one `KBASE_IOCTL_MEM_SYNC` per pending CPU-cached BO range | ioctl × N (N not measured) | `:3044`, `kbase_kmod.c:2593` |
| `kbase_subqueue_emit_job` per subqueue: build ~512 B ring entry, DC clean | small CPU | `:478` |
| ring full → `kbase_subqueue_wait_seqno` (full subqueue drain) | CPU block, only with >128 entries in flight (64 KiB / 512 B) | `:432` |
| `kbase_subqueue_publish`: CS_INSERT store, barrier, `KBASE_IOCTL_CS_QUEUE_KICK` | 1-3 ioctls per submit | `:737`, `:766` |
| tiler heap renewal every N submits / 64k work: `kbase_try_destroy_retired_heap` → **`KBASE_IOCTL_CS_TILER_HEAP_TERM`** per retired heap | **1.5 ms p50, 4.8 ms p90, 23 ms max** (stutter-followup, 4988 calls / 159 s ≈ 31/s on `dxvk-submit`) | `:2395` |
| renewal: `kbase_spare_heap_take` (waits for the 104 worker only if HEAP_INIT is still running), descriptor write + clean | usually zero | `:2451` |
| renewal backpressure (all 3 retired heaps still busy at 4× budget): wait for the oldest retired heap only (107) | rare CPU block | `:3124` |
| signals: `panvk_kbase_sync_set_pending` per signal | mutex | `:3143` |

Also synchronous, but outside vkQueueSubmit:
- sync_fd export (`kbase_cpu_sync_export_sync_file`, `panvk_physical_device.c:1059`): waits PENDING, then one KCPU fence ioctl (`kbase_kmod_csf_export_cqs64_fence`). It falls back to a full CPU wait only for targets without seqno addresses.
- The X11 software present fence wait is on the present thread since 106.

Steady-state DXVK frame (one graphics queue, binary semaphores from the same queue): no CPU wait for the GPU, only ioctls. The largest item by far was HEAP_TERM: ~47 ms of every second at p50 on `dxvk-submit`, with spikes up to 23 ms. Kicks and MEM_SYNC are probably tens of µs each, but they were never measured separately.

## 192: what changed

- `kbase_try_destroy_retired_heap()` still does the seqno check on the submitting thread: both graphics subqueues must be past the retirement, so the firmware no longer references the heap. The contexts that pass go to `kbase_heap_term_push()`.
- `kbase_heap_term_push()` appends them under a mutex to `queue->kbase_heap_term.ctxs[16]`. If no job is queued, it queues `kbase_heap_term_job` on the existing `panvk_heap` util_queue (104, which already runs HEAP_INIT for the spare heap, and that path has device runs). The job drains the list until it is empty, then clears `queued` under the lock.
- Destroying after the proof point is always safe. The VA-reuse hazard described above `kbase_try_destroy_retired_heap` only applies to destroying too early. The descriptor slot is released by the seqno check, the same as before 192.
- The thread holds at most one spare job and one term job (max_jobs 2), so `util_queue_add_job` never blocks. Before adding a job, the code waits on the previous term fence: a job that has just cleared `queued` signals it a few instructions later.
- Fallbacks: list full, util_queue missing, or `PANVK_KBASE_ASYNC_HEAP_TERM=0` → destroy inline, as before 192.
- Teardown (`kbase_spare_heap_finish`, from `cleanup_tiler`): wait for the term fence, then `util_queue_destroy`. That function drops jobs that have not started, so it must come after the wait. Then any leftovers are destroyed inline.
- Side effect: a spare HEAP_INIT kicked right after a term job runs after it, so it can start ~1.5 ms later. The next renewal is ~30 ms away.
- Peak memory: retired heaps live a few ms longer. That is bounded by the list size (16), and in practice it is 1-3 heaps.

Default on. kbase ioctls on one context fd from several threads are normal (Arm's own DDK does it), and the heap thread already issues HEAP_INIT concurrently with submits.

## Risks / unproven

- Not run on a Mali device. Concurrent HEAP_TERM vs CS_QUEUE_KICK / HEAP_INIT on the same kctx: kbase locks per heap list and per queue. I expect no new contention, but this is not measured.
- Not measured: the actual gain on `dxvk-submit`. Expect ~31 TERM calls per second off the submit thread and the 23 ms spikes gone from it. fps is not expected to change (CPU bound elsewhere).
- If HEAP_TERM gets slow enough that 16 contexts back up, the inline fallback brings back the old behaviour.

## Device checks (G615)

1. PanProbe 17/17 and the CTS synchronization + memory gate (compare against beta.16's 56 known failures).
2. NFS race with simpleperf on `dxvk-submit` and `panvk_heap`: `kbase_kmod_csf_tiler_heap_destroy` should appear only on `panvk_heap`. Check that the submit-thread blocked time drops and there are no "Terminate ctx" / CSG faults in dmesg.
3. A/B with `PANVK_KBASE_ASYNC_HEAP_TERM=0`.
4. Device teardown/recreate loop (DXVK creates three VkDevices in NFS): no leaked heap contexts (kbase debugfs `tiler_heaps` / mem_profile if available).

## Plan if a submit thread is wanted later

Do this only if a profile shows `dxvk-submit` (or a non-DXVK app's render thread) blocked in vkQueueSubmit and that limits frame rate.

1. Mesa's own `VK_QUEUE_SUBMIT_MODE_THREADED` is used only for ASSISTED timelines. With EMULATED timelines it would technically work: the thread waits for timeline points with PENDING, and `vk_sync_timeline` handles wait-before-signal with its condition variable. But:
   - `vk_queue_submit_move_binary_waits_to_temps(..., true)` (`vk_queue.c:1144`) runs for every binary permanent-payload wait, and DXVK uses those for acquire/present. Each one does a `vk_sync_wait(PENDING, UINT64_MAX)` on the caller (blocking until the thread has processed the signalling submit), plus creating a temp `kbase_cpu_sync` (mutex and condvar init) and a move.
   - The WSI present submit waits on the frame's last semaphore, so vkQueuePresentKHR would block on the submit thread every frame. That gives back the overlap.
   - Prototype: an env-gated `dev->vk.submit_mode = VK_QUEUE_SUBMIT_MODE_THREADED` after `vk_device_init` for kbase CSF, then CTS `synchronization.*` / `wsi.*` / `api.*fence*` / timeline wait-before-signal, and a measurement on device. 1-2 days plus device time.
2. A cheaper driver-side variant: keep DEFERRED, and only hand the publish (CS_INSERT + KICK ioctls) and MEM_SYNC to a per-queue thread. Seqnos and sync targets are already assigned at emit time, so fences, `vkQueueWaitIdle` (seqno-based) and sync_fd export (seqno address + value) stay correct without changes. Ordering rules: the kick thread must flush MEM_SYNC before the kick, and `kbase_subqueue_wait_seqno` rekicks would need the same thread or a lock. This is only worth it if kicks are measured to be slow (expected tens of µs).
3. Cross-queue binary waits and imported sync_file waits (`:3034`) still block the caller. A GPU-side wait on another queue's seqno address (SYNC_WAIT64 on its cell, CSF event memory) would remove the first case. Apps with one graphics queue (DXVK on panvk) do not hit it.
