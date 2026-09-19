#!/usr/bin/env python3
"""Schema + parser checks for P10/P11. No device required."""
from __future__ import annotations

import importlib.util
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def _load(name: str, filename: str):
    path = ROOT / "scripts" / filename
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


cap = _load("capture_runtime_features", "capture-runtime-features.py")
col = _load("collect_validation", "collect-validation.py")

DUMP = """\
=== INSTANCE EXTENSIONS (12) ===
  VK_KHR_android_surface (v6)
  VK_KHR_surface (v25)
  VK_KHR_get_physical_device_properties2 (v2)
=== PHYSICAL DEVICE ===
  deviceName: Mali-G615 MC6
  deviceID: 0xb8a31030
  vendorID: 0x13b5
  apiVersion: 1.4.363
  driverVersion: 0x6802063
  driverID: 20
  driverName: panvk
  driverInfo: Mesa 26.3.0-devel
=== DEVICE EXTENSIONS (3) ===
  VK_KHR_push_descriptor (v2)
  VK_KHR_swapchain (v70)
  VK_KHR_external_memory (v1)
=== VULKAN 1.0 CORE FEATURES ===
  robustBufferAccess                   : YES
  geometryShader                       : NO
  tessellationShader                   : NO
  multiViewport                        : NO
  shaderClipDistance                   : NO
  shaderCullDistance                   : NO
  shaderFloat64                        : NO
  depthBounds                          : NO
  wideLines                            : YES
  largePoints                          : YES
  samplerAnisotropy                    : YES
  textureCompressionETC2               : YES
  textureCompressionASTC_LDR           : YES
  textureCompressionBC                 : NO
=== VULKAN 1.1 / 1.2 / 1.3 FEATURES ===
  descriptorIndexing                   : YES
  timelineSemaphore                    : YES
  bufferDeviceAddress                  : YES
  dynamicRendering                     : YES
  synchronization2                     : YES
"""


class ParseDump(unittest.TestCase):
    def test_counts_and_ids(self):
        m = cap.parse_dump(DUMP)
        self.assertEqual(m["deviceName"], "Mali-G615 MC6")
        self.assertEqual(m["vendorID"], "0x13b5")
        self.assertEqual(m["deviceID"], "0xb8a31030")
        self.assertEqual(m["driverID"], "20")
        self.assertEqual(m["apiVersion"], "1.4.363")
        self.assertEqual(m["instanceExtensionCount"], 12)
        self.assertEqual(m["deviceExtensionCount"], 3)

    def test_exposed_vs_tested(self):
        m = cap.parse_dump(DUMP)
        f = m["features"]
        for k in ("upstreamImplemented", "runtimeExposed", "runtimeTested"):
            self.assertIn(k, f["geometryShader"])
        self.assertFalse(f["geometryShader"]["runtimeExposed"])
        self.assertFalse(f["geometryShader"]["runtimeTested"])
        self.assertTrue(f["wideLines"]["runtimeExposed"])
        self.assertFalse(f["wideLines"]["runtimeTested"])
        self.assertTrue(f["timelineSemaphore"]["runtimeExposed"])
        self.assertFalse(f["timelineSemaphore"]["runtimeTested"])
        self.assertTrue(f["descriptorIndexing"]["runtimeExposed"])
        self.assertFalse(f["descriptorIndexing"]["runtimeTested"])
        self.assertTrue(f["VK_KHR_swapchain"]["runtimeExposed"])
        self.assertTrue(f["VK_KHR_swapchain"]["runtimeTested"])
        self.assertTrue(f["VK_KHR_android_surface"]["runtimeExposed"])
        self.assertTrue(f["VK_KHR_android_surface"]["runtimeTested"])
        self.assertTrue(f["computeShader"]["runtimeTested"])
        self.assertFalse(f["textureCompressionBC"]["runtimeExposed"])
        self.assertFalse(f["textureCompressionBC"]["runtimeTested"])


