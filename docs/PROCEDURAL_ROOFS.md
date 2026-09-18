# Procedural roof and detail descriptors

The generator enriches real OSM/GIS building footprints without claiming invented details are surveyed facts.

Priority:
1. use explicit OSM roof tags when present;
2. otherwise choose a deterministic profile fallback;
3. label every fallback as `procedural_fallback`.

It emits roof shape/height/ridge orientation plus bounded chimney and dormer counts. These are descriptors for a later Unreal/Blender geometry pass, not final geometry.

## Remaining
- implement mesh generation for supported roof shapes;
- place chimneys/dormers only inside valid roof surfaces;
- add collision/LOD rules;
- compare hero buildings against real references and override procedural data;
- wire output into city bake only after UE validation.
