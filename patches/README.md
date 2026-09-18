# Patch families

Every patch file must carry a header:

```text
Purpose: ...
Source/Reference: <url + commit>
Tested GPU: ...
Tested Kbase UAPI: ...
Mesa base range: ...
Dependencies: ...
Validation test: ...
```

Directories:

- `common/` — upstream-safe fixes, build fixes, logging.
- `android/` — Bionic/NDK build, Android WSI, gralloc/AHB interop.
- `kbase-common/` — pan_kmod/Kbase abstraction shared by JM + CSF.
- `csf/` — CSF queue/group/register/bind/kick lifecycle (all CSF).
- `csf-v10/ csf-v11/ csf-v12/ csf-v13/ csf-v14/` — arch-scoped CSF quirks.
- `jm-v7/` — Bifrost v7 JM backend.
- `jm-v9/` — Valhall v9 JM backend (a real port, not a flag).
- `app-loader/` — generic ICD exports (`vk_icdGetInstanceProcAddr`,
  `vk_icdNegotiateLoaderICDInterfaceVersion`) coexisting with Android WSI.
  The G52 lesson: enabling Android WSI must not select the HAL-only symbol
  scheme and hide the ICD entry points.
- `wsi/` — swapchain/acquire/present/resize, acquire-present sync.

Rules:

- Fail the build if a required patch does not apply (never continue after
  patch drift silently).
- No release from an unrecorded Mesa commit (`sources.lock` is normative).
- P3 rebase target: pinned Mesa in `sources.lock` (26.3.0-devel/main).
