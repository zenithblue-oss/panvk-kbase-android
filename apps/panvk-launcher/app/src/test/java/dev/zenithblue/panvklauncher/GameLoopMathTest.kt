// SPDX-License-Identifier: MIT
package dev.zenithblue.panvklauncher

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test
import java.io.File

class GameLoopMathTest {
    private fun asset() = JSONObject(File("src/main/assets/gameloop.json").readText())
    private fun cfg() = GameLoopMath.parseConfig(asset())

    @Test fun shippedConfigScenarios() {
        val c = cfg()
        assertEquals(listOf(1, 2, 3, 4, 5, 6), c.scenarios.keys.sorted())
        assertEquals(20, c.warmupSec); assertEquals(60, c.measureSec); assertEquals(88, c.testSecs)
        assertEquals("bundled", c.driver)
        fun labels(n: Int) = GameLoopMath.passes(c, n).map { it.label }
        assertEquals(listOf("01-d3d11-default"), labels(1))
        assertEquals(listOf("01-d3d9-default"), labels(2))
        assertEquals(listOf("01-d3d11-default", "02-d3d9-default"), labels(3))
        // A/B: each cube with the old-behaviour knobs, then with the defaults.
        assertEquals(listOf("01-d3d11-old", "02-d3d11-default", "03-d3d9-old", "04-d3d9-default"), labels(4))
        assertEquals("builtin-d3d11-arm64ec", c.tasks.getValue("d3d11").shortcut)
        assertEquals("builtin-d3d9-i686", c.tasks.getValue("d3d9").shortcut)
        // D3D12 (vkd3d-proton): SquidRoom, then gears, one pass each with the driver defaults.
        assertEquals(listOf("01-d3d12-default"), labels(5))
        assertEquals(listOf("01-d3d12gears-default"), labels(6))
        assertEquals("builtin-d3d12-arm64ec", c.tasks.getValue("d3d12").shortcut)
        assertEquals("builtin-d3d12gears-arm64ec", c.tasks.getValue("d3d12gears").shortcut)
        // 'old' restores 114 for every default-on knob of staging-115 pa, pb and pc; 'default' sets nothing.
        val old = c.envSets.getValue("old").env
        assertEquals(21, old.size)
        assertEquals("0", old["PANVK_CSF_DESC_DIRTY"]); assertEquals("32", old["PANVK_TESS_VS_REGION_MB"])
        assertEquals("1", old["PANVK_KBASE_ZERO_ALL"]); assertEquals("0", old["PANVK_WSI_SW_CACHED"])
        assertTrue(old.keys.all { it.startsWith("PANVK_") })
        assertNull(old["PANVK_KBASE_COHERENT_POOLS"]); assertNull(old["PANVK_ZS_PRELOAD_ALWAYS"])
        assertTrue(c.envSets.getValue("default").env.isEmpty())
    }

    @Test fun d3d12Patterns() {
        val t = cfg().tasks.getValue("d3d12")
        assertEquals("DX12TEST_SECS", t.secsEnv)
        assertTrue(t.firstFramePattern!!.containsMatchIn("DX12: Present frame=1 hr=0x00000000"))
        assertFalse(t.firstFramePattern.containsMatchIn("DX12: first frame startup=3.2"))
        val pass = GameLoopMath.parseTest(t, sequenceOf("DX12: fps=58.0 frames=59 t=1.0",
            "DX12: PASS test=d3d12-multithreading frames=5104 secs=88.0 fps=57.99 startup=6.41", "exit=0"))
        assertEquals(true, pass.ok); assertEquals(5104L, pass.frames)
        assertEquals(57.99, pass.fps!!, 1e-9); assertEquals(6.41, pass.startupSec!!, 1e-9)
        val failed = GameLoopMath.parseTest(t, sequenceOf("DX12: FAIL D3D12CreateDevice hr=0x887a0004"))
        assertEquals(false, failed.ok); assertNull(failed.fps)
        // The cube's PASS line has no fps or startup field.
        val cube = GameLoopMath.parseTest(cfg().tasks.getValue("d3d11"), sequenceOf("CUBE: PASS api=d3d11 frames=3912"))
        assertNull(cube.fps); assertNull(cube.startupSec)
    }

    @Test fun cubePatterns() {
        val t = cfg().tasks.getValue("d3d11")
        assertTrue(t.firstFramePattern!!.containsMatchIn("CUBE: d3d11 Present frame=0 hr=0x00000000"))
        assertFalse(t.firstFramePattern.containsMatchIn("CUBE: d3d11 create hr=0x00000000 fl=0xb000"))
        val pass = GameLoopMath.parseTest(t, sequenceOf("CUBE: d3d11 Present frame=0 hr=0x00000000", "  CUBE: PASS api=d3d11 frames=3912 (check pixels)", "exit=0"))
        assertEquals(true, pass.ok); assertEquals(3912L, pass.frames)
        val failed = GameLoopMath.parseTest(t, sequenceOf("CUBE: FAIL Present hr=0x887a0005", "CUBE: PASS api=d3d11 frames=1"))
        assertEquals(false, failed.ok)
        assertNull(GameLoopMath.parseTest(t, sequenceOf("wine: crashed")).ok)
    }

