# BCn compatibility suite

`cases.json` is the Phase 6 coverage ledger. `validate_cases.py` rejects
missing formats, operations, edge cases, invalid statuses, and any `PASS`
without an evidence reference.

Run:

```sh
python3 tests/bcn/validate_cases.py \
  validation/g615-v11-csf/beta3-phase6-bcn.json
```

The pinned layer builds and Android's loader discovers it, but the tested loader
path selected the system ICD rather than PanVK and failed device creation.
Therefore PanVK device workloads remain truthfully `NOT_RUN` or `BLOCKED`; this
directory is test specification plus evidence validation, not a correctness
claim.
