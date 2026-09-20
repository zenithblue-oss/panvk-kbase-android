# Upstream PanVK Audit For DXVK/vkd3d

## Scope

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`
- Audited `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`
- Audit date: `2026-09-20`
- Runtime source: `consumer-capabilities.json`, exact Android beta.3 ICD SHA-256 `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`

`work/mesa` is generated and dirty with the existing Kbase patch stack. It was inspected only. No generated source was edited.

## Plan Filename Mapping

The pre-existing consumer tooling is broader than the DXVK/vkd3d-only names. Thin wrappers preserve one implementation:

| Required name | Authoritative implementation/output |
|---|---|
| `tests/dxvk-vkd3d/g615_caps.c` | wraps `tests/consumer-compat/g615_consumer_caps.c` |
| `scripts/capture-dxvk-vkd3d-caps.py` | wraps `scripts/capture-consumer-capabilities.py` |
| `scripts/import-dxvk-profile.py` | constrained wrapper over `scripts/import-consumer-profile.py` |
| `scripts/import-vkd3d-profile.py` | constrained wrapper over `scripts/import-consumer-profile.py` |
| `scripts/evaluate-vulkan-profile.py` | wraps `scripts/evaluate-consumer-profile.py` |
| `validation/g615-v11-csf/dxvk-vkd3d-caps.json` | `validation/g615-v11-csf/consumer-capabilities.json` is the authoritative superset |

## Result

The pinned Mesa commit is already close to current upstream. Only three later commits touch PanVK/Panfrost Vulkan/compiler/kmod paths:

| Commit | Subject | Relevance |
|---|---|---|
| `1a63ca3af98` | `pan/nir: Optimize Cubemaps on v11+` | Unrelated optimization |
| `a78c40578d6` | `kraid: Wire up projection_enable flag` | Unrelated compiler plumbing |
| `2092558f77d` | `pan/nir: Add projection_enable flag to pan_va_tex_flags` | Unrelated compiler plumbing |

No post-pin commit implements a failed DXVK/vkd3d requirement. Minimal coherent P4 backport set: **empty**.

## Failed Capability Audit

| Capability | Beta.3 | Pinned source | Current upstream | P4 decision |
|---|---:|---|---|---|
| `geometryShader` | false | explicitly false | explicitly false | No backport exists |
| `tessellationShader` | false | explicitly false | explicitly false | No backport exists |
| `fillModeNonSolid` | false | explicitly false | explicitly false | No backport exists |
| `multiViewport` | false | explicitly false | explicitly false | No backport exists |
| `shaderClipDistance` | false | explicitly false | explicitly false | No backport exists |
| `shaderCullDistance` | false | explicitly false | explicitly false | No backport exists |
| `pipelineStatisticsQuery` | false | explicitly false | explicitly false | No backport exists |
| `VK_EXT_transform_feedback` | absent | absent | absent | No backport exists |
| `textureCompressionBC` | false | hardware format mask | hardware format mask | Native G615 unsupported; no bit spoofing |
| sparse binding | false | generic pan-kmod conditional path | same generic path | Kbase VM equivalence unproven; do not enable |

Source locations for explicit core reporting are `src/panfrost/vulkan/panvk_vX_physical_device.c:306-350` in both revisions. Generic sparse plumbing in current Mesa does not establish custom Kbase UAPI 1.21 semantics.

## Already Present

The captured beta.3 runtime already exposes modern upstream work needed before P5: Vulkan 1.4.363, descriptor indexing and required indexing bits, 1M-class reported UpdateAfterBind per-set limits, maintenance5/6, dynamic rendering, synchronization2, buffer device address, push descriptors, robustness2 `robustBufferAccess2` and `nullDescriptor`. These are exposure observations only. P5 must prove descriptor behavior and capacity; reported limits are not workload proof.

## Unknowns

- No CTS or new device workload ran during P0-P4.
- `robustImageAccess2` is false in the capture; semantic support remains unproven.
- Sparse behavior through custom Kbase is `BLOCKED`, not inferred from generic pan-kmod.
- Hardware results beyond the preserved direct-ICD capture remain `UNKNOWN`.
