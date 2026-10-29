package dev.zenithblue.panvklauncher

import android.content.Context
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.selection.toggleable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.rounded.ArrowBack
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import org.json.JSONObject
import java.io.File

/**
 * MangoHud (component "MangoHud", scripts/build-mangohud.sh): on/off plus every MangoHud.conf option, global and per game.
 * Saved in prefs "mangohud" (key "global", "game_<shortcut id>" = per-game override). At launch [env] writes the
 * MangoHud.conf and an implicit layer manifest pointing at the component's libMangoHud.so, and returns the env that
 * the imagefs Vulkan loader (libvulkan.so.1 in Wine's unix side) needs to load it.
 */
object MangoHudSettings {
    const val TYPE = "MangoHud"
    private const val LAYER = "VK_LAYER_MANGOHUD_overlay_panplay"
    /** imagefs ships this manifest without its library; disabled so the loader does not try to load it. */
    private const val STOCK_LAYER = "VK_LAYER_MANGOHUD_overlay_aarch64"

    /** kind: b = switch, c = choice, n = number, t = text, k = colour (RRGGBB). */
    class Opt(val key: String, val label: String, val kind: Char = 'b', val def: String = "", val hint: String = "", val choices: List<String> = emptyList())

    private fun b(k: String, l: String, on: Boolean = false) = Opt(k, l, 'b', if (on) "1" else "0")
    private fun n(k: String, l: String, hint: String = "", def: String = "") = Opt(k, l, 'n', def, hint)
    private fun t(k: String, l: String, hint: String = "") = Opt(k, l, 't', "", hint)
    private fun k(k: String, l: String, hint: String) = Opt(k, l, 'k', "", hint)
    private fun c(k: String, l: String, def: String, vararg ch: String) = Opt(k, l, 'c', def, "", ch.toList())

