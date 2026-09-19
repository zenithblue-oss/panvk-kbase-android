/*
 * texture_features.c — Kbase texture feature register probe & compressed format decoder
 * Reads RAW_TEXTURE_FEATURES_0..3 from /dev/mali0 via Kbase GPUPROPS ioctl.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "drm-uapi/mali_kbase_ioctl.h"

/* Helper to extract a property from Kbase GPU props buffer */
static uint64_t
kbase_gpuprop_get(const uint8_t *buf, size_t buf_size,
                  uint32_t target_key, uint64_t default_val)
{
   size_t offset = 0;
   while (offset + 4 <= buf_size) {
      uint32_t hdr;
      memcpy(&hdr, buf + offset, 4);
      offset += 4;

      uint32_t key = hdr >> 2;
      uint32_t size_code = hdr & 0x3;
      uint32_t val_size = 1u << size_code;

      if (offset + val_size > buf_size)
         break;

      if (key == target_key) {
         uint64_t val = 0;
         memcpy(&val, buf + offset, val_size);
         return val;
      }

      offset += val_size;
   }
   return default_val;
}

struct format_desc {
   unsigned bit;
   const char *name;
};

static const struct format_desc compressed_formats[] = {
   { 1,  "ETC2_RGB8" },
   { 2,  "ETC2_R11_UNORM" },
   { 3,  "ETC2_RGBA8" },
   { 4,  "ETC2_RG11_UNORM" },
   { 7,  "BC1_UNORM" },
   { 8,  "BC2_UNORM" },
   { 9,  "BC3_UNORM" },
   { 10, "BC4_UNORM" },
   { 11, "BC4_SNORM" },
   { 12, "BC5_UNORM" },
   { 13, "BC5_SNORM" },
   { 14, "BC6H_UF16" },
   { 15, "BC6H_SF16" },
   { 16, "BC7_UNORM" },
   { 17, "ETC2_R11_SNORM" },
   { 18, "ETC2_RG11_SNORM" },
   { 19, "ETC2_RGB8A1" },
   { 20, "ASTC_3D_LDR" },
   { 21, "ASTC_3D_HDR" },
   { 22, "ASTC_2D_LDR" },
   { 23, "ASTC_2D_HDR" },
   { 24, "AFBC" },
   { 25, "AFRC" },
   { 30, "ASTC_2D_NARROW" },
   { 31, "ASTC_3D_NARROW" },
};

int main(int argc, char **argv)
{
   const char *devnode = "/dev/mali0";
   if (argc > 1)
      devnode = argv[1];

   int fd = open(devnode, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      fprintf(stderr, "FAIL: open(%s): %s\n", devnode, strerror(errno));
      return 1;
   }

   /* Version handshake */
   struct kbase_ioctl_version_check ver = { 0 };
   if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK_CSF, &ver) == 0) {
      printf("kbase flavor=CSF major=%u minor=%u\n", ver.major, ver.minor);
   } else {
      memset(&ver, 0, sizeof(ver));
      if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK_JM, &ver) == 0) {
         printf("kbase flavor=JM major=%u minor=%u\n", ver.major, ver.minor);
      } else {
         fprintf(stderr, "FAIL: kbase version check: %s\n", strerror(errno));
         close(fd);
         return 1;
      }
   }

   /* Set flags */
   struct kbase_ioctl_set_flags set_flags = { 0 };
   if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &set_flags) != 0) {
      fprintf(stderr, "FAIL: SET_FLAGS: %s\n", strerror(errno));
      close(fd);
      return 1;
   }

   /* Tracking page mmap */
   void *tp = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE);
   if (tp == MAP_FAILED) {
      fprintf(stderr, "FAIL: mmap tracking page: %s\n", strerror(errno));
      close(fd);
      return 1;
   }

   /* Query GPU props buffer size */
   struct kbase_ioctl_get_gpuprops req = { 0 };
   int ret = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &req);
   if (ret < 0) {
      fprintf(stderr, "FAIL: GET_GPUPROPS size probe: %s\n", strerror(errno));
      munmap(tp, 4096);
      close(fd);
      return 1;
   }
   size_t props_size = (size_t)ret;
   uint8_t *props_buf = malloc(props_size);
   if (!props_buf) {
      fprintf(stderr, "FAIL: malloc(%zu)\n", props_size);
      munmap(tp, 4096);
      close(fd);
      return 1;
   }

   req.buffer = (uintptr_t)props_buf;
   req.size = (uint32_t)props_size;
   ret = ioctl(fd, KBASE_IOCTL_GET_GPUPROPS, &req);
   if (ret < 0) {
      fprintf(stderr, "FAIL: GET_GPUPROPS fill: %s\n", strerror(errno));
      free(props_buf);
      munmap(tp, 4096);
      close(fd);
      return 1;
   }

   uint32_t raw_gpu_id = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_RAW_GPU_ID, 0);
   printf("raw_gpu_id=0x%08x\n", raw_gpu_id);

   /* Read texture feature registers */
   uint32_t tf[4] = { 0 };
   tf[0] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_RAW_TEXTURE_FEATURES_0, 0);
   tf[1] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_RAW_TEXTURE_FEATURES_1, 0);
   tf[2] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_RAW_TEXTURE_FEATURES_2, 0);
   tf[3] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_RAW_TEXTURE_FEATURES_3, 0);

   /* Fallbacks if raw keys were not found */
   if (!tf[0])
      tf[0] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_TEXTURE_FEATURES_0, 0);
   if (!tf[1])
      tf[1] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_TEXTURE_FEATURES_1, 0);
   if (!tf[2])
      tf[2] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_TEXTURE_FEATURES_2, 0);
   if (!tf[3])
      tf[3] = (uint32_t)kbase_gpuprop_get(props_buf, props_size, KBASE_GPUPROP_TEXTURE_FEATURES_3, 0);

   uint32_t compressed_format_mask = tf[0];

   printf("texture_features[0]=0x%08x\n", tf[0]);
   printf("texture_features[1]=0x%08x\n", tf[1]);
   printf("texture_features[2]=0x%08x\n", tf[2]);
   printf("texture_features[3]=0x%08x\n", tf[3]);
   printf("compressed_format_mask=0x%08x\n", compressed_format_mask);

   printf("=== Symbolic decode ===\n");
   for (size_t i = 0; i < sizeof(compressed_formats) / sizeof(compressed_formats[0]); i++) {
      bool supported = (compressed_format_mask & (1u << compressed_formats[i].bit)) != 0;
      printf("%s=%u\n", compressed_formats[i].name, supported ? 1 : 0);
   }

   /* BCn full mask check */
   uint32_t bc_mask = (1u << 7)  | /* BC1_UNORM */
                      (1u << 8)  | /* BC2_UNORM */
                      (1u << 9)  | /* BC3_UNORM */
                      (1u << 10) | /* BC4_UNORM */
                      (1u << 11) | /* BC4_SNORM */
                      (1u << 12) | /* BC5_UNORM */
                      (1u << 13) | /* BC5_SNORM */
                      (1u << 14) | /* BC6H_UF16 */
                      (1u << 15) | /* BC6H_SF16 */
                      (1u << 16);  /* BC7_UNORM */

   bool all_bc = (compressed_format_mask & bc_mask) == bc_mask;
   printf("BC_ALL_10_FORMATS_MASK=0x%08x\n", bc_mask);
   printf("BC_SUPPORTED_MASK=0x%08x\n", compressed_format_mask & bc_mask);
   printf("textureCompressionBC_native_eligible=%s\n", all_bc ? "true" : "false");

   free(props_buf);
   munmap(tp, 4096);
   close(fd);
   return 0;
}
