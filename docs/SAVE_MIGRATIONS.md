# Save migration registry

Campaign save compatibility is now routed through one explicit policy instead of an inline version exception. Unknown/newer versions and unknown coordinate spaces are rejected without being overwritten.

Currently supported paths remain exactly the ones already implemented by the project:
- legacy version 2 history replay into the current campaign state;
- version 3 BlockoutV1 coordinates transformed to WroclawGISV1 through CampaignMigrationDefinition.

## Remaining
1. make migrations a composable version-by-version chain instead of one compatibility predicate;
2. migrate newly added economy, parking, NPC schedule, quest and season state;
3. add stable-ID remap tables for removed/renamed POIs, buildings and objectives;
4. keep a backup before first migration and never overwrite a failed/newer save;
5. add golden binary save fixtures for every supported historic version;
6. test migration in packaged UE 5.8 and across GIS regeneration.
