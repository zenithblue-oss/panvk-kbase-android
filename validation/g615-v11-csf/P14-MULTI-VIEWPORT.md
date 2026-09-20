# P14 multiViewport

## Result

`BLOCKED_SAFE_FALSE`

This is **not** `multiview`. G615 already reports `multiview=true` /
`maxMultiviewViewCount=8`. That is per-view framebuffer layers. `multiViewport`
is multiple `VkViewport` / scissors plus `ViewportIndex`. They are independent.

No complete PanVK, compiler, or Valhall v11 implementation exists for multiple
viewports. The G615 runtime remains:

```text
multiViewport=false
maxViewports=1
shaderOutputViewportIndex=false
```

`vkCreateDevice` with `multiViewport=true` or `shaderOutputViewportIndex=true`
returns `VK_ERROR_FEATURE_NOT_PRESENT`.

No PanVK patch or feature-reporting change was made. P15 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Post-pin Panfrost commits: `1a63ca3af98`, `a78c40578d6`, `2092558f77d`. None
  implement multiViewport.
- Pinned and current upstream PanVK both report `multiViewport=false`,
  `maxViewports=1`, `shaderOutputViewportIndex=false` in
  `src/panfrost/vulkan/panvk_vX_physical_device.c`.
- No coherent PanVK multiViewport series exists to backport.

### Command stream programs viewport 0 only

CSF `prepare_vp()` (`panvk_vX_cmd_draw.c`) always reads
`dynamic_graphics_state.vp.viewports[0]` and `scissors[0]`. JM
`panvk_emit_viewport()` does the same and returns if `viewport_count < 1`.

Shared FAU sysvals (`panvk_shader.h`, `cmd_prepare_draw_sysvals`) store one
`viewport.scale` and one `viewport.offset`. `nir_lower_viewport_transform`
always loads that single pair. Every VS position is transformed into viewport 0
screen space.

On Valhall v11 the IDVS state registers are:

```text
SCISSOR_BOX        = 42   single 64-bit box
LOW_DEPTH_CLAMP    = 44
HIGH_DEPTH_CLAMP   = 45
```

There is no `VIEWPORT_HIGH` / `VIEWPORT_LOW`. Those appear in v12+ XML and are
used only under `#if PAN_ARCH >= 12`. G615 is PAN_ARCH=11. v11 depth range is
one clamp pair, not a per-viewport depth array.

### Scissor array is not a viewport array

v11 `Primitive Flags` has `Scissor array enable` at bit 21. CSF decode documents
that when set, `SCISSOR_BOX` is a pointer: bottom 56 bits address, top 8 bits
length. PanVK never assigns `scissor_array_enable = true`. `decode_csf.c`
asserts it is false. The v12 CSF path still has:

```c
/* XXX: Switch scissor_array_enable to true and use array based variant
 * for future proofness */
```

and still programs scissors[0] only.

A scissor array would still be incomplete:

1. VS transform remains viewport 0.
2. v11 depth clamp remains one range.
3. No proven ViewportIndex → scissor-slot mapping on v11.
4. Overlap, different origins, different sizes, and different depth ranges
   all require the transform and depth range, not only scissors.

### Compiler ViewportIndex is a stub, and collides with Layer

`pan_nir_collect_varyings.c` lists `VARYING_SLOT_VIEWPORT` as uint8 in the
ATTRIBS section at offset 2. `VARYING_SLOT_LAYER` uses the same offset 2.
`va_shader_output_from_loc()` maps `LAYER` and `PRIMITIVE_ID` to the attribute
FIFO, not `VIEWPORT`. The compiler never names `ViewportIndex` /
`viewport_index`. CSF draw sets `layer_index_enable` from `VARYING_BIT_LAYER`
only. There is no viewport-index enable.

Without GS (P12 `geometryShader=false`) the last pre-rasterization stage is VS.
Vulkan then requires `shaderOutputViewportIndex` for VS `ViewportIndex` writes.
That bit is also false.

### Gallium is single-viewport

Gallium Panfrost emits one `pipe_viewport_state` / one scissor. It does not
implement a viewport array. OpenGL support cannot be inferred into Vulkan
`multiViewport`.

## Semantic Blocker

Vulkan `multiViewport=true` requires, for every advertised viewport slot:

1. Independent `VkViewport` origin, size, minDepth, maxDepth.
2. Independent scissor.
3. `ViewportIndex` from the last pre-rasterization stage selecting transform,
   scissor, and exclusive scissor.
