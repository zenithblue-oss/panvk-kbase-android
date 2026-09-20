# Kbase UAPI 1.21 sparse / VM feasibility

## Decision

**NO-GO.** Do not enable PanVK sparse on the custom `mali_kbase` backend.

Do not set:

```text
sparseBinding
sparseResidencyBuffer
sparseResidencyImage2D
sparseResidencyImage3D
sparseResidencyAliased
sparseResidency2Samples
sparseResidency4Samples
sparseResidency8Samples
sparseResidency16Samples
shaderResourceResidency
shaderResourceMinLod
VK_QUEUE_SPARSE_BINDING_BIT
```

Do not enable Mesa 26.2 pan/kmod sparse, VM-operation buffering, or sparse mapping emulation on this ICD. That stack targets Panthor DRM `VM_BIND`, not Kbase UAPI 1.21.

P22 is investigation only. No PanVK patch. No feature-bit change. No CTS/G615 sparse workload.

## Scope

- Device: Mali-G615 MC6 / Valhall v11 / CSF / Poco X6 Pro
- Kernel path: custom `mali_kbase` (`/dev/mali0`), CSF UAPI 1.21
- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b` (26.3.0-devel)
- Runtime capture: `validation/g615-v11-csf/consumer-capabilities.json`
- ICD SHA-256: `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`

Sparse is required only for higher D3D feature levels (DXVK FL12, vkd3d tiled resources / FL 12_0). It is not required for D3D9/D3D10/D3D11 or basic D3D12 device creation. P21 already reports implied `D3D_FEATURE_LEVEL_11_0` with `D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED`.

## Runtime bits (unchanged)

| Bit / property | G615 capture |
|---|---|
| `sparseBinding` | false |
| `sparseResidencyBuffer` | false |
| `sparseResidencyImage2D` | false |
| `sparseResidencyImage3D` | false |
| `sparseResidencyAliased` | false |
| `sparseResidency{2,4,8,16}Samples` | false |
| `shaderResourceResidency` | false |
| `shaderResourceMinLod` | false |
| `VK_QUEUE_SPARSE_BINDING_BIT` | absent (no BIND family advertised) |
| `residencyStandard2DBlockShape` | true (property only; feature bits false) |
| `residencyStandard2DMultisampleBlockShape` | false |
| `residencyStandard3DBlockShape` | false |
| `residencyAlignedMipSize` | false |
| `residencyNonResidentStrict` | false |
| `sparseAddressSpaceSize` | `0x000000feff7f0000` (VA range size, not sparse proof) |

`vkCreateDevice` with `sparseBinding=true` must keep returning `VK_ERROR_FEATURE_NOT_PRESENT`.

## Custom mali_kbase vs upstream pan/kmod sparse

Upstream Mesa 26.2 PanVK sparse is a Panthor path:

1. Userspace VA heap (`panvk_as_alloc`) picks an explicit GPU VA.
2. `pan_kmod_vm_bind` issues `DRM_IOCTL_PANTHOR_VM_BIND`.
3. Panthor maps a BO at that VA, unmaps a VA range, or maps a sparse (unbacked) range (`DRM_PANTHOR_VM_BIND_OP_MAP_SPARSE`).
4. Async bind uses `DRM_PANTHOR_VM_BIND_ASYNC` plus DRM syncobjs.
5. `panthor_kmod.c` sets `supported_vm_op_flags |= PAN_KMOD_VM_OP_OP_MAP_SPARSE` from Panthor uAPI 1.9.
6. `panvk_async_bind.c` implements `vkQueueBindSparse`.
7. `GetPhysicalDeviceQueueFamilyProperties2` adds `VK_QUEUE_SPARSE_BINDING_BIT` on the GPU family and a dedicated BIND family when `sparseBinding` is true.

This project does not use Panthor. The kbase backend:

- is not a DRM driver; there is no GEM handle and no `drmPrimeFDToHandle`
- forces SAME_VA on 64-bit non-executable allocations; kernel assigns VA
- implements `vm_bind` MAP as "report the VA assigned at `MEM_ALLOC` time"
- implements `vm_bind` UNMAP as a no-op until `MEM_FREE` / munmap
- rejects `PAN_KMOD_VM_OP_MODE_ASYNC`
- rejects any MAP whose `va.start` is not the kbase-assigned address
- never sets `supported_vm_op_flags` (`PAN_KMOD_VM_OP_OP_MAP_SPARSE` is unset)
- never calls `KBASE_IOCTL_MEM_COMMIT`

The tree already gates exposure:

```c
bool has_sparse = PAN_ARCH >= 10 && !device->kbase_node_path[0];
```

in `panvk_vX_physical_device.c` (`patches/kbase-common/006-device-queue-wsi-kbase.patch`). BIND-family submit on kbase is a stub that marks the queue lost: `"kbase: sparse binding queues are not implemented yet"`. That stub must not become reachable by flipping `has_sparse`.

Mesa sparse-mapping emulation (`pan_kmod_vm_multi_op_emulate_sparse`) still calls `vm_bind` MAP at an explicit VA onto a dummy 2 MiB BO. Kbase `vm_bind` fails that MAP. Emulation does not rescue this backend.

## Operation map (Vulkan / PanVK → Kbase UAPI 1.21)

### 1. Reserve VA

**Vulkan / PanVK:** `vkCreateBuffer` / `vkCreateImage` with `SPARSE_BINDING` allocates a VA range from `device->as.heap` (`panvk_as_alloc`) without committing physical pages. Later `vkQueueBindSparse` maps backing into that range.

**Kbase 1.21:** `KBASE_IOCTL_MEM_ALLOC` / `MEM_ALLOC_EX` take `va_pages` and `commit_pages`. `va_pages > commit_pages` reserves extra GPU VA. `MEM_ALLOC_EX` (CSF ≥ 1.9, nr 59) adds `fixed_address` when `BASE_MEM_FIXED` is set.

**Result: INSUFFICIENT.** Reservation exists only as a kernel-owned region. 64-bit clients get SAME_VA: the kernel returns a mmap cookie, mmap establishes CPU VA == GPU VA, and userspace cannot pick the GPU VA. `kbase_kmod_alias_create` documents that kbase `get_unmapped_area` rejects `MAP_FIXED` and address hints. `BASE_MEM_FIXED` / `FIXABLE` are whole-allocation placement flags, not a sparse VA heap. PanVK sparse requires a userspace-chosen VA that later MAP/UNMAP ops target. Kbase has no such heap.

### 2. Map pages

**Vulkan / PanVK:** `VkSparseMemoryBind` / opaque image bind maps a memory object into `[resourceVA + offset, size)`.

**Kbase 1.21:** Physical backing is created at `MEM_ALLOC` (`commit_pages`) or grown with `KBASE_IOCTL_MEM_COMMIT` (`gpu_addr`, `pages`) on a growable region (`BASE_MEM_GROW_ON_GPF`). Commit changes the committed page count of one region. It does not map an arbitrary existing BO into an arbitrary hole.

**Result: INSUFFICIENT.** Grow-from-front of one region is not sparse bind. The kbase backend never issues `MEM_COMMIT`. `PAN_KMOD_BO_FLAG_ALLOC_ON_FAULT` maps to `GROW_ON_GPF` for tiler heaps (2 MiB extension), not for `vkQueueBindSparse`.

### 3. Unmap pages

**Vulkan / PanVK:** bind with `VK_NULL_HANDLE` memory, or `PAN_KMOD_VM_OP_TYPE_UNMAP` / `MAP_SPARSE` blackhole, punches a hole in the resource VA.

**Kbase 1.21:** `MEM_COMMIT` with a smaller `pages` shrinks growable backing from the end of the region. `MEM_FREE` / SAME_VA munmap destroys the whole region. There is no hole-punch ioctl.

**Result: INSUFFICIENT.** Cannot unmap an interior page range while keeping the VA reservation.

### 4. Rebind pages

**Vulkan / PanVK:** replace backing of a VA range with a different memory object / offset. Panthor `VM_BIND` MAP over an existing range.

**Kbase 1.21:** No ioctl maps a different handle into an existing region's subrange. `MEM_FLAGS_CHANGE` only toggles `DONT_NEED` / coherency. Replacing backing means free + alloc, which yields a new kernel VA.

**Result: INFEASIBLE on 1.21.**

### 5. Alias mappings

**Vulkan:** `sparseResidencyAliased` — same physical pages in more than one sparse resource VA.

**Kbase 1.21:** `KBASE_IOCTL_MEM_ALIAS` packs existing regions (`base_mem_aliasing_info` handle/offset/length in pages) into a new alias. Kernel picks the alias VA. CPU_WR is forbidden on alias regions. Source must be GPU-cached in the same coherency domain. `BASE_MEM_ALIAS_MAX_ENTS` is 24576. The kbase helper `kbase_kmod_alias_create` uses this only to repeat one BO for CSF ring wraparound, then mmap SAME_VA.

**Result: INSUFFICIENT for Vulkan sparse aliasing.** A new kernel-chosen read-mostly VA over existing pages is not "bind these pages at this sparse resource offset". Upstream PanVK already reports `sparseResidencyAliased=false` on Panthor (`panfrost/mesa#237`). Do not advertise it here.

