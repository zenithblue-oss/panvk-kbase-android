import java.net.URI
import java.security.MessageDigest

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

// Bundled driver: pinned released binary (src/main/assets/bundled-driver.json), downloaded at build
// time and sha256-checked; never committed. Override with -PpanvkSo=<path> (skips the pin check).
@Suppress("UNCHECKED_CAST")
val bundledDriver = groovy.json.JsonSlurper()
    .parse(file("src/main/assets/bundled-driver.json")) as Map<String, Any?>
@Suppress("UNCHECKED_CAST")
val bundledRelease = bundledDriver["release"] as Map<String, Any?>
val pinnedSha = bundledRelease["sha256"] as String
val pinnedUrl = "https://github.com/${bundledRelease["repo"]}/releases/download/${bundledRelease["tag"]}/${bundledRelease["asset"]}"
val pinnedCache = File(System.getProperty("user.home"), ".cache/panvk-launcher/${pinnedSha}.so")

fun sha256Of(f: File): String {
    val md = MessageDigest.getInstance("SHA-256")
    f.inputStream().use { ins ->
        val buf = ByteArray(1 shl 16)
        while (true) { val n = ins.read(buf); if (n < 0) break; md.update(buf, 0, n) }
    }
    return md.digest().joinToString("") { "%02x".format(it) }
}

val panvkSoProp = providers.gradleProperty("panvkSo").orNull
val panvkSoFile = if (panvkSoProp != null) file(panvkSoProp) else pinnedCache

val checkPanvkSo = tasks.register("checkPanvkSo") {
    inputs.property("panvkSoPath", panvkSoFile.absolutePath)
    inputs.property("pinnedSha", pinnedSha)
    outputs.upToDateWhen { panvkSoFile.exists() && (panvkSoProp != null || sha256Of(panvkSoFile) == pinnedSha) }
    doLast {
        if (panvkSoProp != null) {
            if (!panvkSoFile.exists()) throw GradleException("panvkSo not found: ${panvkSoFile.absolutePath}")
            if (sha256Of(panvkSoFile) != pinnedSha) logger.warn("WARNING: -PpanvkSo sha256 != bundled-driver.json pin ($pinnedSha): the APK label is derived from the .so")
            return@doLast
        }
        if (!(pinnedCache.exists() && sha256Of(pinnedCache) == pinnedSha)) {
            if (bundledRelease["published"] == false) throw GradleException(
                "Bundled driver ${bundledRelease["tag"]} is not published yet; build with -PpanvkSo=<path to sha256 $pinnedSha>")
            pinnedCache.parentFile.mkdirs()
            val tmp = File(pinnedCache.parentFile, "${pinnedSha}.part")
            logger.lifecycle("Downloading bundled driver $pinnedUrl")
            URI(pinnedUrl).toURL().openStream().use { ins -> tmp.outputStream().use { ins.copyTo(it) } }
            val got = sha256Of(tmp)
            if (got != pinnedSha) {
                tmp.delete()
                throw GradleException("Bundled driver sha256 mismatch: expected $pinnedSha, got $got")
            }
            tmp.renameTo(pinnedCache)
        }
    }
}

val copyPanvkSo = tasks.register<Copy>("copyPanvkSo") {
    dependsOn(checkPanvkSo)
    from(panvkSoFile)
    into(file("build/generated/panvkJni/arm64-v8a"))
    rename { "libvulkan_panfrost.so" }
}

tasks.named("preBuild") {
    dependsOn(copyPanvkSo)
}

// The APK's assets/bundled-driver.json is derived from the bundled .so (driverInfo, git sha, sha256, size), as
// PanProbe does, so the PanPlay label can never go stale; src/main/assets/bundled-driver.json stays the pin.
fun indexOfBytes(b: ByteArray, key: ByteArray, from: Int): Int {
    outer@ for (i in from until b.size - key.size) {
        if (b[i] != key[0]) continue
        for (j in key.indices) if (b[i + j] != key[j]) continue@outer
        return i
    }
    return -1
}

