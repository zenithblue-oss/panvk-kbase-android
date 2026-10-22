# RC5-dev-bc5 build (2026-10-22): bc4 + fork-safe util_queue (csf-v11/209)

bc5 = bc4 (csf-v11 204-208, all defaults kept) plus 209. Goal: fix the
vkd3d_heap hang that 204 exposed, with the disk shader cache still on.

## Root cause of the vkd3d_heap hang

It is not hashing, blob size, eviction or fsync. It is fork().

- `tests/vkd3d/heap/vkd3d_heap.c` creates the VkInstance and enumerates the
  physical device **in the parent**, then `supervise_case()` fork()s one
  child per case (ssbo, texel, image, mutable). The child creates the
  device, runs the case and calls `vkDestroyInstance`.
- panvk creates the disk cache at physical-device init
  (`panvk_physical_device.c` `init_shader_caches` -> `disk_cache_create`),
  and `disk_cache_create` -> `util_queue_init` starts the `disk$` worker
  thread **in the parent**.
- A fork() child has only the forking thread. The child's pipeline compile
  misses the cache and calls `disk_cache_put` -> `util_queue_add_job`. The
  job is queued for a worker that does not exist in the child (or a second
  put starts thread 1, but thread 0 is still missing).
- The child's `vkDestroyInstance` -> `free_disk_cache` -> `disk_cache_destroy`
  -> `util_queue_finish` waits on a barrier/fences that the missing worker
  would signal, so it blocks until the 12 s ICD watchdog fires.
- This fits all the evidence:
  - the hang comes after "kbase_queue_wait_current: wait completed", at
    teardown, in all 4 cases;
  - `MESA_SHADER_CACHE_DISABLE=true` gives no queue, so it passes (266 ms);
  - `MESA_DISK_CACHE_SINGLE_FILE=0` changes only the file backend, the
    queue is the same, so it still fails;
  - bc3 passed because before 204 the cache dir failed ("/data/.cache"),
    so there was no queue;
  - no cache entry is ever written, so it fails on every run.
- Checked and ruled out:
  - `panvk_vX_descriptor_set_layout.c` hashes per binding (type, flags,
    desc_count, ...), not per array element. A 1M-element heap is one
    binding, so the cost is O(bindings).
  - The cache type is MULTI_FILE by default, with no single huge blob.

## Patch

**209 `util/u_queue`: restart the workers in a fork() child**
(`patches/csf-v11/209-restart-util-queue-workers-in-a-fork-child.patch`,
mesa-bc commit cc9e48fc).

- `global_init` (the same call_once as the atexit handler) registers
  `pthread_atfork` (POSIX only). On bionic this is `__register_atfork` with
  `__dso_handle`, so it is unregistered on dlclose.
- **prepare** takes exit_mutex and every queue lock, and **parent** unlocks
  them. The child therefore gets consistent ring state and unlocked mutexes.
- **child**, for each queue:
  - it signals the fences of the jobs it inherited (they belong to the
    parent and are not run twice) and clears the ring;
  - it sets num_threads = 0, `respawn_after_fork` = true (only if the queue
    had live workers) and create_threads_on_demand = true;
  - it re-inits both condvars with cnd_init and no cnd_destroy, because they
    may still count the parent's waiters and glibc's destroy would block.
- `util_queue_add_job` in the child starts worker 0 again on the first job.
  So the child's cache writes are kept: a forked process still fills and
  uses the cache, the cache stays enabled, and no blob limits are needed.
- `finish` and `destroy` in a child that never added a job return at once,
  because num_threads is 0.
- New field `struct util_queue::respawn_after_fork`. disk_cache.c embeds
  util_queue and is rebuilt in the same tree.

**Host repro:** `tools/u-queue-fork-test/` (`build.py B M out`; B = a host
Mesa build dir, M = the tree under test). The test:
1. the parent runs disk_cache_create and one put, then forks;
2. the child runs N puts and disk_cache_destroy, with a 5 s alarm;
3. the parent then reads back every entry the child wrote.

| tree | result |
|---|---|
| mesa-bc4-strict (bc4) | `HANG child disk_cache_destroy did not return in 5 s` (exit 2) |
| mesa-bc + 209 | PASS for N = 0, 1, 3, 100 (all child entries readable); stress 40/40 PASS at N = 50 |

## Series check

`git worktree add /home/flux/src/mesa-bc5-strict 5a07217f` plus
`scripts/apply-patches.sh --profile g615-v11-csf` gives `OK applied=204`,
with no drift. 209 only touches `src/util/u_queue.[ch]`.

## Build

- **Command:** `PANVK_KBASE_RELEASE=RC5-dev-bc5 PANVK_JOBS=4
  MESA=/home/flux/src/mesa-bc5-strict BDIR=/home/flux/build-bc5/bionic-r30-RC5-dev-bc5
  DDIR=/home/flux/build-out/2026-10-22-bc5 HOST_TOOLS=.../build/host-tools/bin
  flock /home/flux/build-cache/build.lock sh scripts/build-android.sh
  --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/30.0.16248370`.
  Thermal was NONE, so 4 jobs. The BDIR was fresh and the build exited 0.
- **Driver .so:** `/home/flux/build-out/2026-10-22-bc5/libvulkan_panfrost.so`,
  26331032 bytes, sha256
  `e41f0fbf2f1e9d6301f24edc9898a69f29a075f8297002fefd71e6710bd58494`.
  It contains RC5-dev-bc5, the 205/208 strings (PANVK_GPU_PRERAST_ARENA,
  astc_sf) and imports `__register_atfork` (209).
