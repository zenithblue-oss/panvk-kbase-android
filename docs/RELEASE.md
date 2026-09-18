# Release

Tiers: `dev` < `alpha` < `beta` < `rc` < `stable`. First Poco build is never
stable. Tags like `g615-v11-csf-v0.1.0-beta.1`. Immutable: patch change = new
release even if Mesa SHA unchanged.

Android asset: `PanVK-Kbase-Android-<profile>-v<N>-<shortsha>.adpkg.zip`
(flat: `libvulkan_panfrost.so + meta.json + MANIFEST.json`).
glibc asset: `PanVK-Kbase-<profile>-v<N>-<shortsha>-EMULATOR.zip`
(+ `panfrost_icd.aarch64.json`).

Every release: `SHA256SUMS.txt + SOURCE.json + VALIDATION.json` with Mesa
commit, patch hash, compiler/NDK/Meson, host, API, Build ID, SHAs, device,
GPU ID, UAPI, all gate results. CI builds `dev` only; beta/rc/stable require
Poco (or declared device) hardware validation evidence. CI success is never a
substitute for `/dev/mali0` runtime testing.

Fault safety: hard timeouts, small allocations, tiny images, single queue,
bounded frames, one change per run. Never spam CSF kicks after fault, fake
DRM nodes/features, guess Kbase structs across DDKs, or mix JM/CSF ioctls.
After severe fault: save logs + kernel evidence, stop, reboot if needed.
