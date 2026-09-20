#!/usr/bin/env python3
import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
MESA = ROOT / "work" / "mesa"
PHYSICAL_DEVICE = (
    MESA / "src/panfrost/vulkan/panvk_vX_physical_device.c"
)
PANVK = MESA / "src/panfrost/vulkan"


class ClipCullSafeFalseTest(unittest.TestCase):
    def test_features_remain_disabled(self):
        source = PHYSICAL_DEVICE.read_text()
        self.assertRegex(source, r"\.shaderClipDistance\s*=\s*false,")
        self.assertRegex(source, r"\.shaderCullDistance\s*=\s*false,")
        self.assertNotRegex(source, r"\.shader(?:Clip|Cull)Distance\s*=\s*true,")

    def test_panvk_has_no_clip_cull_lowering(self):
        patterns = (
            re.compile(r"nir_lower_clip_fs\s*\("),
            re.compile(r"clip_distance_array_size"),
            re.compile(r"cull_distance_array_size"),
        )
        matches = []
        for path in PANVK.rglob("*.[ch]"):
            source = path.read_text(errors="replace")
            for pattern in patterns:
                if pattern.search(source):
                    matches.append(f"{path.relative_to(MESA)}: {pattern.pattern}")
        self.assertEqual(matches, [], "unexpected PanVK implementation:\n" + "\n".join(matches))


if __name__ == "__main__":
    unittest.main()
