// SPDX-License-Identifier: MIT
package dev.zenithblue.panvklauncher

import org.json.JSONArray
import org.json.JSONObject

/**
 * Pure part of the Firebase game-loop harness ([GameLoop]): config (assets/gameloop.json), scenario -> passes,
 * and the fps math over the measured window. No Android imports: unit-tested on the JVM (GameLoopMathTest).
 *
 * Config shape (see assets/gameloop.json):
 *   warmupSec / measureSec / tailSec / startTimeoutSec / gapSec / fpsLogMs / screenshotAtSec / driver,
 *   env {..}                      every pass,
 *   envSets {id: {note, env}}     one env set per pass (A/B: "old" = perf knobs at their old behaviour, "default"),
 *   tasks {id: {title, shortcut, env, secsEnv, firstFramePattern, okPattern, failPattern}},
 *   scenarios {"1".."N": {name, tasks [..], envSets [..] (default ["default"])}}.
 * A scenario runs, for each task in order, one pass per env set in order. Scenario numbers are 1..N without gaps
 * (Firebase runs --scenario-numbers out of com.google.test.loops = N, which the build takes from this file).
 */
object GameLoopMath {
    data class EnvSet(val id: String, val note: String, val env: Map<String, String>)

    data class Task(
        val id: String, val title: String, val shortcut: String, val env: Map<String, String>,
        /** Env var that sets the test's own run time (the cube's DXCUBE_SECS); "" = harness stops it. */
        val secsEnv: String,
        /** Test stdout line that marks its first frame (else: the driver fps log appearing). */
        val firstFramePattern: Regex?,
        val okPattern: Regex?, val failPattern: Regex?
    )

    data class Scenario(val number: Int, val name: String, val tasks: List<String>, val envSets: List<String>)

    data class Config(
        val warmupSec: Int, val measureSec: Int, val tailSec: Int, val startTimeoutSec: Int, val gapSec: Int,
        val fpsLogMs: Int, val screenshotAtSec: Int,
        /** "bundled" (the APK's driver), "selected" (the launcher's choice) or a driver id. */
        val driver: String,
        val env: Map<String, String>, val envSets: Map<String, EnvSet>, val tasks: Map<String, Task>,
        val scenarios: Map<Int, Scenario>
    ) {
        /** What a self-timed test is told to run: warm-up + measured window + tail, from its first frame. */
        val testSecs get() = warmupSec + measureSec + tailSec
    }

    data class Pass(val index: Int, val task: Task, val envSet: EnvSet) {
        /** Directory name of this pass: 01-d3d11-default. */
        val label get() = "%02d-%s-%s".format(index, safe(task.id), safe(envSet.id))
    }

    fun safe(s: String) = s.replace(Regex("[^A-Za-z0-9._-]+"), "_")

    /** Deep merge: objects merge key by key, anything else in [over] replaces [base]. Inputs are not changed. */
    fun merge(base: JSONObject, over: JSONObject): JSONObject {
        val out = JSONObject(base.toString())
        for (k in over.keys()) {
            val o = over.get(k)
            val b = out.opt(k)
            out.put(k, if (o is JSONObject && b is JSONObject) merge(b, o) else o)
        }
        return out
    }

    private fun strMap(o: JSONObject?): Map<String, String> {
        if (o == null) return emptyMap()
        val m = LinkedHashMap<String, String>()
        for (k in o.keys()) if (!k.startsWith("#")) m[k] = o.get(k).toString()
        return m
    }

    private fun strList(a: JSONArray?): List<String> = if (a == null) emptyList() else (0 until a.length()).map { a.getString(it) }

    private fun regex(s: String): Regex? = s.takeIf { it.isNotEmpty() }?.let { Regex(it) }

