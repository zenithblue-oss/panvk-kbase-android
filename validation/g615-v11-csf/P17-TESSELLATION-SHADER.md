# P17 tessellationShader

## Result

`BLOCKED_SAFE_FALSE`

No native Valhall v11 tessellation stage and no complete PanVK/shared
implementation exist. The G615 runtime remains:

```text
tessellationShader                         false
shaderTessellationAndGeometryPointSize     false
multiviewTessellationShader                false
extendedDynamicState3TessellationDomainOrigin false
primitiveTopologyPatchListRestart          false
maxTessellationGenerationLevel             0
maxTessellationPatchSize                   0
maxTessellationControlPerVertexInputComponents  0
maxTessellationControlPerVertexOutputComponents 0
maxTessellationControlPerPatchOutputComponents  0
maxTessellationControlTotalOutputComponents     0
maxTessellationEvaluationInputComponents        0
maxTessellationEvaluationOutputComponents       0
```

`vkCreateDevice` with `tessellationShader=true` returns
`VK_ERROR_FEATURE_NOT_PRESENT`.

No PanVK patch or feature-reporting change was made. P18 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Post-pin Panfrost commits: `1a63ca3af98`, `a78c40578d6`, `2092558f77d`. None
  implement tessellation. The pin-to-`origin/main` Panfrost diff has zero
  tessellation-related lines.
- Pinned and current upstream PanVK both report `tessellationShader=false` in
  `src/panfrost/vulkan/panvk_vX_physical_device.c`. All tessellation limits are
  zero under the comment `Tesselation shaders not supported.`
  `shaderTessellationAndGeometryPointSize=false`.
  `multiviewTessellationShader=false`.
- `panvk_vX_shader.c` compiles only `MESA_SHADER_VERTEX`,
  `MESA_SHADER_FRAGMENT`, and `MESA_SHADER_COMPUTE`. Any other stage hits
  `UNREACHABLE("Unknown shader stage")`. Bind rejects non-VS/FS/CS with
  `assert(!"Unsupported stage")`.
- The compiler never implements `MESA_SHADER_TESS_CTRL` or
  `MESA_SHADER_TESS_EVAL`. `pan_shader_stage()` maps unknown stages to
  `MALI_SHADER_STAGE_COMPUTE`. Kraid's NIR options set
  `support_indirect_inputs` bits for TESS_CTRL/TESS_EVAL/FRAGMENT. That is a
  generic NIR unrolling flag, not a tessellation compiler path. Bifrost still
  documents `support_indirect_inputs = 0 /* TODO support indirect varyings */`.
- Valhall v11 XML `Shader stage` enumerates only Compute=1, Fragment=2,
  Vertex=3. There is no Tess Control / Tess Eval value. v11 has no
  `IDVS Pipeline Stage` enum.
- v12+ XML adds `IDVS Pipeline Stage` Tess Control=3, Tess Eval=4, Geometry=5.
  The `Shader stage` enum is still Compute/Fragment/Vertex. That later
  architecture is unused by G615 / PAN_ARCH=11. Do not treat v12 IDVS stage
  names as G615 hardware.
- Draw Mode on v9–v14 has Points/Lines/Triangles/Quads and adjacency variants.
  There is no Patch value. `VK_PRIMITIVE_TOPOLOGY_PATCH_LIST` is unnamed in
  Panfrost. CSF draw is IDVS-only (`assert(idvs)` / `FIXME: support non-IDVS`).
- VS variants are only `PANVK_VS_VARIANT_HW` (VS followed by FS). There is no
  VS-before-TCS variant.
- Gallium Panfrost fills shader caps only for VS/FS/CS
  (`for (i = 0; i <= MESA_SHADER_COMPUTE; i++)` then `default: continue`).
  OpenGL tessellation support cannot be inferred.
- Mesa NIR has `nir_passthrough_tcs`, `nir_lower_tess_coord_z`, and
  `nir_gather_tcs_info`. Those are shared helpers, not a PanVK tessellation
  pipeline.
- Mesa `src/poly` (`poly_nir_lower_tcs`, `poly_nir_lower_tes`,
  `tessellation.cl`, `poly_tess_params`) is HoneyKrisp/Asahi compute
  emulation: TCS as compute writing a patch buffer, a software tessellator
  emitting coords/indices, TES as VS or compute. Consumers are
  `src/asahi/vulkan/hk_shader.c`, `src/gallium/drivers/asahi/agx_state.c`,
  and `src/kosmickrisp/vulkan/kk_shader.c`. PanVK meson does not depend on
  `libpoly`. PanVK uses only `poly_compact_prim` / `poly/geometry.h` for
  primitives-generated queries.
- No coherent PanVK tessellation series exists to backport. Cargo-culting
  RADV/Turnip/NVK or Asahi poly into PanVK would be a new implementation, not
  an upstream backport.

## Semantic Blocker

Vulkan `tessellationShader=true` requires a tessellation control stage and a
tessellation evaluation stage that can:

1. Consume `VK_PRIMITIVE_TOPOLOGY_PATCH_LIST` with 1..32 control points.
2. Run TCS invocations with `gl_InvocationID`, per-vertex and per-patch
   outputs, and output/control barriers (`OpControlBarrier` /
   `memory_scope` against tessellation control output).
3. Tessellate triangles, quads, and isolines with equal, fractional-even,
   and fractional-odd spacing, CW and CCW domain origin, inner and outer
   tessellation levels, and point mode.
