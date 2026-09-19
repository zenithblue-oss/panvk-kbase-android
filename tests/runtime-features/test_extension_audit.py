#!/usr/bin/env python3
"""Unit tests for scripts/audit-runtime-extensions.py.

Verifies beta.2 extension audit numbers and schema per WORKER.md Sections 4, 5, 6, 8, 35.
"""
from __future__ import annotations

import importlib.util
import json
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


audit = _load("audit_runtime_extensions", "audit-runtime-extensions.py")


class TestExtensionAuditBeta2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mesa_dir = ROOT / "work" / "mesa"
        if not cls.mesa_dir.exists():
            cls.mesa_dir = ROOT / "work" / "ref-g720-beta"
        cls.upstream_json = ROOT / "snapshots" / "matrix-2026-09-18-5a07217" / "upstream-feature-matrix.json"
        cls.runtime_json = ROOT / "validation" / "g615-v11-csf" / "beta2-runtime-features-2026-09-19.json"

        cls.result = audit.audit_extensions(
            mesa_dir=cls.mesa_dir,
            upstream_json_path=cls.upstream_json,
            runtime_json_path=cls.runtime_json,
            platform="android",
            arch="v11",
        )

    def test_beta2_counts_exact(self):
        s = self.result["summary"]
        self.assertEqual(s["upstreamPanvkDone"], 207, "Upstream PanVK DONE must be 207")
        self.assertEqual(s["runtimeExposed"], 178, "Runtime exposed total must be 178")
        self.assertEqual(s["runtimeInstance"], 12, "Runtime instance extensions must be 12")
        self.assertEqual(s["runtimeDevice"], 166, "Runtime device extensions must be 166")
        self.assertEqual(s["runtimeOverlap"], 177, "Runtime overlap with matrix must be 177")
        self.assertEqual(s["runtimeExtra"], 1, "Runtime extra must be 1 (VK_KHR_android_surface)")
        self.assertEqual(s["missingCount"], 30, "Missing extensions gap must be exactly 30")
        self.assertEqual(s["platformInapplicable"], 10, "Desktop/display excludes must be exactly 10")
        self.assertEqual(s["auditCandidatesCount"], 20, "Audit candidates pool must be exactly 20")

    def test_candidates_breakdown(self):
        cb = self.result["summary"]["candidatesBreakdown"]
        self.assertEqual(cb["highConfidence"], 17, "High-confidence candidates must be 17")
        self.assertEqual(cb["investigate"], 3, "Investigate candidates must be 3")
        self.assertEqual(cb["requiresTests"], 13, "Section 25 explicit workload tests must be 13")

    def test_section_8_schema_and_keys(self):
        # Must support root-level access per WORKER.md Section 8 example
        self.assertIn("VK_KHR_wayland_surface", self.result)
        wayland = self.result["VK_KHR_wayland_surface"]
        self.assertTrue(wayland["matrix"])
        self.assertFalse(wayland["runtime"])
        self.assertEqual(wayland["classification"], "platform-inapplicable")
        self.assertIn("Android/Bionic", wayland["reason"])

        self.assertIn("VK_KHR_compute_shader_derivatives", self.result)
        csd = self.result["VK_KHR_compute_shader_derivatives"]
        self.assertTrue(csd["matrix"])
        self.assertFalse(csd["runtime"])
        self.assertEqual(csd["classification"], "source-supported-not-exposed")
        self.assertEqual(csd["archRequirement"], "PAN_ARCH >= 9")

    def test_schema_classifications_present(self):
        classifications = self.result["summary"]["classifications"]
        # Required classifications from Section 8 schema:
        # runtime, platform-inapplicable, source-supported-not-exposed,
        # source-not-present, requires-wsi, requires-runtime-test
        self.assertGreater(classifications.get("runtime", 0), 0)
        self.assertGreater(classifications.get("platform-inapplicable", 0), 0)
        self.assertGreater(classifications.get("source-supported-not-exposed", 0), 0)
        self.assertGreater(classifications.get("source-not-present", 0), 0)
        self.assertGreater(classifications.get("requires-wsi", 0), 0)
        self.assertGreater(classifications.get("requires-runtime-test", 0), 0)

    def test_markdown_generation(self):
        md = audit.generate_markdown(self.result)
        self.assertIn("# PanVK Extension Gap Analysis & Runtime Audit", md)
        self.assertIn("Upstream PanVK Matrix DONE | 207", md)
        self.assertIn("Runtime Exposed (Total) | 178", md)
        self.assertIn("Missing at Runtime (Total Gap) | 30", md)
        self.assertIn("High-Confidence Candidates (17)", md)
        self.assertIn("Investigate Candidates (3)", md)
        self.assertIn("Workload Tests Required (13)", md)

    def test_platform_sensitivity(self):
        # On glibc, desktop extensions are not platform-inapplicable
        res_glibc = audit.audit_extensions(
            mesa_dir=self.mesa_dir,
            upstream_json_path=self.upstream_json,
            runtime_json_path=self.runtime_json,
            platform="glibc",
            arch="v11",
        )
        self.assertEqual(res_glibc["summary"]["platformInapplicable"], 0)
        wayland = res_glibc["gap"]["VK_KHR_wayland_surface"]
        self.assertEqual(wayland["classification"], "source-supported-not-exposed")


if __name__ == "__main__":
    unittest.main()
