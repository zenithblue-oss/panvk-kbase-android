# Grok 4.7 High — D3D12 FL 12_0 Architecture Council Review (Round 2)

**Score: 7.0 / 10**

### Round 1 blockers

| Blocker | Status |
|---|---|
| Global FL/Tier 2 lie | Closed. No driver cap claim. Opt-in only: Wine `VKD3D_CAPS_OVERRIDE=feature_level=12.0`, proton `VKD3D_FEATURE_LEVEL=12_0`. Census must show no `UpdateTileMappings` / `CopyTileMappings` / alias. |
| JM/v14 16 GiB eager arena + LMK | Closed. Per-resource SAME_VA `GROW_ON_GPF`, freed with resource. Cap `min(PANVK_SPARSE_CAP_MB=256, heap budget)`. Over-cap → `VK_ERROR_OUT_OF_DEVICE_MEMORY`. |
| CSF unbound read kills group | Partially. S0-GPU gate before S1: (a) GPF growth, (b) dummy sink, else eager. (b) correctly dead if sink cannot be FIXED (`mem_linux.c:1759`). |
| Unfenced `MEM_COMMIT` UAF | Closed. Deferred worker, bind-fence wait, generation cancel, frees wait on fence. |
| Full write-back every unbind | Closed. Dirty bit: write targets only; copy-in skipped if source never written. |
| Wrong runtime / missing prereqs | Closed. Both env vars named. S-Pre: `robustImageAccess2` + `VK_KHR_shader_float_controls` before sparse. Census can kill S1–S6. |

### Remaining risks

1. **Override still lies to the app.** vkd3d-proton `VKD3D_FEATURE_LEVEL=12_0` reports tier 2 (`device.c:10035-10040`) while `sparseResidencyAliased` stays false. Census must include placed-on-reserved and heap alias, not only tile APIs.
2. **Path (a) fights strict non-resident.** GPF growth on read allocates a zero page until the worker re-uncommits. A second fault inflates the cap. If kbase does not grow GPU reads, (a) fails → CSF eager under 256 MiB.
3. **256 MiB is not a streaming budget.** JM cannot punch holes, so reserved resources pay full size at create. Cap stops LMK but fails titles with large reservations.
4. **Region ceiling still unmeasured.** CSF: one region per 64 KiB tile. 256 MiB = 4096 regions. S0-GPU must fail closed on region-alloc ENOSPC.
5. **Dirty set is coarse.** Non-copy writes mark every bound tile. UAV/RT touches dirty the whole resource.

### Final Recommendation
Ship the paper plan. Do not start S1. Order stays: S-Pre → census → S0-GPU on CSF v10/v11 and JM v9. Gate fails closed unless one of (a) or (b) gives read-0, bound/unbound bilinear and border, no group fault, no `DEVICE_LOST`, and region alloc stays under cap. If census shows zero tiled use, stop after S5 profiles.
