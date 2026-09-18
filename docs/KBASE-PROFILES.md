# Kbase profiles

Selection key (minimum): `pan_arch + gpu_id + frontend + kbase_uapi`.
Preferred: `<arch>-<gpu-id>-<frontend>-<uapi>`. Never select from the Android
marketing GPU string.

Primary: `g615-v11-csf` — Poco X6 Pro, Dimensity 8300-Ultra, G615 MC6
(`Mali-G615 6 cores r1p3 0xB8A3`), CSF, `/dev/mali0` mode `crw-rw-rw-`.

Probe (`tests/kbase-probe/`) captures before any Vulkan: SoC/GPU model/ID,
pan arch, `/dev/mali0` existence + O_RDONLY/O_RDWR + errno (ROOT/SHELL and
target-app UID separately), Kbase UAPI, DDK/module version if safely readable,
JM/CSF, shader-core mask, coherency, memory, CSF interface properties.
A driver that only opens `/dev/mali0` as root is not app-loadable.
