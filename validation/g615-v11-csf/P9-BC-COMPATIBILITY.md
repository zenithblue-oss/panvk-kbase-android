# P9 BC1-BC7 Compatibility

## Result

`BLOCKED_SAFE_FALSE`

G615 native BC remains unavailable. Transparent emulation is not complete, not
packaged, and not advertised. Runtime state remains:

```text
native=false
emulated=false
textureCompressionBC=false
```

## Upstream Audit

The pinned `bcn_layer` commit
`50993a2d51772567de9c36de4d523652773f0899` equals its current upstream `main`
HEAD on 2026-09-20. Current Mesa HEAD was
`e1f3f372c4a661cd0e71a0cdafc4d469d56ecf35`; no in-tree PanVK BC emulation was
found. There is no newer complete implementation to reuse or backport.

The layer has useful decode machinery and its uncompressed path maps BC1/2/3/7
SRGB to `R8G8B8A8_SRGB`, other BC1/2/3/7 to `R8G8B8A8_UNORM`, BC4/5 SNORM to
`R8G8B8A8_SNORM`, and BC6H to `R16G16B16A16_SFLOAT`. Those source mappings are
architecturally suitable, including HDR-safe BC6H storage. They are not a
runtime proof. BC4/5 use four-channel storage rather than the preferred compact
R8/RG backing, preserving numeric channels at extra memory cost.

The optional ASTC/ETC2 recompression path remains semantically invalid for this
workstream: it explicitly converts BC4/5 SNORM and BC6H float data to UNORM8.
It must remain disabled for G615 compatibility.

## Blocking Defects

1. PanVK tests and target consumers directly load `libvulkan_panfrost.so`
   through `vk_icdGetInstanceProcAddr`. This bypasses loader layers. The only
   tested layer path selected the system Mali ICD, not staged PanVK.
2. `bcn_layer` sets `textureCompressionBC=true` unconditionally in feature
   queries although required semantics remain absent.
3. `vkCmdCopyImage` and `vkCmdCopyImage2` explicitly do not support copying from
   decoded BC images. They forward incompatible backing images to the driver.
4. Decoded `vkCmdCopyImageToBuffer` and `vkCmdCopyImageToBuffer2` are not
   intercepted. Correct BC block reconstruction/readback is absent.
5. Source sizing computes padded `bufferRowLength` and `bufferImageHeight`
   strides but returns only tightly packed block bytes. Padded regions can be
   truncated during diagnostics and lack proof in the decode path.
6. No component correctness tests exist upstream. No G615 run proves all 16
   formats, small/edge extents, mips, arrays, cube maps, offsets, padded strides,
   multiple regions, filtering, SRGB, SNORM, HDR, lifecycle, or memory pressure.
7. Vulkan CTS is unavailable locally and on the connected target. No BC CTS
   counts or conformance claim can be produced.

## Safe Completion Design

Port the decode path into PanVK image handling, not a wrapper ICD and not an
implicitly enabled layer. Preserve the application's BC format metadata while
allocating an internal expanded image:

| Vulkan format | Internal backing |
|---|---|
| BC1/2/3/7 UNORM | `R8G8B8A8_UNORM` |
| BC1/2/3/7 SRGB | `R8G8B8A8_SRGB` |
| BC4 UNORM/SNORM | `R8_UNORM` / `R8_SNORM` |
| BC5 UNORM/SNORM | `R8G8_UNORM` / `R8G8_SNORM` |
| BC6H UFLOAT/SFLOAT | `R16G16B16A16_SFLOAT` |

The implementation must own BC-aware image metadata, memory requirements,
views, barriers, buffer-to-image decode, image copies, and image-to-buffer
behavior. Keep native and emulated capability fields separate. Promote
`emulated=true` and `textureCompressionBC=true` only when every required entry
in `tests/bcn/cases.json` has target-device `PASS` evidence.

## Verification

`validation/g615-v11-csf/p9-bcn.json` is the machine-readable P9 ledger. Its
validator rejects any advertised BC state while one required format, operation,
case, or stress entry is not `PASS`.

No BC format or operation is proven through transparent PanVK emulation in P9.
Earlier system-ICD layer interception is not counted as PanVK proof.

## Exit

P9 exits `BLOCKED_SAFE_FALSE`. The architecture cannot be completed safely in
this phase without an in-ICD image virtualization implementation and full
target-device/CTS validation. No P10 work was started.
