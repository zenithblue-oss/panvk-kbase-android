package dev.zenithblue.panvklauncher

import java.io.ByteArrayOutputStream
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import org.junit.Test
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull

class RenderApiTest {
    /** Smallest PE32+ with one section holding an import table for [dlls] (and [extra] bytes appended, e.g. engine markers). */
    private fun pe(dlls: List<String>, extra: ByteArray = ByteArray(0)): File {
        val b = ByteBuffer.allocate(0x400 + 0x1000).order(ByteOrder.LITTLE_ENDIAN)
        b.putShort(0, 0x5a4d); b.putInt(0x3c, 0x40)
        b.putInt(0x40, 0x4550); b.putShort(0x44, 0x8664.toShort()); b.putShort(0x46, 1); b.putShort(0x54, 0xF0.toShort())
        val opt = 0x58
        b.putShort(opt, 0x20b)
        b.putInt(opt + 112 + 8, 0x1000) // import directory RVA
        b.putInt(opt + 112 + 12, (dlls.size + 1) * 20)
        val sec = opt + 0xF0
        b.putInt(sec + 8, 0x1000); b.putInt(sec + 12, 0x1000); b.putInt(sec + 16, 0x1000); b.putInt(sec + 20, 0x400)
        var name = 0x400 + (dlls.size + 1) * 20
        dlls.forEachIndexed { i, d ->
            b.putInt(0x400 + i * 20 + 12, 0x1000 + (name - 0x400))
            d.toByteArray().forEachIndexed { k, c -> b.put(name + k, c) }
            name += d.length + 1
        }
        val out = ByteArrayOutputStream().apply { write(b.array()); write(extra) }
        return File.createTempFile("pe_", ".exe").apply { deleteOnExit(); writeBytes(out.toByteArray()) }
    }

    @Test fun readsImports() {
        val f = pe(listOf("KERNEL32.dll", "OPENGL32.dll"))
        assertEquals(setOf("kernel32.dll", "opengl32.dll"), PeInfo.imports(f.path))
        assertEquals("x86_64", PeInfo.arch(f.path))
    }

    @Test fun detectsApiFromImports() {
        assertEquals("opengl", RenderApi.detect(pe(listOf("OPENGL32.dll", "GDI32.dll")).path).api)
        assertEquals("d3d11", RenderApi.detect(pe(listOf("d3d11.dll", "dxgi.dll", "opengl32.dll")).path).api)
        assertEquals("d3d12", RenderApi.detect(pe(listOf("D3D12.dll", "d3d11.dll")).path).api)
        assertEquals("d3d9", RenderApi.detect(pe(listOf("d3d9.dll", "ddraw.dll")).path).api)
        assertEquals("ddraw", RenderApi.detect(pe(listOf("DDRAW.dll")).path).api)
        assertEquals("vulkan", RenderApi.detect(pe(listOf("vulkan-1.dll")).path).api)
        assertNull(RenderApi.detect(pe(listOf("KERNEL32.dll")).path).api)
    }

    @Test fun detectsGodotMajor() {
        fun marks(vararg s: String) = s.joinToString("\u0000").toByteArray()
        val g3 = RenderApi.detect(pe(listOf("OPENGL32.dll"), marks("Godot Engine", "  --video-driver <driver>")).path)
        assertEquals("Godot 3", g3.engine); assertEquals("OpenGL (Godot 3)", g3.text)
        val g4 = RenderApi.detect(pe(listOf("KERNEL32.dll"), marks("Godot Engine", "--rendering-driver")).path)
        assertEquals("Godot 4", g4.engine)
        assertEquals(listOf("--rendering-driver", "vulkan"), RenderApi.godotArgs(RenderApi.VULKAN, g4))
        assertEquals(emptyList<String>(), RenderApi.godotArgs(RenderApi.VULKAN, g3))
    }

    @Test fun overridesAppendOnly() {
        assertEquals("", RenderApi.overrides(RenderApi.AUTO))
        assertEquals(true, RenderApi.overrides(RenderApi.WINED3D).startsWith(";"))
        assertEquals(RenderApi.OPENGL, RenderApi.Detected("opengl", null).suggested)
        assertEquals(RenderApi.AUTO, RenderApi.norm("bogus"))
    }

    @Test fun usedFromLogs() {
        val dir = java.nio.file.Files.createTempDirectory("ra").toFile()
        val d11 = File(dir, "Game_d3d11.log").apply { writeText("x") }
        assertEquals("DXVK (D3D11)", RenderApi.used(listOf(d11, File(dir, "Game_dxgi.log")), "", false, false))
        assertEquals("OpenGL (Zink)", RenderApi.used(emptyList(), "trace:loaddll:build_module Loaded L\"C:\\\\windows\\\\system32\\\\opengl32.dll\"", true, false))
        assertEquals("cnc-ddraw", RenderApi.used(emptyList(), "", false, true))
    }
}