    // Defaults follow MangoHud's own, except position: top-right, so it does not cover the DXVK HUD (top-left).
    val GROUPS: List<Pair<String, List<Opt>>> = listOf(
        "Layout and position" to listOf(
            c("position", "Position", "top-right", "top-left", "top-center", "top-right", "middle-left", "middle-right", "bottom-left", "bottom-center", "bottom-right"),
            n("offset_x", "Offset X", "0"), n("offset_y", "Offset Y", "0"),
            n("width", "Width (0 = auto)", "0"), n("height", "Height", "140"),
            n("table_columns", "Table columns", "3"), n("cellpadding_y", "Cell padding Y", "-0.085"),
            n("round_corners", "Round corners", "0"),
            t("preset", "Preset (-1 custom, 0-4)", "-1"),
            b("full", "Show everything (full)"), b("fps_only", "FPS only"), b("legacy_layout", "Legacy layout", true),
            b("hud_no_margin", "No margin"), b("hud_compact", "Compact"), b("horizontal", "Horizontal"),
            b("horizontal_stretch", "Horizontal stretch", true), b("no_display", "Start hidden (no_display)")
        ),
        "FPS" to listOf(
            b("fps", "FPS", true), b("frame_count", "Frame count"), b("show_fps_limit", "Show FPS limit"),
            n("fps_sampling_period", "Sampling period (ms)", "500"),
            b("fps_color_change", "Colour by FPS"), t("fps_value", "FPS thresholds", "30,60"), t("fps_color", "FPS colours", "B22222,FDFD09,39F900"),
            t("fps_text", "FPS label"), t("fps_metrics", "FPS metrics", "avg,0.01"),
            t("fps_limit", "FPS limit (0 = off, list allowed)", "0"), c("fps_limit_method", "FPS limit method", "", "", "late", "early"),
            c("vsync", "Vulkan vsync (-1 default, 0 adaptive, 1 off, 2 mailbox, 3 on)", "", "", "-1", "0", "1", "2", "3"),
            c("vulkan_present_mode", "Vulkan present mode", "", "", "immediate", "mailbox", "fifo", "fifo_relaxed")
        ),
        "Frame time graph" to listOf(
            b("frametime", "Frame time value", true), b("frame_timing", "Frame time graph", true), b("frame_timing_detailed", "Detailed frame timing"),
            b("dynamic_frame_timing", "Dynamic graph range"), b("histogram", "Histogram instead of line graph"),
            t("graphs", "Extra graphs", "gpu_load,cpu_load,ram,vram"), b("throttling_status", "Throttling status"),
            b("throttling_status_graph", "Throttling in graph")
        ),
        "GPU" to listOf(
            b("gpu_stats", "GPU load", true), b("gpu_name", "GPU name"), b("vulkan_driver", "Vulkan driver"), b("gpu_temp", "GPU temperature"),
            b("gpu_junction_temp", "Junction temperature"), b("gpu_mem_temp", "VRAM temperature"), b("gpu_core_clock", "Core clock"),
            b("gpu_mem_clock", "Memory clock"), b("gpu_power", "Power"), b("gpu_power_limit", "Power limit"), b("gpu_fan", "Fan"),
            b("gpu_voltage", "Voltage"), b("gpu_efficiency", "Efficiency"), b("vram", "VRAM"), b("proc_vram", "Process VRAM"),
            t("gpu_text", "GPU label"), b("gpu_load_change", "Colour by load"), t("gpu_load_value", "Load thresholds", "60,90"),
            t("gpu_load_color", "Load colours", "39F900,FDFD09,B22222"), t("gpu_list", "GPU list", "0,1"), t("pci_dev", "PCI device", "0000:03:00.0")
        ),
        "CPU" to listOf(
            b("cpu_stats", "CPU load", true), b("cpu_temp", "CPU temperature"), b("cpu_power", "CPU power"), b("cpu_mhz", "CPU MHz"),
            b("cpu_efficiency", "Efficiency"), b("core_load", "Per-core load"), b("core_load_change", "Colour per-core load"),
            b("core_bars", "Per-core bars"), b("core_type", "Core type"), t("cpu_text", "CPU label"), b("cpu_load_change", "Colour by load"),
            t("cpu_load_value", "Load thresholds", "60,90"), t("cpu_load_color", "Load colours", "39F900,FDFD09,B22222")
        ),
        "Memory and I/O" to listOf(
            b("ram", "RAM"), b("swap", "Swap"), b("procmem", "Process memory"), b("procmem_shared", "Process shared memory"),
            b("procmem_virt", "Process virtual memory"), b("io_read", "Disk read"), b("io_write", "Disk write")
        ),
        "Info" to listOf(
            b("time", "Clock"), b("time_no_label", "Clock without label"), t("time_format", "Clock format", "%T"), b("version", "MangoHud version"),
            b("engine_version", "Engine version (DXVK/vkd3d)"), b("engine_short_names", "Short engine names"), b("wine", "Wine version"),
            b("exec_name", "Executable name"), b("arch", "Architecture"), b("present_mode", "Present mode"), b("resolution", "Resolution"),
            b("refresh_rate", "Refresh rate"), b("display_server", "Display server"), b("winesync", "Wine sync method"), b("hdr", "HDR"),
            b("fsr", "FSR"), b("hide_fsr_sharpness", "Hide FSR sharpness"), b("vkbasalt", "vkBasalt"), b("gamemode", "GameMode"),
            b("fex_stats", "FEX stats"), b("debug", "Debug info"), b("temp_fahrenheit", "Fahrenheit"), b("flip_efficiency", "Flip efficiency (J/frame)"),
            t("custom_text_center", "Centered text"), t("custom_text", "Custom text"), t("exec", "Command output")
        ),
        "Battery, network, media" to listOf(
            b("battery", "Battery"), b("battery_icon", "Battery icon"), b("battery_watt", "Battery watts"), b("battery_time", "Battery time"),
            t("device_battery", "Device battery", "gamepad,mouse"), b("device_battery_icon", "Device battery icon"),
            t("network", "Network interfaces", "wlan0"), b("media_player", "Media player"), t("media_player_name", "Media player name"),
            t("media_player_format", "Media player format", "title,artist,album")
        ),
        "Font" to listOf(
            n("font_size", "Font size", "24"), n("font_scale", "Font scale", "1.0"), n("font_size_text", "Text font size", "24"),
            n("font_scale_media_player", "Media player font scale", "0.55"), b("no_small_font", "No small font"),
            t("font_file", "Font file (device path)"), t("font_file_text", "Text font file"), t("font_glyph_ranges", "Glyph ranges", "korean,chinese,japanese,cyrillic"),
            b("text_outline", "Text outline", true), k("text_outline_color", "Outline colour", "000000"), n("text_outline_thickness", "Outline thickness", "1.5")
        ),
        "Colours" to listOf(
            n("background_alpha", "Background alpha", "0.5"), n("alpha", "HUD alpha", "1.0"),
            k("background_color", "Background", "020202"), k("text_color", "Text", "FFFFFF"), k("gpu_color", "GPU", "2E9762"),
            k("cpu_color", "CPU", "2E97CB"), k("vram_color", "VRAM", "AD64C1"), k("ram_color", "RAM", "C26693"), k("engine_color", "Engine", "EB5B5B"),
            k("io_color", "I/O", "A491D3"), k("frametime_color", "Frame time", "00FF00"), k("wine_color", "Wine", "EB5B5B"),
            k("battery_color", "Battery", "FF9078"), k("network_color", "Network", "E07B85"), k("media_player_color", "Media player", "FFFFFF"),
            k("horizontal_separator_color", "Horizontal separator", "AD64C1")
        ),
        "Texture filtering (Vulkan)" to listOf(
            n("picmip", "Mip LOD bias (-16..16)", "0"), n("af", "Anisotropic filtering (0-16)", "0"),
            b("bicubic", "Bicubic"), b("trilinear", "Trilinear"), b("retro", "Retro (nearest)")
        ),
        "Keybinds (USB keyboard)" to listOf(
            t("toggle_hud", "Toggle HUD", "Shift_R+F12"), t("toggle_hud_position", "Toggle position", "Shift_R+F11"),
            t("toggle_preset", "Next preset", "Shift_R+F10"), t("toggle_fps_limit", "Toggle FPS limit", "Shift_L+F1"),
            t("toggle_logging", "Toggle logging", "Shift_L+F2"), t("reload_cfg", "Reload config", "Shift_L+F4"),
            t("reset_fps_metrics", "Reset FPS metrics", "Shift_R+F9"), t("upload_log", "Upload log", "Shift_L+F3")
        ),
        "Logging and FCAT" to listOf(
            n("autostart_log", "Start logging after (s)"), n("log_duration", "Log duration (s)"), n("log_interval", "Log interval (ms)", "0"),
            t("output_folder", "Log folder (device path)"), b("log_versioning", "Log versioning"), n("permit_upload", "Permit upload (0/1)", "0"),
            b("fcat", "FCAT overlay"), n("fcat_overlay_width", "FCAT width", "24"), n("fcat_screen_edge", "FCAT screen edge", "0"),
            t("blacklist", "Process blacklist", "pamac-manager,lutris")
        )
    )
    private val ALL = GROUPS.flatMap { it.second }

