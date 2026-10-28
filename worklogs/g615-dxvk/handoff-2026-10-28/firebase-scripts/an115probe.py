"""an115probe.py <run dir 115pre> <run dir 114>: PanProbe scenario 3 (env A/B) analysis for 115pre on FTL."""
import glob
import json
import re
import sys
from collections import Counter, OrderedDict

run, run114 = sys.argv[1], sys.argv[2]


def one(pat):
    g = sorted(glob.glob(pat, recursive=True))
    return g[0] if g else None


def passes_of(autorun):
    """autorun.txt -> OrderedDict name -> OrderedDict test -> (status, ms, line)."""
    out = OrderedDict()
    cur = "single"
    for line in open(autorun, errors="replace"):
        m = re.match(r"GAMELOOP pass=(\d+)/(\d+) name=(\S+)", line)
        if m:
            cur = m.group(3)
            out.setdefault(cur, OrderedDict())
            continue
        m = re.match(r"RESULT (\S+) (\S+) .*?ms=(\d+)", line)
        if m:
            out.setdefault(cur, OrderedDict())[m.group(1)] = (m.group(2), int(m.group(3)), line.strip())
    return out


def hashes(line):
    return re.findall(r"opt=(\S+) gs=(\d) L=(\S+) .*?hash=([0-9a-f]+)", line or "")


a = one(run + "/raw/*/artifacts/**/autorun.txt")
b = one(run114 + "/raw/*/artifacts/**/autorun.txt")
P = passes_of(a)
Q = passes_of(b)
base = list(Q.values())[0]
print("== passes")
for name, tests in P.items():
    c = Counter(v[0] for v in tests.values())
    print(name, dict(c), "tests", len(tests), "total ms", sum(v[1] for v in tests.values()))
print("114:", dict(Counter(v[0] for v in base.values())), "total ms", sum(v[1] for v in base.values()))
for line in open(a, errors="replace"):
    if line.startswith(("GAMELOOP", "DRIVER", "AUTORUN", "INFO")):
        print("  ", line.strip()[:200])

names = list(P.keys())
print("== timing (s)")
hdr = "| test | " + " | ".join(names) + " | 114 | " + " | ".join(f"{n} vs 114" for n in names) + (f" | {names[0]} vs {names[1]}" if len(names) > 1 else "") + " |"
print(hdr)
print("|" + "---|" * (hdr.count("|") - 1))
order = list(base.keys())
for n in names:
    for t in P[n]:
        if t not in order:
            order.append(t)
tot = {n: 0 for n in names}
tot114 = 0
for t in order:
    row = [t]
    for n in names:
        v = P[n].get(t)
        row.append(f"{v[1]/1000:.1f}" + ("" if v[0] == "PASS" else f" {v[0]}") if v else "-")
    v114 = base.get(t)
    row.append(f"{v114[1]/1000:.1f}" if v114 else "-")
    for n in names:
        v = P[n].get(t)
        row.append(f"{100*(v[1]-v114[1])/v114[1]:+.0f}%" if v and v114 and v114[1] else "-")
    if len(names) > 1:
        x, y = P[names[0]].get(t), P[names[1]].get(t)
        row.append(f"{100*(x[1]-y[1])/y[1]:+.0f}%" if x and y and y[1] else "-")
    print("| " + " | ".join(row) + " |")
    for n in names:
        if t in P[n] and t in base:
            tot[n] += P[n][t][1]
    if all(t in P[n] for n in names) and v114:
        tot114 += v114[1]
print("| total (tests in all) | " + " | ".join(f"{tot[n]/1000:.1f}" for n in names) + f" | {tot114/1000:.1f} | " +
      " | ".join(f"{100*(tot[n]-tot114)/tot114:+.1f}%" for n in names) +
      (f" | {100*(tot[names[0]]-tot[names[1]])/tot[names[1]]:+.1f}%" if len(names) > 1 else "") + " |")

