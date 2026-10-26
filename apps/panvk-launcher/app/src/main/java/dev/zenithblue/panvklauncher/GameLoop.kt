// SPDX-License-Identifier: MIT
package dev.zenithblue.panvklauncher

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.util.Log
import android.view.WindowManager
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.winlator.renderer.GLRenderer
import kotlinx.coroutines.runBlocking
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.lang.ref.WeakReference
import java.security.MessageDigest
import java.text.SimpleDateFormat
import java.util.Collections
import java.util.Date
import java.util.Locale
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.atomic.AtomicReference

/**
 * Firebase Test Lab game loop: `com.google.intent.action.TEST_LOOP`, `--ei scenario N`, results file in intent.data.
 * Runs the passes of assets/gameloop.json's scenario N (see [GameLoopMath]) unattended on a fresh install: bundled
 * components, container setup, built-in cube exes, then each pass through the same ContainerManager.runExe path as
 * the Games screen (built-in X server, DXVK HUD on). Per pass: the driver fps log (PANVK_FPS_LOG, csf-v11 233; not
 * written by drivers without it), the DXVK stats file, compositor counters, a screenshot, all session logs.
 * Results: files/gameloop/<run>/ in the external files dir (Firebase --directories-to-pull) and intent.data.
 * Then the activity finishes. The interactive launcher is not shown and not changed.
 *
 * Local run: adb shell am start -a com.google.intent.action.TEST_LOOP -n dev.zenithblue.panvklauncher/.MainActivity
 *   --ei scenario 1 [--ei warmup_sec 5 --ei measure_sec 10] [--es gameloop_config /path/override.json]
 */
object GameLoop {
    const val ACTION = "com.google.intent.action.TEST_LOOP"
    private const val TAG = "PanPlayGameLoop"
    private const val MAX_LINES = 50_000

    /** True while a loop runs (XServerActivity hides the touch overlay then). */
    @Volatile var active = false; private set

    /** Status lines for [GameLoopScreen]; changed on the main thread only. */
    val status = mutableStateListOf<String>()
    val title = mutableStateOf("PanPlay game loop")

    private val main = Handler(Looper.getMainLooper())
    @Volatile private var activity: WeakReference<Activity>? = null
    @Volatile private var logFile: File? = null

    fun isGameLoop(i: Intent?) = i?.action == ACTION

    /** From MainActivity (create or new intent). Starts the loop once; while it runs a new activity only re-attaches. */
    fun start(a: Activity, intent: Intent) {
        activity = WeakReference(a)
        a.window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        synchronized(this) {
            if (active) {
                log("game loop already running: activity re-attached, new intent ignored")
                return
            }
            active = true
        }
        val ctx = a.applicationContext
        val scenario = intent.getIntExtra("scenario", 1)
        val uri = intent.data
        val extra = JSONObject()
        if (intent.hasExtra("warmup_sec")) extra.put("warmupSec", intent.getIntExtra("warmup_sec", 20))
        if (intent.hasExtra("measure_sec")) extra.put("measureSec", intent.getIntExtra("measure_sec", 60))
        val cfgPath = intent.getStringExtra("gameloop_config")
        main.post { status.clear(); title.value = "PanPlay game loop: scenario $scenario" }
        Thread({
            try {
                runLoop(ctx, scenario, uri, cfgPath, extra)
            } catch (t: Throwable) {
                log("game loop crashed: $t")
            } finally {
                active = false
                main.post { activity?.get()?.finish() }
            }
        }, "panplay-gameloop").start()
    }

    private fun log(msg: String) {
        Log.i(TAG, msg)
        val line = "[" + SimpleDateFormat("HH:mm:ss", Locale.US).format(Date()) + "] " + msg
        logFile?.let { f -> try { synchronized(this) { f.appendText(line + "\n") } } catch (_: Exception) {} }
        main.post {
            status.add(line)
            if (status.size > 300) status.removeRange(0, status.size - 300)
        }
    }

