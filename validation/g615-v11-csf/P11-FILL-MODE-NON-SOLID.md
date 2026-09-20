# P11 fillModeNonSolid

## Result

`BLOCKED_SAFE_FALSE`

No direct Valhall raster mode or coherent PanVK/shared implementation exists
for converting triangle polygons to lines or points. The G615 runtime remains:

```text
fillModeNonSolid=false
extendedDynamicState3PolygonMode=false
polygonModePointSize=false
```

`vkCreateDevice` with `fillModeNonSolid=true` returns
`VK_ERROR_FEATURE_NOT_PRESENT`. The same reject applies to
`extendedDynamicState3PolygonMode=true` even though
`VK_EXT_extended_dynamic_state3` is present.

No PanVK patch or feature-reporting change was made. P12 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Pinned and current upstream PanVK explicitly report
  `fillModeNonSolid=false` and
  `extendedDynamicState3PolygonMode=false` in
  `src/panfrost/vulkan/panvk_vX_physical_device.c`.
- Neither tree consumes `VkPolygonMode` / `rs.polygon_mode` in PanVK draw
  emission. `translate_prim()` maps input topology only
  (`MESA_PRIM_*` → `MALI_DRAW_MODE_*`).
- Valhall v11 XML `Draw Mode` enumerates input topologies, including
  `Polygon=13` (n-gon topology, not fill mode). Primitive Size stores a
  point/line size. There is no polygon-mode or fill-mode rasterizer field
  that converts a triangle into its edges or vertices.
- Panfrost Gallium never reads `fill_front` / `fill_back` /
  `PIPE_POLYGON_MODE`. Point size is programmed only when
  `active_prim == MESA_PRIM_POINTS`. G610 and G57 CI record OpenGL
  `polygon-mode*` tests as Fail.
- Mesa's shared Gallium draw module (`draw_pipe_unfilled.c`) has CPU-side
  unfilled-triangle conversion, but PanVK does not use Gallium draw. It
  cannot be wired into Vulkan command recording without a new
  vertex/primitive replay implementation.
- No NIR pass converts arbitrary post-vertex triangles into line or point
  primitives. Existing `nir_lower_point_size` and
  `nir_lower_poly_line_smooth` do not perform topology expansion.

## Complete Emulation Assessment

A correct software-assisted implementation must run after the last
pre-rasterization shader and before tiling. For every assembled triangle it
must:

1. Preserve triangle assembly, strip/fan parity, primitive restart, indexed
   draws, base vertex, instances, and direct/indirect draw parameters.
2. Determine front/back facing and apply Vulkan culling before emitting the
   triangle's three boundary edges or vertices.
3. Preserve provoking-vertex and flat interpolation semantics, `FrontFacing`,
   primitive IDs, clipping, viewport transform, depth bias, and adjacency edge
   ownership.
4. Emit real line primitives for `VK_POLYGON_MODE_LINE`, honoring dynamic line
   width and Valhall line rasterization/sample coverage.
5. Emit real point primitives for `VK_POLYGON_MODE_POINT`. Because
   `polygonModePointSize=false`, each point must use size 1.0 rather than a
   vertex shader `PointSize` output.
6. Retain normal depth, stencil, MSAA, render-pass/dynamic-rendering, query,
   conditional-rendering, and synchronization behavior.
7. Support static polygon mode and `VK_DYNAMIC_STATE_POLYGON_MODE_EXT` before
   advertising the corresponding dynamic-state feature.

PanVK has no geometry stage, primitive-expansion stage, transformed-vertex
capture/replay path, or generated-index/indirect-count path on Valhall. A
fragment-shader edge test is not equivalent: it cannot produce Vulkan line
rasterization/coverage, point mode, line-width behavior, correct clipping, or
per-primitive culling. Reissuing each triangle as three lines/points at command
record time is also invalid because indexed and indirect data and transformed
positions are GPU-resident and may change before execution.

The blocker is therefore a missing post-vertex primitive expansion facility,
not a missing feature bit or one rasterizer register.

## Required Test Matrix

Rendering tests are invalid while `fillModeNonSolid` is disabled. These cases
remain blocked pending the complete implementation:

```text
wireframe triangle
point polygon mode
front-face culling
back-face culling
depth test/write
front/back stencil operations
1x and multisample rasterization
line width 1.0 and wide dynamic widths
static and dynamic polygon mode
indexed draw with primitive restart/base vertex
indirect and indexed-indirect draw
```

`tests/dxvk-vkd3d/test_p11_fill_mode_safe_false.py` guards against accidental
exposure and detects a future PanVK, Gallium, or Valhall implementation that
requires this assessment to be repeated.

`tests/dxvk-vkd3d/p11-fill-mode.c` is the G615 runtime probe: it asserts the
three feature/property bits stay false and that enabling either
`fillModeNonSolid` or `extendedDynamicState3PolygonMode` is rejected. It is
not a substitute for rendering or CTS.

The connected G615 runtime and regression results are recorded in
`p11-fill-mode-2026-09-20.txt`. Vulkan CTS is `BLOCKED`: no `deqp-vk` binary or
matching CTS test list is installed on the host or connected G615. CTS version,
test-list hash, and pass/fail/skip/crash/device-lost counts are `NOT_TESTED`.

## Exit

P11 exits `BLOCKED_SAFE_FALSE`. `VK_POLYGON_MODE_LINE` and
`VK_POLYGON_MODE_POINT` remain unsupported, so `fillModeNonSolid` stays false.
The required next engineering step is a Valhall post-vertex primitive expansion
design, followed by the full rendering matrix and Vulkan CTS. P12 was not
started.
