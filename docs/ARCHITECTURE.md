# Architecture

```text
Android app (Samba S3 / loader-test APK)
 -> libvulkan_panfrost.so (Bionic, hot-loadable ICD)
    -> generic ICD exports (vk_icdGetInstanceProcAddr, vk_icdNegotiateLoaderICDInterfaceVersion)
    -> Android WSI (VK_KHR_android_surface + swapchain)
    -> AHardwareBuffer / gralloc external-memory interop
    -> PanVK core (Mesa main / 26.3.0-devel + patch series)
    -> pan_kmod Kbase backend (CSF queue/group/register/bind/kick; JM job-chain)
    -> Kbase ioctls -> /dev/mali0 -> Mali
```

glibc path:

```text
glibc Vulkan app (Bachata S4 / NativeCode)
 -> glibc libvulkan loader -> glibc libvulkan_panfrost.so + panfrost_icd.aarch64.json
 -> Kbase ioctls -> /dev/mali0 -> Mali
```

Key lessons carried forward:

- Kbase sync != DRM syncobj. Output semaphores signal through the active
  `vk_sync` type (G52 output-semaphore fix).
- Android WSI must coexist with generic ICD symbols; a HAL-only symbol scheme
  hides `vk_icdGetInstanceProcAddr` (G52 Android-platform issue).
- Never guess `native_handle_t->data[0]` as the dma-buf; use the real
  gralloc/AHB metadata path. Never map a missing DRM modifier to LINEAR.
- Valhall v9 is a backend port, not a build flag.
- Synchronous Kbase submission first; async only after correctness.
