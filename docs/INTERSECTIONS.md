# Intersections and turn restrictions

The intersection pipeline converts OSM control nodes and restriction relations into runtime-facing road graph descriptors.

## Runtime contract

- traffic signals, stop signs, give-way nodes and crossings carry explicit priority, stop-line offset, pedestrian-conflict and reservation metadata;
- node-via OSM turn restrictions are mapped to directed `from_edge -> to_edge` pairs using the imported road graph's OSM `way` IDs;
- unsupported via-way restrictions and relations that cannot be mapped are preserved in `unresolved_restrictions` with a reason instead of silently disappearing;
- `resolved_pairs` can be consumed directly by GPS, DriverAI and intersection reservation logic.

The mapping respects `car_forward`/`car_backward`, so a restriction is attached to the direction in which a vehicle actually enters and leaves the junction.

## Extension points

Signal timing plans and authored lane-level geometry can augment these descriptors later without changing stable restriction IDs or the road graph edge contract.
