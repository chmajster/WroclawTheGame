# City audio runtime plan

The city audio plan is now environment-aware while remaining asset-license neutral. It contains semantic emitter roles only; actual recordings must be project-owned or properly licensed.

## Sector mix

Sector counts can enable traffic, tram, park, river, crowd and industrial layers. Density selects an acoustic profile, occlusion strength and local voice budget. Each layer has a priority and stable emitter role suitable for MetaSounds or conventional SoundCue bindings.

Day/night and weather states modify the mix. Night adds a dedicated nocturnal ambience role, while rain/fog/storm alter layer gains without replacing the source catalogue.

## Streaming and acoustics

The plan exposes per-sector voice budgets, a global voice budget and prefetch duration. Reverb profiles distinguish dense street canyons from open-air areas, and an occlusion factor can feed runtime traces/portals.

No unlicensed audio files are added by this pipeline.
