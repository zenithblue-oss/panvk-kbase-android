# 122f Test Readiness — 2026-10-30

> **Status: HOLD.** BC and WSI agent reviews in progress. No device tests until reviews clear and a post-fix build is tagged.

---

## 1. Source State

| Item | Value |
|---|---|
| Mesa source | `/var/tmp/panvk/mesa-122f-dev` |
| HEAD | `3e17670cf5c` — `wsi/x11: pick the sw readback memory type per swapchain from its frame rate` |
| Working tree | **clean** (verified `git status` Oct 30) |
| Note | Post-fix source may become dirty; re-verify HEAD before building |

---

## 2. Build State

| Item | Path | Observed |
|---|---|---|
| Build dir | `/var/tmp/panvk/bionic-p122f` | Present; `build.ninja` + `meson-info` intact |
| Dist dir | `/var/tmp/panvk/bionic-p122f-dist` | Present |
| Dist SO | `/var/tmp/panvk/bionic-p122f-dist/libvulkan_panfrost.so` | 30,819,208 B, Oct 30 10:23 |
| Build exit | `/var/tmp/panvk/b122f/build.exit` | `BUILD-EXIT 0 10:23:45` |
| Tagged artifact (last) | `/var/tmp/panvk/build-out/2026-10-30-dev10/libvulkan_panfrost.so` | sha256 prefix `3f1d79bca394222b` |

**Build profile** (`bionic-p122f`, via `scripts/build-android.sh --profile g615-v11-csf`):
- `buildtype=release`, `optimization=3`, `b_ndebug=if-release`
- `vulkan-drivers=['panfrost']`, `panfrost-kmds=['kbase']`
- 1169/1169 targets linked at last build

### Build command (post-fix, assign a new tag)
```sh
export MESA=/var/tmp/panvk/mesa-122f-dev
export BDIR=/var/tmp/panvk/bionic-p122f
export DDIR=/var/tmp/panvk/bionic-p122f-dist
export PANVK_JOBS=4
export HOST_TOOLS=/home/abhaybyte/repos/panvk/tmp/rel9/src/build/host-tools/bin
export LD_LIBRARY_PATH=/var/tmp/panvk/llvm22/usr/lib
export TMPDIR=/var/tmp/panvk/tmp-122f
O=/var/tmp/panvk/b122f
sh /home/abhaybyte/repos/panvk/scripts/build-android.sh --profile g615-v11-csf > $O/build.out 2>&1
# Check: cat $O/build.exit  ->  must show BUILD-EXIT 0
# Tag:   bash scripts/build.sh <newtag>
#        -> /var/tmp/panvk/build-out/2026-10-30-<newtag>/libvulkan_panfrost.so
```

---

## 3. APK Availability

| APK | Status |
|---|---|
| `/var/tmp/panvk/build-out/apks/panprobe-122f.apk` | Present |
| `/var/tmp/panvk/build-out/apks/panprobe-116c.apk` | Present |
| `/var/tmp/panvk/build-out/apks/panplay-122f.apk` | Present |
| `/var/tmp/panvk/build-out/apks/panplay-116c.apk` | Present |
| `/var/tmp/panvk/build-out/apks/panplay-rc4c.apk` | Present |

---

## 4. Device Availability

| Device | Serial | Status |
|---|---|---|
| X6 Pro v11 | `Y5WWBMJVOZSK4HU8` | **Connected** — confirmed `adb devices` Oct 30 |
| S11 | — | **Absent** — not listed in `adb devices`; confirmed by prior tester |
| TB336FU | `HNY07QHT` | **Not verified** — not listed in `adb devices`; serial known only from harness scripts |

TB336FU reachability was not confirmed by bounded `adb devices`. Do not assume available.

---

## 5. Lock Status (observed Oct 30)

| Lock | Status |
|---|---|
| `/var/tmp/panvk/device.lock` | **FREE** |
| `/var/tmp/panvk/tb336fu.lock` | **LOCKED** |
| `/var/tmp/panvk/tablet.lock` | FREE |
| `/var/tmp/panvk/rtl.lock` | FREE |
| `/var/tmp/panvk/firebase.lock` | FREE |
| `/var/tmp/panvk/firebase-slot.lock` | FREE |
| `/var/tmp/panvk/panvk-test-gradle.lock` | FREE |

