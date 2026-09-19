# Smart Object candidates

The generator maps tagged GIS features to interaction roles and now emits a runtime-ready slot layout for every candidate.

## Runtime contract

Each object carries a stable ID, role, source feature, reservation mode, behavior ID, interaction/approach distance, Data Layer, accessibility tag and explicit capacity. Spatial slots have stable IDs, world positions, facing yaw and enabled state.

Role policy controls slot count and radius:
- entrances use a single exclusive interaction slot;
- shops and transit stops expose queue-oriented multi-slot layouts;
- amenities expose shared slots;
- parking exposes a vehicle reservation slot.

The slot geometry is deterministic, so save data and reservations can refer to `smart:<feature>/slot/<index>` without depending on transient Actor IDs.

## Integration

StateTree/animation implementations can bind by the semantic `behavior` field. Entrance slots can resolve to procedural interiors, transit slots to route stops, and parking slots to the parking system while keeping one stable GIS identity.
