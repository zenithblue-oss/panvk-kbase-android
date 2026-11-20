# Hy4 Flash Free — D3D12 FL 12_0 Architecture Council Review (Round 4)

**Score: 8.8 / 10**

## 1. Verdict: Are Round 3 risks/contradictions fully resolved?

All Round 3 contradictions are fully resolved; all four consensus recommendations are folded in. The two governing documents now agree. The residual items are execution/test gaps, not architectural defects, and Path 3A's default-kill posture contains them.

- **Test A kill-switch contradiction**: Fixed. Expected ~60 MiB prefix is explicitly not a kill; Path 3A fires strictly on CSG fatal fault/`DEVICE_LOST` or <2 B2 titles.
- **Path 3A vs budget-rule contradiction**: Resolved. Prefix growth/pool leak now ban `GROW_ON_GPF` and switch Path 3B to eager pre-commit + explicit GPU zeroing.
- **Read-vs-write underspecification**: A-READ texel fetch and A-WRITE storage store are separate runs in fresh regions, recording `used_pages`, fault latency, read result, and context state.
- **Two docs disagree / dead text**: Tombstones and struck history now align across README and memory docs.

## 2. Key Technical Strengths

- **Mip-tail memory bomb genuinely mitigated**: Reverse-mip (tail at offset 0) and split mip-tail BO evaluated in Test D, selected by measured footprint.
- **Cap coherence**: Byte cap and region cap agree exactly at defaults (4096 × 64 KiB = 256 MiB), checked at reservation and bind.
- **Dirty-tracking dual granularity**: Per-tile for CPU-known transfer bounds, whole-resource for shader writes, never skip host-visible copy-in, write-back strictly before copy-in.
- **Worker drain before region free**: Deterministically prevents `MEM_COMMIT` on freed regions.
- **Zero-tile idle definition**: 0 bound tiles × N>=60 frames + last-submit fence signaled is objective and safe.
- **Honest tier stance preserved**: Tier 1-minus-Aliased, FL 12_0 only via per-title opt-in override, FL 12_1/12_2 deferred.

## 3. Residual Risks (execution-level)

1. **Compressed/3D tile NULL semantics**: Zero-page BC7/ASTC decode-is-not-black and 3D standard tiles should be assigned to Bucket C.
2. **First-tail-bind prefix-commit latency**: Test C covers shrink and D covers footprint, but initial tail-bind grow latency under 60 FPS load is unmeasured.
3. **Raw 2-titles threshold**: Consider priority-weighting B2 titles.

## 4. Final Recommendation & Readiness

**GO — all three near-term phases are ready to execute.** Path 3B remains strictly opt-in and gated on Tests A–D + census.
- Phase 0 (S-Pre): Ready.
- Census (Phase 1): Ready.
- S0-GPU (Phase 2): Ready.