    private fun sha256(f: File): String? = try {
        val md = MessageDigest.getInstance("SHA-256")
        f.inputStream().use { i -> val b = ByteArray(1 shl 16); while (true) { val n = i.read(b); if (n < 0) break; md.update(b, 0, n) } }
        md.digest().joinToString("") { "%02x".format(it) }
    } catch (_: Exception) { null }

    private fun sha256(s: String): String =
        MessageDigest.getInstance("SHA-256").digest(s.toByteArray()).joinToString("") { "%02x".format(it) }

    // ---- config ----

    /** assets/gameloop.json, deep-merged with the first override found, then the intent's numbers. */
    private fun loadConfig(ctx: Context, extraPath: String?, extra: JSONObject): Pair<JSONObject, JSONArray> {
        val src = JSONArray().put("assets/gameloop.json")
        var j = JSONObject(ctx.assets.open("gameloop.json").bufferedReader().use { it.readText() })
        val candidates = listOfNotNull(
            extraPath, ctx.getExternalFilesDir(null)?.let { File(it, "gameloop.json").path },
            "/sdcard/panplay-gameloop.json", "/data/local/tmp/panplay-gameloop.json"
        )
        for (p in candidates) {
            val text = try { File(p).takeIf { it.isFile }?.readText() } catch (_: Exception) { null } ?: continue
            j = GameLoopMath.merge(j, JSONObject(text))
            src.put("$p sha256=${sha256(text)}")
            break
        }
        if (extra.length() > 0) { j = GameLoopMath.merge(j, extra); src.put("intent extras $extra") }
        return j to src
    }

    // ---- run ----

    private fun runLoop(ctx: Context, scenario: Int, resultUri: Uri?, cfgPath: String?, extra: JSONObject) {
        val started = System.currentTimeMillis()
        val runId = SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(Date(started)) + "-s$scenario"
        val ext = ctx.getExternalFilesDir(null) ?: File(ctx.filesDir, "external")
        val out = File(ext, "gameloop/$runId").apply { mkdirs() }
        val work = File(ctx.filesDir, "gameloop/$runId").apply { mkdirs() }
        logFile = File(out, "harness.log")
        log("scenario $scenario, results in ${out.path}, results file ${resultUri ?: "(none)"}")

        val pi = ctx.packageManager.getPackageInfo(ctx.packageName, 0)
        val res = JSONObject().apply {
            put("harness", "panplay-gameloop"); put("version", 1); put("scenario", scenario); put("runId", runId)
            put("startedUnixMs", started)
            put("app", JSONObject().put("package", ctx.packageName).put("versionName", pi.versionName).put("versionCode", pi.longVersionCode))
            put("device", deviceJson(ctx))
            put("passes", JSONArray())
        }
        fun publish() {
            res.put("updatedUnixMs", System.currentTimeMillis())
            val text = res.toString(2)
            try { File(out, "results.json").writeText(text) } catch (t: Throwable) { log("results.json: $t") }
            try { File(out, "results.txt").writeText(summaryText(res)) } catch (t: Throwable) { log("results.txt: $t") }
            if (resultUri != null) try { writeUri(ctx, resultUri, text) } catch (t: Throwable) { log("results file $resultUri: $t") }
        }

        try {
            val (cfgJson, cfgSrc) = loadConfig(ctx, cfgPath, extra)
            File(out, "config.json").writeText(cfgJson.toString(2))
            res.put("configSource", cfgSrc)
            val cfg = GameLoopMath.parseConfig(cfgJson)
            val passes = GameLoopMath.passes(cfg, scenario)
            val sc = cfg.scenarios.getValue(scenario)
            res.put("scenarioName", sc.name).put("tasks", JSONArray(sc.tasks)).put("envSets", JSONArray(sc.envSets))
            res.put("window", JSONObject().put("warmupSec", cfg.warmupSec).put("measureSec", cfg.measureSec)
                .put("tailSec", cfg.tailSec).put("fpsLogMs", cfg.fpsLogMs).put("from", "first frame"))
            res.put("envSetNotes", JSONObject().apply { for (e in sc.envSets) put(e, cfg.envSets.getValue(e).note) })
            main.post { title.value = "PanPlay game loop: scenario $scenario (${sc.name}), ${passes.size} pass(es)" }
            publish()

            prepare(ctx)
            val drv = driver(ctx, cfg)
            res.put("driver", JSONObject().put("id", drv.id).put("name", drv.name).put("version", drv.version)
                .put("bundled", drv.bundled).put("libPath", drv.libPath).put("size", File(drv.libPath).length())
                .put("sha256", sha256(File(drv.libPath)) ?: JSONObject.NULL))
            publish()

            for (p in passes) {
                log("pass ${p.index}/${passes.size}: ${p.task.id} (${p.task.title}), env set ${p.envSet.id}")
                val r = try { runPass(ctx, cfg, p, drv, out, work) } catch (t: Throwable) {
                    JSONObject().put("index", p.index).put("task", p.task.id).put("envSet", p.envSet.id)
                        .put("status", "fail").put("reason", "harness error: $t")
                }
                if (!res.has("driverInfo") && r.has("driverInfo")) res.put("driverInfo", r.get("driverInfo"))
                res.getJSONArray("passes").put(r)
                log("pass ${p.index}: ${r.optString("status")} ${r.optString("reason")} fps=${r.opt("fps") ?: "n/a"}")
                res.put("ab", abTable(res.getJSONArray("passes"), sc.envSets))
                publish()
                if (p.index < passes.size && cfg.gapSec > 0) Thread.sleep(cfg.gapSec * 1000L)
            }
        } catch (t: Throwable) {
            log("game loop error: $t")
            res.put("error", t.toString())
        } finally {
            val ps = res.getJSONArray("passes")
            res.put("ok", !res.has("error") && ps.length() > 0 && (0 until ps.length()).all { ps.getJSONObject(it).optString("status") == "ok" })
            res.put("endedUnixMs", System.currentTimeMillis())
            publish()
            try { work.deleteRecursively() } catch (_: Exception) {}
            log("done: ok=${res.optBoolean("ok")}")
        }
    }