    // Options with no data source inside an Android app (tested on Mali-G615 kbase, Android 14): shown greyed out.
    // The MangoHud build also forces the switches among them off (runtimes/mangohud/android-bionic.patch).
    private const val NO_GPU = "Not available on this device: Android blocks the Mali GPU load, clock and sensor files for apps"
    val NOT_AVAILABLE: Map<String, String> = buildMap {
        listOf("gpu_stats", "gpu_temp", "gpu_junction_temp", "gpu_mem_temp", "gpu_core_clock", "gpu_mem_clock", "gpu_power",
            "gpu_power_limit", "gpu_fan", "gpu_voltage", "gpu_efficiency", "vram", "proc_vram", "gpu_text", "gpu_load_change",
            "gpu_load_value", "gpu_load_color", "gpu_list", "pci_dev", "throttling_status", "throttling_status_graph").forEach { put(it, NO_GPU) }
        listOf("core_load", "core_load_change", "core_bars").forEach { put(it, "Not available on this device: Android blocks /proc/stat for apps") }
        listOf("cpu_power", "cpu_efficiency", "flip_efficiency").forEach { put(it, "Not available on this device: no CPU power sensor readable by apps") }
        put("network", "Not available on this device: Android blocks /sys/class/net for apps")
        listOf("media_player", "media_player_name", "media_player_format").forEach { put(it, "Not available on this device: needs D-Bus") }
        listOf("device_battery", "device_battery_icon").forEach { put(it, "Not available on this device: HID batteries are not readable by apps") }
        put("core_type", "Not available on this device: shown only with per-core load, which Android blocks")
        put("display_server", "Not available on this device: MangoHud cannot detect the Android X server")
        put("refresh_rate", "Not available on this device: needs gamescope")
        put("fex_stats","Not available on this device: FEX stats are not exported by FEXCore for ARM64EC")
        listOf("toggle_hud", "toggle_hud_position", "toggle_preset", "toggle_fps_limit", "toggle_logging", "reload_cfg",
            "reset_fps_metrics", "upload_log").forEach { put(it, "Not available on this device: the X server has no keyboard state query (QueryKeymap)") }
    }