    /** Throws IllegalArgumentException with a readable message on a bad file. Keys starting with "#" are comments. */
    fun parseConfig(j: JSONObject): Config {
        val envSets = LinkedHashMap<String, EnvSet>()
        j.optJSONObject("envSets")?.let { o ->
            for (k in o.keys()) {
                if (k.startsWith("#")) continue
                val e = o.getJSONObject(k)
                envSets[k] = EnvSet(k, e.optString("note", ""), strMap(e.optJSONObject("env")))
            }
        }
        if ("default" !in envSets) envSets["default"] = EnvSet("default", "driver defaults", emptyMap())
        val tasks = LinkedHashMap<String, Task>()
        j.optJSONObject("tasks")?.let { o ->
            for (k in o.keys()) {
                if (k.startsWith("#")) continue
                val t = o.getJSONObject(k)
                val sc = t.optString("shortcut", "")
                require(sc.isNotEmpty()) { "task $k: no shortcut" }
                tasks[k] = Task(
                    k, t.optString("title", k), sc, strMap(t.optJSONObject("env")), t.optString("secsEnv", ""),
                    regex(t.optString("firstFramePattern", "")), regex(t.optString("okPattern", "")),
                    regex(t.optString("failPattern", ""))
                )
            }
        }
        val scenarios = LinkedHashMap<Int, Scenario>()
        j.optJSONObject("scenarios")?.let { o ->
            for (k in o.keys()) {
                if (k.startsWith("#")) continue
                val n = k.toIntOrNull() ?: throw IllegalArgumentException("scenario key '$k' is not a number")
                val s = o.getJSONObject(k)
                val ts = strList(s.optJSONArray("tasks"))
                val es = strList(s.optJSONArray("envSets")).ifEmpty { listOf("default") }
                require(ts.isNotEmpty()) { "scenario $n: no tasks" }
                for (t in ts) require(t in tasks) { "scenario $n: unknown task '$t'" }
                for (e in es) require(e in envSets) { "scenario $n: unknown env set '$e'" }
                scenarios[n] = Scenario(n, s.optString("name", "scenario$n"), ts, es)
            }
        }
        require(scenarios.isNotEmpty()) { "no scenarios" }
        require(scenarios.keys.sorted() == (1..scenarios.size).toList()) { "scenario numbers must be 1..${scenarios.size}" }
        fun int(k: String, d: Int, min: Int) = j.optInt(k, d).coerceAtLeast(min)
        return Config(
            warmupSec = int("warmupSec", 20, 0), measureSec = int("measureSec", 60, 1), tailSec = int("tailSec", 8, 0),
            startTimeoutSec = int("startTimeoutSec", 300, 10), gapSec = int("gapSec", 5, 0),
            fpsLogMs = int("fpsLogMs", 1000, 100), screenshotAtSec = j.optInt("screenshotAtSec", -1),
            driver = j.optString("driver", "bundled").ifEmpty { "bundled" },
            env = strMap(j.optJSONObject("env")), envSets = envSets, tasks = tasks, scenarios = scenarios
        )
    }

    /** Passes of [scenario] in run order: each task, then each env set. Throws on an unknown scenario. */
    fun passes(cfg: Config, scenario: Int): List<Pass> {
        val s = cfg.scenarios[scenario] ?: throw IllegalArgumentException(
            "unknown scenario $scenario (have ${cfg.scenarios.keys.sorted().joinToString()})"
        )
        var i = 0
        return s.tasks.flatMap { t -> s.envSets.map { e -> Pass(++i, cfg.tasks.getValue(t), cfg.envSets.getValue(e)) } }
    }

    /**
     * Env of one pass, later wins: common env, task env, env set, then what the harness itself needs (fps log path,
     * interval, the test's run time). DXVK_HUD and DISPLAY are owned by ContainerManager (HUD stays "full").
     */
    fun passEnv(cfg: Config, p: Pass, fpsLogPath: String?): Map<String, String> {
        val m = LinkedHashMap<String, String>()
        m.putAll(cfg.env); m.putAll(p.task.env); m.putAll(p.envSet.env)
        if (fpsLogPath != null) {
            m["PANVK_FPS_LOG"] = fpsLogPath
            m["PANVK_FPS_LOG_MS"] = cfg.fpsLogMs.toString()
        }
        if (p.task.secsEnv.isNotEmpty()) m[p.task.secsEnv] = cfg.testSecs.toString()
        return m
    }

    // ---- driver fps log (csf-v11 233, PANVK_FPS_LOG) ----

    /** One "fps" line: the interval that ended [tMs] after the first present. */
    data class FpsLine(val tMs: Long, val frames: Long, val fps: Double, val avgMs: Double, val maxMs: Double, val total: Long)

