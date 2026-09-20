# P12 geometryShader

## Result

`BLOCKED_SAFE_FALSE`

No native Valhall v11 geometry stage and no complete PanVK/shared implementation
exist. The G615 runtime remains:

```text
geometryShader=false
shaderTessellationAndGeometryPointSize=false
maxGeometryShaderInvocations=0
maxGeometryInputComponents=0
maxGeometryOutputComponents=0
maxGeometryOutputVertices=0
maxGeometryTotalOutputComponents=0
```

`vkCreateDevice` with `geometryShader=true` returns
`VK_ERROR_FEATURE_NOT_PRESENT`.

No PanVK patch or feature-reporting change was made. P13 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Post-pin Panfrost commits: `1a63ca3af98`, `a78c40578d6`, `2092558f77d`. None
  implement geometry shaders.
- Pinned and current upstream PanVK both report `geometryShader=false` in
  `src/panfrost/vulkan/panvk_vX_physical_device.c`. Geometry limits are all
  zero. `shaderTessellationAndGeometryPointSize=false`.
- `panvk_vX_shader.c` compiles only `MESA_SHADER_VERTEX`,
  `MESA_SHADER_FRAGMENT`, and `MESA_SHADER_COMPUTE`. Any other stage hits
  `UNREACHABLE("Unknown shader stage")`. Bind rejects non-VS/FS/CS with
  `assert(!"Unsupported stage")`.
- The compiler never names `MESA_SHADER_GEOMETRY`. `pan_shader_stage()` maps
  unknown stages to `MALI_SHADER_STAGE_COMPUTE`.
- Valhall v11 XML `Shader stage` enumerates only Compute=1, Fragment=2,
  Vertex=3. There is no Geometry value. CSF resource bits are Compute,
  Fragment, Tiler, IDVS, Ray Tracing. No GS resource bit.
- Midgard/Bifrost JM XML has Job Type Geometry=6, documented in `pan_jc.h` as
  `GEOMETRY: runs a geometry shader (unimplemented)`. G615 is CSF Valhall v11,
  not that JM job.
- v12+ XML adds `IDVS Pipeline Stage` Geometry=5. That is a later architecture
  and is unused by G615 / PAN_ARCH=11.
- CSF draw is IDVS-only (`assert(idvs)` / `FIXME: support non-IDVS`). Input
  topology, including adjacency Draw Mode values, is programmed as the tiler
  input, not as a GS expansion stage.
- VS variants are only `PANVK_VS_VARIANT_HW` (VS followed by FS). There is no
  VS-before-GS variant.
- Gallium Panfrost fills shader caps only for VS/FS/CS and never mentions
  `MESA_SHADER_GEOMETRY`. OpenGL GS support cannot be inferred.
- Mesa NIR has `nir_lower_gs_intrinsics`, `nir_gs_count_vertices_and_primitives`,
  and `nir_passthrough_gs`. Those are shared helpers, not a PanVK GS pipeline.
- Mesa `src/poly` (`poly_nir_lower_gs`) is HoneyKrisp/Asahi compute emulation:
  count shader, copy/rast shader, pre-GS, heap, index buffer, XFB. Consumers
  are `src/asahi/vulkan/hk_shader.c` and `src/gallium/drivers/asahi`. PanVK
  meson does not depend on `libpoly`. PanVK uses only `poly_compact_prim` for
  primitives-generated queries.
- Gallium `draw_gs.c` is a CPU GS. PanVK does not use Gallium draw.
- No coherent PanVK GS series exists to backport. Cargo-culting RADV/Turnip/NVK
  or Asahi poly into PanVK would be a new implementation, not an upstream
  backport.

## Semantic Blocker

Vulkan `geometryShader=true` requires a last pre-rasterization geometry stage
that can:

1. Consume points, lines, triangles, and advertised adjacency topologies.
2. Emit points, line strips, or triangle strips with multiple `EmitVertex`
   calls, `EndPrimitive` / `RestartStrip`, and optional streams.
3. Preserve `PrimitiveID`, `Layer`, `ViewportIndex`, clip/cull distances,
   and XFB stream semantics when those features are advertised.
4. Feed the expanded primitives into the existing Valhall IDVS/tiler path.