4. Run TES invocations that read tessellation coordinates, patch constants,
   and per-vertex TCS outputs, then feed expanded primitives into the
   existing Valhall IDVS/tiler path.

G615 has none of:

- a hardware TCS or TES stage
- a PanVK TCS/TES compiler path
- a VS-before-TCS IDVS variant
- patch memory / TCS output buffer allocation
- a tessellator (count + emit) wired into command recording
- `PATCH_LIST` input assembly

Flipping `tessellationShader` would advertise invalid pipelines.

## Required Design

A future complete implementation must pick one coherent path and finish it
before exposure.

### Path A — native Valhall tessellation (not available on v11)

Requires a documented v11 TCS/TES shader stage or IDVS pipeline stage,
compiler support for `MESA_SHADER_TESS_CTRL` / `MESA_SHADER_TESS_EVAL`,
patch-list draw emission, and CSF job sequencing. Current v11 XML and
compiler do not provide this. Do not treat v12 `IDVS Pipeline Stage`
Tess Control=3 / Tess Eval=4 as G615 hardware.

### Path B — software tessellation via `src/poly` compute lowering

Reuse Mesa poly as a semantic reference, not an Asahi copy. Required PanVK
work:

1. Compile TCS NIR with `poly_nir_lower_tcs` into a compute shader that
   writes per-vertex and per-patch outputs into a TCS buffer. Preserve
   barriers: poly lowers shader-out barriers to global/subgroup scope
   because a patch fits in a subgroup. Valhall compute must honor that.
2. Add a VS-before-TCS variant that writes per-vertex TCS inputs to a heap,
   not the IDVS position/varying FIFOs used by VS→FS.
3. Run the poly tessellator (`POLY_TESS_MODE_COUNT` then
   `POLY_TESS_MODE_WITH_COUNTS`) to emit tess coords and an index buffer
   for triangles, quads, and isolines, with equal / fractional-even /
   fractional-odd partitioning, CW/CCW (`poly_tess_params.ccw`), inner and
   outer levels, and point mode.
4. Compile TES with `poly_nir_lower_tes(..., to_hw_vs=true)` into a Valhall
   hardware VS that reads the TCS buffer and tess coords, then IDVS/tiler
   draws the expanded topology. `to_hw_vs=false` is only for TES-before-GS
   (P12 is also absent).
5. Allocate `poly_tess_params`: heap, patch_coord_buffer, coord_allocs,
   out_draws, tcs_buffer, counts, index_buffer, statistic. Size TCS stride
   from `poly_tcs_output_stride`.
6. Implement `VK_PRIMITIVE_TOPOLOGY_PATCH_LIST` in input assembly. Advertise
   `maxTessellationPatchSize` only after 1..N control points work.
7. Preserve:
   - per-vertex TCS outputs and per-patch constants
   - TCS barriers
   - TES inputs (`TessCoord`, patch vertices, tess levels)
   - multiple control points
   - domain origin CW/CCW
   - inner/outer levels including zero (cull patch)
8. Handle indexed, instanced, indirect, restart (only if
   `primitiveTopologyPatchListRestart` is implemented), and query
   interaction with the existing CSF IDVS-only draw path.
9. Do not advertise `shaderTessellationAndGeometryPointSize` until TES
   point-size writes are compiled and rasterized.
10. Run the full matrix and CTS before setting `tessellationShader=true`.

Path B is a new PanVK architecture. Poly NIR alone is not a PanVK
tessellation pipeline. PanVK has no libpoly meson dependency and no
tessellator precomp.

Do not expose a subset (triangles-only, equal spacing only, no barriers,
no per-patch outputs).

## Required Test Matrix

Rendering tests are invalid while `tessellationShader` is disabled. These
cases remain blocked:

```text
TCS
TES
triangles
quads
isolines
equal spacing
fractional even
fractional odd
CW
CCW
outer levels
inner levels
per-vertex outputs
per-patch outputs
barriers
multiple control points
```

`tests/dxvk-vkd3d/test_p17_tessellation_shader_safe_false.py` guards against
accidental exposure and detects a future PanVK, compiler, Gallium, Valhall, or
poly-wired implementation that requires this assessment to be repeated.

`tests/dxvk-vkd3d/p17-tessellation-shader.c` is the G615 runtime probe: it
asserts the feature stays false, tessellation limits stay zero, and enabling
`tessellationShader` is rejected. It is not a substitute for rendering or CTS.

The G615 identity check in this session returned `duchamp` / `2311DRK48I`.
The staged Android ICD SHA-256 is
`576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`, matching
the P10–P16 ICD. Serial `Y5WWBMJVOZSK4HU8` stayed connected. The live probe
reported `tessellationShader=NO`, all tessellation limits zero, and
`vkCreateDevice` with `tessellationShader=true` as
`VK_ERROR_FEATURE_NOT_PRESENT`. Result `BLOCKED_SAFE_FALSE`.

Vulkan CTS is `BLOCKED`: no `deqp-vk` binary or matching CTS test list is
installed on the host or G615. CTS version, test-list hash, and
pass/fail/skip/crash/device-lost counts are `NOT_TESTED`.

## Exit

P17 exits `BLOCKED_SAFE_FALSE`. `tessellationShader` stays false. Next
engineering step: Valhall v11 native TCS/TES proof or a complete PanVK
poly/compute lowering, then the full rendering matrix and Vulkan CTS.
P18 was not started.