### 6. Buffer sparse bind

**Vulkan / PanVK:** `vkQueueBindSparse` → `panvk_async_bind_sparse_memory_bind` → `vm_bind` MAP at `buffer->vk.device_address + resourceOffset`.

**Kbase 1.21:** `kbase_kmod_vm_bind` MAP succeeds only when `op->va.start` equals `kbase_bo->gpu_va + bo_offset`. Explicit VA returns -1: `"cannot map BO at explicit VA"`.

**Result: INFEASIBLE.** The PanVK buffer sparse path cannot run on this backend.

### 7. Image opaque bind

**Vulkan:** `VkSparseImageOpaqueMemoryBindInfo` treats the image as a byte VA range (mip tail, metadata, whole-resource bind).

**Kbase 1.21:** Same explicit-VA MAP requirement as buffers. Image metadata/mip-tail placement still needs a stable userspace VA.

**Result: INFEASIBLE.** Same VM gap as buffer sparse bind.

### 8. Image sparse bind

**Vulkan:** `VkSparseImageMemoryBind` at texel-block granularity (`STANDARD_SPARSE_BLOCK_SIZE_B` = 65536 in `panvk_image.c`). PanVK reports `residencyStandard2DBlockShape=true` for 2D.

**Kbase 1.21:** Page size is 4 KiB (`LOCAL_PAGE_SHIFT` / `PAN_PGSIZE_4K`). Growable commit is in 4 KiB pages, from the start of one region, not 64 KiB image tiles at arbitrary mip/array/layer offsets.

