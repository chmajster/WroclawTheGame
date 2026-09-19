# Runtime GPS routing core

The navigation core now routes on the verified city road graph with the same stable edge IDs used by intersections and traffic.

## Navigation features

- car/foot directionality;
- arbitrary world-position snapping to the nearest routable graph node;
- Point 12 `resolved_pairs` imported as prohibited turns;
- ETA from edge length, OSM/fallback speed limits and a runtime traffic multiplier;
- geometry-based left/right/continue/U-turn classification;
- roundabout recognition from edge junction metadata;
- reroute from the player's current world position.

Route results retain both edge lists and directed steps, making them suitable for phone-map polylines and later C++ consumption. The core remains deterministic and does not require Unreal actors to exist.
