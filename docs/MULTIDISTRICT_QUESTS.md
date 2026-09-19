# Multi-district quests

Quest stages reference stable district IDs rather than hard-coded streamed actors. The validator checks that consecutive districts are connected by the generated city topology.

## Remaining
1. bind stages to specific resolved POIs/landmarks/entrances;
2. route-aware objectives and phone GPS;
3. persist stage + world mutations through save/load and GIS revisions;
4. streaming-safe spawn/despawn of quest actors;
5. failure/recovery paths when content changes;
6. packaged EXE playthrough across multiple districts.
