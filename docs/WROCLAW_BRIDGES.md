# Wrocław bridges and engineering structures

This catalogue expands bridge coverage without pretending the existing interpolated decks are surveyed bridge models.

`resolve_wroclaw_bridges.py`:
- matches named bridges to OSM features tagged as bridges;
- reports which existing `city_structures.json` prototype ways already cover them;
- keeps geometry/elevation/engine verification as separate facts.

## Remaining
1. obtain authoritative deck/elevation geometry or high-quality 3D mesh for hero bridges;
2. resolve multi-way bridge assemblies and pedestrian/rail components;
3. model piers, parapets, arches/cables and approach geometry;
4. apply exact road graph Z to rendered/collision geometry;
5. verify clearance, collision and drivable continuity in UE 5.8;
6. only after verification replace the existing interpolated prototype decks.
