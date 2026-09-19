# Automated aggregate world validation

The validator is now the source-level structural gate for the expanded city. It produces one deterministic PASS/FAIL report and a non-zero exit code for errors.

## Checks

Core world:
- duplicate/non-finite/outlier GIS features;
- road node/edge integrity and edge-length bounds;
- orphan entrances and quality decisions;
- unknown quest districts and unroutable compiled quest legs;
- unknown streaming sectors and unresolved hero landmarks.

Simulation/content systems:
- parking IDs, slot IDs, capacities and finite slot positions;
- Smart Object IDs, source-feature references, slot capacities and positions;
- crowd corridor edge references/capacities;
- traffic lane edge references, speed limits and capacities;
- Data Layer upstream status, assignments and activation-set references;
- HLOD building/Data Layer references, memory budget and hero silhouette protection;
- terrain/floating-object probe offsets;
- critical emergency-service route probes;
- benchmark report status;
- save migration registry coverage for economy, parking, NPC schedule, quests and season plus backup policy.

## Local gate

`Tests/test_world_validation.py` exercises the validator as part of the repository's existing `python3 -m unittest discover` step in `Scripts/test.sh`. This makes structural regressions part of the normal local source test gate without requiring Windows/UE editor execution.

Optional runtime-derived inputs (terrain probes, service probes, benchmark report) strengthen the same report when available; absence of those optional files does not cause invented measurements or fake failures.
