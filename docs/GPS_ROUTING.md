# Runtime GPS routing core

The offline/runtime-ready core performs Dijkstra routing on the same verified road graph used by the city. It supports car/foot directionality and an explicit blocked-turn set.

## Remaining
1. convert OSM restriction relations into blocked/only turn edge pairs;
2. nearest-road snapping for arbitrary player/waypoint positions;
3. turn geometry classification (left/right/roundabout/U-turn) rather than road-name changes only;
4. ETA using speed profiles/traffic;
5. runtime C++ service and phone-map route polyline;
6. reroute after deviation and save waypoint state;
7. validate routes through bridges/tunnels/intersections in UE.
