# Result: panvk-g615-beta3-feature-bcn-worker

## Task

Finalize the PanVK Mali-G615 beta.3 capability expansion and BCn investigation from `PANVK_G615_BETA3_FEATURE_BCN_WORKER.md`, preserving truthful native capability reporting, validating the candidate on the target device, packaging the validated Android and glibc artifacts, and keeping optional BC compatibility excluded until its complete correctness suite passes.

## Final Status

**DONE / CANDIDATE_READY, NOT_PUBLISHED before repository push.** All 15 applicable mandatory release gates pass; the conditional shipped-BC gate is `NOT_APPLICABLE` because BC compatibility is excluded from both packages. No tag or GitHub release exists.

## Workflow Summary

| Stage | Evidence | Verdict |
|---|---|---|
| Baseline and capability audit | `validation/g615-v11-csf/beta3-phase8-extension-gap.json` | PASS |
| Implementation and feature workloads | `validation/g615-v11-csf/beta3-phase5-features-2026-09-19.json`, `validation/g615-v11-csf/beta3-phase7-extension-workloads-2026-09-19.json` | PASS |
| Target-device regression | `validation/g615-v11-csf/beta3-phase8-summary.json` | PASS |
| Native BC investigation | `validation/g615-v11-csf/beta3-final-native-bc-evidence.json` | PASS: native BC unavailable |
| Optional BC compatibility | `validation/g615-v11-csf/beta3-phase6-bcn.json` | BLOCKED and excluded |
| Package validation | `validation/g615-v11-csf/beta3-final-package-android-2026-09-19.txt`, `validation/g615-v11-csf/beta3-final-package-glibc-2026-09-19.txt` | PASS |
| Final gate audit | `validation/g615-v11-csf/beta3-final-gates.json` | CANDIDATE_READY |

## Implementation

- Expanded the tested runtime from the beta.2 baseline of 12 instance and 166 device extensions (178 total) to 13 instance and 181 device extensions (194 total), with 16 additions workload-tested and no removals.
- Added runtime-extension auditing with explicit Android platform exclusions and source/runtime proof instead of count-driven exposure.
- Added targeted feature workloads for descriptor indexing, timeline semaphores, dynamic rendering, synchronization2, buffer device address, push descriptors, robust buffer access, anisotropy, wide lines, large points, ETC2, ASTC LDR, and ASTC HDR.
- Added Kbase texture-feature and Vulkan BC-format probes. Native `textureCompressionBC` remains false: the BC mask is zero and all 16 queried BC formats are unsupported.
- Recorded the optional `bcn_layer` reference at commit `50993a2d51772567de9c36de4d523652773f0899`, its MIT licensing, Android build support, and its staged-PanVK composition blocker. It is not enabled or packaged.
- Updated WSI capability handling, package metadata, provenance, validation collection, documentation, and deterministic patch-series tracking.

## Files Changed

- Driver patch stack: `patches/wsi/016-android-surface-swapchain.patch`, `patches/bcn-layer/001-android-layer-enumeration.patch`.
- Build/package/validation tooling: `.gitignore`, `scripts/build-android.sh`, `scripts/build-bcn-layer-android.sh`, `scripts/audit-runtime-extensions.py`, `scripts/collect-validation.py`, `scripts/generate-release-manifest.py`, `scripts/package-android-adpkg.sh`, `scripts/package-glibc.sh`, `scripts/validate-package.sh`.
- Runtime and device tests: `tests/android-loader-app/`, `tests/features/`, `tests/kbase-probe/`, `tests/bcn/`, `tests/runtime-features/`, `tests/validate-package-selfcheck.py`.
- Documentation and provenance: `README.md`, `NOTICE.md`, `LICENSES/`, `sources.lock`, `docs/MESA-MATRIX.md`, `docs/RUNTIME-FEATURES.md`, `docs/BCN-COMPAT.md`, `docs/BCN-INTEGRATION.md`, `tests/README.md`.
- Validation evidence: beta.3 Phase 4/5/6/7/8 and final records under `validation/g615-v11-csf/`, plus the updated canonical `runtime-feature-matrix.json` and corrected beta.2 evidence captures.
- Repository-convention package asset: tracked `tests/android-loader-app/panvk-loader-test.apk` and `.idsig` are included. Release candidate archives under ignored `dist/` remain untracked.

## Tests

| Check | Result | Evidence |
|---|---|---|
| Schema and extension-audit unit tests | PASS, 12/12 | Final run: `python3 -m unittest tests/runtime-features/test_matrix_schema.py tests/runtime-features/test_extension_audit.py` |
| BCn evidence ledger | PASS | `python3 tests/bcn/validate_cases.py validation/g615-v11-csf/beta3-phase6-bcn.json` |
| Package validator self-check | PASS | `python3 tests/validate-package-selfcheck.py` |
| Android package validation | PASS | `validation/g615-v11-csf/beta3-final-package-android-2026-09-19.txt` |
| glibc package validation | PASS | `validation/g615-v11-csf/beta3-final-package-glibc-2026-09-19.txt` |
| Diff whitespace check | PASS | Final `git diff --check` |

No expensive rebuild was run during finalization. Existing clean-build evidence remains in `validation/g615-v11-csf/beta3-phase8-android-build-2026-09-19.txt` and `validation/g615-v11-csf/beta3-phase8-glibc-build-2026-09-19.txt`.

## Runtime / Manual Verification

No separate manual-tester report is claimed. Runtime verification uses the recorded target-device Phase 5, Phase 7, and Phase 8 evidence from Poco X6 Pro (`2311DRK48I`), Mali-G615 MC6:

