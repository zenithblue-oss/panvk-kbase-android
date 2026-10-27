package dev.zenithblue.panvklauncher

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class DriverUpdateTagTest {
    private fun release(tag: String) =
        """{"tag_name":"$tag","draft":false,"html_url":"u","body":"b","assets":[{"name":"libvulkan_panfrost-android-aarch64.so","browser_download_url":"https://x/y.so","size":1}]}"""

    @Test fun rcTagsParse() {
        assertNotNull(DriverUpdate.versionKey("0.1.0-rc4"))
        assertNotNull(DriverUpdate.versionKey("0.1.0-rc4-lowmem"))
        assertEquals(DriverUpdate.versionKey("0.1.0-rc4"), DriverUpdate.versionKey("0.1.0-rc4-lowmem"))
        assertEquals("rc4", DriverUpdate.label("0.1.0-rc4-lowmem"))
    }
    @Test fun rcOrdering() {
        assertTrue(DriverUpdate.isNewer("0.1.0-rc4", "0.1.0-rc3"))
        assertFalse(DriverUpdate.isNewer("0.1.0-rc3", "0.1.0-rc4"))
        assertTrue(DriverUpdate.isNewer("0.1.0", "0.1.0-rc4"))
    }
    @Test fun newestPicksRc4() {
        val json = "[" + listOf("panvk-kbase-v0.1.0-rc3", "panvk-kbase-v0.1.0-rc4", "panvk-kbase-v0.1.0-rc4-lowmem").joinToString(",") { release(it) } + "]"
        val n = DriverUpdate.newest(json)
        assertNotNull(n)
        assertEquals("panvk-kbase-v0.1.0-rc4", n!!.first.tag)
    }
    @Test fun mesaStyleInstalledVersion() {
        // bundled-driver.json packageVersion from build 115 on: shown as is; the RC releases are not offered as updates.
        assertEquals("26.3.0-devel", DriverUpdate.label("26.3.0-devel"))
        assertFalse(DriverUpdate.isNewer("0.1.0-rc4-lowmem", "26.3.0-devel"))
        assertTrue(DriverUpdate.isNewer("26.4.0-devel", "26.3.0-devel"))
    }
}
