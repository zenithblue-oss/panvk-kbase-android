package dev.zenithblue.panvklauncher

import java.io.File
import java.io.RandomAccessFile

/**
 * Per-game "Render API" ([Shortcut.renderApi]): which graphics stack Wine routes the game through.
 *  - auto:    today's behaviour (DXVK for D3D8-11, vkd3d-proton for D3D12, cnc-ddraw for DirectDraw-only games); a game that
 *             imports opengl32 additionally gets the OpenGL stack (nothing else would give it a GL).
 *  - dxvk / vkd3d / wined3d / cnc-ddraw: DLL overrides appended to the defaults (never replacing nsiproxy.sys=d).
 *  - opengl:  Mesa EGL + Zink on PanVK through winex11's EGL backend (component "Mesa-Zink", scripts/build-mesa-zink.sh).
 *  - vulkan:  native Vulkan (winevulkan -> PanVK); no D3D translation is involved, only Godot 4 style engines get a switch.
 */
object RenderApi {
    const val AUTO = "auto"
    const val DXVK = "dxvk"
    const val VKD3D = "vkd3d"
    const val WINED3D = "wined3d"
    const val OPENGL = "opengl"
    const val VULKAN = "vulkan"
    const val CNC = "cnc-ddraw"
    const val MESA_TYPE = "Mesa-Zink"

    val options = listOf(
        AUTO to "Auto (recommended)",
        DXVK to "DXVK (D3D8-11 on Vulkan)",
        VKD3D to "VKD3D (D3D12 on Vulkan)",
        WINED3D to "WineD3D (D3D on OpenGL)",
        OPENGL to "OpenGL (Zink on Vulkan)",
        VULKAN to "Vulkan (native)",
        CNC to "cnc-ddraw (DirectDraw)"
    )

    fun norm(s: String) = if (options.any { it.first == s }) s else AUTO
    fun label(s: String) = options.firstOrNull { it.first == s }?.second ?: s

    /** What the exe talks to (from its imports) plus an optional engine hint. [api] is one of the ids below. */
    data class Detected(val api: String?, val engine: String?) {
        /** Option to pick for [api], or AUTO when unknown. */
        val suggested: String get() = when (api) {
            "d3d12" -> VKD3D
            "d3d11", "d3d10", "d3d9", "d3d8" -> DXVK
            "ddraw" -> CNC
            "opengl" -> OPENGL
            "vulkan" -> VULKAN
            else -> AUTO
        }
        val apiLabel: String? get() = when (api) {
            "d3d12" -> "Direct3D 12"; "d3d11" -> "Direct3D 11"; "d3d10" -> "Direct3D 10"; "d3d9" -> "Direct3D 9"
            "d3d8" -> "Direct3D 8"; "ddraw" -> "DirectDraw"; "opengl" -> "OpenGL"; "vulkan" -> "Vulkan"; else -> null
        }
        val text: String get() = when {
            apiLabel != null && engine != null -> "$apiLabel ($engine)"
            apiLabel != null -> apiLabel!!
            engine != null -> "unknown ($engine)"
            else -> "unknown (loaded at run time)"
        }
    }

    /** Newest API first: a game importing d3d11 and opengl32 is a D3D11 game that keeps a GL fallback. Input: lower-case DLL names. */
    fun fromImports(imports: Set<String>): String? {
        fun has(vararg p: String) = imports.any { i -> p.any { i == it || i.startsWith(it) } }
        return when {
            has("d3d12") -> "d3d12"
            has("d3d11", "d3d10", "dxgi") -> if (has("d3d10")) "d3d10" else "d3d11"
            has("d3d9") -> "d3d9"
            has("d3d8") -> "d3d8"
            has("vulkan-1") -> "vulkan"
            has("ddraw") -> "ddraw"
            has("opengl32") -> "opengl"
            else -> null
        }
    }

    /** Scan [exe] (and its folder) for engine hints; Godot's major version tells which --video/--rendering-driver exists. */
    fun engine(exe: File): String? {
        val dir = exe.parentFile
        val names = dir?.list()?.map { it.lowercase() } ?: emptyList()
        if ("unityplayer.dll" in names || "${exe.nameWithoutExtension.lowercase()}_data" in names) return "Unity"
        if (exe.path.contains("/Binaries/Win", ignoreCase = true)) return "Unreal"
        return try { godot(exe) } catch (_: Throwable) { null }
    }