4. Dynamic viewport/scissor counts and values (`vkCmdSetViewport`,
   `vkCmdSetScissor`, and pipeline viewportCount/scissorCount > 1).
5. Correct overlap: primitives in different viewports rasterize independently
   even when rectangles overlap.

G615 currently has none of:

- a viewport array in the v11 IDVS registers
- a per-viewport VS transform
- a compiler path that exports ViewportIndex without clobbering Layer
- a wired scissor-array index source
- GS ViewportIndex (P12 blocked)

Flipping `multiViewport` would advertise illegal `viewportCount > 1` pipelines.

## Required Design

A future complete implementation must pick one coherent path and finish it
before exposure. Do not enable `scissor_array_enable` alone.

### Path A — native v11 scissor array + per-viewport transform

Only if hardware evidence shows v11 indexes the scissor array from a vertex
attribute ViewportIndex:

1. Prove the FIFO attribute, bit width, and out-of-range behavior on G615.
   Resolve the Layer/Viewport shared offset 2 packing.
2. Export ViewportIndex from VS when `shaderOutputViewportIndex` is
   implemented, or from GS if P12 exists. Same value for all vertices of a
   primitive.
3. Replace the single FAU `viewport.scale/offset` with an indexed table of
   size `maxViewports`. VS transform must use the primitive's ViewportIndex.
4. Program per-viewport depth ranges. v11 has one LOW/HIGH_DEPTH_CLAMP; a
   native array or a VS-written depth path is required.
5. Enable `scissor_array_enable`, upload N boxes, keep dynamic viewport/scissor
   updates coherent with FAU and depth state.
6. Keep `multiview` (view mask / layers) independent of viewport index.

This is new PanVK/compiler work, not an upstream backport. v12
`VIEWPORT_HIGH`/`VIEWPORT_LOW` is a later architecture and is unused by G615.

### Path B — split draws in software

Replay the draw once per used ViewportIndex with viewport/scissor 0 rebound.
Invalid for GPU-written ViewportIndex, indirect draws, and GS. Reject unless
ViewportIndex is a pipeline constant, which is not the Vulkan feature.

Do not expose a subset (two hardcoded viewports, scissors-only, no
ViewportIndex, no dynamic count).

## Required Test Matrix

Rendering tests are invalid while `multiViewport` is disabled. These cases
remain blocked:

```text
2 viewports
4 viewports
maximum supported
different origins
different sizes
different depth ranges
overlap
dynamic viewport
dynamic scissor
ViewportIndex from last pre-rasterization stage
```

`tests/dxvk-vkd3d/test_p14_multi_viewport_safe_false.py` guards against
accidental exposure and detects a future PanVK, compiler, or Valhall
implementation that requires this assessment to be repeated.

`tests/dxvk-vkd3d/p14-multi-viewport.c` is the G615 runtime probe: it asserts
the feature stays false, `maxViewports` stays 1, `shaderOutputViewportIndex`
stays false, and enabling `multiViewport` is rejected. It is not a substitute
for rendering or CTS. It also prints `multiview` so the two features are not
confused.

The connected G615 runtime is recorded in `p14-multi-viewport-2026-09-20.txt`.
Serial `Y5WWBMJVOZSK4HU8` (`duchamp` / `2311DRK48I`) ran the probe against ICD
SHA-256 `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`:

```text
device=Mali-G615 MC6
api=1.4.363
multiViewport=NO
maxViewports=1
shaderOutputViewportIndex=NO
multiview=YES
create_multiViewport_true=VK_ERROR_FEATURE_NOT_PRESENT
create_shaderOutputViewportIndex_true=VK_ERROR_FEATURE_NOT_PRESENT
result=BLOCKED_SAFE_FALSE
```

Vulkan CTS is `BLOCKED`: no `deqp-vk` binary or matching CTS test list is
installed on the host or G615. CTS version, test-list hash, and
pass/fail/skip/crash/device-lost counts are `NOT_TESTED`.

Host guard: `python3 -m unittest tests.dxvk-vkd3d.test_p14_multi_viewport_safe_false`
`9/9` PASS.

## Exit

P14 exits `BLOCKED_SAFE_FALSE`. `multiViewport` stays false. Next engineering
step: hardware proof of v11 ViewportIndex plus a complete per-viewport
transform/scissor/depth path, then the full matrix and Vulkan CTS. P15 was not
started.
