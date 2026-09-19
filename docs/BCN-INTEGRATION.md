# BCn integration

## Conclusion: layer load proven; direct-PanVK composition blocked

The pinned component is a conventional Vulkan layer. A Vulkan loader can
theoretically compose `VK_LAYER_BCN_BCnLayer` above an ICD using its layer
manifest. Phase 6 proved discovery/interception only with the system Mali ICD,
not with PanVK. This remains experimental.

This repository's primary tests and custom-driver consumers directly
`dlopen(libvulkan_panfrost.so)` and call `vk_icdGetInstanceProcAddr`. That path
bypasses loader layer discovery, so the BCn layer cannot intercept it. A
wrapper ICD was rejected as fragile. No code was ported into PanVK.

Current decision:

- Loader path: Android enumerated and loaded `VK_LAYER_BCN_BCnLayer`; all BC
  format queries were intercepted. The loader selected the system Mali ICD,
  not the staged PanVK ICD, and device creation failed.
- Direct ICD path: unsupported by this layer; consumers must adopt a Vulkan
  loader before compatibility can be available.
- Base `libvulkan_panfrost.so`: unchanged; native `textureCompressionBC=false`.
- Packaging: rejected. The artifact remains experimental and is only embedded
  in the debug test APK when present.
- Shipping/default enablement: none. Both beta.3 candidate packages exclude it.