    /** Bundled components (the launcher's first-start rule), the Wine prefix, the built-in exes. */
    private fun prepare(ctx: Context) {
        val prefs = ctx.getSharedPreferences("components", Context.MODE_PRIVATE)
        val stamp = ctx.packageManager.getPackageInfo(ctx.packageName, 0).lastUpdateTime
        val installed = ContentManager.list(ctx)
        val todo = ContentManager.CATALOG.map { ContentManager.stateFor(it, installed) }
            .filter { it.status != ComponentStatus.Installed && ContentManager.bundledAsset(ctx, it.entry) != null }
        var failed = false
        for (st in todo) {
            val e = st.entry
            if (st.status == ComponentStatus.Incomplete) st.installed.forEach { ContentManager.delete(ctx, it) }
            val asset = ContentManager.bundledAsset(ctx, e) ?: continue
            log("installing bundled ${e.name}")
            var last = -1
            val r = runBlocking {
                ContentManager.installAsset(ctx, asset, rootfs = (e.type == "imagefs")) { pct ->
                    if (pct / 25 != last / 25) { last = pct; log("  ${e.title}: $pct%") }
                }
            }
            r.onSuccess { inst ->
                if (st.status == ComponentStatus.UpdateAvailable)
                    st.installed.filter { it.dir != inst.dir }.forEach { ContentManager.delete(ctx, it) }
                if (inst.type == "VKD3D" && ContainerManager.isVkd3dEnabled(ctx)) ContainerManager.setVkd3dEnabled(ctx, true)
                log("installed ${inst.type}/${inst.versionName}")
            }.onFailure { failed = true; log("install of ${e.name} failed: ${it.message}") }
        }
        if (!failed) prefs.edit().putLong("bundledInstalledFor", stamp).apply()
        if (!ContainerManager.isSetup(ctx)) {
            log("setting up the Wine prefix (wineboot -u)")
            val r = ContainerManager.setup(ctx) { line -> Log.i(TAG, "setup: $line") }
            if (r.isFailure) throw IllegalStateException("container setup failed: ${r.exceptionOrNull()?.message}")
            log("Wine prefix ready")
        }
        BuiltinTests.extract(ctx)
    }

