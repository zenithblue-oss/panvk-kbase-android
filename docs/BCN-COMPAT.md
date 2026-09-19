# BC1-BC7 compatibility

## Status

Experimental layer build/load only. Nothing is packaged or enabled by default.
Native PanVK remains truthful with `textureCompressionBC=false`.

Reference: `https://github.com/leegao/bcn_layer` pinned at
`50993a2d51772567de9c36de4d523652773f0899`, MIT. The source checkout remains
untracked under `work/bcn_layer`; provenance is in `sources.lock`.

## Intended mode

Correct uncompressed backing is the only acceptable default:

- BC1/2/3/7 UNORM and SRGB: RGBA8 with matching color-space semantics.
- BC4/5 UNORM and SNORM: channel-preserving normalized backing.
- BC6H UFLOAT and SFLOAT: HDR-safe float backing.

ASTC or ETC2 recompression is optional and lossy. It is forbidden for BC4/5
SNORM and BC6H until numeric semantics are proven. It is not enabled here.

## Build result

The pin tracks `src/analyze_astc_parameters.slang`, not the generated `.comp`.
It has no LFS objects, uses the pinned `libGPUCounters` submodule at
`9fe9f4b3e9ee7d9ddb0274991ecf7ae9679be542`, and has no option to omit the ASTC
sources. `FORCE_GLSL=ON` is not usable because that mode expects the absent
generated fallback. The pinned Android CI installs Slang `2026.10.2`; its run
for this exact commit succeeded. Upstream releases are older experimental
snapshots, not artifacts with commit-level provenance, so none were imported.

`scripts/build-bcn-layer-android.sh` downloads that exact Slang release with a
pinned SHA-256, generates shaders from the pinned `.slang` files, applies only
the documented Android enumeration patch, and builds with NDK r30. Result:
Android API 26 AArch64 ELF `libbcn_layer.so`, SHA-256
`08c49c398cf1fcb746593dbd07565fe51a5bb79a20af500656dd3285774c44a5`.

On the Poco X6 Pro, Android 16 discovered and loaded the APK layer. All 16 BC
format queries returned optimal-tiling features `0x9401`. This proves Android
layer discovery/interception against the system Mali ICD only. Android's system
loader did not select the staged PanVK ICD; direct PanVK loading bypasses the
loader and layer. Device creation then failed, so no known-block correctness
workload ran. See `validation/g615-v11-csf/beta3-phase6-bcn-loader.txt`.

## Correctness blockers

- Copying from a decoded BC image is explicitly unimplemented for both
  `vkCmdCopyImage` and `vkCmdCopyImage2`.
- Buffer-size calculation ignores non-default row and image-height strides.
- ASTC/ETC2 paths map BC4/5 SNORM and BC6H to UNORM, losing required semantics.
- No device evidence exists for known blocks, SRGB, SNORM, HDR, mip, array,
  cube, partial copy, filtering, synchronization, or resource stress.
- No tested Android path currently composes the loader, this layer, and the
  staged PanVK ICD in one process.

Consequently the layer cannot truthfully expose the complete Vulkan
`textureCompressionBC` contract in this release.

Phase 9 decision: exclude `libbcn_layer.so` from Android and glibc beta.3
candidate packages. No shipping or default-enablement claim is made.
