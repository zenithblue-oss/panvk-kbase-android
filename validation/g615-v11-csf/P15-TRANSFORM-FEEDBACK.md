# P15 VK_EXT_transform_feedback

## Result

`BLOCKED_SAFE_FALSE`

No complete PanVK implementation of `VK_EXT_transform_feedback` exists on
Valhall v11 / G615. The G615 runtime remains:

```text
VK_EXT_transform_feedback        absent
transformFeedback                false
geometryStreams                  false
transformFeedbackQueries         false
transformFeedbackDraw            false
transformFeedbackRasterizationStreamSelect false
transformFeedbackStreamsLinesTriangles     false
```

`vkCreateDevice` with `VK_EXT_transform_feedback` returns
`VK_ERROR_EXTENSION_NOT_PRESENT`. An XFB feature struct in `pNext` without
the extension is ignored (`VK_SUCCESS` does not enable XFB).

No PanVK patch or feature-reporting change was made. Required commands are
unimplemented:

```text
vkCmdBindTransformFeedbackBuffersEXT
vkCmdBeginTransformFeedbackEXT
vkCmdEndTransformFeedbackEXT
vkCmdDrawIndirectByteCountEXT
indexed VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT
```

P16 was not started.

## Source Audit

- Pinned Mesa: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Audited Mesa `origin/main`: `e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`.
- Post-pin Panfrost commits: `1a63ca3af98`, `a78c40578d6`, `2092558f77d`. None
  implement transform feedback.
- Pinned and current upstream PanVK both omit `EXT_transform_feedback` from
  `get_device_extensions()` in `src/panfrost/vulkan/panvk_vX_physical_device.c`.
  There is no `.transformFeedback =` / `.geometryStreams =` assignment. Core
  `geometryShader` stays false. `vertexPipelineStoresAndAtomics` is v13+ or
  driconf-forced; G615 is PAN_ARCH=11 so the bit is false.
- `00-panvk-defaults.conf` states ANGLE needs the extension or
  `vertexPipelineStoresAndAtomics`, and that PanVK currently does not support
  the extension.
- PanVK command recording has no `CmdBindTransformFeedback*`,
  `CmdBegin/EndTransformFeedback*`, or `CmdDrawIndirectByteCount*` symbols.
  Mesa `vk_common_Cmd*` wrappers in `src/vulkan/runtime/vk_command_buffer.c`
  only forward to driver `*2EXT` entrypoints. PanVK does not provide those.
- `panvk_vX_shader.c` compiles only `MESA_SHADER_VERTEX`,
  `MESA_SHADER_FRAGMENT`, and `MESA_SHADER_COMPUTE`. Other stages hit
  `UNREACHABLE("Unknown shader stage")`. Bind rejects non-VS/FS with
  `assert(!"Unsupported stage")`. There is no XFB compute variant.
- The compiler helper `pan_nir_remove_xfb()` strips XFB info. It zeros
  `io_xfb`, frees `nir->xfb_info`, and clears `has_transform_feedback_varyings`.
  It does not emit `store_global` captures.
- CSF query code comments `TODO: transform feedback` and `assert(index == 0)`.
  Query pools accept occlusion, timestamp, and
  `VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT` only. Indexed
  `VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT` is unreachable.
- No coherent PanVK XFB series exists to backport. Cargo-culting Turnip, RADV,
  or NVK would be a new implementation, not an upstream backport. Those drivers
  were used as API/semantic reference only.

### Gallium stream output is not Vulkan XFB

Gallium Panfrost implements OpenGL stream output by compiling a second
non-IDVS vertex variant:

1. `nir_io_add_intrinsic_xfb_info` + `nir_lower_xfb_to_stores` with
   `nir_address_format_64bit_global`.
