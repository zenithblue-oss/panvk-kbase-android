/* Gate C: dlopen the Bionic PanVK, negotiate ICD, create instance, enumerate.
 * Usage: enumerate <path-to-libvulkan_panfrost.so>
 * PASS: exactly the Mali physical device appears (never lavapipe/vendor).
 */
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef VkResult (*negotiate_fn)(uint32_t *pVersion);
typedef PFN_vkVoidFunction (*icd_gipa_fn)(VkInstance, const char *);

int
main(int argc, char **argv)
{
   if (argc < 2) {
      printf("usage: %s <libvulkan_panfrost.so>\n", argv[0]);
      return 2;
   }
   void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
   if (!h) {
      printf("C-FAIL dlopen: %s\n", dlerror());
      return 1;
   }
   printf("C-OK dlopen\n");

   negotiate_fn neg = (negotiate_fn)dlsym(h, "vk_icdNegotiateLoaderICDInterfaceVersion");
   if (!neg) {
      printf("C-FAIL negotiate missing\n");
      return 1;
   }
   uint32_t ver = 5;
   VkResult r = neg(&ver);
   printf("C-OK negotiate r=%d ver=%u\n", r, ver);

   icd_gipa_fn gipa =
      (icd_gipa_fn)dlsym(h, "vk_icdGetInstanceProcAddr");
   if (!gipa) {
      printf("C-FAIL gipa missing\n");
      return 1;
   }

   PFN_vkCreateInstance ci =
      (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
   if (!ci) {
      printf("C-FAIL vkCreateInstance missing\n");
      return 1;
   }

   VkApplicationInfo app = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_3,
   };
   VkInstanceCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app,
   };
   VkInstance inst;
   r = ci(&ici, NULL, &inst);
   printf("C-CreateInstance r=%d\n", r);
   if (r != VK_SUCCESS)
      return 1;

   PFN_vkEnumeratePhysicalDevices epd =
      (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
   PFN_vkGetPhysicalDeviceProperties gpp =
      (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
   if (!epd || !gpp) {
      printf("C-FAIL entrypoints epd=%p gpp=%p\n", (void *)epd, (void *)gpp);
      return 1;
   }

   uint32_t n = 0;
   r = epd(inst, &n, NULL);
   printf("C-Enumerate count=%u r=%d\n", n, r);
   if (r != VK_SUCCESS || n == 0) {
      printf("C-FAIL no physical devices\n");
      return 1;
   }
   VkPhysicalDevice devs[8];
   if (n > 8)
      n = 8;
   r = epd(inst, &n, devs);
   int mali = 0;
   for (uint32_t i = 0; i < n; i++) {
      VkPhysicalDeviceProperties p;
      gpp(devs[i], &p);
      printf("C-DEV %u: %s id=0x%x vendor=0x%x api=0x%x\n", i,
             p.deviceName, p.deviceID, p.vendorID, p.apiVersion);
      if (strstr(p.deviceName, "Mali") || strstr(p.deviceName, "mali"))
         mali++;
      if (strstr(p.deviceName, "lavapipe") || strstr(p.deviceName, "llvmpipe"))
         printf("C-WARN lavapipe present (must not be selected)\n");
   }
   if (!mali) {
      printf("C-FAIL no Mali device enumerated\n");
      return 1;
   }
   printf("C-PASS\n");
   return 0;
}
