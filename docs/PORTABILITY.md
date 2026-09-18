# Portability

One source commit + patch series produces two ABIs:

- `android-aarch64-bionic`: `libvulkan_panfrost.so`, hot-loadable ICD +
  Android WSI + AHB. Never `libc.so.6`, never X11, never HAL-only symbols.
- `linux-aarch64-glibc`: `libvulkan_panfrost.so` + `panfrost_icd.aarch64.json`.
  No Bionic-only libs unless intentionally bridged.

Never mix ABIs in one loader directory. Freedreno filename
(`libvulkan_freedreno.so`) is never the canonical name; a compat adapter only
after proven load. System-HAL/Magisk experiments live under
`experimental/system-hal/` and are not deliverables; never require vendor
driver replacement, SELinux changes, or `/dev/mali0` chmod as release
requirements.
