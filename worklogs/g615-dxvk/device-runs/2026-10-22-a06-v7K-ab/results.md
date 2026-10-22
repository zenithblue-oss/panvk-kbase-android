# A06 v7K A/B runs 2026-10-22 (INCOMPLETE)

The RTL session ended before any run finished. No results were collected.

- Device localhost:54520 (SM_A065F). v7K was not installed at first; the user installed it, and
  `pm list packages panvktest` then showed dev.zenithblue.panvktest.v7k.
- Run A (rc5, autorun robust_image_access2,pipeline_stats, MESA_SHADER_CACHE_DISABLE=true): force-stop,
  telemetry, `am start` done. Waited about 20 min; no autorun.txt was pulled, so the run's outcome is unknown. NOT COMPLETE.
- Run B (PANVK_V7_DESC_REORDER=0): not started.
- Run C (autorun all, stability of 38/0/0): not started.

Host telemetry before A (from nativecode-agent telemetry; the host, not necessarily the A06): battery 95%,
not charging, 34.1 C, thermal SEVERE, avail mem 2521/11541 MB.

Tables for ria2/ria1 ms, pipeline_stats fps and the full-run totals: no data.
