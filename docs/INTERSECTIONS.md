# Intersections and turn restrictions

This pass preserves OSM control nodes and restriction relations as explicit runtime-preparation data.

## Remaining
1. bind restriction relations to generated road-graph edge IDs;
2. generate lane-level turns and stop lines;
3. implement priority/right-of-way state;
4. add traffic signal phases and pedestrian crossings;
5. teach VehicleAIDriver to reserve/clear junctions;
6. validate roundabouts, no-turn/only-turn restrictions and UE behavior.