    data class Cfg(val enabled: Boolean = false, val values: Map<String, String> = emptyMap()) {
        fun get(o: Opt) = values[o.key] ?: o.def
    }

    private fun prefs(c: Context) = c.getSharedPreferences("mangohud", Context.MODE_PRIVATE)
    private fun key(gameId: String?) = if (gameId.isNullOrEmpty()) "global" else "game_$gameId"

    /** Saved config: global (gameId null) or the game's override (null when the game uses the global one). */
    fun load(c: Context, gameId: String?): Cfg? {
        val s = prefs(c).getString(key(gameId), null) ?: return if (gameId.isNullOrEmpty()) Cfg() else null
        return try {
            val j = JSONObject(s)
            val v = j.optJSONObject("values")
            Cfg(j.optBoolean("enabled"), v?.keys()?.asSequence()?.associateWith { v.optString(it) } ?: emptyMap())
        } catch (_: Exception) { Cfg() }
    }

    fun save(c: Context, gameId: String?, cfg: Cfg) {
        prefs(c).edit().putString(key(gameId), JSONObject().put("enabled", cfg.enabled).put("values", JSONObject(cfg.values)).toString()).apply()
    }

    fun clear(c: Context, gameId: String) = prefs(c).edit().remove(key(gameId)).apply()

    fun effective(c: Context, gameId: String?): Cfg = (if (gameId.isNullOrEmpty()) null else load(c, gameId)) ?: load(c, null) ?: Cfg()

    /** MangoHud.conf text. Switches that are off by default are only written when on (some are not 0/1 parsers). */
    fun confText(cfg: Cfg): String = buildString {
        append("# Written by PanPlay at launch (MangoHud page). Edits here are overwritten.\n")
        for (o in ALL) {
            val v = cfg.get(o).replace('\n', ' ').replace('\r', ' ').trim()
            val na = o.key in NOT_AVAILABLE
            when {
                na -> if (o.kind == 'b' && o.def == "1") append(o.key).append("=0\n")
                o.kind == 'b' -> when {
                    v == "1" -> append(o.key).append('\n')
                    o.def == "1" -> append(o.key).append("=0\n")
                }
                v.isNotEmpty() -> append(o.key).append('=').append(v).append('\n')
            }
        }
    }

    /** Env for the Wine process; empty when MangoHud is off for this game or the component is missing. */
    fun env(c: Context, gameId: String?, containerDir: File): Map<String, String> {
        val cfg = effective(c, gameId)
        if (!cfg.enabled) return emptyMap()
        val lib = ContentManager.current(c, TYPE)?.let { File(it.dir, "lib/libMangoHud.so") }?.takeIf { it.isFile } ?: return emptyMap()
        val dir = File(containerDir, "mangohud").apply { mkdirs() }
        val layerDir = File(dir, "layer").apply { mkdirs() }
        val conf = File(dir, "MangoHud.conf")
        return try {
            conf.writeText(confText(cfg))
            File(layerDir, "MangoHud.panplay.json").writeText(JSONObject().put("file_format_version", "1.0.0").put("layer", JSONObject()
                .put("name", LAYER).put("type", "GLOBAL").put("api_version", "1.3.0").put("library_path", lib.absolutePath)
                .put("implementation_version", "1").put("description", "MangoHud overlay (PanPlay)")
                .put("functions", JSONObject().put("vkGetInstanceProcAddr", "overlay_GetInstanceProcAddr").put("vkGetDeviceProcAddr", "overlay_GetDeviceProcAddr"))
                .put("enable_environment", JSONObject().put("MANGOHUD", "1"))
                .put("disable_environment", JSONObject().put("DISABLE_MANGOHUD", "1"))).toString(2))
            startBatteryMirror(c, File(dir, "power_supply"))
            mapOf(
                "MANGOHUD" to "1",
                "MANGOHUD_POWER_SUPPLY" to File(dir, "power_supply").absolutePath,
                "MANGOHUD_CONFIGFILE" to conf.absolutePath,
                "VK_ADD_IMPLICIT_LAYER_PATH" to layerDir.absolutePath,
                "VK_LOADER_LAYERS_DISABLE" to STOCK_LAYER
            )
        } catch (_: Exception) { emptyMap() }
    }

