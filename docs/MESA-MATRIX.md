# Mesa Matrix and Runtime Gap

Mesa Matrix is source truth, not target-device runtime truth. The exact pinned
checkout reports 207 PanVK `DONE` entries. The published beta.3 release
retains Mesa `5a07217f034b3e50d8c7c7794f97a2df1742613b` (`26.3.0-devel`).

## Runtime comparison

| Runtime | Instance | Device | Total |
|---|---:|---:|---:|
| beta.2 tag | 12 | 166 | 178 |
| beta.3 release | 13 | 181 | 194 |

Beta.3 overlaps 193 Matrix entries and adds Android-specific
`VK_KHR_android_surface`. Fourteen Matrix entries are absent:

- 10 `platform-inapplicable`: Android excludes desktop/display WSI.
- 3 `source-supported-not-exposed`: `VK_KHR_depth_clamp_zero_one`, `VK_KHR_pipeline_binary`, `VK_KHR_robustness2`; required workloads incomplete.
- 1 `requires-wsi`: `VK_GOOGLE_display_timing`; no proven real Android timing path.

Authoritative classifications: `dist/extension-gap.json` and
`validation/g615-v11-csf/beta3-phase8-extension-gap.json`.

## Mesa decision

Candidate upstream Mesa `590bf21d918c86908d96d1f4590ecd25b9657171` was 28 commits
ahead but had zero PanVK/WSI commits and no PanVK/WSI source diff. Rebasing the
dirty Kbase integration offered no capability gain and required full
requalification. Decision: retain pinned Mesa; no extension-only backport.

The exposure gain uses Mesa's `-Dandroid-strict=false`, removing the generated
Android allowlist filter while leaving PanVK's supported-extension tables
authoritative. No local extension support bit was added.
