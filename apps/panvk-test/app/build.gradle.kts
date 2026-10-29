import java.security.MessageDigest

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

// Two bundled drivers: RC4 (release build) and RC5 (built here). -PpanvkSoRc4 / -PpanvkSoRc5 override the paths.
val panvkSoRc4 = file(providers.gradleProperty("panvkSoRc4").orElse("/home/flux/build-out/rc4/libvulkan_panfrost-android-aarch64.so").get())
val panvkSoRc5 = file(providers.gradleProperty("panvkSoRc5").orElse(
    providers.gradleProperty("panvkSo").orElse("/home/flux/build-out/2026-10-21-v7/libvulkan_panfrost.so").get()).get())
val panvkSlots = linkedMapOf("rc4" to panvkSoRc4, "rc5" to panvkSoRc5)

fun sha256Of(f: File): String = MessageDigest.getInstance("SHA-256")
    .digest(f.readBytes()).joinToString("") { "%02x".format(it) }

val pinnedJsonFile = file("../bundled-drivers.json")
val genAssetsDir = file("build/generated/panvkAssets")
val genJniDir = file("build/generated/panvkJni/arm64-v8a")

fun gnuBuildId(b: ByteArray): String? {
    val pat = byteArrayOf(4, 0, 0, 0, 0x14, 0, 0, 0, 3, 0, 0, 0, 'G'.code.toByte(), 'N'.code.toByte(), 'U'.code.toByte(), 0)
    val lim = minOf(b.size - 36, 1 shl 20)
    outer@ for (i in 0 until lim) {
        for (j in pat.indices) if (b[i + j] != pat[j]) continue@outer
        return (0 until 20).joinToString("") { "%02x".format(b[i + 16 + it]) }
    }
    return null
}

fun indexOfBytes(b: ByteArray, key: ByteArray, from: Int): Int {
    outer@ for (i in from until b.size - key.size) {
        if (b[i] != key[0]) continue
        for (j in key.indices) if (b[i + j] != key[j]) continue@outer
        return i
    }
    return -1
}

fun cStringAt(b: ByteArray, i: Int): String {
    var e = i
    while (e < b.size && b[e] != 0.toByte()) e++
    return String(b, i, e - i, Charsets.UTF_8)
}

// The driverInfo string the .so reports. Builds from p115-int on report it like upstream Mesa,
// "Mesa 26.3.0-devel (git-6a5d4e2756)": a whole NUL-terminated string (the fps-log header that also contains it
// starts with "# panvk-fps "). RC4 up to rc5g-114 reported "PanVK-kbase <release> (Mesa ...)".
fun driverInfoOf(b: ByteArray): String? {
    // Also "(git-<sha>, p<NNNN>, <date>)" (jm-v7-wip 0154).
    val upstream = Regex("""Mesa \d+\.\d+\.\d+[-.0-9A-Za-z]* \(git-[0-9a-f]+(, [^)]*)?\)""")
    val mesa = "Mesa ".toByteArray()
    var i = indexOfBytes(b, mesa, 1)
    while (i >= 0) {
        if (b[i - 1] == 0.toByte()) {
            val s = cStringAt(b, i)
            if (upstream.matches(s)) return s
        }
        i = indexOfBytes(b, mesa, i + 1)
    }
    val legacy = indexOfBytes(b, "PanVK-kbase ".toByteArray(), 0)
    return if (legacy >= 0) cStringAt(b, legacy) else null
}

