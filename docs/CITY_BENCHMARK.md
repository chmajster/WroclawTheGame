# Repeatable city benchmark

Two stable workloads are defined: walking Nadodrze→Centre and a driving loop through all Wave 1 sectors. The analyzer consumes measured runtime CSV; it never invents FPS.

## Remaining
1. implement UE automation that drives these exact routes;
2. export CSV Profiler + Unreal Insights counters with commit/build metadata;
3. include actor count, draw calls, streaming misses, RAM and VRAM;
4. run Development and Shipping on target hardware;
5. store baseline/variance and compare regressions;
6. tune thresholds only from measured data.
