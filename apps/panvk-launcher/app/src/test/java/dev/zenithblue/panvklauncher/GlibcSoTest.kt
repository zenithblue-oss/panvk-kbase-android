package dev.zenithblue.panvklauncher

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GlibcSoTest {
    @Test fun glibcNeededDetected() {
        assertTrue(DriverUpdate.isGlibcBytes("\u0000libm.so.6\u0000libc.so.6\u0000GLIBC_2.17".toByteArray()))
    }
    @Test fun bifrostDetected() {
        assertTrue(UploadLogs.bifrostMessage("panvk: Unknown gpu_id (0x74021000) or variant (0x2)") != null)
        assertTrue(UploadLogs.bifrostMessage("panvk: gpu_id 0x72120000 variant 0x0") == null)
        assertTrue(UploadLogs.isBifrost("panvk: gpu_id 0x72120000 variant 0x0"))
        assertTrue(UploadLogs.bifrostMessage("panvk: gpu_id 0xb8a31030 variant 0x4 arch v11") == null)
    }
    @Test fun bionicNotFlagged() {
        assertFalse(DriverUpdate.isGlibcBytes("\u0000libc.so\u0000libdl.so\u0000liblog.so".toByteArray()))
    }
}