    /** "Godot 3" / "Godot 4" from marker strings in the exe, else null. Streams the file; the markers are in .rdata. */
    fun godot(exe: File): String? {
        if (exe.length() > 600_000_000L) return null
        val godot = "Godot Engine".toByteArray(); val v3 = "--video-driver".toByteArray(); val v4 = "--rendering-driver".toByteArray()
        var isGodot = false; var has3 = false; var has4 = false
        val buf = ByteArray(1 shl 20); val keep = 32
        RandomAccessFile(exe, "r").use { f ->
            var carry = 0
            while (true) {
                val n = f.read(buf, carry, buf.size - carry)
                if (n <= 0) break
                val len = carry + n
                if (indexOf(buf, len, godot) >= 0) isGodot = true
                if (indexOf(buf, len, v3) >= 0) has3 = true
                if (indexOf(buf, len, v4) >= 0) has4 = true
                if (isGodot && (has3 || has4)) break
                carry = minOf(keep, len); System.arraycopy(buf, len - carry, buf, 0, carry)
            }
        }
        return if (!isGodot) null else if (has4) "Godot 4" else if (has3) "Godot 3" else "Godot"
    }

    private fun indexOf(b: ByteArray, len: Int, p: ByteArray): Int {
        outer@ for (i in 0..len - p.size) {
            for (j in p.indices) if (b[i + j] != p[j]) continue@outer
            return i
        }
        return -1
    }

    fun detect(exePath: String): Detected {
        val f = File(exePath)
        if (!f.isFile) return Detected(null, null)
        val eng = engine(f)
        val api = fromImports(PeInfo.imports(exePath))
        // Unity / Unreal pick the graphics DLL at run time; both default to D3D11 on Windows.
        return Detected(api ?: if (eng == "Unity" || eng == "Unreal") "d3d11" else null, eng)
    }

    /** True when the game needs the GL stack and has no other way to render. */
    fun wantsGl(api: String, det: Detected) = api == OPENGL || api == WINED3D || (api == AUTO && det.api == "opengl")

    /** DLL overrides appended after the defaults (leading ';'). Wine's later entry for the same DLL wins. */
    fun overrides(api: String): String = when (api) {
        DXVK -> ";d3d8,d3d9,d3d10core,d3d11,dxgi=n,b"
        VKD3D -> ";d3d12,d3d12core=n,b"
        // Builtin wined3d (OpenGL) for every D3D version; dxgi goes with it.
        WINED3D -> ";d3d8,d3d9,d3d10core,d3d11,dxgi,d3d12,d3d12core=b"
        else -> ""
    }

    /** Godot 4 selects the backend with --rendering-driver; Godot 3 only has GLES2/GLES3 (via GL) and needs no switch. */
    fun godotArgs(api: String, det: Detected): List<String> {
        if (det.engine != "Godot 4") return emptyList()
        return when (api) {
            VULKAN -> listOf("--rendering-driver", "vulkan")
            OPENGL -> listOf("--rendering-driver", "opengl3")
            VKD3D -> listOf("--rendering-driver", "d3d12")
            else -> emptyList()
        }
    }

    /** Why [api] cannot work for the detected game, or null. */
    fun warning(api: String, det: Detected, mesaInstalled: Boolean, vkd3dInstalled: Boolean): String? = when {
        wantsGl(api, det) && !mesaInstalled -> "OpenGL needs the Mesa-Zink component (Components)."
        api == VKD3D && !vkd3dInstalled -> "vkd3d-proton is not installed (Components)."
        api == VULKAN && det.engine == "Godot 3" -> "Godot 3 has no Vulkan renderer: choose OpenGL."
        api == VULKAN && det.api != null && det.api != "vulkan" && det.engine != "Godot 4" -> "This game does not use Vulkan itself."
        api == OPENGL && det.api != null && det.api != "opengl" && det.engine != "Godot 4" -> "This game does not use OpenGL."
        else -> null
    }

    /**
     * The API a finished run actually used, from what it left behind: DXVK per-DLL logs (d3d8/9/10core/11 are written when
     * the device is created), vkd3d.log, the run log's +loaddll lines (opengl32 / wined3d, only traced when the GL stack was on)
     * and whether cnc-ddraw was swapped in.
     */
    fun used(gfxLogs: List<File>, runLog: String, glActive: Boolean, cncSwapped: Boolean): String {
        val parts = mutableListOf<String>()
        val dx = gfxLogs.mapNotNull { Regex("_(d3d(?:8|9|10core|11))\\.log$").find(it.name)?.groupValues?.get(1) }.distinct()
            .map { it.removeSuffix("core").uppercase() }
        if (dx.isNotEmpty()) parts += "DXVK (${dx.joinToString("/")})"
        if (gfxLogs.any { it.name == "vkd3d.log" && it.length() > 0 }) parts += "VKD3D (D3D12)"
        if (glActive) {
            val gl = Regex("(?i)loaddll:.*opengl32\\.dll").containsMatchIn(runLog)
            val wd = Regex("(?i)loaddll:.*wined3d\\.dll").containsMatchIn(runLog)
            val err = Regex("(?i):err:wgl:.*(OpenGL support is disabled|Failed to load)").containsMatchIn(runLog)
            if (gl && !err) parts += if (wd) "WineD3D (OpenGL via Zink)" else "OpenGL (Zink)"
            else if (err) parts += "OpenGL failed (no GL driver)"
        }
        if (cncSwapped) parts += "cnc-ddraw"
        return parts.joinToString(" + ")
    }
}
