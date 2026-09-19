# Repeatable city benchmark

The benchmark analyzer now treats the two city routes as separate required workloads and never invents performance measurements.

## Result contract

CSV samples must identify `route_id`, commit, build configuration and hardware ID. Results are grouped per route and checked against frame/CPU/GPU/memory/VRAM/streaming/draw-call budgets. Missing required routes or metadata fail the aggregate report.

An optional baseline file can contain previously measured route summaries. When supplied, the analyzer reports percentage regressions against that measured baseline using the configured tolerance. No baseline values are shipped or synthesized by this change.

This gives CI/local profiling a stable regression format even though actual UE benchmark execution remains external to the source analyzer.
