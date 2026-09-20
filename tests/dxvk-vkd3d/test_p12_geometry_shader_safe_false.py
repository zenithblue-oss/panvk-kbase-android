#!/usr/bin/env python3
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
MESA = ROOT / "work" / "mesa"
PHYSICAL_DEVICE = MESA / "src/panfrost/vulkan/panvk_vX_physical_device.c"
SHADER = MESA / "src/panfrost/vulkan/panvk_vX_shader.c"
VALHALL_XML = MESA / "src/panfrost/genxml/v11.xml"
PANVK = MESA / "src/panfrost/vulkan"
COMPILER = MESA / "src/panfrost/compiler"
GALLIUM = MESA / "src/gallium/drivers/panfrost"


class GeometryShaderSafeFalseTest(unittest.TestCase):
    def test_features_remain_disabled(self):
        source = PHYSICAL_DEVICE.read_text()
        self.assertRegex(source, r"\.geometryShader\s*=\s*false,")
        self.assertRegex(
            source, r"\.shaderTessellationAndGeometryPointSize\s*=\s*false,"
        )
        self.assertNotRegex(source, r"\.geometryShader\s*=\s*true,")
        self.assertRegex(source, r"Geometry shaders not supported\.")
        self.assertRegex(source, r"\.maxGeometryShaderInvocations\s*=\s*0,")
        self.assertRegex(source, r"\.maxGeometryInputComponents\s*=\s*0,")
        self.assertRegex(source, r"\.maxGeometryOutputComponents\s*=\s*0,")
        self.assertRegex(source, r"\.maxGeometryOutputVertices\s*=\s*0,")
        self.assertRegex(source, r"\.maxGeometryTotalOutputComponents\s*=\s*0,")

    def test_panvk_compile_rejects_geometry_stage(self):
        source = SHADER.read_text()
        self.assertIn('UNREACHABLE("Unknown shader stage")', source)
        self.assertIn('assert(!"Unsupported stage")', source)
        self.assertNotIn("MESA_SHADER_GEOMETRY", source)

    def test_panvk_has_no_gs_lowering(self):
        patterns = (
            re.compile(r"poly_nir_lower_gs\s*\("),
            re.compile(r"nir_lower_gs_intrinsics\s*\("),
            re.compile(r"nir_create_passthrough_gs\s*\("),
            re.compile(r"MESA_SHADER_GEOMETRY"),
        )
        matches = []
        for path in PANVK.rglob("*.[ch]"):
            source = path.read_text(errors="replace")
            for pattern in patterns:
                if pattern.search(source):
                    matches.append(f"{path.relative_to(MESA)}: {pattern.pattern}")
        self.assertEqual(
            matches,
            [],
            "unexpected PanVK GS path; reassess P12:\n" + "\n".join(matches),
        )

    def test_compiler_has_no_geometry_stage(self):
        matches = []
        for path in COMPILER.rglob("*.[ch]"):
            source = path.read_text(errors="replace")
            if re.search(r"MESA_SHADER_GEOMETRY", source):
                matches.append(str(path.relative_to(MESA)))
        self.assertEqual(
            matches,
            [],
            "compiler now names MESA_SHADER_GEOMETRY; reassess P12:\n"
            + "\n".join(matches),
        )

    def test_valhall_v11_shader_stage_has_no_geometry(self):
        source = VALHALL_XML.read_text()
        match = re.search(
            r'<enum name="Shader stage">([\s\S]*?)</enum>', source
        )
        self.assertIsNotNone(match)
        block = match.group(1)
        self.assertIn('<value name="Compute" value="1"/>', block)
        self.assertIn('<value name="Fragment" value="2"/>', block)
        self.assertIn('<value name="Vertex" value="3"/>', block)
        self.assertNotIn("Geometry", block)

    def test_gallium_panfrost_skips_geometry_caps(self):
        source = (GALLIUM / "pan_screen.c").read_text()
        self.assertIn("case MESA_SHADER_VERTEX:", source)
        self.assertIn("case MESA_SHADER_FRAGMENT:", source)
        self.assertIn("case MESA_SHADER_COMPUTE:", source)
        self.assertNotIn("MESA_SHADER_GEOMETRY", source)

    def test_poly_gs_not_wired_into_panfrost(self):
        pattern = re.compile(r"poly_nir_lower_gs")
        matches = []
        for root in (PANVK, GALLIUM, COMPILER):
            for path in root.rglob("*.[ch]"):
                if pattern.search(path.read_text(errors="replace")):
                    matches.append(str(path.relative_to(MESA)))
        self.assertEqual(
            matches,
            [],
            "poly GS lowering now consumed by Panfrost; reassess P12:\n"
            + "\n".join(matches),
        )


if __name__ == "__main__":
    unittest.main()