    @Test fun passEnvOrderAndHarnessKeys() {
        val j = GameLoopMath.merge(asset(), JSONObject("""{"env":{"A":"common","B":"common"},
            "envSets":{"old":{"env":{"B":"old","PANVK_X":"0"}}},"tasks":{"d3d11":{"env":{"A":"task"}}}}"""))
        val c = GameLoopMath.parseConfig(j)
        val p = GameLoopMath.passes(c, 4)
        val old = GameLoopMath.passEnv(c, p[0], "/x/panvk-fps-%p.log")
        assertEquals("task", old["A"]); assertEquals("old", old["B"]); assertEquals("0", old["PANVK_X"])
        assertEquals("/x/panvk-fps-%p.log", old["PANVK_FPS_LOG"]); assertEquals("1000", old["PANVK_FPS_LOG_MS"])
        assertEquals("88", old["DXCUBE_SECS"])
        val def = GameLoopMath.passEnv(c, p[1], null)
        assertEquals("common", def["B"]); assertNull(def["PANVK_X"]); assertNull(def["PANVK_FPS_LOG"])
        // The merge kept the rest of the task and of the env sets.
        assertEquals("builtin-d3d11-arm64ec", c.tasks.getValue("d3d11").shortcut)
        assertTrue("default" in c.envSets)
    }

    @Test fun passDriverInstallsAndShortPass() {
        val j = GameLoopMath.merge(asset(), JSONObject("""{"installDrivers":{"r114":"drivers/114.so"},"mapsSnapshotSec":1,
            "envSets":{"d114":{"driver":"r114","env":{}},"short":{"env":{"DXCUBE_SECS":"12"}}},
            "scenarios":{"2":{"name":"d3d9","tasks":["d3d9"],"envSets":["default","d114","short"]}}}"""))
        val c = GameLoopMath.parseConfig(j)
        assertEquals(mapOf("r114" to "drivers/114.so"), c.installDrivers); assertEquals(1, c.mapsSnapshotSec)
        val p = GameLoopMath.passes(c, 2)
        assertEquals("", p[0].envSet.driver); assertEquals("r114", p[1].envSet.driver)
        // An env set that sets the test's run time keeps it; the others get the window's.
        assertEquals("88", GameLoopMath.passEnv(c, p[0], null)["DXCUBE_SECS"])
        assertEquals("12", GameLoopMath.passEnv(c, p[2], null)["DXCUBE_SECS"])
        assertEquals(0, cfg().mapsSnapshotSec); assertTrue(cfg().installDrivers.isEmpty())
        try { GameLoopMath.parseConfig(GameLoopMath.merge(asset(), JSONObject("""{"installDrivers":{"../x":"a.so"}}"""))); fail() }
        catch (_: IllegalArgumentException) {}
    }

    @Test fun badConfigs() {
        fun bad(s: String) {
            try { GameLoopMath.parseConfig(JSONObject(s)); fail("accepted: $s") } catch (_: IllegalArgumentException) {}
        }
        bad("""{"tasks":{"a":{"shortcut":"x"}},"scenarios":{"1":{"tasks":["b"]}}}""")
        bad("""{"tasks":{"a":{"shortcut":"x"}},"scenarios":{"1":{"tasks":["a"]},"3":{"tasks":["a"]}}}""")
        bad("""{"tasks":{"a":{"shortcut":"x"}},"scenarios":{"1":{"tasks":["a"],"envSets":["nope"]}}}""")
        bad("""{"tasks":{"a":{}},"scenarios":{"1":{"tasks":["a"]}}}""")
        bad("""{"tasks":{"a":{"shortcut":"x"}},"scenarios":{}}""")
        try { GameLoopMath.passes(cfg(), 9); fail() } catch (_: IllegalArgumentException) {}
    }

