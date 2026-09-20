#!/usr/bin/env python3
import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent
requirements = json.loads((root / "cases.json").read_text())
evidence_path = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root.parents[1] / "validation/g615-v11-csf/p9-bcn.json"
evidence = json.loads(evidence_path.read_text())
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

assert isinstance(evidence["nativeTextureCompressionBC"], bool)
assert isinstance(evidence["emulatedTextureCompressionBC"], bool)
assert isinstance(evidence["advertisedTextureCompressionBC"], bool)
assert isinstance(evidence["packagedByDefault"], bool)
assert not (evidence["nativeTextureCompressionBC"] and evidence["emulatedTextureCompressionBC"])
assert not evidence["packagedByDefault"] or evidence["advertisedTextureCompressionBC"], \
    "BC compatibility cannot be packaged before it is advertised and proven"
if evidence["advertisedTextureCompressionBC"]:
    assert evidence["nativeTextureCompressionBC"] or evidence["emulatedTextureCompressionBC"]
    incomplete = [
        f"{group}/{name}"
        for group, required in requirements.items()
        for name in required
        if evidence["results"][group][name]["status"] != "PASS"
    ]
    assert not incomplete, f"advertised BC contract has incomplete cases: {incomplete}"
print("PASS: BCn evidence complete; advertisement matches proven contract")