    @Volatile private var mirror: Thread? = null

    /**
     * Apps cannot read /sys/class/power_supply, so MangoHud's battery items read this mirror of BatteryManager
     * (sysfs units: %, µA, µV, µW, µAh), refreshed every second while Wine runs.
     */
    private fun startBatteryMirror(c: Context, root: File) {
        if (mirror?.isAlive == true) return
        val bat = File(root, "BAT0").apply { mkdirs() }
        val app = c.applicationContext
        mirror = Thread({
            val bm = app.getSystemService(Context.BATTERY_SERVICE) as? android.os.BatteryManager
            val t0 = System.currentTimeMillis()
            while (System.currentTimeMillis() - t0 < 15_000 || ContainerManager.isRunning()) {
                try {
                    val i = app.registerReceiver(null, android.content.IntentFilter(android.content.Intent.ACTION_BATTERY_CHANGED))
                    val pct = bm?.getIntProperty(android.os.BatteryManager.BATTERY_PROPERTY_CAPACITY) ?: -1
                    val ua = bm?.getLongProperty(android.os.BatteryManager.BATTERY_PROPERTY_CURRENT_NOW) ?: Long.MIN_VALUE
                    val chargeUah = bm?.getLongProperty(android.os.BatteryManager.BATTERY_PROPERTY_CHARGE_COUNTER) ?: Long.MIN_VALUE
                    val uv = (i?.getIntExtra(android.os.BatteryManager.EXTRA_VOLTAGE, 0) ?: 0) * 1000L
                    val status = when (i?.getIntExtra(android.os.BatteryManager.EXTRA_STATUS, -1)) {
                        android.os.BatteryManager.BATTERY_STATUS_CHARGING -> "Charging"
                        android.os.BatteryManager.BATTERY_STATUS_FULL -> "Full"
                        android.os.BatteryManager.BATTERY_STATUS_DISCHARGING, android.os.BatteryManager.BATTERY_STATUS_NOT_CHARGING -> "Discharging"
                        else -> "Unknown"
                    }
                    fun put(n: String, v: Any) { val f = File(bat, "$n.tmp"); f.writeText("$v\n"); f.renameTo(File(bat, n)) }
                    if (pct in 0..100) put("capacity", pct)
                    put("status", status)
                    // Some devices report mA instead of µA: treat small magnitudes as mA.
                    val cur = if (ua == Long.MIN_VALUE || ua == 0L) null else if (kotlin.math.abs(ua) < 10_000) ua * 1000 else ua
                    if (cur != null && uv > 0) {
                        put("current_now", kotlin.math.abs(cur)); put("voltage_now", uv)
                        put("power_now", kotlin.math.abs(cur) * uv / 1_000_000)
                    }
                    if (chargeUah > 0 && pct in 1..100) { put("charge_now", chargeUah); put("charge_full", chargeUah * 100 / pct) }
                } catch (_: Throwable) {}
                try { Thread.sleep(1000) } catch (_: InterruptedException) { break }
            }
        }, "mangohud-battery").apply { isDaemon = true; start() }
    }
}