// Copies each driver under a distinct name, and generates the bundled-driver.json asset from the REAL .so files
// (sha256, size, build-id, driverInfo string) so the label can never go stale; warns when a .so differs from the pin.
val prepareBundledDrivers = tasks.register("prepareBundledDrivers") {
    inputs.property("paths", panvkSlots.values.joinToString { it.absolutePath })
    inputs.files(pinnedJsonFile)
    outputs.upToDateWhen { false }
    doLast {
        @Suppress("UNCHECKED_CAST")
        val pinned = groovy.json.JsonSlurper().parse(pinnedJsonFile) as Map<String, Any?>
        @Suppress("UNCHECKED_CAST")
        val drivers = pinned["drivers"] as Map<String, Map<String, Any?>>
        genJniDir.deleteRecursively(); genJniDir.mkdirs()
        genAssetsDir.mkdirs()
        val outDrivers = linkedMapOf<String, Any?>()
        for ((slot, so) in panvkSlots) {
            if (!so.exists()) throw GradleException("Bundled driver $slot not found at ${so.absolutePath} (-PpanvkSo${slot.replaceFirstChar { it.uppercase() }}=<path>)")
            val pin = drivers[slot] ?: throw GradleException("no pinned entry for $slot in $pinnedJsonFile")
            val bytes = so.readBytes()
            val sha = sha256Of(so)
            @Suppress("UNCHECKED_CAST")
            val pinSha = (pin["release"] as Map<String, Any?>)["sha256"]
            val info = driverInfoOf(bytes)
            val m = LinkedHashMap(pin)
            m["buildId"] = gnuBuildId(bytes)
            m["driverInfo"] = info
            m["release"] = mapOf("sha256" to sha, "size" to so.length(), "pinnedSha256" to pinSha)
            m["matchesPin"] = (sha == pinSha)
            if (sha != pinSha) {
                logger.warn("WARNING: $slot .so ${so.absolutePath} sha256 $sha != pinned $pinSha in bundled-drivers.json; " +
                    "the APK asset is derived from the actual .so (driverInfo='$info'), not from the pin.")
            }
            so.copyTo(File(genJniDir, pin["soName"] as String), overwrite = true)
            outDrivers[slot] = m
        }
        val out = linkedMapOf<String, Any?>("packageVersion" to pinned["packageVersion"], "derivedFromSo" to true, "drivers" to outDrivers)
        File(genAssetsDir, "bundled-driver.json").writeText(groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson(out)))
    }
}

tasks.named("preBuild") {
    dependsOn(prepareBundledDrivers)
}

android {
    namespace = "dev.zenithblue.panvktest"
    compileSdk = 36
    providers.gradleProperty("panvkBuildTools").orNull?.let { buildToolsVersion = it } // e.g. 36.1.0 on hosts lacking the AGP default
    ndkVersion = "29.0.14206865"

    defaultConfig {
        applicationId = "dev.zenithblue.panvktest"
        // -PappIdSuffix=.foo installs a side-by-side copy (own data, own driver).
        providers.gradleProperty("appIdSuffix").orNull?.let { applicationIdSuffix = it }
        // -PpanprobeSuffix=v7A (with -PpanvkSoRc5=<variant .so>): side-by-side per-variant test APK, id suffix ".v7a".
        providers.gradleProperty("panprobeSuffix").orNull?.let { applicationIdSuffix = "." + it.lowercase().replace(Regex("[^a-z0-9_]"), "_") }
        minSdk = 29
        targetSdk = 36
        versionCode = 20
        versionName = "1.3.1"

        ndk {
            abiFilters.add("arm64-v8a")
        }
        externalNativeBuild {
            cmake {
                // Newer Vulkan headers than the NDK's (tests need VK_HEADER_VERSION >= ~300).
                providers.gradleProperty("panvkVulkanInclude").orNull?.let { arguments("-DVULKAN_INCLUDE_DIR=$it") }
            }
        }
    }

    buildTypes {
        debug {
            isDebuggable = true
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildFeatures {
        compose = true
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets.getByName("main") {
        jniLibs.directories.add("build/generated/panvkJni")
        assets.directories.add("build/generated/panvkAssets")
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2026.02.01")
    implementation(composeBom)
    implementation("androidx.compose.material3:material3")
    implementation("androidx.activity:activity-compose:1.13.0")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.foundation:foundation")
}