- Phase 5: all 13 targeted exposed-feature workloads pass in `validation/g615-v11-csf/beta3-phase5-features-2026-09-19.json`.
- Phase 7: 13 non-WSI extension workloads and four Android WSI extension workloads pass in `validation/g615-v11-csf/beta3-phase7-extension-workloads-2026-09-19.json` and `validation/g615-v11-csf/beta3-phase8-wsi-2026-09-19.txt`.
- Phase 8: enumeration, device creation, compute 10/10, offscreen rendering, AHB import/render/readback, synchronization, Android surface/swapchain, 300/300 presents, cold second-process launch, and glibc runtime gates pass in `validation/g615-v11-csf/beta3-phase8-summary.json`.
- Final native BC capture records `TF0=0xc7fe001e`, `TF1-TF3=0`, BC supported mask zero, and unsupported results for all 16 BC formats in `validation/g615-v11-csf/beta3-final-kbase-texture-features-2026-09-19.txt` and `validation/g615-v11-csf/beta3-final-bc-format-properties-2026-09-19.txt`.

## Review Findings

- Final gate review: 15 mandatory `PASS`, zero mandatory `FAIL`/`BLOCKED`, one conditional `NOT_APPLICABLE`; recorded in `validation/g615-v11-csf/beta3-final-gates.json`.
- Three unproven candidates remain disabled and unexposed: `VK_KHR_depth_clamp_zero_one`, `VK_KHR_pipeline_binary`, and `VK_KHR_robustness2`.
- `VK_GOOGLE_display_timing` remains unexposed because no proven Android timing implementation exists.
- Optional BC compatibility cannot compose with staged PanVK; correctness, stress, and performance suites remain `NOT_RUN`/`BLOCKED`. Exclusion from both packages prevents an unsupported capability claim.
- Final artifact audit found no untracked build-cache or oversized generated artifact eligible for staging. Ignored `work/`, `build/`, and `dist/` outputs remain outside Git by repository convention.

## Evidence

- Plan: `/home/abhaybyte/Downloads/plans/PANVK_G615_BETA3_FEATURE_BCN_WORKER.md`.
- Worker report: `validation/g615-v11-csf/BETA3-WORKER-REPORT.md`.
- Final gates: `validation/g615-v11-csf/beta3-final-gates.json`.
- Target-device summary: `validation/g615-v11-csf/beta3-phase8-summary.json`.
- Mesa commit: `5a07217f034b3e50d8c7c7794f97a2df1742613b`.
- Patch series: `sha256:c0bbdeef591b206a2f3ae36dc6191c08854399039075f8d69f103e33c0eca2f8`.
- Android ICD: `sha256:576e37de9a3b60dda9a972a6f91c3a50e09238bd31a9888e04215c7b554c803a`.
- glibc ICD: `sha256:95a019b21d42f9d91bf3ee697a4e84b09485fdd8f0c298dda4494356cb983ec0`.
- Android package: `dist/PanVK-Kbase-Android-g615-v11-csf-v0.1.0-beta.3-5a07217f.adpkg.zip`, `sha256:e94945ad928881919bdd8ce2273de097f0d385c3ba57e87ef959e4cf40442561`.
- glibc package: `dist/PanVK-Kbase-g615-v11-csf-v0.1.0-beta.3-5a07217f-EMULATOR.zip`, `sha256:f7f13c5dc7a8c07169151710d18a9b134f6cb8bafb83134fd9f07ef4c2ad36c4`.
- Native BC proof: `validation/g615-v11-csf/beta3-final-native-bc-evidence.json`.
- Package evidence: `validation/g615-v11-csf/beta3-final-package-android-2026-09-19.txt`, `validation/g615-v11-csf/beta3-final-package-glibc-2026-09-19.txt`.

## Remaining Limitations

- Native BC1-BC7 is unavailable on the tested G615 target. `textureCompressionBC` correctly remains false.
- Optional BC compatibility remains experimental, blocked with staged PanVK, excluded from packages, and disabled by default.
- BC correctness, stress, and performance claims are not made.
- Candidate publication remains separate work: no beta.3 tag, prerelease, or GitHub release is created by this finalization.

## Final Acceptance Criteria

| Criterion | Status | Evidence |
|---|---|---|
| Exact beta.2 extension gap explained | VERIFIED | `beta3-phase8-extension-gap.json` |
| Safe additions source-proven and workload-tested | VERIFIED | `runtime-feature-matrix.json`, Phase 7 evidence |
| Important exposed features targeted | VERIFIED | Phase 5 evidence |
| Native BC capability proven | VERIFIED | final Kbase and BC-format evidence |
| No spoofed BC feature bit | VERIFIED | canonical runtime matrix and final gates |
| Optional BC path completed or concrete blocker documented | VERIFIED: blocker documented, component excluded | Phase 6 evidence and BC docs |
| All beta.2 regression gates pass | VERIFIED | Phase 8 summary |
| Android and glibc packages validate | VERIFIED | final package logs |
| Publish only with truthful reproducible result | VERIFIED for candidate; publication not performed | final gates and worker report |

## Final Verdict

**DONE / CANDIDATE_READY.** Mandatory beta.3 criteria pass. Native BC reporting remains truthful. Optional BC compatibility remains experimental and excluded. Repository push is authorized; release publication is not performed.

## Verification Statement

The beta.3 candidate is verified by target-device Phase 5/7/8 evidence, final native-BC probes, regression gates, package validation, 12 passing unit tests, BCn ledger validation, package self-checks, and `git diff --check`. BC compatibility correctness, stress, performance, and staged-PanVK composition are not verified and are not shipped. Status before this repository push: `DONE`, `CANDIDATE_READY`, `NOT_PUBLISHED`.