// "Mesa 26.3.0-devel (git-<sha>, p<NNNN>, <date>)", a whole NUL-terminated string in .rodata.
fun driverInfoOf(b: ByteArray): String? {
    val re = Regex("""Mesa \d+\.\d+\.\d+[-.0-9A-Za-z]* \(git-[0-9a-f]+(, [^)]*)?\)""")
    val key = "Mesa ".toByteArray()
    var i = indexOfBytes(b, key, 1)
    while (i >= 0) {
        if (b[i - 1] == 0.toByte()) {
            var e = i
            while (e < b.size && b[e] != 0.toByte()) e++
            val s = String(b, i, e - i, Charsets.UTF_8)
            if (re.matches(s)) return s
        }
        i = indexOfBytes(b, key, i + 1)
    }
    return null
}

val mainAssetsDir = layout.buildDirectory.dir("generated/mainAssets")
val syncMainAssets = tasks.register<Sync>("syncMainAssets") {
    dependsOn(checkPanvkSo)
    outputs.upToDateWhen { false }
    into(mainAssetsDir)
    from("src/main/assets") { exclude("bundled-driver.json") }
    doLast {
        val bytes = panvkSoFile.readBytes()
        val info = driverInfoOf(bytes)
            ?: throw GradleException("no driverInfo string in ${panvkSoFile.absolutePath}")
        val git = Regex("""\(git-([0-9a-f]+)""").find(info)!!.groupValues[1]
        val oldGit = bundledDriver["buildId"] as String
        val sha = sha256Of(panvkSoFile)
        val m = LinkedHashMap(bundledDriver)
        m["id"] = "mesa-panvk-${bundledDriver["packageVersion"]}-git-$git"
        m["displayVersion"] = info.removePrefix("Mesa ")
        m["driverVersion"] = info
        m["buildId"] = git
        m["description"] = (bundledDriver["description"] as String).replace(oldGit, git)
        m["release"] = LinkedHashMap(bundledRelease).apply { put("sha256", sha); put("size", panvkSoFile.length()) }
        m["derivedFromSo"] = true
        m["matchesPin"] = (sha == pinnedSha)
        File(mainAssetsDir.get().asFile, "bundled-driver.json")
            .writeText(groovy.json.JsonOutput.prettyPrint(groovy.json.JsonOutput.toJson(m)))
    }
}

tasks.named("preBuild") {
    dependsOn(syncMainAssets)
}

@Suppress("UNCHECKED_CAST")
val gameLoopScenarios = ((groovy.json.JsonSlurper().parse(file("src/main/assets/gameloop.json")) as Map<String, Any?>)
    .getValue("scenarios") as Map<String, Any?>).keys.count { !it.startsWith("#") }

