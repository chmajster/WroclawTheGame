# Save migration registry

Save compatibility is now a composable migration system rather than one inline version exception.

## Campaign migration chain

The current registered graph supports:
- legacy v2 / BlockoutV1 → v3 / BlockoutV1 through `legacy_history_replay`;
- v3 / BlockoutV1 → v3 / WroclawGISV1 through `CampaignMigrationDefinition`.

`BuildCampaignMigrationPlan` searches the registered graph and rejects newer, unknown or disconnected states. A migration-required load creates a `*_pre_migration_backup` slot before mutating in-memory state. A failed migration leaves the original save untouched.

## New system state

`USliceSave` now persists `FWTGEconomySaveState`. Parking, NPC schedules, quest runtime state and season/calendar state use versioned generic system payloads. Older saves receive explicit schema-v1 defaults; payloads newer than the runtime understands are rejected.

Economy transactions/ledger members are marked `SaveGame` and the mission imports/exports the economy subsystem during load/checkpoint.

## Stable IDs

The registry contains explicit remap tables for POIs, buildings and objectives. They are intentionally empty until a real rename occurs. Unknown IDs are never guessed; identity is preserved unless an explicit mapping is registered.

## Verification

`Scripts/save/validate_save_migrations.py` validates registry structure, computes migration plans, defaults system payloads and applies ID remaps. Golden JSON fixtures cover legacy-v2 and v3-Blockout migration paths without requiring UE binary serialization.
