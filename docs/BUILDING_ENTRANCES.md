# Building entrances and addresses

The GIS importer now preserves OSM nodes carrying `entrance=*` or address tags. `resolve_building_entrances.py` links those nodes to the nearest building polygon.

Priority:
1. real OSM entrance/address node;
2. explicit procedural edge candidate only for an addressed building with no entrance node.

The fallback is intentionally labelled and should not be treated as surveyed door placement.

## Remaining
- use road/street geometry to choose the street-facing edge for fallback entrances;
- support multiple staircases/entrances per building and unit references;
- spawn interaction actors in Unreal and connect them to building-linked interiors;
- validate wheelchair/access tags;
- verify door position/collision visually before enabling gameplay entry.
