#!/usr/bin/env python3
"""gal115.py <out dir>: build the build 115 gallery spec from summary115.jsonl and run gallery115.py."""
import json
import os
import subprocess
import sys

sys.path.insert(0, "/tmp/claude-1000/-home-flux-repos-panvk-kbase-android/db381bd6-e816-4c4b-b498-8633cd891ac4/scratchpad")
import fl115  # noqa: E402

SP = "/tmp/claude-1000/-home-flux-repos-panvk-kbase-android/db381bd6-e816-4c4b-b498-8633cd891ac4/scratchpad"
DR = "/home/flux/repos/panvk-kbase-android/worklogs/g615-dxvk/device-runs"
ORDER = [("starlte", "v6"), ("a31", "v7"), ("oriole", "v9"), ("panther", "v10"), ("shiba", "v11")]
recs = {}
for line in open(SP + "/summary115.jsonl"):
    r = json.loads(line)
    recs[(r["model"], r["kind"])] = r  # the last record of a run wins

CAP = {
    "video-start.png": "video 30 s in",
    "video-mid.png": "video middle",
    "video-end.png": "video end",
    "d3d11-default-harness.png": "D3D11 cube (DXVK), harness capture",
    "d3d9-default-harness.png": "D3D9 cube (DXVK), harness capture",
    "d3d9-default-hud.png": "D3D9 cube: DXVK HUD close-up",
    "d3d12-squidroom-harness.png": "D3D12 Multithreading (SquidRoom, vkd3d-proton), harness capture",
    "d3d12gears-harness.png": "vkd3d gears (D3D12), harness capture",
    "video-d3d11-cube.png": "D3D11 cube, video frame (no harness capture)",
    "video-d3d9-cube.png": "D3D9 cube, video frame (no harness capture)",
    "video-d3d12gears.png": "vkd3d gears (D3D12), video frame (no harness capture)",
    "video-d3d12-squidroom.png": "D3D12 SquidRoom, video frame (no harness capture)",
}


def cell(text, ok=None):
    return {"text": text, "cls": "" if ok is None else ("ok" if ok else "bad")}


rows, devices = [], []
for model, arch in ORDER:
    p, g = recs.get((model, "probe")), recs.get((model, "play"))
    name = (p or g or {}).get("name", model)
    row = [f"{model} ({name})", arch]
    for i in range(2):
        ps = (p or {}).get("passes") or []
        if i < len(ps):
            x = ps[i]
            allpass = x["counts"].get("PASS", 0) == x["tests"]
            row.append(cell(f"{x['counts'].get('PASS', 0)}/{x['tests']}, {x['total']} s", allpass))
        else:
            row.append(cell("not run" if not p else "no data", False if p else None))
    tasks = (g or {}).get("tasks") or []
    for want in ("d3d11", "d3d9", "d3d12", "d3d12gears"):
        t = next((t for t in tasks if t["task"] == want), None)
        if not t:
            row.append(cell("not run" if not g else "not run (no result)", False if g else None))
            continue
        ok = t["status"] == "ok"
        fps = t["testFps"] if t["testFps"] is not None else t["fps"]
        row.append(cell((f"{fps:.1f} fps" if isinstance(fps, (int, float)) else "PASS") if ok else
                        f"FAIL ({(t.get('reason') or '')[:40]})", ok))
    probe_dir = os.path.join(DR, f"2026-10-26-ftl-{arch}-{model}-115")
    fl = fl115.analyse(probe_dir) if p else None
    row.append(cell(f"{fl['fl']} / SM {fl['sm'].replace('_', '.')}" if fl else "-"))
    rows.append(row)
    imgs = []
    for kind, rec, sub in (("probe", p, f"2026-10-26-ftl-{arch}-{model}-115"), ("play", g, f"2026-10-26-ftl-{arch}-{model}-115-panplay")):
        if not rec:
            continue
        shots = os.path.join(DR, sub, "screenshots")
        for f in sorted(os.listdir(shots)) if os.path.isdir(shots) else []:
            if not f.endswith(".png"):
                continue
            label = ("PanProbe: " if kind == "probe" else "PanPlay: ") + CAP.get(f, f)
            if kind == "probe" and f == "video-end.png":
                label += ", after pass 2 (114-knobs)"
            dst = f"{kind}-{f}"
            imgs.append({"src": os.path.join(shots, f), "caption": label, "name": dst})
    devices.append({"id": model, "name": f"{model}: {name} ({arch})",
                    "sub": " · ".join(x for x in [p and f"PanProbe {p.get('matrix')}", g and f"PanPlay {g.get('matrix')}"] if x),
                    "images": imgs})

spec = {"title": "PanVK build 115 on Firebase Test Lab",
        "sub": "Mesa 26.3.0-devel (git-9dde9e7a18), driver sha256 73709644. PanProbe 1.3.0: env A/B, pass 1 defaults, "
               "pass 2 114-knobs. PanPlay 1.3.0 game loop: DXVK cubes and two D3D12 tests. One Firebase run at a time.",
        "columns": ["Device", "Arch", "PanProbe defaults", "PanProbe 114-knobs", "D3D11 cube", "D3D9 cube", "D3D12 SquidRoom",
                    "D3D12 gears", "D3D12 max FL / SM"],
        "rows": rows,
        "notes": ["Times are the sum of the PanProbe test times per pass. Cube fps is the driver's present rate over the "
                  "60 s window (D3D9 runs with vsync). D3D12 fps is the test's own number.",
                  "Max FL / SM: vkd3d-proton 2230755's own rules applied to the PanProbe Vulkan caps (its release build does not log the level). 12_0 needs sparse residency (tiled resources tier 2) on every device; 12_1 also needs VK_EXT_fragment_shader_interlock and VK_EXT_conservative_rasterization (shiba has the latter).",
                  "a31 (v7): D3D12 gears prints PASS but draws only the red gear; SquidRoom is correct at 0.9 fps. panther (v10): the cube and gears failures are a GPU queue hang (kbase subqueue timeout). starlte (v6): Wine prefix setup fails on Android 10, so no D3D test ran.",
                  "Source: worklogs/g615-dxvk/device-runs/2026-10-26-ftl-<arch>-<model>-115[-panplay]/results.md"],
        "devices": devices}
json.dump(spec, open(SP + "/gallery115-spec.json", "w"), indent=1)
# gallery115.py copies by basename: give each image a unique name first
stage = os.path.join(SP, "gallery115-stage")
os.makedirs(stage, exist_ok=True)
for d in spec["devices"]:
    for im in d["images"]:
        s = os.path.join(stage, d["id"] + "-" + im.pop("name"))
        subprocess.run(["cp", im["src"], s], check=True)
        im["src"] = s
json.dump(spec, open(SP + "/gallery115-spec.json", "w"), indent=1)
subprocess.run([sys.executable, SP + "/gallery115.py", SP + "/gallery115-spec.json", sys.argv[1]], check=True)
