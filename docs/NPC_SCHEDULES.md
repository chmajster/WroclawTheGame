# NPC schedules

NPC schedules now model off-screen movement instead of instant logical relocation. Schedule state remains independent from physical actors.

## Runtime state

- active and next slot resolve across the 24-hour wrap;
- travel time is estimated from logical distance and policy walk speed;
- position is interpolated during the travel window;
- state records from/to slot IDs and travel progress for save/load;
- player distance selects physical, simplified or logical representation;
- NPC-specific schedule overrides can be conditioned on weather, weekday or quest context.

The stable logical state is the source of truth. Physical actors can be destroyed/recreated without losing schedule progress, and no visible teleport is required when an NPC crosses the physicalization boundary.
