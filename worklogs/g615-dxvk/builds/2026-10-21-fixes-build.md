# 2026-10-21 fixes build, RC5-dev-fixes

- Mesa: 5a07217f034b3e50d8c7c7794f97a2df1742613b, tag upstream-5a07217, own worktree /home/flux/src/mesa-fixes (branch fixes-work: baseline commit of the 192-patch series, then one commit per new patch)
- Profile: g615-v11-csf (series: common android kbase-common app-loader wsi csf csf-v11 jm-v9 jm-v7), 194 patches, strict apply checked on a fresh worktree (/home/flux/src/mesa-fixcheck), no drift. patches/jm-v7-wip NOT applied.
- New patches in this build: csf-v11/198-x11-create-the-surface-when-the-window-is-gone.patch, csf-v11/199-x11-clip-damage-rects-to-the-image.patch
- Repo commit at build time: 992041a1bdc485353a80a00517995fbab7ee22c2
- Host: aarch64 PRoot, host tools build/host-tools/bin, NDK r29 (29.0.14206865) bin on PATH, API 29
- Command: MESA=/home/flux/src/mesa-fixes BDIR=/home/flux/build-fixes/bionic DDIR=/home/flux/build-fixes/dist HOST_TOOLS=build/host-tools/bin PANVK_JOBS=2 PANVK_KBASE_RELEASE=RC5-dev-fixes flock /home/flux/build-cache/build.lock ./scripts/build-android.sh --profile g615-v11-csf --api 29 --ndk /opt/android-sdk/ndk/29.0.14206865 (PANVK_JOBS=2: phone thermal status SEVERE; the baseline build used 4)
- Artifact: libvulkan_panfrost.so (llvm-strip), 23998592 bytes, sha256 28eacae56b24ba785868bc91f1a32b3407daebb6694aaae5048623b930324a81
- Unstripped sha256: 059ace2fa9411abfd3eb8c23168fc13fab585f62861bdb21264a0690275e27ce
- BuildID: 00ce51e33f9c62b1cb2928d13a4bf48d8295118b
- driverInfo: `PanVK-kbase RC5-dev-fixes (Mesa 26.3.0-devel (git-8dd4173eab))`. The git hash is the fixes-work worktree HEAD (series committed on top of 5a07217f), not upstream.
- Local copy: /home/flux/build-out/2026-10-21-fixes/ (libvulkan_panfrost.so, libvulkan_panfrost.unstripped.so, sha256, series.sha256 with 194 entries)
- Device test: none yet. Item notes: worklogs/driver-remaining/fixes-2026-10-21.md

## Device checks for this build

- X11 (198): PanPlay DXVK sessions that exit or go to the background (Just Cause 2 on G68, D3D9 cube). The DXVK log must no longer show `Failed to create Vulkan surface: VK_ERROR_OUT_OF_HOST_MEMORY`; SURFACE_LOST on the capability query is still expected while the window is gone.
- X11 (199): an app with VK_KHR_incremental_present and damage rects starting at negative x/y on the MIT-SHM path. The visible part must update with no stale strip. Also a plain DXVK run (no damage) as a regression check.
- G615 regression: full PanProbe (38/38 expected; both patches touch only the X11 WSI).