class CollectValidation(unittest.TestCase):
    def test_pass_from_logs(self):
        with tempfile.TemporaryDirectory() as td:
            vdir = pathlib.Path(td)
            (vdir / "gateC-enumerate-x.txt").write_text("C-PASS\n")
            (vdir / "gateDE-compute-x.txt").write_text(
                "D-OK vkCreateDevice\nE-compute 10/10\nD+E-PASS\n"
            )
            (vdir / "gateF-triangle-x.txt").write_text("F-PASS\n")
            (vdir / "gateG-ahb-x.txt").write_text("G-PASS\n")
            (vdir / "beta2-vulkan-surface-x.txt").write_text("ANDROID_SURFACE=pass\n")
            (vdir / "beta2-present-300-x.txt").write_text("PRESENT_FRAMES=300/300\n")
            (vdir / "p16-glibc-x.txt").write_text(
                "C-PASS\nE-compute 10/10\nD+E-PASS\nF-PASS\n"
            )
            enum = col.evidence(
                vdir,
                "enumeration",
                ["gateC-enumerate*.txt"],
                col.PASS_TOKEN["enumeration"],
                "abc",
            )
            self.assertEqual(enum["status"], "pass")
            self.assertTrue(enum["evidenceFile"].endswith("gateC-enumerate-x.txt"))
            self.assertEqual(enum["binarySha256"], "abc")
            self.assertIn("timestamp", enum)
            cold = col.evidence(
                vdir,
                "coldSecondLaunch",
                ["beta2-cold-second-launch*.txt"],
                col.PASS_TOKEN["coldSecondLaunch"],
                None,
            )
            self.assertEqual(cold["status"], "untested")
            self.assertNotIn("evidenceFile", cold)

    def test_require_beta_gates_flags_untested(self):
        doc = {
            "android": {k: {"status": "pass"} for k in col.ANDROID_GATES},
            "glibc": {k: {"status": "pass"} for k in col.GLIBC_GATES},
        }
        self.assertEqual(col.require_beta_gates(doc), [])
        doc["android"]["coldSecondLaunch"] = {"status": "untested"}
        miss = col.require_beta_gates(doc)
        self.assertEqual(miss, ["android.coldSecondLaunch=untested"])

    def test_collect_real_profile_keys(self):
        doc = col.collect("g615-v11-csf")
        self.assertEqual(doc["profile"], "g615-v11-csf")
        self.assertEqual(doc["device"], "Poco X6 Pro")
        self.assertEqual(doc["gpuId"], "0xb8a31030")
        self.assertEqual(doc["kbaseUapi"], "1.21")
        self.assertTrue(str(doc["patchSeriesId"]).startswith("sha256:"))
        self.assertEqual(doc["android"]["enumeration"]["status"], "pass")
        self.assertEqual(doc["android"]["deviceCreate"]["status"], "pass")
        self.assertEqual(doc["android"]["compute10"]["status"], "pass")
        self.assertEqual(doc["android"]["offscreen"]["status"], "pass")
        self.assertEqual(doc["android"]["ahb"]["status"], "pass")
        self.assertEqual(doc["android"]["vulkanSurface"]["status"], "pass")
        self.assertEqual(doc["android"]["swapchain300"]["status"], "pass")
        self.assertEqual(doc["android"]["coldSecondLaunch"]["status"], "pass")
        self.assertEqual(doc["glibc"]["enumeration"]["status"], "pass")
        self.assertEqual(doc["glibc"]["compute10"]["status"], "pass")
        self.assertEqual(doc["glibc"]["offscreen"]["status"], "pass")
        self.assertIn("evidenceFile", doc["android"]["swapchain300"])
        self.assertIn("timestamp", doc["android"]["swapchain300"])
        self.assertIn("binarySha256", doc["android"]["swapchain300"])


if __name__ == "__main__":
    unittest.main()
