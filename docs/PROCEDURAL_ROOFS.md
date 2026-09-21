# Procedural roof and detail descriptors

The generator enriches real OSM/GIS building footprints without claiming invented details are surveyed facts.

Priority:
1. use explicit OSM roof tags when present;
2. otherwise choose a deterministic profile fallback;
3. label every fallback as `procedural_fallback`.

It emits roof shape/height/ridge orientation, a concrete roof asset ID and bounded chimney/dormer instances. `prepare_geography.py` consumes these records and bakes scalable roof, chimney and dormer meshes through the existing HISM facade clusters. Official GUGiK buildings remain excluded from this procedural overlay.

## Remaining
- run Unreal visual/LOD/performance QA for the generated roof modules;
- compare hero buildings against real references and override procedural data where the official model or authored reference is better.
- wire output into city bake only after UE validation.
