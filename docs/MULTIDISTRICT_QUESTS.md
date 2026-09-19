# Multi-district quests

Quest stages now compile into streaming-safe runtime objectives instead of referencing transient world actors.

## Runtime objective contract

- every stage has a stable `objective_id = quest:stage`;
- targets support district anchors today and optional POI/landmark/entrance IDs when curated catalogues are supplied;
- target IDs are validated instead of silently falling back to an invented place;
- consecutive stages compile a shortest district path for GPS/route-aware objectives;
- GPS and recovery behavior are explicit per stage;
- save state persists quest ID, stage ID and world mutation IDs.

The shipped example intentionally uses district anchors until specific story POIs are curated. This avoids presenting an arbitrary real business/private entrance as story content while still providing the full binding mechanism.
