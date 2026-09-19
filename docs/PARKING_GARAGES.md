# Parking and garages

Parking extraction now produces stable runtime slots instead of only OSM areas.

## Runtime parking model

Each parking feature exposes access, fee, maxstay, legal/illegal evaluation, stable capacity and deterministic slot descriptors. Area/garage slots are oriented from the dominant source geometry; point parking produces a single slot. A seeded occupancy flag can populate parked vehicles without persisting hundreds of actors.

Economy hooks reference the central price keys for hourly parking and tow release. Persistent player/owned vehicles can store parking ID + slot ID + vehicle ID while procedural occupancy remains regenerable.

Restricted/private parking is never treated as legal public parking merely because geometry exists.