    /** "bundled" (default: the driver this APK carries), "selected", or a driver id. */
    private fun driver(ctx: Context, cfg: GameLoopMath.Config): Driver {
        val all = DriverManager.getDrivers(ctx)
        val bundled = all.firstOrNull { it.bundled } ?: DriverManager.bundledDriver(ctx)
        return when (cfg.driver) {
            "bundled" -> bundled
            "selected" -> DriverManager.getSelectedDriver(ctx, all)
            else -> DriverManager.find(ctx, all, cfg.driver) ?: bundled.also { log("driver ${cfg.driver} not found, using the bundled one") }
        }
    }

    private class Sample(val atMs: Long, val drawn: Long, val content: Long)

    private fun sample() = Sample(SystemClock.elapsedRealtime(), GLRenderer.framesDrawn.get(), GLRenderer.contentUpdates.get())

    private fun runPass(ctx: Context, cfg: GameLoopMath.Config, p: GameLoopMath.Pass, drv: Driver, out: File, work: File): JSONObject {
        val r = JSONObject().put("index", p.index).put("task", p.task.id).put("title", p.task.title)
            .put("envSet", p.envSet.id).put("dir", p.label)
        val dir = File(out, p.label).apply { mkdirs() }
        val wdir = File(work, p.label).apply { mkdirs() }
        val sc = ShortcutStore.find(ctx, p.task.shortcut)
            ?: return r.put("status", "fail").put("reason", "shortcut ${p.task.shortcut} not found")
        val exePath = ShortcutStore.resolveExe(ctx, sc.exe)
        r.put("shortcut", sc.id).put("exe", exePath)

        val recording = PerfRecorder.recording(ctx)
        val ownStats = File(wdir, "dxvk-stats.csv")
        val env = LinkedHashMap(GameLoopMath.passEnv(cfg, p, File(wdir, "panvk-fps-%p.log").absolutePath))
        if (!recording) env["DXVK_STATS_FILE"] = "Z:" + ownStats.absolutePath
        r.put("env", JSONObject(env as Map<*, *>))
        val base = ShortcutStore.launchOptions(ctx, sc)
        val opts = base.copy(env = base.env + env, driverId = drv.id)

        val lines = Collections.synchronizedList(ArrayList<String>())
        val launcherLog = StringBuffer()
        val firstFrame = AtomicLong(0)
        val done = AtomicBoolean(false)
        val stopReason = AtomicReference<String?>(null)
        val harnessEnd = AtomicBoolean(false)
        val s0 = AtomicReference<Sample?>(null)
        val s1 = AtomicReference<Sample?>(null)
        val shot = File(dir, "screen.png")
        val t0 = SystemClock.elapsedRealtime()
        val startMs = System.currentTimeMillis()
        val windowEnd = (cfg.warmupSec + cfg.measureSec) * 1000L

        // Never throws: an uncaught exception in any thread would kill the app (and the Firebase run).
        val watchdog = Thread({ try {
            var shotTaken = cfg.screenshotAtSec < 0
            while (!done.get()) {
                try { Thread.sleep(250) } catch (_: InterruptedException) { break }
                val now = SystemClock.elapsedRealtime()
                if (firstFrame.get() == 0L && (wdir.list()?.any { it.startsWith("panvk-fps-") } == true))
                    if (firstFrame.compareAndSet(0, now)) log("first frame (driver fps log) after ${(now - t0) / 1000} s")
                val ff = firstFrame.get()
                if (ff == 0L) {
                    if (now - t0 > cfg.startTimeoutSec * 1000L) {
                        stopReason.set("no first frame after ${cfg.startTimeoutSec} s")
                        ContainerManager.stop(ctx); break
                    }
                    continue
                }
                val since = now - ff
                if (s0.get() == null && since >= cfg.warmupSec * 1000L) s0.set(sample())
                if (s1.get() == null && since >= windowEnd) s1.set(sample())
                if (!shotTaken && since >= cfg.screenshotAtSec * 1000L) {
                    shotTaken = true
                    log(if (XServerActivity.capture(shot)) "screenshot ${shot.name}" else "screenshot failed (no display view)")
                }
                // Self-timed tests exit by themselves (+45 s grace); others are stopped once the window is done.
                val limit = cfg.testSecs * 1000L + (if (p.task.secsEnv.isEmpty()) 0L else 45_000L)
                if (since >= limit) {
                    if (p.task.secsEnv.isEmpty()) harnessEnd.set(true)
                    else stopReason.set("still running ${limit / 1000} s after the first frame")
                    ContainerManager.stop(ctx); break
                }
            }
        } catch (_: InterruptedException) {
        } catch (t: Throwable) { log("watchdog: $t") } }, "gameloop-watchdog")

        ControllerInput.config = ControllerConfig.resolve(ctx, sc.id, exePath)
        GameSession.set(sc, exePath)
        DisplayServer.launchOverride = sc.resolution.takeIf { Resolution.parse(it) != null }
        var code = Int.MIN_VALUE
        var perfId: String? = null
        watchdog.start()
        try {
            PerfRecorder.start(ctx, sc, exePath)
            code = ContainerManager.runExe(ctx, exePath, { line ->
                launcherLog.append(line).append('\n')
                if (lines.size < MAX_LINES) lines.add(line)
                if (firstFrame.get() == 0L && p.task.firstFramePattern?.containsMatchIn(line.trim()) == true &&
                    firstFrame.compareAndSet(0, SystemClock.elapsedRealtime()))
                    log("first frame after ${(SystemClock.elapsedRealtime() - t0) / 1000} s: ${line.trim()}")
                if (p.task.okPattern?.containsMatchIn(line.trim()) == true || p.task.failPattern?.containsMatchIn(line.trim()) == true)
                    log(line.trim())
            }, opts)
        } catch (t: Throwable) {
            r.put("error", t.toString())
        } finally {
            done.set(true)
            watchdog.interrupt()
            try { watchdog.join(5000) } catch (_: Exception) {}
            perfId = PerfRecorder.finish(ctx)
            DisplayServer.launchOverride = null
        }
        val wallMs = SystemClock.elapsedRealtime() - t0
        r.put("exit", if (code == Int.MIN_VALUE) JSONObject.NULL else code).put("wallSec", wallMs / 1000.0)
        firstFrame.get().takeIf { it > 0 }?.let { r.put("firstFrameSec", (it - t0) / 1000.0) }

        // Logs: the launcher's session folder (wine-run.log, dxvk-*.log, logcat.txt, mesa-panvk.txt, session.json ...).
        try {
            val sd = SessionLogs.collect(ctx, sc, exePath, startMs, launcherLog.toString(), true)
            sd.copyRecursively(File(dir, "session"), overwrite = true)
            r.put("sessionReason", SessionLogs.summary(sd).optString("reason"))
        } catch (t: Throwable) { log("session logs: $t") }
        if (perfId != null) {
            for (suffix in listOf(".csv", "-sys.csv", ".json")) {
                val f = File(PerfRecorder.dir(ctx), perfId + suffix)
                if (f.isFile) try { f.copyTo(File(dir, "perf" + suffix), overwrite = true) } catch (_: Exception) {}
            }
            try { r.put("perfSummary", PerfRecorder.summary(ctx, perfId)) } catch (_: Exception) {}
        }
        val fpsFiles = wdir.listFiles { f -> f.name.startsWith("panvk-fps-") }?.sortedBy { it.name } ?: emptyList()
        for (f in fpsFiles) try { f.copyTo(File(dir, f.name), overwrite = true) } catch (_: Exception) {}
        if (ownStats.isFile) try { ownStats.copyTo(File(dir, "dxvk-stats.csv"), overwrite = true) } catch (_: Exception) {}

        // fps over the measured window, from the first frame of each source.
        val fromMs = cfg.warmupSec * 1000L
        val logs = fpsFiles.map { it to GameLoopMath.parseFpsLog(it.readLines().asSequence()) }
        val drvLog = logs.maxByOrNull { it.second.total }
        if (drvLog != null) {
            val (f, l) = drvLog
            r.put("fpsLog", f.name).put("driverPresents", l.total)
            l.driverInfo?.let { r.put("driverInfo", it) }
            if (logs.size > 1) r.put("fpsLogsOther", JSONArray(logs.filter { it !== drvLog }.map { "${it.first.name} total=${it.second.total}" }))
            GameLoopMath.driverWindow(l, fromMs, windowEnd)?.let { w ->
                r.put("driverFps", JSONObject().put("fps", w.fps).put("frames", w.frames).put("spanMs", w.spanMs)
                    .put("intervals", w.intervals).put("minIntervalFps", w.minIntervalFps)
                    .put("maxIntervalFps", w.maxIntervalFps).put("maxFrameMs", w.maxFrameMs))
            }
        }
        val statsFile = if (recording && perfId != null) File(PerfRecorder.dir(ctx), "$perfId.csv") else ownStats
        if (statsFile.isFile) try {
            val d = PerfMath.parseDxvk(statsFile.useLines { it.toList() }.asSequence())
            GameLoopMath.dxvkWindow(d.frameT, d.frameUs, fromMs, windowEnd)?.let { f ->
                val o = JSONObject().put("avgFps", f.avgFps).put("frames", f.count).put("low1Fps", f.low1).put("low01Fps", f.low01)
                    .put("p50Ms", f.p50Ms).put("p95Ms", f.p95Ms).put("p99Ms", f.p99Ms).put("stutters", f.stutters)
                if (d.frameT.isNotEmpty()) {
                    val lo = d.frameT[0] + fromMs * 1000; val hi = d.frameT[0] + windowEnd * 1000
                    d.rows.filter { it.tUs in lo..hi }.takeIf { it.isNotEmpty() }?.let { rows ->
                        o.put("gpuLoadAvg", rows.map { it.gpuLoad }.average()).put("drawCallsAvg", rows.map { it.draws }.average())
                    }
                }
                r.put("dxvkFps", o)
            }
        } catch (t: Throwable) { log("dxvk stats: $t") }
        val a = s0.get(); val b = s1.get()
        if (a != null && b != null && b.atMs > a.atMs) {
            val sec = (b.atMs - a.atMs) / 1000.0
            r.put("compositor", JSONObject().put("drawnFps", (b.drawn - a.drawn) / sec).put("contentFps", (b.content - a.content) / sec))
        }
        if (shot.isFile) r.put("screenshot", shot.name)
        val fps = r.optJSONObject("driverFps")?.optDouble("fps") ?: r.optJSONObject("dxvkFps")?.optDouble("avgFps")
        if (fps != null && !fps.isNaN()) r.put("fps", fps).put("fpsSource", if (r.has("driverFps")) "driver" else "dxvk")

        // Verdict.
        val test = GameLoopMath.parseTest(p.task, lines.toList().asSequence())
        r.put("test", JSONObject().put("ok", test.ok ?: JSONObject.NULL).put("line", test.line ?: JSONObject.NULL)
            .put("frames", test.frames ?: JSONObject.NULL).put("fps", test.fps ?: JSONObject.NULL)
            .put("startupSec", test.startupSec ?: JSONObject.NULL))
        val reason = when {
            stopReason.get() != null -> stopReason.get()
            r.has("error") -> "run error: ${r.getString("error")}"
            test.ok == false -> "test failed: ${test.line}"
            p.task.okPattern != null && test.ok != true -> "no pass line (${p.task.okPattern.pattern})"
            !harnessEnd.get() && code != 0 -> "exit code $code"
            fps == null -> "no fps data in the window (no frames ${cfg.warmupSec}-${cfg.warmupSec + cfg.measureSec} s after the first)"
            else -> null
        }
        r.put("status", if (reason == null) "ok" else "fail")
        if (reason != null) r.put("reason", reason)
        if (harnessEnd.get()) r.put("stoppedByHarness", true)
        try { File(dir, "result.json").writeText(r.toString(2)) } catch (_: Exception) {}
        return r
    }