`tb336fu.lock` held at observation time. TB336FU reachability unverified regardless.

---

## 6. Focused Test Plan (post-review, post-build)

Scope: compile/link verification, then two focused paths on X6 Pro only. No full gate. No performance suites.

### Step 1 — Build (after reviews clear, on post-fix source)
```sh
# As above in §2. Verify BUILD-EXIT 0. Tag the SO.
bash /home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-local-122f/scripts/build.sh <newtag>
```
Success criterion: `build.exit` contains `BUILD-EXIT 0` and SO is present in `build-out/2026-10-30-<newtag>/`.

### Step 2 — BC correctness on X6 Pro (imported slot, cold)

One `bc_decode` pass, cold, via `imp.sh`. Lock: `device.lock`. Serial: `Y5WWBMJVOZSK4HU8`.

```sh
# Push SO to device first (inside lock):
adb -s Y5WWBMJVOZSK4HU8 push \
  /var/tmp/panvk/build-out/2026-10-30-<newtag>/libvulkan_panfrost.so \
  /data/local/tmp/p122f/<newtag>.so

# BC correctness — cold (clears shader cache):
CLEAR=1 bash /var/tmp/panvk/b122f/imp.sh \
  Y5WWBMJVOZSK4HU8 \
  /var/tmp/panvk/device.lock \
  /data/local/tmp/p122f/<newtag>.so \
  bc_decode \
  /var/tmp/panvk/r122f/bc-fix-cold.txt
```
Success criterion: `RESULT bc_decode PASS mismatch=0` in output file.

### Step 3 — WSI auto on X6 Pro (scenario 6)

One play.sh invocation, config `wsi-x6-auto9.json`, scenario 6 (d3d11 + d3d11fl_11_1 + d3d9, 4 envsets). Lock: `device.lock`.

```sh
bash /var/tmp/panvk/b122f/play.sh \
  Y5WWBMJVOZSK4HU8 \
  /var/tmp/panvk/device.lock \
  /home/abhaybyte/repos/panvk/worklogs/g615-dxvk/device-runs/2026-10-30-local-122f/wsi-x6-auto9.json \
  /var/tmp/panvk/r122f/wsi-fix \
  6 <newtag> 122e
```
Success criterion: all passes show `PASS`, `"deviceLost": false`.

---

## 7. Historical Runs (reference only — do not re-run)

Runs were against pre-fix builds. Recorded for context; no diagnosis here.

**X6 Pro WSI auto** (`x6auto1` run `20261030-094341-s6`, `x6auto9` run `20261030-100654-s6`):
- All 8 / 12 passes: PASS, 0 deviceLost.

**TB336FU mem-exe** (`tbmem2` run `20261030-094353-s6`):
- 24 passes: all PASS, 0 deviceLost.
- Device reachability at that time not re-confirmed today.

**PanProbe probe tables** (`b122f/x6f-probe-cold.md`, `x6f-probe-warm.md`, `tbf-probe-cold.md`, `tbf-probe-warm.md`):
- Pre-fix data; flags noted in those files. Owned by respective agents.

**Historical gate** (`b122f/gate-x6.out`, `b122f/gate-tb.out`):
- 122f: pass=41 fail=0 lost=0 on X6; pass=41 fail=0 lost=0 on TB (pre-fix build).
- PanPlay phase terminated early on both runs. Recorded as-is; no diagnosis.

---

## 8. Pre-test Checklist

- [ ] BC agent review cleared
- [ ] WSI agent review cleared
- [ ] Post-fix source committed; `git status` clean; HEAD recorded
- [ ] Build runs to `BUILD-EXIT 0`; SO tagged
- [ ] `device.lock` free before X6 tests
- [ ] X6 Pro `Y5WWBMJVOZSK4HU8` still listed in `adb devices` at test time
- [ ] SO pushed to `/data/local/tmp/p122f/<newtag>.so` on device before `imp.sh`
