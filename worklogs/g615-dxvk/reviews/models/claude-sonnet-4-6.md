# Claude Sonnet 4.6 — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.5 / 10**

### Round 1 Blockers — Resolution Verdict

| R1 Blocker | Status |
|---|---|
| False Tier 2 claim | **Resolved.** "Tier 1-minus-Aliased" is now the stated ceiling. Opt-in env-var override is correct posture. |
| 16 GiB monolithic eager arena | **Resolved.** Per-resource SAME_VA GROW_ON_GPF + 256 MiB context cap is the right model. |
| CSF group fault on unbound tiles | **Resolved in principle.** S0-GPU kill-switch gate tests this on real HW before S1 is committed. |
| Bind sync / MEM_COMMIT hot-path cost | **Resolved.** Dirty bits + deferred fence-gated uncommit worker is correct. |
| Prerequisite ordering | **Resolved.** S-Pre → Census → S0-GPU → S1-S6 is a proper gated sequence. |

All five R1 blockers are addressed at the architecture level.

### Remaining Risks & Nuances

1. **Risk 1 — CSF sink cannot be FIXED (medium)**: The plan notes the zero-sink cannot be FIXED. For TMU bilinear/border taps that cross a bound/unbound tile edge, the hardware texel address is computed by the fixed-function unit before the driver can intercept it. If the sink region isn't at the FIXED tile VA, redirecting those taps is undefined behaviour — the S0-GPU gate must explicitly test this boundary-sampling case and declare the per-title "safe to use" verdict from hardware evidence, not from the design assumption. If GPF-growth passes but sink does not, eager is the only safe fallback for CSF too.
2. **Risk 2 — Compressed tile format unbound-read semantics (medium)**: "Zero pages on unbound tiles" is correct for integer/unorm formats. For block-compressed textures (BC1/BC7, ASTC), an all-zero committed page does not decode as black — the compressed block bitfield `0x00000000...` maps to implementation-defined colour values. Title census must flag whether any sparse resource uses a compressed format; if so, unbound reads are not safe even with GROW_ON_GPF zeroing, and the S0-GPU gate needs a BC/ASTC format variant.
3. **Risk 3 — FL 12_0 override pulls in more than tiled resources (low-medium)**: `VKD3D_FEATURE_LEVEL=12_0` raises the reported feature level, which makes vkd3d-proton enable additional FL 12_0 caps: typed UAV loads (requires `VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT` on more formats), resource binding tier 2 (unbounded arrays), and potentially conservative raster queries. The plan covers sparse but doesn't audit which other FL 12_0 feature queries the driver must answer honestly under the override. Add a "feature audit" column to the census: for each target title, which FL 12_0 caps does it actually query and exercise?
4. **Risk 4 — Census timing estimate is optimistic (low)**: "1-2 d" for census assumes Wine/Box64/vkd3d is already deployed on a test device and titles are known to run to main menu without crashing. In practice the census is 1-2 d per title that runs cleanly, not total. Flag this in the timeline; it doesn't block the architecture but it blocks deciding whether S1-S6 are needed.
5. **Risk 5 — Deferred uncommit worker memory pressure window (low)**: The worker is gated on the bind fence. If a title runs many bind/unbind cycles per frame (tile streaming), and the bind fence is on a CPU-side timeline that tracks submission rather than GPU completion, the worker might hold committed pages for 2-3 frames, tripling peak sparse memory pressure. The generation counter prevents stale frees correctly, but the memory accounting should include in-flight uncommit as "pending free" for heap budget queries.
6. **Risk 6 — Concurrent bind recording (low, implementation-level)**: Per-tile dirty bits are shared state. Vulkan allows concurrent command buffer recording. Dirty-bit writes from multiple threads must be atomic (at minimum `std::atomic<uint8_t>` or a lock-free bitmap).

### Final Recommendation
**Approve for S-Pre + Census start. Conditional on S0-GPU gate results before S1.** Proceed to S-Pre immediately (robustImageAccess2 + float controls are unblocked). Do not begin S1 bind engine work until S0-GPU gate passes with the expanded test matrix.