android {
    namespace = "dev.zenithblue.panvklauncher"
    compileSdk = 36
    providers.gradleProperty("panvkBuildTools").orNull?.let { buildToolsVersion = it } // e.g. 36.1.0 on hosts lacking the AGP default
    ndkVersion = "29.0.14206865"

    defaultConfig {
        applicationId = "dev.zenithblue.panvklauncher"
        minSdk = 28
        // targetSdk 28: W^X (targetSdk>=29) blocks execve of wine/wineserver from app data; linker64 fails ("could not exec the wine loader"). Same as Winlator/GameNative legacy.
        targetSdk = 28
        versionCode = 44
        versionName = "1.3.0"
        // Firebase game loop (GameLoop.kt): com.google.test.loops = number of scenarios in assets/gameloop.json.
        manifestPlaceholders["gameLoopScenarios"] = gameLoopScenarios.toString()

        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    // Release APK is the debug build (debug key keeps upgrades working). AGP signs v2-only at minSdk 28;
    // some installers report "invalid package" for that, so also add v1 (JAR) and v3 signatures.
    signingConfigs.getByName("debug") {
        enableV1Signing = true
        enableV2Signing = true
        enableV3Signing = true
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
    }

    // targetSdk 28 stays. targetSdk>=29 W^X blocks execve of wine/wineserver
    // from app-private storage. Sideloaded executable runtime, not a Play target.
    lint {
        disable += "ExpiredTargetSdkVersion"
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
    implementation("androidx.compose.material:material-icons-extended")
    implementation("androidx.activity:activity-compose:1.13.0")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.foundation:foundation")
    implementation("org.apache.commons:commons-compress:1.28.0")
    implementation("org.tukaani:xz:1.10")
    implementation("com.github.luben:zstd-jni:1.5.7-4@aar")
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.json:json:20180813") // real org.json for JVM tests (android.jar's is a stub); not in the APK
}

// Bundled components (rootfs, Proton, FEX, DXVK): pinned in bundled-components.json, copied from
// -PcomponentsDir (default /var/tmp/panvk/components; filled by scripts/fetch-launcher-components.sh,
// build-fex-windows.sh, build-dxvk.sh) into assets/components, sha256-checked; never committed.
// -PbundleComponents=false builds a download-only APK.
val bundleComponents = providers.gradleProperty("bundleComponents").orNull != "false"
val componentsDir = file(providers.gradleProperty("componentsDir").orNull ?: "/var/tmp/panvk/components")
@Suppress("UNCHECKED_CAST")
val bundledComponents = (groovy.json.JsonSlurper().parse(file("bundled-components.json")) as Map<String, Any?>)
    .getValue("components") as List<Map<String, String>>
val componentAssetsDir = layout.buildDirectory.dir("generated/componentAssets")

val copyComponents = tasks.register<Sync>("copyComponents") {
    into(componentAssetsDir.map { it.dir("components") })
    if (bundleComponents) {
        val files = bundledComponents.map { File(componentsDir, it["file"]!!) }
        from(files)
        doFirst {
            for (c in bundledComponents) {
                val f = File(componentsDir, c["file"]!!)
                if (!f.isFile) throw GradleException(
                    "Missing bundled component $f: run scripts/fetch-launcher-components.sh, " +
                        "build-fex-windows.sh, build-dxvk.sh (or -PbundleComponents=false)"
                )
                val got = sha256Of(f)
                if (got != c["sha256"]) throw GradleException("Bundled component $f sha256 $got != pinned ${c["sha256"]}")
            }
        }
    }
}

tasks.named("preBuild") {
    dependsOn(copyComponents)
}

// D3D12 Multithreading test data (SquidRoom.bin, 43 MB, MIT, Microsoft DirectX-Graphics-Samples 0f86e460; see
// tests/dx12/d3d12-multithreading/SOURCES.txt): copied from -Pdx12AssetsDir (default the staging dir below) into
// assets/builtin next to the committed test exes and shaders, sha256-checked; never committed.
// -PbundleSquidRoom=false leaves it out (the SquidRoom test then fails; gears still runs).
val bundleSquidRoom = providers.gradleProperty("bundleSquidRoom").orNull != "false"
val dx12AssetsDir = file(providers.gradleProperty("dx12AssetsDir").orNull ?: "/home/flux/build-cache/dx12-test/d3d12-multithreading")
val squidRoomSha = "02c3d5421b4d4714971bd34e5416e9de619f993510da4c66192b15653858e0aa"
val testAssetsDir = layout.buildDirectory.dir("generated/testAssets")

val copyTestAssets = tasks.register<Sync>("copyTestAssets") {
    into(testAssetsDir.map { it.dir("builtin") })
    if (bundleSquidRoom) {
        from(File(dx12AssetsDir, "SquidRoom.bin"))
        doFirst {
            val f = File(dx12AssetsDir, "SquidRoom.bin")
            if (!f.isFile) throw GradleException("Missing $f (D3D12 Multithreading data, tests/dx12/d3d12-multithreading/SOURCES.txt), or -PbundleSquidRoom=false")
            val got = sha256Of(f)
            if (got != squidRoomSha) throw GradleException("$f sha256 $got != pinned $squidRoomSha")
        }
    }
}

tasks.named("preBuild") {
    dependsOn(copyTestAssets)
}

android {
    sourceSets.getByName("main") {
        // src/main/assets reaches the APK through syncMainAssets (derived bundled-driver.json).
        assets.directories.remove("src/main/assets")
        assets.directories.add(mainAssetsDir.get().asFile.path)
        assets.directories.add(componentAssetsDir.get().asFile.path)
        assets.directories.add(testAssetsDir.get().asFile.path)
    }
    // Archives are already xz/zstd: store them so they are not compressed twice and can be openFd'd.
    androidResources {
        noCompress += listOf("txz", "wcp")
    }
}