    // Lines written by the csf-v11 233 counter in its host test (60, 30 and 45 fps, a 100 ms stall, two swapchains).
    private val fpsLog = """
        # panvk-fps Mesa 26.3.0-devel (git-0123456789) pid=19183 interval_ms=1000 first_present_unix_ms=1792977816430
        fps t_ms=1016 frames=61 fps=60.0 avg_ms=16.67 max_ms=16.67 total=62
        fps t_ms=2033 frames=61 fps=60.0 avg_ms=16.67 max_ms=16.67 total=123
        fps t_ms=3099 frames=59 fps=55.3 avg_ms=18.08 max_ms=100.00 total=182
        fps t_ms=4133 frames=31 fps=30.0 avg_ms=33.33 max_ms=33.33 total=213
        fps t_ms=5099 frames=29 fps=30.0 avg_ms=33.33 max_ms=33.33 total=242
        fps-summary t_ms=5099 frames=241 fps=47.3 avg_ms=21.16 max_ms=100.00 total=242 reason=swapchain
        fps t_ms=6107 frames=44 fps=43.7 avg_ms=22.90 max_ms=52.22 total=286
        fps t_ms=6529 frames=21 fps=49.7 avg_ms=20.11 max_ms=22.22 total=307
        fps-summary t_ms=6529 frames=306 fps=46.9 avg_ms=21.34 max_ms=100.00 total=307 reason=exit
    """.trimIndent()

    @Test fun driverFpsLog() {
        val l = GameLoopMath.parseFpsLog(fpsLog.lineSequence())
        assertEquals("Mesa 26.3.0-devel (git-0123456789)", l.driverInfo)
        // Builds before the upstream-style driverInfo (csf-v11 099 + 233 headers) still parse.
        assertEquals("PanVK-kbase RC5-universal-114-fps (Mesa 26.3.0-devel (git-3071c6dfa2))", GameLoopMath.parseFpsLog(sequenceOf(
            "# panvk-fps PanVK-kbase RC5-universal-114-fps (Mesa 26.3.0-devel (git-3071c6dfa2)) pid=1 interval_ms=1000 first_present_unix_ms=1")).driverInfo)
        assertEquals(7, l.lines.size); assertEquals(2, l.summaries.size); assertEquals(307L, l.total)
        // Intervals fully inside [1000, 5100] ms: 1016-2033 .. 4133-5099.
        val w = GameLoopMath.driverWindow(l, 1000, 5100)!!
        assertEquals(180L, w.frames); assertEquals(4083L, w.spanMs); assertEquals(4, w.intervals)
        assertEquals(180 * 1000.0 / 4083, w.fps, 1e-9)
        assertEquals(30.0, w.minIntervalFps, 0.0); assertEquals(60.0, w.maxIntervalFps, 0.0); assertEquals(100.0, w.maxFrameMs, 0.0)
        // Whole file: every interval; the fps lines add up to the exit summary.
        val all = GameLoopMath.driverWindow(l, 0, Long.MAX_VALUE)!!
        assertEquals(306L, all.frames); assertEquals(6529L, all.spanMs)
        assertNull(GameLoopMath.driverWindow(l, 7000, 9000))
        assertNull(GameLoopMath.parseFpsLog(sequenceOf("garbage", "fps t_ms=x")).driverInfo)
    }

    @Test fun dxvkWindow() {
        // 100 frames at 10 ms, then 100 at 20 ms; DXVK timestamps start at an arbitrary offset.
        val t = ArrayList<Long>(); val f = ArrayList<Long>()
        var now = 5_000_000L
        repeat(100) { now += 10_000; t += now; f += 10_000 }
        repeat(100) { now += 20_000; t += now; f += 20_000 }
        // First row is t0; window (t0 + 1.0 s, t0 + 2.0 s]: frames 2..3 s after start at 20 ms.
        val w = GameLoopMath.dxvkWindow(t.toLongArray(), f.toLongArray(), 1000, 2000)!!
        assertEquals(50, w.count); assertEquals(50.0, w.avgFps, 1e-9); assertEquals(20.0, w.p50Ms, 0.0)
        val first = GameLoopMath.dxvkWindow(t.toLongArray(), f.toLongArray(), 0, 500)!!
        assertEquals(50, first.count); assertEquals(100.0, first.avgFps, 1e-9)
        assertNull(GameLoopMath.dxvkWindow(LongArray(0), LongArray(0), 0, 1000))
        assertNull(GameLoopMath.dxvkWindow(t.toLongArray(), f.toLongArray(), 10_000, 20_000))
    }

    @Test fun deltaAndLabels() {
        assertEquals(10.0, GameLoopMath.deltaPct(40.0, 44.0)!!, 1e-9)
        assertNull(GameLoopMath.deltaPct(null, 44.0)); assertNull(GameLoopMath.deltaPct(0.0, 44.0))
        assertEquals("a_b", GameLoopMath.safe("a b"))
        val m = GameLoopMath.merge(JSONObject("""{"a":{"x":1,"y":2},"b":[1]}"""), JSONObject("""{"a":{"y":3},"b":[2]}"""))
        assertEquals(1, m.getJSONObject("a").getInt("x")); assertEquals(3, m.getJSONObject("a").getInt("y"))
        assertEquals(2, m.getJSONArray("b").getInt(0))
        assertNotNull(cfg().envSets["old"])
    }
}
