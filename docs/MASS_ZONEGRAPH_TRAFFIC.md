# Mass/ZoneGraph traffic handoff

The road import now preserves traffic-facing OSM metadata on every graph edge: road class, max speed, lane count, junction type and surface. The ZoneGraph planner consumes those fields instead of producing anonymous direction-only lanes.

## Runtime descriptors

Each directional lane contains:
- stable edge/way identifiers;
- direction and world-space points;
- road class, junction and surface;
- parsed speed limit in km/h with road-class fallback;
- directional lane count;
- estimated logical vehicle capacity;
- lane width.

The global plan also exposes physical/Mass limits plus handoff hysteresis and despawn grace time. These values are intended to prevent representation thrashing when a vehicle moves near the physical/Mass boundary.

## Integration

Point 12 restriction pairs can now be attached directly by edge ID. The traffic state can persist logical vehicle progress using stable lane IDs while physical actors remain transient.
