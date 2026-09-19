#!/usr/bin/env python3
import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent
requirements = json.loads((root / "cases.json").read_text())
evidence = json.loads(pathlib.Path(sys.argv[1]).read_text())
allowed = {"PASS", "FAIL", "NOT_RUN", "BLOCKED", "NOT_APPLICABLE"}

for group, required in requirements.items():
    results = evidence["results"][group]
    missing = set(required) - set(results)
    assert not missing, f"{group}: missing {sorted(missing)}"
    for name in required:
        item = results[name]
        assert item["status"] in allowed, f"{group}/{name}: invalid status"
        if item["status"] == "PASS":
            assert item.get("evidence"), f"{group}/{name}: PASS lacks evidence"

assert evidence["nativeTextureCompressionBC"] is False
assert evidence["packagedByDefault"] is False
print("PASS: BCn requirements complete; no unsupported PASS claims")
