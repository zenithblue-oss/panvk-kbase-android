# App compatibility

- **Samba S3** (primary Android): release ZIP imported/bundled -> custom loader
  handle -> Mali physical device -> RPCSX gets loader -> surface/present ->
  RPCSX Vulkan init -> lightweight title renders. Samba code changes live in
  the Samba repo; here only adapter metadata/tests.
- **Bachata S4** (primary glibc): exact Bachata/shadPS4 build; glibc ICD
  selected -> PanVK enumerated -> feature/extension probe (1.3 +
  swapchain + push_descriptor minimum, plus current renderer deltas) ->
  renderer init -> simple title frames. Never fake features.
- **NativeCode AI**: glibc build for vulkaninfo/vkmark/Zink/feature capture +
  primary on-device build env.
- **Winlator** (secondary, after loader+WSI): AdrenoTools import -> native
  Vulkan -> Wine Vulkan -> DXVK D3D11 -> VKD3D only if sufficient. Import !=
  success; stock DXVK may reject missing feature bits — keep app requirements
  separate from core correctness.
- **GameHub** (secondary): injection -> runtime selects library -> PanVK
  device -> simple Vulkan -> DXVK title -> clean rollback. GameHub aliases stay
  out of the canonical package unless proven necessary.