    /** Per task: each env set's fps and its change against the scenario's first env set. */
    private fun abTable(passes: JSONArray, envSets: List<String>): JSONArray {
        val outA = JSONArray()
        if (envSets.size < 2) return outA
        val byTask = LinkedHashMap<String, MutableMap<String, Double?>>()
        for (i in 0 until passes.length()) {
            val p = passes.getJSONObject(i)
            val f = if (p.optString("status") == "ok" && p.has("fps")) p.getDouble("fps") else null
            byTask.getOrPut(p.optString("task")) { LinkedHashMap() }[p.optString("envSet")] = f
        }
        val baseSet = envSets.first()
        for ((task, m) in byTask) {
            val o = JSONObject().put("task", task).put("baseline", baseSet)
            val fps = JSONObject(); val delta = JSONObject()
            for ((set, f) in m) {
                fps.put(set, f ?: JSONObject.NULL)
                if (set != baseSet) delta.put(set, GameLoopMath.deltaPct(m[baseSet], f) ?: JSONObject.NULL)
            }
            outA.put(o.put("fps", fps).put("deltaPct", delta))
        }
        return outA
    }

    private fun f1(v: Any?) = (v as? Number)?.toDouble()?.takeIf { !it.isNaN() }?.let { String.format(Locale.US, "%.1f", it) } ?: "-"

