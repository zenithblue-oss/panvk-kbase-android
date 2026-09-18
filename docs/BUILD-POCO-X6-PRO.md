# Build on Poco X6 Pro (duchamp, Dimensity 8300-Ultra, G615)

Preferred: NativeCode AI -> ARM64 Alpaquita/glibc PRoot -> NDK cross compiler
(Bionic) + native compiler (glibc). Keep checkout + ccache on fast internal
storage (e.g. `/opt/panvk-build/` or app-private bind mount), never SAF/FUSE.

Prerequisites (PRoot): `ccache ninja git meson python clang` + Android NDK
(host has NDKs under `/opt/android-sdk/ndk/`; latest 30.0.14904198).

```sh
./scripts/fetch-mesa.sh
./scripts/bootstrap-host-tools.sh     # build machine = ARM64 glibc, target helpers NOT executed in guest
./scripts/apply-patches.sh --profile g615-v11-csf
./scripts/build-android.sh --profile g615-v11-csf   # Bionic, minimal: PanVK + Kbase + Android WSI
./scripts/build-glibc.sh --profile g615-v11-csf     # glibc ICD for Bachata/NativeCode
```

Incremental loop: `ninja -C build/android-bionic` + `ccache`; never full clean
rebuild per patch; never build unrelated Gallium drivers; no Zink in the
Android app driver unless a consumer needs it.

Dev loop on rooted Poco: sync source -> build changed target -> package
locally -> static validate -> loader-test APK -> bounded Vulkan tests ->
save logs -> Samba S3 (Android gates pass) / NativeCode (glibc gates pass) ->
release candidate only after full gate set.
