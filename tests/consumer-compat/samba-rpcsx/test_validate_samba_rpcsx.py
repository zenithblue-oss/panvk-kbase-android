import importlib.util
import json
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("validate_samba_rpcsx", ROOT / "scripts/validate-samba-rpcsx.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class SambaRpcsxValidationTests(unittest.TestCase):
    def test_packet_manifest_and_evidence(self):
        MODULE.validate(ROOT / "validation/g615-v11-csf/samba-rpcsx/source-manifest.json")

    def test_rejects_panvk_bc_inference(self):
        pattern = r"texture_compression_bc\s*=.*PANVK"
        self.assertIsNotNone(MODULE.re.search(pattern, "texture_compression_bc = feature || vendor == PANVK;"))

    def test_required_runtime_set_is_exact(self):
        evidence = json.loads((ROOT / "validation/g615-v11-csf/samba-rpcsx/runtime-evidence.json").read_text())
        self.assertEqual(set(evidence["results"]), MODULE.REQUIRED_RUNTIME)
        self.assertTrue(all(value["status"] in MODULE.ALLOWED for value in evidence["results"].values()))


if __name__ == "__main__":
    unittest.main()