    private fun summaryText(res: JSONObject) = buildString {
        val d = res.optJSONObject("device")
        appendLine("PanPlay game loop, scenario ${res.opt("scenario")} (${res.optString("scenarioName")}), run ${res.optString("runId")}")
        if (d != null) appendLine("Device: ${d.optString("manufacturer")} ${d.optString("model")} (${d.optString("device")}), SDK ${d.opt("sdk")}, SoC ${d.optString("socModel", "?")}")
        res.optJSONObject("driver")?.let { appendLine("Driver: ${res.optString("driverInfo", it.optString("name"))}, sha256 ${it.optString("sha256").take(16)}") }
        res.optJSONObject("app")?.let { appendLine("App: ${it.optString("package")} ${it.optString("versionName")} (${it.opt("versionCode")})") }
        res.optJSONObject("window")?.let { appendLine("Window: ${it.opt("warmupSec")} s warm-up, ${it.opt("measureSec")} s measured, from the first frame") }
        res.optString("error").takeIf { it.isNotEmpty() }?.let { appendLine("ERROR: $it") }
        appendLine()
        appendLine(String.format(Locale.US, "%-3s %-8s %-9s %-5s %8s %7s %7s %7s %8s %8s %6s %6s %6s %8s  %s",
            "#", "task", "env", "stat", "drv-fps", "1s-min", "1s-max", "max-ms", "dxvk-fps", "1%-low", "p50", "p95", "p99", "drawn", "test / reason"))
        val ps = res.optJSONArray("passes") ?: JSONArray()
        for (i in 0 until ps.length()) {
            val p = ps.getJSONObject(i)
            val dr = p.optJSONObject("driverFps"); val dx = p.optJSONObject("dxvkFps"); val c = p.optJSONObject("compositor")
            appendLine(String.format(Locale.US, "%-3s %-8s %-9s %-5s %8s %7s %7s %7s %8s %8s %6s %6s %6s %8s  %s",
                "%02d".format(p.optInt("index")), p.optString("task"), p.optString("envSet"), p.optString("status"),
                f1(dr?.opt("fps")), f1(dr?.opt("minIntervalFps")), f1(dr?.opt("maxIntervalFps")), f1(dr?.opt("maxFrameMs")),
                f1(dx?.opt("avgFps")), f1(dx?.opt("low1Fps")), f1(dx?.opt("p50Ms")), f1(dx?.opt("p95Ms")), f1(dx?.opt("p99Ms")),
                f1(c?.opt("drawnFps")), p.optString("reason").ifEmpty { p.optJSONObject("test")?.optString("line") ?: "" }))
        }
        val ab = res.optJSONArray("ab")
        if (ab != null && ab.length() > 0) {
            appendLine()
            for (i in 0 until ab.length()) {
                val o = ab.getJSONObject(i)
                val fps = o.getJSONObject("fps"); val dl = o.getJSONObject("deltaPct")
                appendLine("A/B ${o.optString("task")}: " + fps.keys().asSequence().joinToString(", ") { k ->
                    "$k ${f1(fps.opt(k))} fps" + ((dl.opt(k) as? Number)?.let { String.format(Locale.US, " (%+.1f%%)", it.toDouble()) } ?: "")
                } + " (change vs ${o.optString("baseline")})")
            }
        }
    }

