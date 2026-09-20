# P22 sparse / Kbase feasibility

## Result

`NO-GO`

Investigation only. No PanVK patch. No feature-bit change. Mesa 26.2 pan/kmod
sparse was not enabled. No CTS. No G615 sparse workload.

```text
DECISION=NO-GO
UAPI=1.21
BACKEND=mali_kbase (not Panthor/DRM)
SPARSE_BINDING=false
SPARSE_RESIDENCY_BUFFER=false
SPARSE_RESIDENCY_IMAGE2D=false
SPARSE_RESIDENCY_IMAGE3D=false
SPARSE_RESIDENCY_ALIASED=false
SHADER_RESOURCE_RESIDENCY=false
SHADER_RESOURCE_MIN_LOD=false
VK_QUEUE_SPARSE_BINDING_BIT=absent
MESA_26_2_SPARSE=not enabled
CTS=not run
NEXT=P23 final DXVK/vkd3d compliance matrix
```

Authoritative mapping: `docs/KBASE-SPARSE-FEASIBILITY.md`.
Machine copy: `validation/g615-v11-csf/p22-kbase-sparse-feasibility.json`.

## Why P22 ran

P21 implied D3D12 FL 12_0 needs tiled resources (`sparseBinding` plus residency).
That is higher-feature-level work. It is not required for D3D9/D3D10/D3D11 or
basic D3D12 device create. P22 investigates whether Kbase can host PanVK sparse
before any implementation.

## UAPI feasibility

Kbase CSF UAPI 1.21 can reserve VA (`MEM_ALLOC` `va_pages` vs `commit_pages`),
grow/shrink one region (`MEM_COMMIT`, `GROW_ON_GPF`), and create a kernel-chosen
alias (`MEM_ALIAS`). It cannot:

- pick a userspace GPU VA and later MAP an existing BO into it
- UNMAP an interior range while keeping the reservation
- rebind a subrange to a different handle
- run `PAN_KMOD_VM_OP_MODE_ASYNC` with DRM syncobjs
- expose a sparse bind queue

`kbase_kmod_vm_bind` MAP fails unless `va.start` is the alloc-time address.
UNMAP is a no-op. `supported_vm_op_flags` is unset, so native `MAP_SPARSE` is
off. Mesa dummy-BO sparse emulation still MAPS at an explicit VA, so it also
fails on this backend.

## Operation map

| Operation | Feasibility |
|---|---|
| reserve VA | INSUFFICIENT |
| map pages | INSUFFICIENT |
| unmap pages | INSUFFICIENT |
| rebind pages | INFEASIBLE |
| alias mappings | INSUFFICIENT |
| buffer sparse bind | INFEASIBLE |
| image opaque bind | INFEASIBLE |
| image sparse bind | INFEASIBLE |
| page granularity | MISMATCH_NOT_SOLE_BLOCKER (4 KiB vs 64 KiB) |
| fault behavior | INFEASIBLE (`GROW_ON_GPF` is not blackhole) |
| queue ordering | INFEASIBLE |
| synchronization | INFEASIBLE |

## Custom mali_kbase vs upstream pan/kmod sparse

Upstream PanVK sparse is Panthor `DRM_IOCTL_PANTHOR_VM_BIND` plus
`panvk_async_bind.c`. This ICD uses `/dev/mali0`. The existing gate stays:

```c
bool has_sparse = PAN_ARCH >= 10 && !device->kbase_node_path[0];
```

Do not remove it. The kbase BIND-queue stub still reports
`"kbase: sparse binding queues are not implemented yet"` and must not become
reachable.

## Remaining sparse bits

All KEEP_FALSE / KEEP_ABSENT:

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

`shaderResourceResidency` and `shaderResourceMinLod` are false even on
upstream Panthor sparse. Independent later gates after a real VM path exists.

## Recommended action

Keep every sparse bit false. Do not backport or enable Mesa 26.2 sparse on
kbase. P21 DEVICE_CREATE still fails on `robustImageAccess2` and
`transformFeedbackQueries`; that is not a P22 job. P23 writes the final matrix
with implied D3D12 FL 11_0 and tiled resources unsupported.