G615 has none of:

- a hardware GS stage
- a PanVK GS compiler path
- a VS-before-GS IDVS variant
- a compute GS lowering wired into command recording
- a generated-index / prefix-sum / heap allocator for expanded primitives

Adjacency Draw Mode values and VS `LAYER` / fragment `PrimitiveID` do not
constitute a geometry shader. Layer is a VS output into the tiler FIFO.
`shaderOutputViewportIndex` remains false. `geometryStreams` / transform
feedback remain P15.

Flipping `geometryShader` would advertise invalid pipelines.

## Required Design

A future complete implementation must pick one coherent path and finish it
before exposure.

### Path A — native Valhall GS (not available on v11)

Requires a documented v11 GS/IDVS pipeline stage, compiler support for
`MESA_SHADER_GEOMETRY`, and CSF job emission. Current v11 XML and compiler
do not provide this. Do not treat v12 `IDVS Pipeline Stage Geometry=5` as
G615 hardware.

### Path B — software GS via `src/poly` compute lowering

Reuse Mesa poly as a semantic reference, not an Asahi copy. Required PanVK
work:

1. Compile GS NIR with `poly_nir_lower_gs` into count, main, pre-GS, and
   rast/copy compute-or-VS shaders that the Valhall compiler can accept.
2. Add a VS-before-GS variant that writes per-vertex GS inputs to a heap,
   not the IDVS position/varying FIFOs used by VS→FS.
3. Allocate count, index, and output heaps; run prefix-sum when counts are
   dynamic; emit a follow-up IDVS/tiler draw of the expanded topology.
4. Preserve:
   - primitive IDs (`PrimitiveID` / `PrimitiveIDIn`)
   - `Layer` through `layer_index_enable`
   - `ViewportIndex` only if multiViewport is implemented (P14)
   - adjacency input assembly
   - multiple emits, `EndPrimitive`, strip restart
   - streams / XFB only when P15 exists; otherwise reject extra streams
5. Handle indexed, instanced, indirect, restart, and query interaction with
   the existing CSF IDVS-only draw path.
6. Advertise adjacency input topologies only when the lowering consumes them
   as GS inputs rather than as tiler Draw Mode.
7. Run the full matrix and CTS before setting `geometryShader=true`.

Path B is a new PanVK architecture. Poly NIR alone is not a PanVK GS.

Do not expose a subset (points-only, no adjacency, no `EndPrimitive`).

## Required Test Matrix

Rendering tests are invalid while `geometryShader` is disabled. These cases
remain blocked:

```text
points -> points
points -> lines
points -> triangles
lines
triangles
adjacency if advertised
multiple emits
layer output
viewport output
primitive ID
EndPrimitive
RestartStrip
streams
```

`tests/dxvk-vkd3d/test_p12_geometry_shader_safe_false.py` guards against
accidental exposure and detects a future PanVK, compiler, Gallium, Valhall, or
poly-wired implementation that requires this assessment to be repeated.

`tests/dxvk-vkd3d/p12-geometry-shader.c` is the G615 runtime probe: it asserts
the feature stays false, geometry limits stay zero, and enabling
`geometryShader` is rejected. It is not a substitute for rendering or CTS.

The G615 identity check in this session returned `duchamp` / `2311DRK48I`.
The staged Android ICD SHA-256 is
`576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`, matching
the P10/P11 ICD. Serial `Y5WWBMJVOZSK4HU8` disconnected after that identity
check, so `p12-geometry-shader.c` did not execute on-device. Feature bits and
zero geometry limits are taken from the same-ICD capture in
`consumer-capabilities.json` (`geometryShader=false`, all
`maxGeometry*=0`). The live create-reject path is therefore `NOT_RUN`.

Vulkan CTS is `BLOCKED`: no `deqp-vk` binary or matching CTS test list is
installed on the host or G615. CTS version, test-list hash, and
pass/fail/skip/crash/device-lost counts are `NOT_TESTED`.

## Exit

P12 exits `BLOCKED_SAFE_FALSE`. `geometryShader` stays false. Next engineering
step: Valhall v11 native GS proof or a complete PanVK poly/compute lowering,
then the full rendering matrix and Vulkan CTS. P13 was not started.
