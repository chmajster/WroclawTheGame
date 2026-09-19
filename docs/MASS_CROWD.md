# Mass crowd handoff

Pedestrian corridors are generated from the verified walk graph and now carry runtime density and physicalization metadata rather than being geometry-only paths.

## Runtime contract

Each corridor exposes road class, base/effective density, logical capacity, walk speed, avoidance radius and representation hysteresis. Effective density is multiplied by the current day/night and weather state, so the same stable corridor IDs can drive morning, daytime, evening, rain and storm population changes without regenerating geometry.

Crossing descriptors include reservation radius and signalized state when source tags provide it.

## Simulation layers

The policy still defines physical, Mass, logical and dormant ranges. The added hysteresis value is intended to prevent pedestrians from oscillating between representations around a distance threshold.

This output can be imported into ZoneGraph/Mass Crowd or consumed by a custom runtime scheduler while preserving deterministic GIS-backed IDs.
