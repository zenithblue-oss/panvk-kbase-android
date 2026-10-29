package dev.zenithblue.panvklauncher

import android.content.Context
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp

/** Release notes shown once per versionCode. Update these two lists each release. */
object WhatsNew {
    val added = listOf(
        "MangoHud 0.8.4 overlay, built from source: install it in Components",
        "MangoHud page (Settings > Graphics, and per game in Edit game): on/off and every MangoHud option, global or per game",
        "DXVK HUD on/off switch and HUD items in Settings > Graphics, plus a DXVK HUD choice per game",
        "Mesa PanVK 26.3.0-devel driver, now also for Mali Bifrost GPUs (v6/v7)",
        "OpenGL games through Mesa Zink 26.2.4",
        "Render API per game: Auto, DXVK, VKD3D, WineD3D, OpenGL (Zink), Vulkan or cnc-ddraw",
        "The launch sheet shows the detected graphics API (with Godot, Unity and Unreal hints) and the API of the last run",
        "Built-in Direct3D feature level tests: D3D12 11_0 to 12_2 and D3D11 10_0 to 11_1",
        "Present path: Default or EGL (frames in AHardwareBuffers, no GL upload), in Settings > Display and per game",
        "WineD3D-Zink Render API, named clearly: D3D9-11 through OpenGL, Zink and PanVK (D3D12 stays on VKD3D)",
        "Built-in Fallout 4 controller key config (\"Fallout 4 (keyboard)\" template, incl. Enter and T); Fallout 4 itself uses the Xbox gamepad by default",
    )
    val fixed = listOf(
        "The DXVK HUD was always on with every item: the game's DXVK_HUD setting was ignored",
        "Fallout 4: launch fixes, less memory, no black heads",
        "Game audio in WMA (xWMA): Fallout 4 music, voices and sound effects no longer go silent once gameplay starts",
        "The launch sheet no longer saves stale values",
        "Browse for .exe: files picked from Downloads (msf: picker URIs) now resolve to the real path",
        "Built-in X server: MIT-SHM requests check the segment owner and the image bounds",
        "Fallout 4: the Xbox gamepad (on-screen or physical) works again; Wine services stay on unless the GOG Galaxy64.dll is present",
    )

    private const val KEY = "changelog_seen"
    private fun prefs(c: Context) = c.getSharedPreferences("launcher", Context.MODE_PRIVATE)
    fun shouldShow(c: Context) = prefs(c).getLong(KEY, 0L) < getAppVersionCode(c)
    fun markSeen(c: Context) = prefs(c).edit().putLong(KEY, getAppVersionCode(c)).apply()
}

/** "What's new" dialog: shows once after install or update, then never again for this versionCode. */
@Composable
fun WhatsNewDialog(enabled: Boolean = true) {
    val context = LocalContext.current
    var show by remember { mutableStateOf(WhatsNew.shouldShow(context)) }
    if (!show || !enabled) return
    val close = { WhatsNew.markSeen(context); show = false }
    AlertDialog(
        onDismissRequest = close,
        title = { Text("What's new in ${getAppVersion(context)}") },
        text = {
            LazyColumn(Modifier.heightIn(max = 420.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                for ((head, lines) in listOf("Added" to WhatsNew.added, "Fixed" to WhatsNew.fixed)) {
                    item { SectionTitle(head) }
                    items(lines) { Text("• $it", style = MaterialTheme.typography.bodyMedium) }
                }
            }
        },
        confirmButton = { TextButton(onClick = close) { Text("OK") } }
    )
}
