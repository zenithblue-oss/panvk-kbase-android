# PanVK Kbase G615 Beta.1 Baseline Review & Evidence Freeze

## Provenance
- Repository Commit: `26e1396f44bb3d09576a6e511a8908c59884c75e`
- Release Tag: `g615-v11-csf-v0.1.0-beta.1`
- Mesa Pinned Commit: `5a07217f034b3e50d8c7c7794f97a2df1742613b` (Mesa 26.3.0-devel)
- Target Device: Poco X6 Pro (`2311DRK48I`, `duchamp`, MT6897 Dimensity 8300-Ultra, Mali-G615 MC6, Pan arch v11 CSF, Linux 6.1.170-GKID, Android 16 / HyperOS OS3.0.6.0.WNLMIXM)

## Binary Hashes (SHA-256)
- Android Bionic (`libvulkan_panfrost.so`):
  `caf37c7aa125d3d87de1e1e9638f8221b12afe04f04559709ae37185259cd24e`
- Linux glibc (`libvulkan_panfrost_glibc.so`):
  `c5601cbca591a4f6e6c3841fa624e12972705697195eb631e036cbe48aa2fac8`

## Baseline Validated Capabilities
1. `/dev/mali0` non-root O_RDWR accessible (UID 10266 in standalone APK).
2. `libvulkan_panfrost.so` dlopened successfully by unprivileged Android application; exports `vk_icdGetInstanceProcAddr` and `vk_icdNegotiateLoaderICDInterfaceVersion`.
3. Device enumeration: Mali-G615 MC6 (`0xb8a31030`), Pan arch v11 CSF, Kbase uAPI 1.21.
4. `vkCreateDevice` succeeds.
5. Compute: 10/10 iterations pass with bit-exact `0x12345678` output.
6. Offscreen rendering: Triangle rendered and read back bit-exact.
7. Synchronization: Fence signal/wait/reset, binary semaphore chaining, 20-run submit loop, device/queue wait idle.
8. AHardwareBuffer: Allocation, DMA-buf import, GPU render pass, CPU lock readback bit-exact (`255, 0, 0, 255`).
9. glibc ICD: Enumeration, compute 10/10, offscreen triangle pass on device.

## Known Defects / Blockers to Resolve in Beta.2
1. **Gate H Vulkan WSI Missing**: Presentation used `ANativeWindow_lock` / CPU fill / `unlockAndPost`, not `vkCreateAndroidSurfaceKHR`, `vkCreateSwapchainKHR`, `vkAcquireNextImageKHR`, `vkQueuePresentKHR`.
2. **AHB allocationSize Query Returns `UINT64_MAX`**: `vkGetAndroidHardwareBufferPropertiesANDROID` reports `allocationSize=18446744073709551615` instead of real buffer byte size.
3. **`patchSeriesId` Stale/Placeholder**: Hardcoded `PENDING-P3-REBASE` across manifests instead of deterministic SHA-256 hash.
4. **`VALIDATION.json` Not Generated from Real Logs**: Released manifest contained skeleton with `untested` flags.
5. **glibc Build Missing Explicit Kbase Selection**: `scripts/build-glibc.sh` did not explicitly pass `-Dpanfrost-kmds=kbase`.
6. **Device Profile Metadata Stale**: `profiles/g615-v11-csf.json` contained placeholder strings (`pending-ioctl-discovery`, `pending-P13-loader-APK`, short GPU ID `0xB8A3`).
