# P10 Shader Clip And Cull Distance

## Result

`BLOCKED_SAFE_FALSE`

No complete upstream or shared implementation can be wired into PanVK safely.
The G615 runtime state remains:

```text
shaderClipDistance=false
shaderCullDistance=false
```

No PanVK patch or feature-reporting change was made. P11 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Current upstream and the pinned tree both explicitly report both features
  `false` in `src/panfrost/vulkan/panvk_vX_physical_device.c`.
- `vk_nir.c` gathers and merges SPIR-V clip/cull variables, but this only
  normalizes shader I/O. It does not perform primitive clipping or culling.
- PanVK has no use of `clip_distance_array_size`,
  `cull_distance_array_size`, or `nir_lower_clip_fs`, and no draw state that
  communicates user clip/cull planes to the Valhall tiler.
- Valhall v11 XML exposes depth culling only. No user clip/cull plane descriptor
  was found.
- Panfrost Gallium lowers OpenGL user clip planes with `nir_lower_clip_fs` and
  fragment discard. That is an OpenGL path, not Vulkan support.

## Semantic Blocker

The shared Gallium path cannot satisfy the Vulkan contract:

1. `ClipDistance` clips a primitive at the zero crossing. Fragment discard can
   hide outside samples, but it does not create the clipped primitive boundary
   before rasterization.
2. `CullDistance` discards a primitive only when one array member is negative
   at every vertex. A per-fragment negative test implements different behavior.
3. `nir_lower_clip_fs` uses pixel barycentrics and contains an unresolved
   per-sample interpolation TODO. It is insufficient for required MSAA proof.
4. PanVK currently has no pipeline metadata carrying clip/cull array sizes or
   masks from the last pre-rasterization stage to draw emission.
5. No upstream PanVK implementation, coherent backport, or Valhall hardware
   export configuration exists to validate against.

Flipping either feature bit or reusing the Gallium fragment-discard path would
therefore expose incomplete Vulkan semantics.

## Required Design

A future implementation needs one coherent primitive-stage solution:

1. Preserve merged `ClipDistance` and `CullDistance` NIR I/O through PanVK
   compilation, including array sizes and the combined limit.
2. Extend shader metadata with clip and cull counts/masks for the last active
   pre-rasterization stage.
3. Either configure proven Valhall primitive clip/cull exports, or add a shared
   primitive lowering that can split crossing primitives and evaluate cull
   distance across all primitive vertices before rasterization.
4. Keep fragment inputs available with Vulkan interpolation semantics without
   using fragment discard as the clipping implementation.
5. Handle indexed draws, primitive restart, perspective, depth clipping, and
   sample locations without changing primitive assembly semantics.
6. Expose each feature only after its independent tests and the combined path
   pass on G615.

## Test Gate

No rendering test was submitted with disabled features: SPIR-V modules using
`ClipDistance` or `CullDistance` are invalid unless the corresponding feature
is enabled. Required deterministic pixel cases remain blocked pending the
primitive-stage implementation:

```text
single clip distance
multiple clip distances
cull distance
combined clip and cull
crossing plane
all in
all out
perspective
depth
MSAA
primitive restart
```

`tests/dxvk-vkd3d/test_p10_clip_cull_safe_false.py` guards against accidental
exposure and detects appearance of the missing PanVK lowering/metadata paths.
It is not a substitute for the rendering matrix.

The connected G615 direct-ICD capture and regression outcomes are recorded in
`p10-clip-cull-2026-09-20.txt`. The staged and repository Android ICD hashes
both equal `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`.

Vulkan CTS is `BLOCKED`: no `deqp-vk` executable or matching CTS test list is
installed on the host or connected G615. CTS version, test-list hash, and
pass/fail/skip/crash/device-lost counts are therefore `NOT_TESTED`.

## Exit

P10 exits `BLOCKED_SAFE_FALSE`. Both feature bits remain truthful. The next
required engineering step is Valhall primitive clip/cull hardware research or
a generic pre-rasterization primitive lowering, followed by the complete pixel
matrix and CTS. No P11 work was performed.
