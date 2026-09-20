#!/usr/bin/env python3
import importlib.util
import json
import pathlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/evaluate-vkd3d-profile-baseline.py"
JSON_OUT = ROOT / "validation/g615-v11-csf/p19-vkd3d-profile-baseline.json"
MD_OUT = ROOT / "validation/g615-v11-csf/P19-VKD3D-PROFILE-BASELINE.md"
CAPS = ROOT / "validation/g615-v11-csf/consumer-capabilities.json"
FALSE_BITS = (
    "geometryShader",
    "fillModeNonSolid",
    "shaderClipDistance",
    "shaderCullDistance",
    "textureCompressionBC",
    "multiViewport",
    "transformFeedback",
    "geometryStreams",
    "tessellationShader",
    "pipelineStatisticsQuery",
    "robustImageAccess2",
)
TRUE_HARD = (
    "robustBufferAccess2",
    "nullDescriptor",
    "samplerMirrorClampToEdge",
    "shaderDrawParameters",
    "descriptorIndexing",
)
BASELINE_BLOCKERS = {
    "VK_EXT_transform_feedback",
    "robustImageAccess2",
    "transformFeedback",
    "geometryStreams",
    "geometryShader",
    "tessellationShader",
    "fillModeNonSolid",
    "multiViewport",
    "textureCompressionBC",
    "pipelineStatisticsQuery",
    "shaderClipDistance",
    "shaderCullDistance",
    "transformFeedbackQueries",
    "bufferImageGranularity",
    "subgroupSupportedOperations[0]",
}
ROW_FIELDS = {
    "requirement",
    "current",
    "required",
    "status",
    "sourceVersion",
    "source",
    "upstreamPanVK",
    "implementationStatus",
    "action",
}


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class P19Vkd3dProfileBaselineTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run([sys.executable, str(SCRIPT)], check=True, cwd=ROOT)
        cls.doc = json.loads(JSON_OUT.read_text())
        cls.caps = json.loads(CAPS.read_text())

    def test_feature_bits_remain_false(self):
        for name in FALSE_BITS:
            self.assertIs(self.caps["features"][name], False, name)
            self.assertIs(self.doc["featureBitsUnchanged"][name], False, name)

    def test_hard_bits_already_true(self):
        for name in TRUE_HARD:
            self.assertIs(self.caps["features"][name], True, name)
            self.assertIs(self.doc["featureBitsUnchanged"][name], True, name)

    def test_gates_stay_separate(self):
        self.assertEqual(
            self.doc["gatesUnmerged"],
            ["HARD_REQUIREMENTS", "PROFILE_BASELINE", "DEVICE_CREATE", "FEATURE_LEVEL"],
        )
        self.assertEqual(self.doc["result"], "FAIL")
        self.assertEqual(self.doc["smoke"], "BLOCKED")
        self.assertEqual(self.doc["d3dFeatureLevel"], "NOT_AVAILABLE")
        dumped = json.dumps(self.doc)
        self.assertNotIn('"HARD_REQUIREMENTS": "PASS", "PROFILE_BASELINE": "PASS"', dumped)

    def test_versions_split_gates(self):
        for version in ("2.14.1", "3.0.1"):
            item = self.doc["versions"][version]
            self.assertEqual(item["HARD_REQUIREMENTS"]["status"], "PASS", version)
            self.assertEqual(item["HARD_REQUIREMENTS"]["failCount"], 0, version)
            self.assertEqual(item["PROFILE_BASELINE"]["status"], "FAIL", version)
            self.assertEqual(item["DEVICE_CREATE"]["status"], "FAIL", version)
            self.assertEqual(item["FEATURE_LEVEL"]["status"], "NOT_AVAILABLE", version)
            self.assertEqual(item["FEATURE_LEVEL"]["d3dFeatureLevel"], "NOT_AVAILABLE", version)
            self.assertEqual(item["stockVkd3dSmoke"], "BLOCKED", version)
            self.assertIn("E_INVALIDARG", item["smokeReason"])
            self.assertIn("not a legal host", item["smokeReason"])
            self.assertIn("robustImageAccess2", item["DEVICE_CREATE"]["blockers"], version)
            self.assertEqual(item["DEVICE_CREATE"]["blockers"], ["robustImageAccess2"], version)

    def test_required_row_fields(self):
        rows = []
        for version in ("2.14.1", "3.0.1"):
            item = self.doc["versions"][version]
            rows.extend(item["HARD_REQUIREMENTS"]["requirements"])
            rows.extend(item["PROFILE_BASELINE"]["requirements"])
            rows.extend(item["DEVICE_CREATE"]["requirements"])
        self.assertTrue(rows)
        for row in rows:
            self.assertTrue(ROW_FIELDS.issubset(row), row.keys())

    def test_baseline_blockers_match_evaluator(self):
        evaluator = load_module(
            ROOT / "scripts/evaluate-consumer-profile.py", "evaluate_consumer_profile"
        )
        for version in ("2.14.1", "3.0.1"):
            imported = json.loads(
                (ROOT / f"validation/g615-v11-csf/profiles/vkd3d-{version}.json").read_text()
            )
            result = evaluator.evaluate(self.caps, imported)
            name = "VP_D3D12_FL_11_0_baseline"
            evaluated = result["profiles"][name]
            self.assertEqual(evaluated["status"], "FAIL")
            self.assertEqual(
                evaluated["status"],
                self.doc["versions"][version]["PROFILE_BASELINE"]["status"],
            )
            self.assertEqual(
                len(evaluated["fail"]),
                self.doc["versions"][version]["PROFILE_BASELINE"]["failCount"],
            )
            fail_leaves = {item["path"].rsplit(".", 1)[-1] for item in evaluated["fail"]}
            self.assertEqual(fail_leaves, BASELINE_BLOCKERS)
            self.assertEqual(
                set(self.doc["versions"][version]["PROFILE_BASELINE"]["blockers"]),
                BASELINE_BLOCKERS,
            )

    def test_hard_does_not_require_robust_image_access2(self):
        for version in ("2.14.1", "3.0.1"):
            names = {
                row["requirement"]
                for row in self.doc["versions"][version]["HARD_REQUIREMENTS"]["requirements"]
            }
            self.assertNotIn("robustImageAccess2", names)
            self.assertIn("VK_EXT_robustness2", names)
            self.assertIn("VK_KHR_push_descriptor", names)
            recommended = {
                row["requirement"]
                for row in self.doc["versions"][version]["HARD_REQUIREMENTS"]["recommended"]
            }
            self.assertIn("VK_EXT_descriptor_buffer", recommended)
            descriptor_buffer = next(
                row
                for row in self.doc["versions"][version]["HARD_REQUIREMENTS"]["recommended"]
                if row["requirement"] == "VK_EXT_descriptor_buffer"
            )
            self.assertEqual(descriptor_buffer["status"], "FAIL")

    def test_3_0_1_baseline_includes_unused_attachments(self):
        rows = self.doc["versions"]["3.0.1"]["PROFILE_BASELINE"]["requirements"]
        names = {row["requirement"] for row in rows}
        self.assertIn("VK_EXT_dynamic_rendering_unused_attachments", names)
        unused = next(
            row
            for row in rows
            if row["requirement"] == "dynamicRenderingUnusedAttachments"
            or row["requirement"] == "VK_EXT_dynamic_rendering_unused_attachments"
        )
        self.assertEqual(unused["status"], "PASS")

    def test_higher_vendor_profiles_not_merged_into_baseline(self):
        dumped = json.dumps(self.doc["versions"]["2.14.1"]["PROFILE_BASELINE"])
        self.assertNotIn("VP_D3D12_maximum_radv", dumped)
        self.assertNotIn("VP_D3D12_FL_12_0_baseline", dumped)
        self.assertIn(
            "VP_D3D12_FL_12_0_baseline",
            self.doc["versions"]["2.14.1"]["FEATURE_LEVEL"]["profiles"],
        )
        for name in (
            "VP_D3D12_FL_11_0_baseline",
            "VP_D3D12_FL_11_1_baseline",
            "VP_D3D12_FL_12_0_baseline",
            "VP_D3D12_FL_12_1_baseline",
            "VP_D3D12_FL_12_2_baseline",
        ):
            self.assertEqual(
                self.doc["versions"]["2.14.1"]["FEATURE_LEVEL"]["profiles"][name]["status"],
                "FAIL",
            )

    def test_adapter_and_report(self):
        driver = self.doc["driver"]
        self.assertEqual(driver["adapter"], "Mali-G615 MC6")
        self.assertEqual(driver["driverName"], "panvk")
        self.assertIn("Mesa 26.3.0-devel", driver["driverInfo"])
        self.assertEqual(driver["apiVersionText"], "1.4.363")
        self.assertEqual(driver["deviceIDHex"], "0xb8a31030")
        md = MD_OUT.read_text()
        self.assertIn("VKD3D_2_14_1_HARD=PASS", md)
        self.assertIn("VKD3D_2_14_1_BASELINE=FAIL", md)
        self.assertIn("VKD3D_2_14_1_DEVICE=FAIL", md)
        self.assertIn("VKD3D_2_14_1_FEATURE_LEVEL=NOT_AVAILABLE", md)
        self.assertIn("VKD3D_3_0_1_HARD=PASS", md)
        self.assertIn("VKD3D_3_0_1_BASELINE=FAIL", md)
        self.assertIn("VKD3D_3_0_1_DEVICE=FAIL", md)
        self.assertIn("BLOCKED", md)
        self.assertIn("P19 exits `FAIL`", md)
        self.assertIn("These four gates stay separate", md)


if __name__ == "__main__":
    unittest.main()
