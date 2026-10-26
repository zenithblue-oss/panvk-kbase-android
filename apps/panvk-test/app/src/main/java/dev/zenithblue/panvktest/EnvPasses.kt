package dev.zenithblue.panvktest

import android.content.Context
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest

/**
 * Driver env A/B passes for the Firebase game loop (scenario 3): assets/env-ab.json
 *
 *   {"passes": [{"name": "defaults", "env": {}}, {"name": "114-knobs", "env": {"PANVK_X": "0"}, "tests": "all"}]}
 *
 * One Run all (or the pass's "tests": a comma list of test names) per pass, in order, on the rc5 driver. A pass's
 * env goes into every test / vkinfo child process (buildEnv, after --es env), so each test's driver instance and
 * device read it at init. The in-process swapchain_lifecycle test gets it from the app's own env (set between
 * passes, restored after the last); it loads the driver once per process, so a value the driver caches per process
 * (not per instance or device) keeps its first-pass value in that one test.
 * Keys must look like PANVK_*, MESA_* or PAN_* (as for --es env); others are dropped.
 * No file, a bad file or no passes = one "defaults" pass without env, which runs exactly like scenario 2.
 * Overrides (first found wins, whole file): --es env_ab <path>, <external files>/env-ab.json,
 * /data/local/tmp/panprobe-env-ab.json (Firebase --other-files can push there).
 */
data class EnvPass(val index: Int, val count: Int, val name: String, val env: List<String>, val tests: String?)

object EnvPasses {
    private val KEY = Regex("(PANVK|MESA|PAN)_[A-Za-z0-9_]*")

    /** The passes and where they came from ("assets/env-ab.json", an override path + sha256, or "none"). */
    fun load(ctx: Context, extraPath: String?): Pair<List<EnvPass>, String> {
        val candidates = listOfNotNull(
            extraPath, ctx.getExternalFilesDir(null)?.let { File(it, "env-ab.json").path }, "/data/local/tmp/panprobe-env-ab.json"
        )
        for (p in candidates) {
            val text = try { File(p).takeIf { it.isFile }?.readText() } catch (_: Exception) { null } ?: continue
            return parse(text) to "$p sha256=${sha256(text)}"
        }
        val text = try { ctx.assets.open("env-ab.json").bufferedReader().use { it.readText() } } catch (_: Exception) { null }
            ?: return parse("") to "none"
        return parse(text) to "assets/env-ab.json sha256=${sha256(text)}"
    }

    fun parse(text: String): List<EnvPass> {
        val arr = try { JSONObject(text).optJSONArray("passes") } catch (e: Exception) {
            if (text.isNotBlank()) Log.w("PanProbe", "PANPROBE_ENVAB bad file: $e")
            null
        } ?: JSONArray()
        data class Raw(val name: String, val env: List<String>, val tests: String?)
        val raw = ArrayList<Raw>()
        for (i in 0 until arr.length()) {
            val o = arr.optJSONObject(i) ?: continue
            val name = o.optString("name", "pass${i + 1}").replace(Regex("[^A-Za-z0-9._-]+"), "_").ifEmpty { "pass${i + 1}" }
            val env = ArrayList<String>()
            o.optJSONObject("env")?.let { e ->
                for (k in e.keys()) {
                    if (k.matches(KEY)) env += "$k=${e.get(k)}"
                    else if (!k.startsWith("#")) Log.w("PanProbe", "PANPROBE_ENVAB pass=$name rejected key=$k")
                }
            }
            raw += Raw(name, env.sorted(), o.optString("tests", "").trim().ifEmpty { null })
        }
        if (raw.isEmpty()) raw += Raw("defaults", emptyList(), null)
        return raw.mapIndexed { i, r -> EnvPass(i + 1, raw.size, r.name, r.env, r.tests) }
    }

    fun toJson(p: EnvPass, source: String): JSONObject = JSONObject().put("name", p.name).put("index", p.index)
        .put("count", p.count).put("env", JSONArray(p.env)).put("tests", p.tests ?: "all").put("source", source)

    private fun sha256(s: String) = MessageDigest.getInstance("SHA-256").digest(s.toByteArray()).joinToString("") { "%02x".format(it) }
}