**Result: INFEASIBLE.** Even a commit-based emulation cannot place a 64 KiB tile at an interior image-block offset without an explicit-VA map.

### 9. Page granularity

| Layer | Granularity |
|---|---|
| Kbase UAPI | 4 KiB pages (`MEM_ALLOC` / `MEM_COMMIT` / alias offsets) |
| Grow-on-GPF used by kmod | 2 MiB extension for `ALLOC_ON_FAULT` heaps |
| Vulkan sparse block (PanVK) | 64 KiB standard 2D block |
| Vulkan buffer bind | typically 64 KiB (`sparseAddressSpaceSize` is the VA window, not the bind grain) |

**Result: MISMATCH.** 4 KiB is finer than 64 KiB, so grain size alone is not the blocker. The blocker is inability to map/unmap an arbitrary 4 KiB (or 64 KiB) range at a chosen VA.

### 10. Fault behavior

**Vulkan sparse:** unbacked pages are either a blackhole (implementation-defined reads, discarded writes) or a device-lost fault. PanVK maps `PAN_KMOD_VM_OP_OP_MAP_SPARSE` (Panthor native sparse PTE, or dummy-BO emulation). `residencyNonResidentStrict=false` even on Panthor. `shaderResourceResidency` stays false: shaders cannot query residency.

**Kbase 1.21:** `BASE_MEM_GROW_ON_GPF` commits `extension` pages on GPU fault and grows the region. That is JIT heap growth, not sparse residency. Access to a never-committed hole is a GPU page fault / fatal event, not a blackhole mapping. `vm_query_state` always returns `PAN_KMOD_VM_USABLE`; there is no VM-fault query. `CINSTR_GWT_*` is a write-fault dump for tools, not sparse residency.

**Result: INFEASIBLE** to match Vulkan sparse fault/blackhole semantics with `GROW_ON_GPF`.

### 11. Queue ordering

**Vulkan:** `vkQueueBindSparse` on a queue with `VK_QUEUE_SPARSE_BINDING_BIT`. Binds are ordered with waits/signals on that queue. PanVK uses a dedicated BIND family plus optional SPARSE bit on the GPU family.

**Kbase 1.21:** CSF queue groups (`CS_QUEUE_REGISTER` / bind / kick) submit GPU command streams. There is no VM-bind queue and no ioctl that batches map/unmap with CS work. The kbase BIND-family stub exists only to fail if created.

**Result: INFEASIBLE** to advertise `VK_QUEUE_SPARSE_BINDING_BIT`.

### 12. Synchronization

