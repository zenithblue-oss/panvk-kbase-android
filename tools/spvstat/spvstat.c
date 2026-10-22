/* spvstat: compile a Vulkan compute SPIR-V (with spec constants 2 = mode,
 * 3 = opts) for a Mali arch with the panfrost compiler, roughly the way
 * panvk does, and print the Valhall shader stats (BIFROST_MESA_DEBUG=shaderdb)
 * plus the CPU compile time. batch (default 0) is spec constant 4
 * (csf-v11/214 XC_BATCH of panvk_bc_etc2.comp).
 *   spvstat file.spv arch mode opts [reps] [batch]  */
#include "compiler/glsl_types.h"
#include "compiler/spirv/nir_spirv.h"
#include "panfrost/compiler/bifrost/bifrost_compile.h"
#include "panfrost/compiler/pan_compiler.h"
#include "panfrost/compiler/pan_nir.h"
#include "nir.h"
#include "nir_builder.h"
#include "pan_shader.h"
#include "util/os_file.h"
#include "util/u_dynarray.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void pan_shader_compile(nir_shader *nir, struct pan_compile_inputs *inputs,
                        struct util_dynarray *binary,
                        struct pan_shader_info *info);

static double
now_ms(void)
{
   struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);
   return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static bool
lower_load_constant(nir_builder *b, nir_intrinsic_instr *intr, void *data)
{
   if (intr->intrinsic != nir_intrinsic_load_constant)
      return false;
   b->cursor = nir_before_instr(&intr->instr);
   nir_def *offset = nir_iadd_imm(b, intr->src[0].ssa, nir_intrinsic_base(intr));
   nir_def *base = nir_load_constant_base_ptr(b, 1, 64);
   nir_def *addr = nir_iadd(b, base, nir_u2u64(b, offset));
   nir_def *val = nir_load_global_constant(
      b, intr->def.num_components, intr->def.bit_size, addr,
      .access = ACCESS_CAN_REORDER | ACCESS_NON_WRITEABLE,
      .align_mul = nir_intrinsic_align_mul(intr),
      .align_offset = nir_intrinsic_align_offset(intr));
   nir_def_replace(&intr->def, val);
   return true;
}

static void
optimize(nir_shader *nir)
{
   bool progress;
   do {
      progress = false;
      NIR_PASS(progress, nir, nir_split_var_copies);
      NIR_PASS(progress, nir, nir_lower_var_copies);
      NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
      NIR_PASS(progress, nir, nir_opt_copy_prop);
      NIR_PASS(progress, nir, nir_opt_remove_phis);
      NIR_PASS(progress, nir, nir_opt_dce);
      NIR_PASS(progress, nir, nir_opt_dead_cf);
      NIR_PASS(progress, nir, nir_opt_cse);
      NIR_PASS(progress, nir, nir_opt_algebraic);
      NIR_PASS(progress, nir, nir_opt_constant_folding);
      NIR_PASS(progress, nir, nir_opt_deref);
      NIR_PASS(progress, nir, nir_opt_undef);
      NIR_PASS(progress, nir, nir_opt_loop_unroll);
   } while (progress);
}

int
main(int argc, char **argv)
{
   if (argc < 5) {
      fprintf(stderr, "usage: spvstat file.spv arch mode opts [reps] [batch]\n");
      return 1;
   }
   size_t size = 0;
   uint32_t *words = (uint32_t *)os_read_file(argv[1], &size);
   unsigned arch = atoi(argv[2]);
   uint32_t mode = strtoul(argv[3], NULL, 0), opts = strtoul(argv[4], NULL, 0);
   int reps = argc > 5 ? atoi(argv[5]) : 1;
   uint32_t batch = argc > 6 ? strtoul(argv[6], NULL, 0) : 0;
   if (!words)
      return 1;

   glsl_type_singleton_init_or_ref();
   const nir_shader_compiler_options *nopts =
      pan_get_nir_shader_compiler_options(arch, MESA_SHADER_COMPUTE, false);
   struct spirv_to_nir_options sopts = {
      .environment = NIR_SPIRV_VULKAN,
      .ubo_addr_format = nir_address_format_32bit_index_offset,
      .ssbo_addr_format = nir_address_format_32bit_index_offset,
      .phys_ssbo_addr_format = nir_address_format_64bit_global,
      .push_const_addr_format = nir_address_format_32bit_offset,
      .shared_addr_format = nir_address_format_32bit_offset,
      .min_ubo_alignment = 16,
      .min_ssbo_alignment = 4,
   };
   struct nir_spirv_specialization_entry ents[3] = {
      {.id = 2, .size = 4, .data = (uint8_t *)&mode},
      {.id = 3, .size = 4, .data = (uint8_t *)&opts},
      {.id = 4, .size = 4, .data = (uint8_t *)&batch},
   };
   struct nir_spirv_specialization spec = {3, ents};
   uint64_t gpu_id = (uint64_t)(arch & 0xf) << 28;
   if (arch == 11)
      gpu_id = 0xb8a31030ull; /* G615 (props.gpu_id from the device log) */

   double best = 1e30;
   for (int r = 0; r < reps; r++) {
      double t0 = now_ms();
      nir_shader *nir = spirv_to_nir(words, size / 4, &spec,
                                     MESA_SHADER_COMPUTE, "main", &sopts, nopts);
      NIR_PASS(_, nir, nir_lower_variable_initializers, nir_var_function_temp);
      NIR_PASS(_, nir, nir_lower_returns);
      NIR_PASS(_, nir, nir_inline_functions);
      nir_remove_non_entrypoints(nir);
      NIR_PASS(_, nir, nir_opt_deref);
      NIR_PASS(_, nir, nir_lower_variable_initializers, ~0);
      NIR_PASS(_, nir, nir_lower_global_vars_to_local);
      NIR_PASS(_, nir, nir_lower_vars_to_ssa);
      NIR_PASS(_, nir, nir_lower_system_values);
      nir_lower_compute_system_values_options cso = {
         
      };
      NIR_PASS(_, nir, nir_lower_compute_system_values, &cso);
      pan_preprocess_nir(nir, gpu_id);
      NIR_PASS(_, nir, nir_opt_large_constants, NULL, 32);
      NIR_PASS(_, nir, nir_shader_intrinsics_pass, lower_load_constant,
               nir_metadata_control_flow, NULL);
      NIR_PASS(_, nir, nir_lower_vars_to_explicit_types,
               nir_var_mem_push_const, glsl_get_natural_size_align_bytes);
      NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_push_const,
               nir_address_format_32bit_offset);
      NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_global,
               nir_address_format_64bit_global);
      optimize(nir);

      struct pan_compile_inputs inputs = {.gpu_id = gpu_id};
      struct pan_shader_info info = {0};
      pan_postprocess_nir(nir, &inputs, &info);
      struct util_dynarray bin = UTIL_DYNARRAY_INIT;
      pan_shader_compile(nir, &inputs, &bin, &info);
      double t = now_ms() - t0;
      best = t < best ? t : best;
      if (r == reps - 1)
         printf("binary_B=%u tls_B=%u wls_B=%u work_regs=%u compile_ms=%.1f\n",
                (unsigned)bin.size, info.tls_size, info.wls_size,
                info.work_reg_count, best);
      util_dynarray_fini(&bin);
      ralloc_free(nir);
   }
   return 0;
}