/** Full-screen MangoHud page: global settings (gameId null) or one game's override. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MangoHudPage(gameId: String?, gameName: String?, onDismiss: () -> Unit) {
    val ctx = LocalContext.current
    val perGame = !gameId.isNullOrEmpty()
    var own by remember { mutableStateOf(!perGame || MangoHudSettings.load(ctx, gameId) != null) }
    var cfg by remember { mutableStateOf(MangoHudSettings.effective(ctx, gameId)) }
    val installed = remember { ContentManager.current(ctx, MangoHudSettings.TYPE) != null }
    fun update(n: MangoHudSettings.Cfg) { cfg = n; MangoHudSettings.save(ctx, gameId, n) }

    Dialog(onDismissRequest = onDismiss, properties = DialogProperties(usePlatformDefaultWidth = false)) {
        Surface(Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.surface) {
            Scaffold(topBar = {
                TopAppBar(
                    title = {
                        Column {
                            Text("MangoHud")
                            Text(if (perGame) "For ${gameName ?: gameId}" else "Global (games without their own settings)",
                                style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                        }
                    },
                    navigationIcon = { IconButton(onClick = onDismiss) { Icon(Icons.AutoMirrored.Rounded.ArrowBack, contentDescription = "Back") } }
                )
            }) { pad ->
                PageList(Modifier.padding(pad)) {
                    item {
                        MhCard {
                            if (perGame) MhSwitch("Own settings for this game", "Off uses the global MangoHud settings", own) {
                                own = it
                                if (it) update(cfg) else { MangoHudSettings.clear(ctx, gameId!!); cfg = MangoHudSettings.effective(ctx, null) }
                            }
                            MhSwitch("MangoHud", "Vulkan overlay: FPS, frame time, CPU, RAM, battery. Shown at top-right by default so it does not cover the DXVK HUD. CPU load is the game's share of all cores.", cfg.enabled, own) {
                                update(cfg.copy(enabled = it))
                            }
                            if (!installed) Text("MangoHud is not installed. Install it in Components.", style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.error)
                            OutlinedButton(onClick = { update(MangoHudSettings.Cfg(cfg.enabled)) }, enabled = own, modifier = Modifier.heightIn(min = 48.dp)) { Text("Reset options to defaults") }
                        }
                    }
                    MangoHudSettings.GROUPS.forEach { (title, opts) ->
                        item(key = title) { SectionTitle(title) }
                        item(key = "$title-card") {
                            MhCard {
                                opts.forEach { o -> MhOption(o, cfg.get(o), own) { v -> update(cfg.copy(values = cfg.values + (o.key to v))) } }
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun MhCard(content: @Composable androidx.compose.foundation.layout.ColumnScope.() -> Unit) {
    Card(Modifier.fillMaxWidth(), colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceContainerLow)) {
        Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp), content = content)
    }
}

@Composable
private fun MhSwitch(title: String, subtitle: String?, checked: Boolean, enabled: Boolean = true, onChange: (Boolean) -> Unit) {
    Row(
        Modifier.fillMaxWidth().heightIn(min = 48.dp).toggleable(value = checked, enabled = enabled, role = Role.Switch, onValueChange = onChange),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(Modifier.weight(1f)) {
            Text(title, style = MaterialTheme.typography.bodyLarge)
            if (subtitle != null) Text(subtitle, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        Switch(checked = checked, onCheckedChange = null, enabled = enabled)
    }
}

@Composable
private fun MhOption(o: MangoHudSettings.Opt, value: String, editable: Boolean, onValue: (String) -> Unit) {
    val na = MangoHudSettings.NOT_AVAILABLE[o.key]
    val enabled = editable && na == null
    val note = o.key + (if (o.kind == 'k') " (RRGGBB)" else "") + (na?.let { " · $it" } ?: "")
    when (o.kind) {
        'b' -> MhSwitch(o.label, note, value == "1" && na == null, enabled) { onValue(if (it) "1" else "0") }
        'c' -> DropdownField(o.label, o.choices.map { it to it.ifEmpty { "Default" } }, value, onValue, enabled = enabled)
        else -> OutlinedTextField(
            value, onValue, enabled = enabled, singleLine = true, modifier = Modifier.fillMaxWidth(),
            label = { Text(o.label) },
            placeholder = { if (o.hint.isNotEmpty()) Text(o.hint) },
            supportingText = { Text(note) }
        )
    }
}
