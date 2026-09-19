# Beta.3 Worker Final Report

```text
REPOSITORY:
URL=https://github.com/abhay-byte/panvk-kbase-android.git
BRANCH=feature/g615-beta3-capabilities
COMMIT=d33f9ffd7275704ecc8e74f97f25d9fd3002ed21

BASELINE:
BETA2_TAG=g615-v11-csf-v0.1.0-beta.2
BETA2_COMMIT=aa16a4e83b603afc38b186ed09a62aba61083f4b
BETA2_PATCH_SERIES_ID=sha256:b0573b926c81fb19e2ea67a9ab2465e3dcea4e8f0660db0012f8c98cce31bd4f

MESA:
OLD_COMMIT=5a07217f034b3e50d8c7c7794f97a2df1742613b
NEW_COMMIT=5a07217f034b3e50d8c7c7794f97a2df1742613b
REBASED=NO
NEW_PATCH_SERIES_ID=sha256:c0bbdeef591b206a2f3ae36dc6191c08854399039075f8d69f103e33c0eca2f8

EXTENSIONS:
BETA2_INSTANCE=12
BETA2_DEVICE=166
BETA2_TOTAL=178
NEW_INSTANCE=13
NEW_DEVICE=181
NEW_TOTAL=194
ADDED=16
REMOVED=0
PLATFORM_INAPPLICABLE=10
SOURCE_SUPPORTED_NOT_EXPOSED_REMAINING=3

NEW_EXTENSION_TESTS:
VK_KHR_compute_shader_derivatives=PASS
VK_KHR_depth_clamp_zero_one=NOT_RUN; DISABLED/NOT_EXPOSED
VK_KHR_internally_synchronized_queues=PASS
VK_KHR_maintenance7=PASS
VK_KHR_maintenance8=PASS
VK_KHR_maintenance9=PASS
VK_KHR_pipeline_binary=NOT_RUN; DISABLED/NOT_EXPOSED
VK_KHR_present_id2=PASS
VK_KHR_present_wait2=PASS
VK_KHR_robustness2=NOT_RUN; DISABLED/NOT_EXPOSED
VK_KHR_shader_fma=PASS
VK_KHR_shader_relaxed_extended_instruction=PASS
VK_KHR_shader_untyped_pointers=PASS
VK_KHR_surface_maintenance1=PASS
VK_KHR_swapchain_maintenance1=PASS
VK_KHR_unified_image_layouts=PASS
VK_GOOGLE_display_timing=BLOCKED; NOT_EXPOSED; no proven Android timing implementation
VK_GOOGLE_user_type=PASS

EXPOSED_FEATURE_TESTS:
descriptorIndexing=PASS
timelineSemaphore=PASS
dynamicRendering=PASS
synchronization2=PASS
bufferDeviceAddress=PASS
pushDescriptor=PASS
robustBufferAccess=PASS
samplerAnisotropy=PASS
wideLines=PASS
largePoints=PASS
ETC2=PASS
ASTC_LDR=PASS
ASTC_HDR=PASS

KBASE_TEXTURE_FEATURES:
TF0=0xc7fe001e
TF1=0x00000000
TF2=0x00000000
TF3=0x00000000
COMPRESSED_MASK=0xc7fe001e
PANFROST_TEXFEATURES_RESULT=PASS; BC mask=0x00000000; ETC2/ASTC decoded supported

BC_NATIVE:
BC1=UNSUPPORTED; 4 Vulkan variants queried
BC2=UNSUPPORTED; 2 Vulkan variants queried
BC3=UNSUPPORTED; 2 Vulkan variants queried
BC4_UNORM=UNSUPPORTED
BC4_SNORM=UNSUPPORTED
BC5_UNORM=UNSUPPORTED
BC5_SNORM=UNSUPPORTED
BC6H_UFLOAT=UNSUPPORTED
BC6H_SFLOAT=UNSUPPORTED
BC7=UNSUPPORTED; 2 Vulkan variants queried
ALL_16_BC_FORMATS=legacy flags zero; Properties2 flags zero; Properties3 flags zero; ImageFormatProperties2=VK_ERROR_FORMAT_NOT_SUPPORTED
TEXTURE_COMPRESSION_BC_NATIVE=FALSE; NOT_EXPOSED; DO NOT ALTER

BC_COMPAT:
REFERENCE_REPO=https://github.com/leegao/bcn_layer
REFERENCE_COMMIT=50993a2d51772567de9c36de4d523652773f0899
BUILT=PASS
ANDROID_LOAD=BLOCKED; system Mali ICD load passed, staged PanVK composition blocked
BC1_TEST=NOT_RUN
BC2_TEST=NOT_RUN
BC3_TEST=NOT_RUN
BC4_TEST=BLOCKED
BC5_TEST=BLOCKED
BC6_TEST=BLOCKED
BC7_TEST=NOT_RUN
SRGB=NOT_RUN
SNORM=BLOCKED
HDR=BLOCKED
MIPS=NOT_RUN
ARRAYS=NOT_RUN
CUBE=NOT_RUN
PARTIAL_COPY=NOT_RUN
LINEAR_FILTER=NOT_RUN
BLIT=BLOCKED
STRESS=NOT_RUN
DEFAULT_MODE=NOT_RUN
ASTC_MODE=NOT_RUN
DECODE_MBPS=NOT_RUN
ASTC_MBPS=NOT_RUN
END_TO_END_MBPS=NOT_RUN

REGRESSION:
COMPUTE_10_10=PASS
OFFSCREEN=PASS
AHB=PASS; dmabuf=16384 Vulkan=16384
SYNC=PASS
WSI_300=PASS; acquired=300 rendered=300 presented=300 deviceLost=0 hang=0
COLD_SECOND_LAUNCH=PASS; pidFirst=22074 pidSecond=22166
GLIBC=PASS

BETA3:
PACKAGE=PASS validation; Android SHA256=e94945ad928881919bdd8ce2273de097f0d385c3ba57e87ef959e4cf40442561; glibc SHA256=f7f13c5dc7a8c07169151710d18a9b134f6cb8bafb83134fd9f07ef4c2ad36c4; BC component excluded
TAG=NOT_PUBLISHED
PRERELEASE=NOT_PUBLISHED
RELEASE_URL=NOT_PUBLISHED

BLOCKERS_REMAINING:
BLOCKED: BC compatibility does not compose with staged PanVK; correctness, stress, and performance suites are NOT_RUN/BLOCKED. It is excluded from both packages, so the conditional shipped-BC suite gate is NOT_APPLICABLE.
MANDATORY_BLOCKERS=NONE
RELEASE ELIGIBILITY: CANDIDATE_READY; NOT_PUBLISHED. No tag, push, or release performed.
DEFINITION OF DONE: PASS. Native capability proof confirms BC is unavailable and textureCompressionBC remains false. Section 38 item 6 permits the documented optional compatibility blocker because BC compatibility is excluded.

FINAL_EVIDENCE:
validation/g615-v11-csf/beta3-final-kbase-texture-features-2026-09-19.txt
validation/g615-v11-csf/beta3-final-bc-format-properties-2026-09-19.txt
validation/g615-v11-csf/beta3-final-native-bc-evidence.json
validation/g615-v11-csf/beta3-final-package-android-2026-09-19.txt
validation/g615-v11-csf/beta3-final-package-glibc-2026-09-19.txt

NEXT_STEP:
Keep publication NOT_PUBLISHED. Keep BC compatibility excluded unless its full mandatory suite passes with staged PanVK.
```

Gate details and evidence paths: `beta3-final-gates.json`.