- **APK:** built with `./gradlew assembleDebug -PpanvkSoRc4=/home/flux/build-out/rc4-lowmem/libvulkan_panfrost-android-aarch64.so
  -PpanvkSoRc5=<so above> -PpanprobeSuffix=bc5`, under the build lock.
  - Output: `/home/flux/build-out/apks/panprobe-bc5.apk`, sha256
    `fe71dfaf411b4d44f89be094c8bea06987946fb10d4b229aa339af27c77ff9fc`.
  - aapt2: `dev.zenithblue.panvktest.bc5`, versionCode 15, versionName 1.2.7.
  - The rc5 .so inside is e41f0fbf... (matches). The rc4 .so is c395db09...
    (RC4-lowmem, matches).

## Device plan (G615, not run)

Main (651fa26) makes PanProbe set `MESA_SHADER_CACHE_DISABLE=true` by
default as a workaround. To test 209 with the cache on, override it:

1. Run `--es driver rc5 --es autorun vkd3d_heap --es env MESA_SHADER_CACHE_DISABLE=false`.
   The target is 4/4 PASS, with no watchdog.
2. Run the same command with `--es autorun all`. The target is 38/38 with
   bc_decode PASS. Also check that `/data/user/0/dev.zenithblue.panvktest.bc5/cache/mesa_shader_cache`
   gets entries.
3. Run bc_perf with the cache on and with it off, and confirm the bc4
   upload table holds (bc5 changes no BC code).
4. If these pass, drop the PanProbe default and the launcher README note
   (see PROGRESS.md).

## jm-v7-wip (v6/v7) check

- **Same bug, latent.** u_queue and disk_cache are shared util code. jm-v7-wip sits
  on the 197-patch base (ab7d630). That base has 170 (cache on by default on
  Android) but not 204, so in PanProbe's execve()d helpers the cache dir
  fails, no queue exists, and nothing hangs. That fits FTL v7N at 38/38.
- **What triggers it.** It shows up as soon as jm-v7 is rebased on a base
  with 204, or runs where the cache dir resolves (for example an app
  process). 209 is arch-independent and fixes it there too, so it needs no
  jm-v7 change.
- **Hashing.** 0064 (jm-v7 091) only adds the v6/v7 compile knobs to the
  cache key. 0052 (bindless/mutable) does not touch the layout hash. There
  is no per-element hashing.

## BC1 / BC6H / BC7 speed (priority 2): profiled, not changed in bc5

Measured by the coordinator on bc4: BC1 ties RC4 (2.63 / 2.80 vs 2.63 / 2.91 ms),
BC6H ties (8.57 / 9.77 vs 8.34 / 9.98) and BC7 is +4-7% (6.37 / 6.32 vs
6.12 / 5.88). All are inside the run spread.

Static Valhall v11 stats (tools/spvstat, compute variants; summed over all
branches, so they are upper bounds):

| shader | instrs | cycles | regs / threads | spills |
|---|---|---|---|---|
| bc7 (`panvk_bc_bc7.comp`) | 2425 | 75.4 (74.8 cvt) | 30 / 2 | 0 |
| bc6 (`panvk_bc_bc6.comp`) | 3870 | 120.9 (117.6 cvt) | 30 / 2 | 0 |
| etc2 BC1 (bc4 default 0x20000) | - | 50.81 | 58 / 1 | 17 |

Findings:

- **BC6H and BC7 decode one texel per invocation.** The fragment path
  (`bc_decode_region_gfx`, one render pass per mip) re-decodes the block
  header for each of the 16 texels: `findLSB(mode)`, partition and
  endpoints are recomputed for every texel. Each block is decoded 16 times.
  The texels write the same B10G11R11 / AFRC output size as REF_RGBA8,
  which takes 2.57 ms, so the 6-9 ms is decode ALU.
- **The realistic win is per-block decode.** One invocation per block would
  decode the header once and emit 16 texels:
  - for BC6H, this is compute with a storage write into the B10G11R11
    shadow through an R32UI view;
  - for BC7, the output is AFRC and must be written by a render pass. The
    options are a per-block compute decode into a small RGBA8 staging image
    followed by a blit pass, or a fragment shader that decodes once per quad
    and shares the result through subgroup or quad ops.
  This is a rewrite of the Granite decoders. It changes what bc_decode
  checks, so it needs device verification of bc_decode, 38/38 and
  bc-bench quality.
- **Per-mip fixed cost.** BC6H/BC7 are one render pass per level. The
  levels write different attachments, so they cannot be merged into one
  pass. The small levels (64² and below, about 6 of 12) are a fixed cost of
  roughly 0.3-0.5 ms per chain, estimated, not measured.
- **BC1.** It has 58 registers and 17 spills at 0x20000. Getting to 32 or
  fewer registers needs opts 0, which bc4.md measured at about -6 dB, so
  that was rejected. A cheaper fit without the 17 spills is the next
  candidate.
- **Why nothing changed.** No device was available to show "strictly faster
  than RC4 with no regression of bc4's wins". The spread is up to
  1.1-1.4 ms, which is larger than the BC1/BC6H/BC7 gaps. So bc5 ships no
  BC changes, and the per-block BC6H/BC7 rewrite is the next item (bc6).
