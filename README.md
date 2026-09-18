# PanVK Kbase Android Driver

Standalone patch/build layer around pinned upstream Mesa that produces an
open Mesa PanVK driver talking directly to Android's proprietary
`mali_kbase` kernel interface (`/dev/mali0`).

Target repository name: `JICA98/panvk-kbase-android`

Primary consumers:

1. **Samba S3** — Android/Bionic, in-process custom Vulkan driver
2. **Bachata S4** — ARM64 glibc Vulkan ICD inside the managed runtime
3. **NativeCode AI** — ARM64 glibc PRoot validation/development

Secondary (only after primary driver is correct): Winlator /
AdrenoTools-style loaders, GameHub / component injectors, other Android apps
capable of loading an alternate Vulkan ICD.

This is NOT a Samba-specific or Winlator-specific fork. One driver
source/patch stack, adapter packages around it.

## Reference device

- Phone: Poco X6 Pro (2311DRK48I, `duchamp`)
- SoC: MediaTek Dimensity 8300-Ultra (MT6897)
- GPU: Mali-G615 MC6, Pan arch v11, CSF frontend
- Kernel interface: `/dev/mali0` (`mali_kbase`)
- Observed GPU ID string: `Mali-G615 6 cores r1p3 0xB8A3`

## Layout

```text
sources.lock          exact Mesa commit + reference repos (no moving branches)
profiles/             per-GPU Kbase profiles (arch/gpu-id/frontend/uapi)
patches/              qualified patch families (never one unqualified blob)
meson/                cross files (android-aarch64, linux-aarch64-native)
scripts/              fetch / patch / build / package / validate / release
tests/                kbase-probe, vulkan-smoke, compute, offscreen, ahb,
                      android-surface, sync, android-loader-app
docs/                 architecture, build, portability, matrix, profiles,
                      app-compat, release
.github/workflows/   build / release / source-drift
```

## Quick start (Poco X6 Pro)

See `docs/BUILD-POCO-X6-PRO.md`.

```sh
./scripts/fetch-mesa.sh
./scripts/apply-patches.sh --profile g615-v11-csf
./scripts/build-android.sh --profile g615-v11-csf
./scripts/validate-binary.sh --abi android dist/android-g615-v11-csf/libvulkan_panfrost.so
./scripts/package-android-adpkg.sh --profile g615-v11-csf
```

## Release policy

Tiers: `dev` (build only) < `alpha` (compute/offscreen) < `beta` (WSI +
app-loader on one profile) < `rc` (primary consumer) < `stable` (multi-device
+ soak). First Poco build is never `stable`. Releases are immutable; a patch
change creates a new release even if the Mesa SHA is unchanged.

## Capability truth

Two separate sets are stored per release: `UPSTREAM_MATRIX_CAPABILITIES`
(from `docs/features.txt` of the exact Mesa checkout) and
`RUNTIME_DEVICE_CAPABILITIES` (from on-device `vulkaninfo`/probe). Only
runtime-tested capabilities may be used for compatibility claims. No fake
feature bits.

## License

Mesa code remains under its upstream licenses (see `LICENSES/` and
`NOTICE.md`). Build/patch/test scaffolding in this repository is MIT unless
otherwise noted.