**Vulkan:** sparse binds wait on semaphores and signal semaphores. PanVK `panvk_async_bind.c` translates those to `pan_kmod_sync_op` on `PAN_KMOD_VM_OP_MODE_ASYNC` (DRM syncobj).

**Kbase 1.21:** `PAN_KMOD_VM_OP_MODE_ASYNC` returns -1 (`"PAN_KMOD_VM_OP_MODE_ASYNC not supported"`). No DRM syncobj. Available sync is CSF event memory, `STREAM_CREATE` / fence fd validate, and `kbase_kmod_csf_wait_cqs64`. Those complete GPU CS work, not VM map completion. Immediate `vm_bind` on kbase does not wait for in-flight CS access before a no-op UNMAP.

**Result: INFEASIBLE** to implement Vulkan sparse queue synchronization.

## UAPI inventory (what 1.21 actually has)

Present and relevant:

```text
KBASE_IOCTL_MEM_ALLOC          nr 5     reserve VA + commit pages
KBASE_IOCTL_MEM_ALLOC_EX       nr 59    same + fixed_address (CSF ≥ 1.9)
KBASE_IOCTL_MEM_QUERY          nr 6     COMMIT_SIZE / VA_SIZE / FLAGS
KBASE_IOCTL_MEM_FREE           nr 7     destroy region
KBASE_IOCTL_MEM_COMMIT         nr 20    change committed page count
KBASE_IOCTL_MEM_ALIAS          nr 21    pack existing regions, kernel VA
KBASE_IOCTL_MEM_IMPORT         nr 22    dma-buf / user buffer
KBASE_IOCTL_MEM_FLAGS_CHANGE   nr 23    DONT_NEED / coherency
KBASE_IOCTL_MEM_JIT_INIT       nr 14    JIT VA pool (tiler/CSF, not sparse)
BASE_MEM_GROW_ON_GPF                    grow region on GPU fault
BASE_MEM_SAME_VA / NEED_MMAP            64-bit placement contract
BASE_MEM_FIXED / FIXABLE                whole-allocation GPU VA request
```

Absent (needed for PanVK sparse):

```text
explicit-VA MAP of an existing handle into a reserved range
UNMAP of an interior VA range while keeping the reservation
ASYNC VM_BIND with wait/signal sync objects
MAP_SPARSE / blackhole PTE
userspace VA allocator that the kernel will honor for later binds
sparse bind queue distinct from CSF GPU queues
```

`MEM_ALLOC_EX.fixed_address` does not close the gap: it requests one address at allocation of a new region, not rebind of subranges, and SAME_VA 64-bit clients do not use it.

## Compiler / shader bits

`shaderResourceResidency` and `shaderResourceMinLod` are false on pinned and current upstream PanVK even when Panthor sparseBinding is true. They need shader ISA support for residency queries and MinLod, plus non-resident image semantics. Out of scope until a real VM path exists. Keep false.

## What would be required for a later GO

A future GO needs kernel + userspace work, not a Mesa 26.2 backport:

1. Kbase (or a new ioctl family) must map/unmap arbitrary page ranges of existing allocations at a userspace-reserved GPU VA.
2. Unbacked ranges must be a defined blackhole or a queried fault, not `GROW_ON_GPF`.
3. Bind completion must be waitable (sync_file / CSF event / equivalent) so `vkQueueBindSparse` ordering is real.
4. Image 64 KiB block layout must be proven against that VM.
5. Only then consider `sparseBinding` / residency bits, a BIND queue, and CTS `dEQP-VK.sparse_resources.*`.
6. `sparseResidencyAliased`, `shaderResourceResidency`, and `shaderResourceMinLod` stay independent later gates.

Until that UAPI exists, any userspace-only emulation (full commit of every sparse resource, or dummy-BO maps at kernel-chosen VAs) would either ignore the application's chosen binds or report a GPU VA the GPU does not use. That is spoofing. Forbidden.

## Recommended action

```text
DECISION=NO-GO
KEEP_FALSE=sparseBinding,sparseResidency*,shaderResourceResidency,shaderResourceMinLod
KEEP_ABSENT=VK_QUEUE_SPARSE_BINDING_BIT
DO_NOT_ENABLE=Mesa 26.2 pan/kmod sparse, has_sparse on kbase, BIND queue
NEXT=P23 final DXVK/vkd3d compliance matrix; implied D3D12 FL stays 11_0
```

P21 DEVICE_CREATE blockers (`robustImageAccess2`, `transformFeedbackQueries`) remain the next real engineering work. Sparse is not that work.