    private fun deviceJson(ctx: Context) = JSONObject().apply {
        put("manufacturer", Build.MANUFACTURER); put("model", Build.MODEL); put("device", Build.DEVICE)
        put("product", Build.PRODUCT); put("hardware", Build.HARDWARE); put("board", Build.BOARD)
        put("sdk", Build.VERSION.SDK_INT); put("release", Build.VERSION.RELEASE); put("fingerprint", Build.FINGERPRINT)
        if (Build.VERSION.SDK_INT >= 31) { put("socManufacturer", Build.SOC_MANUFACTURER); put("socModel", Build.SOC_MODEL) }
        put("maliNode", File("/dev/mali0").exists() || File("/dev/mali").exists())
        val dm = ctx.resources.displayMetrics
        put("display", "${dm.widthPixels}x${dm.heightPixels}")
        try {
            ctx.getSystemService(android.hardware.display.DisplayManager::class.java)
                ?.getDisplay(android.view.Display.DEFAULT_DISPLAY)?.let { put("refreshHz", it.refreshRate.toDouble()) }
        } catch (_: Exception) {}
        put("xScreen", DisplayServer.resolution(ctx))
    }

    /** Firebase's results file. Never a file of PanPlay's own (a file:// into its data dir, or its own providers). */
    private fun writeUri(ctx: Context, uri: Uri, text: String) {
        val own = listOf(ctx.applicationInfo.dataDir, ctx.filesDir.parent ?: "").filter { it.isNotEmpty() }
            .map { File(it).canonicalPath + "/" }
        val allowed = when (uri.scheme) {
            "content" -> !(uri.authority ?: "").startsWith(ctx.packageName)
            "file" -> uri.path?.let { p -> val c = File(p).canonicalPath; own.none { c.startsWith(it) } } == true
            else -> false
        }
        if (!allowed) { log("results file $uri refused (points into PanPlay's own data)"); return }
        ctx.contentResolver.openOutputStream(uri, "wt")?.use { it.write(text.toByteArray()) }
    }
}

/** Shown instead of the launcher while a game loop runs. */
@Composable
fun GameLoopScreen() {
    val lines = GameLoop.status
    val state = rememberLazyListState()
    LaunchedEffect(lines.size) { if (lines.isNotEmpty()) state.scrollToItem(lines.size - 1) }
    Surface(modifier = Modifier.fillMaxSize(), color = MaterialTheme.colorScheme.background) {
        Column(Modifier.padding(16.dp)) {
            Text(GameLoop.title.value, fontWeight = FontWeight.Bold, fontSize = 16.sp)
            Text("Firebase game loop: no input needed. Results: Android/data/<package>/files/gameloop/", fontSize = 12.sp)
            LazyColumn(state = state, modifier = Modifier.padding(top = 8.dp)) {
                items(lines) { Text(it, fontFamily = FontFamily.Monospace, fontSize = 11.sp) }
            }
        }
    }
}
