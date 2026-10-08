# Mali-G77 model row (PROGRESS item 19)

Date: 2026-10-08. Patch: `patches/jm-v9/006-add-the-mali-g77-model-row.patch`. Not released, not committed.

## What

The G77 tester phone (POCO 21061110AG, MT6891, 4.14 kernel, Android 13, records `c853b7b6`/`40359b15`) reports gpu_id `0x90800011`. panvk found no model row for it and skipped the device as INCOMPATIBLE_DRIVER, so PanProbe saw no Mali GPU (0/17).

The patch adds one row to `src/panfrost/model/pan_model.c`, just before the G57 rows:

```
VALHALL_MODEL(PAN_PROD_ID(9, 0, 0), 0, "G77", "G77", MODEL_ANISO(ALL), MODEL_TB_SIZES(16384, 8192),
              MODEL_RATES_X(2, 4, 8, 32, 32, 8)),
```

No quirk is added. The G57 and G68 rows have none either.

## Values and sources

- Product id. `pan_prod_id()` (`pan_model.h`) builds `PAN_PROD_ID(arch_major, arch_minor, product_major)` from gpu_id bits 31:28, 27:24 and 19:16. For `0x90800011` that is 9, 0, 0, so the row is `PAN_PROD_ID(9, 0, 0)`. Bits 23:20 (arch rev 8) are not used in the match.
- Name. kbase names `GPU_ID2_MODEL_MAKE(9, 0)` (TTRX) "Mali-G77". The G57 rows (9,0,1 and 9,0,3) are TNAX and LNAX. The built .so contains the string `Mali-G77`.
- Variant 0. `kbase_kmod.c` sets the variant to `CORE_FEATURES & 0xff`. The G57 rows use 0, and the G57 tester enumerates with it.
- Tile buffer sizes and rates. These are the G57 values. The G57 uses the same Valhall v9 shader core as the G77, in smaller configurations, and the table values describe one core. The G57 row already lists "G77" as its performance counter set.
- Anisotropic filtering. `MODEL_ANISO(ALL)`, the same as every other Valhall row.
- Other gates. `panvk_physical_device.c` (kbase path) already accepts arch 9 (jm-v9 series). Nothing else in `src/panfrost` matches on the v9 product id.

## Proof

- Fresh worktree `/var/tmp/panvk/wt-g77` from base `5a07217f`. `scripts/apply-patches.sh --profile g615-v11-csf` applies the full series, 147 patches including jm-v9/006, with rc 0 (`/var/tmp/panvk/g77-apply.log`). The patch also passes `git apply --check` on wt-ria2, which has the series up to 181.
- Android build with `build18.sh wt-g77 g77`: EXIT 0, no errors (`/var/tmp/panvk/g77-android.out`). Output is `/var/tmp/panvk/dist-g77/libvulkan_panfrost.so` (sha256 starts with 292719bd).

## Open

- Needs a G77 tester run. Until then the fix is checked only by the build.
- Variant CONFIRMED 0 (2026-10-08, rc1 upload of the Infinix X6739, zip `293_4672fca0`, `tmp/rc1-logs/`): `panvk: gpu_id 0x90800011 variant 0 arch v9 model unknown texture_features 0xf7fffffe 0xc3fff7ff 0xbfe1ff9f 0x10f6` then `Unknown gpu_id (0x90800011) or variant (0)`. So `PAN_PROD_ID(9, 0, 0)` with variant 0 matches; no patch change needed.
- Texture features differ from the G57 tester (`0xf7fe03fe ... 0x20e` on gpu_id `0x90930010`): word 0 has more format bits and the last word differs. These come from the kernel at runtime, the model row does not encode them, so the row is unaffected. The log has no core count; the row's per-core rates stay as copied from the G57.
- rc2 build (`/var/tmp/panvk/dist-rc2/`, worktree `wt-rc2`) contains the row (`Mali-G77` string present).
- The rates are per core and copied from the G57. They affect only performance hints and counters, not correctness.
- v9 is experimental (item 12). Once the device enumerates, expect PanProbe to fail most tests beyond enumeration, as on the G57 (1-2/17).
