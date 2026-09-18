# NOTICE

## This repository

Build/patch/test scaffolding authored for `panvk-kbase-android` is
MIT-licensed unless a file header states otherwise (see `LICENSES/`).

## Upstream Mesa

All Mesa source code fetched by `scripts/fetch-mesa.sh` and any patch derived
from it remain under their upstream licenses (predominantly MIT/X11 with
Khronos SGI/X11 portions — see the Mesa checkout's `LICENSES/` directory after
fetch).

This repository does not vendor Mesa. `sources.lock` pins the exact upstream
commit; release manifests (`SOURCE.json`) record the Mesa SHA, patch series
hash, compiler, NDK, and build host for every binary.

## Reference Kbase work studied (not vendored)

- https://github.com/wonderkast02/panvk-g720-kbase-csf (CSF primary)
- https://github.com/nangitagamer777-art/PanVK-kbase (history)
- https://github.com/nangitagamer777-art/Panvk_Kmod (secondary)
- https://github.com/leegao/mesa-funnymdzz (secondary)
- https://github.com/LukeValen/panvk-mali-g52 (JM/Bifrost primary)
- https://github.com/Noysz/panvk-g99-jm (JM/v9 primary)

Each patch under `patches/` must carry `purpose / source-reference /
tested-GPU / tested-Kbase-UAPI / Mesa-base-range / dependencies /
validation-test` in its header. Never ship a binary without exact source
provenance (`SOURCE.json` + `VALIDATION.json` + `SHA256SUMS.txt`).