2. `inputs.no_idvs = true` because IDVS conflicts with XFB lowering
   (`pan_screen.c`: side effects in the geometry pipeline "cause trouble with
   IDVS and conflict with our transform feedback lowering").
3. Launch as a 1×instance compute job via `csf_launch_xfb` /
   `panfrost_launch_xfb`.
4. `PAN_SYSVAL_XFB` supplies 64-bit buffer addresses. After the job,
   `pan_nir_remove_xfb` strips XFB from the raster VS.

That path is Gallium-only. PanVK does not call it.

It is also incomplete against the Vulkan contract even as a reference:

```text
TODO: XFB with index buffers          pan_cmdstream.c
TODO: Indexing                        csf_launch_xfb
no max_vertex_streams cap             pan_screen.c
geometryShader=false (P12)            no GS streams
vertexPipelineStoresAndAtomics=false  G615 / v11
```

OpenGL stream-output support cannot be inferred into
`VK_EXT_transform_feedback`. `geometryStreams=true` additionally requires a
geometry stage (P12 `BLOCKED_SAFE_FALSE`).

## Semantic Blocker

Vulkan `transformFeedback=true` requires, for every advertised buffer:

1. `vkCmdBindTransformFeedbackBuffersEXT` with offset and size.
2. `vkCmdBeginTransformFeedbackEXT` / `End` with optional counter buffers
   (resume from a previous counter, pause/end writing a new counter).
3. Capture of the last pre-rasterization stage outputs according to SPIR-V
   `XfbBuffer` / `XfbStride` / `Offset` and NIR `nir_xfb_info`.
4. Primitive-complete writes, overflow stop, and
   `VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT` / counter-read/write barriers.
5. Indexed, instanced, and indirect draws capturing the same vertices the
   rasterizer consumes, including primitive restart.
6. `vkCmdDrawIndirectByteCountEXT` if `transformFeedbackDraw` is advertised.
7. Indexed `VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT` if
   `transformFeedbackQueries` is advertised.

`geometryStreams=true` additionally requires:

1. Geometry-shader stream outputs (`EmitStreamVertex`).
2. `maxTransformFeedbackStreams > 1` or at least stream 0 plus GS stream
   selection.
3. Optional rasterization-stream select only if
   `transformFeedbackRasterizationStreamSelect` is true.

G615 currently has none of:

- the extension or feature bits
- PanVK bind/begin/end/draw-byte-count commands
- a PanVK XFB shader variant (`nir_lower_xfb_to_stores` is unused)
- counter-buffer resume/pause
- indexed XFB queries
- GS streams (P12 blocked)
- `vertexPipelineStoresAndAtomics` on v11 (IDVS forbids VS memory writes,
  which is why Gallium uses a separate no-IDVS compute-like job)

Flipping `transformFeedback` or `geometryStreams` would advertise illegal
pipelines and missing entrypoints.

Do not advertise extra properties. `transformFeedbackQueries`,
`transformFeedbackDraw`, `transformFeedbackRasterizationStreamSelect`, and
`transformFeedbackStreamsLinesTriangles` stay false.

## Required Design

A future complete implementation must pick one coherent path and finish it
before exposure. Do not enable the extension on the Gallium lowering alone.

### Path A — native hardware XFB (not available on v11)

Valhall v11 IDVS has no documented transform-feedback unit, stream-output VA,
or per-stream counter. Do not treat later-architecture stream-output registers
as G615 hardware.

### Path B — software XFB via Gallium-style no-IDVS stores

Reuse Gallium as a hardware/semantic reference, not a copy. Required PanVK
work:

1. Compile an XFB variant from the last pre-rasterization NIR with
   `nir_io_add_intrinsic_xfb_info` + `nir_lower_xfb_to_stores`. Keep the
   raster VS as IDVS. The XFB variant must set `no_idvs` and run as compute.
2. Record `vkCmdBindTransformFeedbackBuffersEXT` into command-buffer state
   (buffer, offset, size). Null bindings must sink writes.
3. `Begin` reads optional 32-bit counter buffers and sets the capture offset.
   `End` writes counters. Resume must continue from the previous counter, not
   from zero. Pause/end must be legal across command buffers.
4. For every draw while XFB is active, launch the XFB job with the same
   vertex/instance/index/indirect parameters as the IDVS draw, then rasterize.
   Indexed draws are mandatory; Gallium's `TODO: XFB with index buffers` must
   be solved, not inherited.
5. Honor size: stop writing when `offset + stride` would exceed the bound
   size. Overflow must not wrap.
6. Insert cache flushes so XFB writes are visible to later vertex fetch and
   to `VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT` barriers.
7. Implement `vkCmdDrawIndirectByteCountEXT` before advertising
   `transformFeedbackDraw`. Leave that property false until the command works.
8. Implement indexed `VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT` before
   advertising `transformFeedbackQueries`. Leave that property false until
   the query path works.
9. `geometryStreams` stays false until P12 geometry shaders exist and extra
   streams are captured. Do not report `geometryStreams=true` on VS-only XFB.
10. Run the full matrix and CTS before setting `transformFeedback=true`.

Path B is a new PanVK architecture. Gallium `csf_launch_xfb` plus NIR lowering
is not a PanVK XFB implementation.

Do not expose a subset (VS-only one buffer, no counters, no indexed draws, no
barriers).

## Required Test Matrix

Capture tests are invalid while the extension is disabled. These cases remain
blocked:

```text
single buffer
multiple buffers
offset
size
counter buffer
resume
pause/end
draw
indexed draw
instanced draw
indirect draw
overflow
barriers
queries
multiple command buffers
```

`tests/dxvk-vkd3d/test_p15_transform_feedback_safe_false.py` guards against
accidental exposure and detects a future PanVK, compiler, or Gallium-wired
implementation that requires this assessment to be repeated.

`tests/dxvk-vkd3d/p15-transform-feedback.c` is the G615 runtime probe: it
asserts the extension stays absent, both feature bits stay false, extra
properties stay false, and enabling the extension is rejected. An XFB
feature struct in `pNext` without the extension is ignored and is not
treated as support. It is not a substitute for capture or CTS.

The connected G615 runtime is recorded in `p15-transform-feedback-2026-09-20.txt`.
Serial `Y5WWBMJVOZSK4HU8` (`duchamp` / `2311DRK48I`) ran the probe against ICD
SHA-256 `576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`:

```text
device=Mali-G615 MC6
api=1.4.363
VK_EXT_transform_feedback=NO
transformFeedback=NO
geometryStreams=NO
transformFeedbackQueries=NO
create_VK_EXT_transform_feedback=VK_ERROR_EXTENSION_NOT_PRESENT
result=BLOCKED_SAFE_FALSE
```

Vulkan CTS is `BLOCKED`: no `deqp-vk` binary or matching CTS test list is
installed on the host or G615. CTS version, test-list hash, and
pass/fail/skip/crash/device-lost counts are `NOT_TESTED`.

Host guard: `python3 -m unittest tests.dxvk-vkd3d.test_p15_transform_feedback_safe_false`
`10/10` PASS.

## Exit

P15 exits `BLOCKED_SAFE_FALSE`. `VK_EXT_transform_feedback` stays absent.
`transformFeedback`, `geometryStreams`, and `transformFeedbackQueries` stay
false. Next engineering step: a complete PanVK no-IDVS XFB variant with
bind/begin/end, counters, indexed draws, and barriers, then the full matrix
and Vulkan CTS. `geometryStreams` additionally waits on P12. P16 was not
started.