    class FpsLog(val header: String?, val lines: List<FpsLine>, val summaries: List<String>) {
        /** Presents counted in the whole file (last line's total). */
        val total get() = lines.lastOrNull()?.total ?: 0L
        /** "PanVK-kbase <release> (Mesa ...)" from the "# panvk-fps" header. */
        val driverInfo get() = header?.let { Regex("# panvk-fps (.*?) pid=").find(it)?.groupValues?.get(1) }
    }

    private val KV = Regex("(\\w+)=(\\S+)")

    fun parseFpsLog(lines: Sequence<String>): FpsLog {
        var header: String? = null
        val out = ArrayList<FpsLine>()
        val sums = ArrayList<String>()
        for (l in lines) {
            when {
                l.startsWith("# panvk-fps") -> if (header == null) header = l
                l.startsWith("fps-summary ") -> sums += l
                l.startsWith("fps ") -> try {
                    val kv = KV.findAll(l).associate { it.groupValues[1] to it.groupValues[2] }
                    out += FpsLine(
                        kv.getValue("t_ms").toLong(), kv.getValue("frames").toLong(), kv.getValue("fps").toDouble(),
                        kv.getValue("avg_ms").toDouble(), kv.getValue("max_ms").toDouble(), kv.getValue("total").toLong()
                    )
                } catch (_: Exception) {}
            }
        }
        return FpsLog(header, out, sums)
    }

    data class WindowStats(
        val fps: Double, val frames: Long, val spanMs: Long, val intervals: Int,
        val minIntervalFps: Double, val maxIntervalFps: Double, val maxFrameMs: Double
    )

    /**
     * The intervals that lie inside [fromMs, toMs] (ms after the first present): fps = their frames over their
     * time, plus the slowest / fastest interval and the longest frame. Null if no interval fits.
     */
    fun driverWindow(log: FpsLog, fromMs: Long, toMs: Long): WindowStats? {
        var prev = 0L
        var frames = 0L; var span = 0L; var n = 0
        var minF = Double.MAX_VALUE; var maxF = 0.0; var maxMs = 0.0
        for (l in log.lines) {
            val start = prev
            prev = l.tMs
            if (start < fromMs || l.tMs > toMs || l.tMs <= start) continue
            frames += l.frames; span += l.tMs - start; n++
            minF = minOf(minF, l.fps); maxF = maxOf(maxF, l.fps); maxMs = maxOf(maxMs, l.maxMs)
        }
        if (n == 0 || span <= 0) return null
        return WindowStats(frames * 1000.0 / span, frames, span, n, minF, maxF, maxMs)
    }

    // ---- DXVK stats (DXVK_STATS_FILE, runtimes/dxvk/panplay-stats.patch) ----

    /**
     * Frame times of the presents [fromMs, toMs] after the first one ("F" rows: timestamp_us, frametime_us).
     * Null if the window has no frame.
     */
    fun dxvkWindow(frameT: LongArray, frameUs: LongArray, fromMs: Long, toMs: Long): PerfMath.Frames? {
        if (frameT.isEmpty()) return null
        val t0 = frameT[0]
        val lo = t0 + fromMs * 1000; val hi = t0 + toMs * 1000
        val sel = ArrayList<Long>()
        for (i in frameT.indices) if (frameT[i] > lo && frameT[i] <= hi) sel += frameUs[i]
        return PerfMath.frames(sel.toLongArray())
    }

    // ---- test stdout (the cube: "CUBE: PASS api=d3d11 frames=N", "CUBE: FAIL <what> hr=0x...") ----

    data class TestResult(val ok: Boolean?, val line: String?, val frames: Long?)

    /** ok = true on an okPattern line, false on a failPattern line (the first FAIL wins), null if neither showed. */
    fun parseTest(task: Task, lines: Sequence<String>): TestResult {
        var ok: TestResult? = null
        for (l in lines) {
            val s = l.trim()
            if (task.failPattern?.containsMatchIn(s) == true) return TestResult(false, s, null)
            if (ok == null && task.okPattern?.containsMatchIn(s) == true)
                ok = TestResult(true, s, Regex("frames=(\\d+)").find(s)?.groupValues?.get(1)?.toLongOrNull())
        }
        return ok ?: TestResult(null, null, null)
    }

    /** [b] relative to [a] in percent (+ = faster), or null. */
    fun deltaPct(a: Double?, b: Double?): Double? = if (a == null || b == null || a <= 0.0) null else (b / a - 1.0) * 100.0
}
