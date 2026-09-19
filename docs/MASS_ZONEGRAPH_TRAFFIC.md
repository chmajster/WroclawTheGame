# Mass/ZoneGraph traffic handoff

The generator converts the verified road graph into directional lane descriptors and defines four simulation representations: physical, Mass, logical and dormant.

## Remaining
1. create/import ZoneGraph lane storage in UE 5.8;
2. implement MassEntity vehicle fragments/processors and archetypes;
3. hand off near vehicles to existing physical ACityTrafficVehicle without teleport/pop;
4. integrate junction reservations/turn restrictions from Point 12;
5. persist only logical traffic state, never hundreds of actors;
6. profile representation thresholds and tune on target hardware.
