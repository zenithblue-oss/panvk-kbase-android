# Claude Opus 5.5 — D3D12 FL 12_0 Architecture Council Review (Round 3)

**Score: 7.4 / 10**

## 1. Verdict

Round 3 fixes most of the Round 1 problems (4.7/10). Making FL 12_0 independent of sparse, limiting sparse to CSF only, dropping fake per-tile dirty bits, banning mid-frame shrink and charging the budget honestly are all correct.

Three problems in the plan's own logic stop it scoring 8 or more:
- The rule for choosing Path 3A or 3B contradicts the Path 3B budget rule.
- The tile swizzle requirement is not needed.
- Dropping per-tile dirty tracking goes too far: it also removes tracking for copy writes, where it is cheap.

## 2. Key Technical Strengths

1. **FL 12_0 no longer depends on sparse.** Bucket A titles ship after Phase 0 alone, and no sparse stage can block them. This is the biggest gain.
2. **CSF v10-v14 only.** This removes the difference between JM and CSF fault handling and sync. Two GPU families to prove, not three.
3. **The budget is honest.** It accepts that kbase commits a contiguous prefix [0 .. high-water mark). Charging the full virtual size against `PANVK_SPARSE_CAP_MB` means no hidden over-commit.
4. **Per-tile dirty tracking is gone for shader writes.** It cannot work with bindless descriptor indexing: the driver cannot see which tile a UAV write touches.
5. **No `MEM_COMMIT` shrink mid-frame.** This avoids `mmap_lock` contention and a cache flush across the whole address space while the GPU is busy.
6. **The census buckets and the vkd3d audit come before any code.** Bucket C (aliasing) is declared out of scope on kbase.
7. **Path 3A is the default.** Sparse work must prove it is needed.

## 3. Remaining Risks, Edge Cases & Recommendations

1. **Test A decision rule contradiction (high)**: kbase `GROW_ON_GPF` grows the backed size up to the fault offset. "Prefix explosion" is a known kernel behaviour. Path 3A says prefix explosion means kill sparse, while Path 3B already assumes prefix explosion. Fix: drop "prefix explosion" as a reason to kill; keep only context survival without CSG fault.
2. **Bucket B depends on patching vkd3d (high)**: vkd3d-proton reports tiled tier NOT_SUPPORTED unless `sparseResidencyAliased` is set, even for Tier 1. 3B needs a vkd3d fork or environment variable. Put "vkd3d patch exists, is maintained and is bundled" in the 3B entry condition.
3. **Census can miss Bucket B titles (high)**: On Mali, vkd3d reports tiled tier 0, so titles take their non-tiled path and never call `CreateReservedResource`. Run the census on a reference GPU that reports Tier 2, or with tiled tier forced on in a patched vkd3d.
4. **Compute swizzle not needed for Buckets A and B (medium)**: vkd3d reports `StandardSwizzle64KBSupported = FALSE`. Copying between backing image and heap can be a strided `vkCmdCopyBuffer` (8 regions of 16x16-block rows per tile) without a custom swizzle shader.
5. **Phase 0 is missing typed UAV loads (medium)**: FL 12_0 needs typed UAV loads for additional formats (`shaderStorageImageReadWithoutFormat`). Add it to Phase 0.
6. **"Native FL 12_0" wording contradicts spec-integrity**: Change to "override-reported FL 12_0" in release notes and PanProbe text.

## 4. Final Recommendation
Approve the gated plan with conditions: run census first with override on and log tier queries; gate override per arch on Resource Binding Tier 2 and typed UAV loads; S-Pre in parallel; S0 GPU gate tests read-0, edges, context survival, region creation rate and fault latency.