print("== tess_perf hashes")
h114 = hashes(base.get("tess_perf", (0, 0, ""))[2])
for n in names:
    h = hashes(P[n].get("tess_perf", (0, 0, ""))[2])
    print(n, "MATCH 114" if h and [x[3] for x in h] == [x[3] for x in h114] else "DIFF", len(h), sorted(set(x[3] for x in h)))
    for x in h:
        if x[2] in ("16",) and x[0] == "default":
            print("   gs=%s L=16 default" % x[1], re.search(r"gs=%s L=16 [^|]*?ms=([\d.]+)" % x[1], P[n]["tess_perf"][2]).group(1), "ms")
for x in h114:
    if x[2] == "16" and x[0] == "default":
        print("   114 gs=%s L=16 default" % x[1], re.search(r"gs=%s L=16 [^|]*?ms=([\d.]+)" % x[1], base["tess_perf"][2]).group(1), "ms")

L = one(run + "/raw/*/logcat")
print("== logcat", L)
pr, envp, coh, drv, faults, tls = [], [], Counter(), Counter(), Counter(), Counter()
first_t = last_t = None
envtimes = []
done_t = None
for line in open(L, errors="replace"):
    if "PANPROBE_RESULT" in line:
        pr.append(line.strip())
    if "PANPROBE_ENVPASS" in line:
        envp.append(line.strip())
        envtimes.append((line[:18], line.strip()))
    if "kbase: coherency mode" in line:
        coh[re.sub(r"^.*?kbase: ", "kbase: ", line.strip())] += 1
    for m in re.finditer(r"Mesa \d+\.\d+\.\d+[-.0-9A-Za-z]* \(git-[0-9a-f]+\)", line):
        drv[m.group(0)] += 1
    if "PanVK-kbase" in line:
        drv["(legacy) " + re.search(r"PanVK-kbase [^)]*\)*", line).group(0)] += 1
    for k in ("DEVICE_LOST", "event=0x", "Fatal signal", "GPU fault", "CS_FATAL", "MMU fault"):
        if k in line:
            faults[k] += 1
    if "panvk: device TLS" in line:
        tls[re.sub(r"^.*?panvk: ", "panvk: ", line.strip())] += 1
    if "AUTORUN DONE" in line:
        done_t = line[:18]
print("PANPROBE_RESULT:")
for x in pr:
    print("  ", x[:260])
print("ENVPASS:")
for x in envp:
    print("  ", x[:260])
print("coherency lines:")
for k, v in coh.most_common():
    print("  ", v, k[:260])
print("driver strings:")
for k, v in drv.most_common():
    print("  ", v, k)
print("faults:", dict(faults) or "none")
print("TLS:")
for k, v in tls.most_common():
    print("  ", v, k[:200])
print("AUTORUN DONE at", done_t)

print("== device_info shaderDeviceClock")
for p in sorted(glob.glob(run + "/raw/*/artifacts/**/device_info.json", recursive=True)):
    s = open(p, errors="replace").read()
    m = re.findall(r'"shaderDeviceClock"\s*:\s*(\w+)', s)
    d = re.findall(r'"driverInfo"\s*:\s*"([^"]*)"', s)
    n = re.findall(r'"deviceName"\s*:\s*"([^"]*)"', s)
    dn = re.findall(r'"driverName"\s*:\s*"([^"]*)"', s)
    print(p.split("/runs/")[-1], "shaderDeviceClock", m[:2], "driverInfo", d[:1], "deviceName", n[:1], "driverName", dn[:1])
for p in sorted(glob.glob(run114 + "/raw/*/artifacts/**/device_info.json", recursive=True))[:1]:
    s = open(p, errors="replace").read()
    print("114", "shaderDeviceClock", re.findall(r'"shaderDeviceClock"\s*:\s*(\w+)', s)[:2])
for p in sorted(glob.glob(run + "/raw/*/artifacts/**/summary.json", recursive=True)):
    j = json.load(open(p))
    print(p.split("/runs/")[-1], "envPass", (j.get("envPass") or {}).get("name"), "driverType", j.get("driverType"),
          "pass", j.get("passCount", j.get("pass")), "fail", j.get("failCount", j.get("fail")))
