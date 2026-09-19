# Emergency services dispatch

The planner selects a reachable service depot over the verified car graph and now produces a runtime response contract rather than only a route-node list.

## Dispatch output

- service-specific incident eligibility;
- required unit count from police Heat or incident severity;
- depot availability filtering;
- true route distance rather than hop count;
- service-specific response speed and ETA;
- high/critical priority with a traffic-yield flag;
- physicalization radius for logical→physical response vehicles;
- explicit `rejected` and `unavailable` states.

Police Heat escalation and ambulance/fire incident rules remain policy-driven, so balancing does not require changing route code. Off-screen dispatch can persist the stable depot ID, route nodes, unit count and incident ID while physical vehicles are spawned only near the player.
