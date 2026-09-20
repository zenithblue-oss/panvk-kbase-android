#!/usr/bin/env python3
import importlib.util
import json
import pathlib
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/evaluate-dxvk-d3d9.py"
JSON_OUT = ROOT / "validation/g615-v11-csf/p13-d3d9.json"
MD_OUT = ROOT / "validation/g615-v11-csf/P13-D3D9.md"
CAPS = ROOT / "validation/g615-v11-csf/consumer-capabilities.json"
FALSE_BITS = (
    "geometryShader",
    "fillModeNonSolid",
    "shaderClipDistance",
    "shaderCullDistance",
    "textureCompressionBC",
    "multiViewport",
)


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class P13D3D9ReevaluationTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run([sys.executable, str(SCRIPT)], check=True, cwd=ROOT)
        cls.doc = json.loads(JSON_OUT.read_text())
        cls.caps = json.loads(CAPS.read_text())

    def test_feature_bits_remain_false(self):
        for name in FALSE_BITS:
            self.assertIs(self.caps["features"][name], False, name)
            self.assertIs(self.doc["featureBitsUnchanged"][name], False, name)

    def test_all_d3d9_versions_fail_and_smoke_blocked(self):
        for version in ("1.10.3", "2.7.1", "3.1.1"):
            item = self.doc["versions"][version]
            self.assertEqual(item["status"], "FAIL", version)
            self.assertEqual(item["stockDxvkSmoke"], "BLOCKED", version)
            self.assertEqual(item["d3dFeatureLevel"], "NOT_AVAILABLE", version)
            self.assertGreater(item["failCount"], 0, version)
            self.assertTrue(item["blockers"], version)
            self.assertIn("illegal", item["smokeReason"])
        self.assertEqual(self.doc["result"], "FAIL")
        self.assertEqual(self.doc["smoke"], "BLOCKED")

    def test_required_row_fields(self):
        required = {
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
        rows = list(self.doc["versions"]["1.10.3"]["requirements"])
        for version in ("2.7.1", "3.1.1"):
            rows.extend(
                self.doc["versions"][version]["profiles"]["VP_DXVK_d3d9_baseline"]["requirements"]
            )
        self.assertTrue(rows)
        for row in rows:
            self.assertTrue(required.issubset(row), row.keys())

    def test_shared_blockers_remain_after_p9_p12(self):
        shared = {
            "geometryShader",
            "fillModeNonSolid",
            "shaderClipDistance",
            "shaderCullDistance",
            "textureCompressionBC",
        }
        self.assertEqual(set(self.doc["versions"]["1.10.3"]["blockers"]), shared | {"multiViewport"})
        self.assertEqual(set(self.doc["versions"]["2.7.1"]["blockers"]), shared)
        self.assertEqual(set(self.doc["versions"]["3.1.1"]["blockers"]), shared)

    def test_official_profiles_match_evaluator(self):
        evaluator = load_module(
            ROOT / "scripts/evaluate-consumer-profile.py", "evaluate_consumer_profile"
        )
        for version in ("2.7.1", "3.1.1"):
            imported = json.loads(
                (ROOT / f"validation/g615-v11-csf/profiles/dxvk-{version}.json").read_text()
            )
            result = evaluator.evaluate(self.caps, imported)
            for name in ("VP_DXVK_d3d9_baseline", "VP_DXVK_d3d9_optimal"):
                self.assertEqual(result["profiles"][name]["status"], "FAIL")
                self.assertEqual(
                    result["profiles"][name]["status"],
                    self.doc["versions"][version]["profiles"][name]["status"],
                )
                self.assertEqual(
                    len(result["profiles"][name]["fail"]),
                    self.doc["versions"][version]["profiles"][name]["failCount"],
                )

    def test_adapter_and_driver_recorded(self):
        driver = self.doc["driver"]
        self.assertEqual(driver["adapter"], "Mali-G615 MC6")
        self.assertEqual(driver["driverName"], "panvk")
        self.assertIn("Mesa 26.3.0-devel", driver["driverInfo"])
        self.assertEqual(driver["apiVersionText"], "1.4.363")
        self.assertEqual(driver["deviceIDHex"], "0xb8a31030")
        md = MD_OUT.read_text()
        self.assertIn("DXVK_1_10_3_D3D9=FAIL", md)
        self.assertIn("DXVK_2_7_1_D3D9=FAIL", md)
        self.assertIn("DXVK_3_1_1_D3D9=FAIL", md)
        self.assertIn("stockDxvkSmoke", json.dumps(self.doc))
        self.assertIn("BLOCKED", md)


if __name__ == "__main__":
    unittest.main()
