# Full game expansion — implementation state

Updated: 2026-09-18
Working branch: `codex/full-game-expansion-pass-1`

This file is the hand-off point for long-running implementation of the full WroclawTheGame expansion. Update it before ending every implementation pass. Do not mark runtime work as complete without an Unreal/Windows verification artifact.

## Current pass

### Completed in code/docs
- [x] Standardized active project documentation and editor-script references on Unreal Engine 5.8.
- [x] Added `Scripts/Assert-UnrealVersion.ps1`.
- [x] `Build-Windows.ps1` now rejects an EngineRoot whose `Build.version` does not match UE 5.8 or whose project EngineAssociation differs.
- [x] `Build-Geography.ps1` uses the same version gate before GIS generation.
- [x] GIS phone map reports the current sector, coverage state, waypoint and straight-line distance, with controller actions to cycle/clear the waypoint.
- [x] Ambient population now separates pedestrian LOD from a bounded pool of physical traffic vehicles driven by `UVehicleAIDriverComponent`.
- [x] Added/extended offline tests for UE version guards, campaign GIS manifest/migrator, race modes and full-pass integration wiring.
- [x] PR body updated with exact code scope, verification limits and remaining runtime gates.

### Last completed code change
- Added an official GUGiK CityGML ingestion path for six central Wrocław landmarks. The full-game build downloads/caches the source, replaces matching OSM blockouts, and bakes the selected building geometry into `/Game/Generated/OfficialBuildings`; Unreal visual QA remains pending.
- Reconciled PR #8 with the current `main` asset-production pipeline while preserving the UE 5.8 guard and the full-game GIS/AI changes.
- Campaign GIS migration, six vehicle scenarios, first pursuit/roadblock runtime and position-aware phone map are present in code but remain subject to Unreal/Windows acceptance.

### Verification state
- Static repository edits: performed.
- Offline test coverage: extended; this connector pass did not execute the local Linux test suite.
- Unreal UHT/UBT: NOT RUN in this environment.
- Unreal Editor content generation: NOT RUN.
- Development/Shipping package: NOT RUN.
- End-to-end gameplay: NOT RUN.
- Performance profiling: NOT RUN.
- Therefore no sector may be promoted to Playable solely because of this pass.

## Global backlog

Status legend: `DONE`, `PARTIAL`, `NOT STARTED`, `REQUIRES UE`.

| Priority | Workstream | Status | Next concrete completion criterion |
|---|---|---|---|
| P0 | UE version/toolchain consistency | PARTIAL | Run UE 5.8 UHT/UBT and both Development + Shipping build entry points on Windows |
| P0 | Full Windows executable | REQUIRES UE | Produce packaged EXE and record commit/logs |
| P0 | End-to-end vertical slice | REQUIRES UE | Menu -> apartment -> puzzles -> street -> chase -> safehouse -> chapter completion |
| P0 | Campaign migration to real GIS | PARTIAL | Generator + transform DataAsset + GIS bake exist; author real interiors, verify checkpoints/save behavior in UE |
| P0 | World Partition / HLOD / streaming | REQUIRES UE | Verify walking + fast driving without missing floor/pop-in blockers |
| P0 | Performance budgets | REQUIRES UE | Record 1080p FPS/frame-time/GPU/CPU/RAM/draw calls with Unreal Insights |
| P0 | GIS collisions/bridges/tunnels | PARTIAL | Physically verify authored structures and finish non-zero layers/remaining crossings |
| P0 | Vehicle AI | PARTIAL | Physical route following/obstacle braking/LOD exist; finish junctions, spacing, turn restrictions and UE tuning |
| P0 | Vehicle pursuit AI | PARTIAL | Multi-pursuer state machine, loss/search and roadblocks exist; verify/rework road-constrained pursuit and reacquisition in UE |
| P1 | Vehicle physics | PARTIAL | Engine-tested handling, suspension, collision, braking, reverse, damage |
| P1 | Player character M/F | PARTIAL | Two production characters with shared gameplay skeleton and validated animations |
| P1 | Animation pass | PARTIAL | Locomotion, crouch, vault, interactions, vehicle entry/exit, combat reactions |
| P1 | Pedestrian NPC population | PARTIAL | Density/LOD, crossing/avoidance/reactions, no full-city expensive simulation |
| P1 | NPC schedules | PARTIAL | Logical off-screen simulation and physical realization near player |
| P1 | Police/Heat response | PARTIAL | Heat drives actual dispatch/search/pursuit escalation |
| P1 | Stealth AI | PARTIAL | Suspicion, communication, search regions, return-to-routine validated in UE |
| P1 | Combat | PARTIAL | Hit reactions, balance, knockdown/stagger/takedown and non-lethal escape paths |
| P1 | Phone GIS map/GPS | PARTIAL | Position-aware GIS map, markers, waypoint and route presentation |
| P1 | Phone camera/gallery | PARTIAL | Real captured image asset/RenderTarget saved and reusable as evidence |
| P1 | Investigation | PARTIAL | Evidence graph, hypotheses/validity/false leads and UI |
| P1 | Puzzle depth | PARTIAL | Spatial/electrical/CCTV/cipher examples using reusable framework |
| P1 | Multi-district quests | NOT STARTED | One quest traverses multiple streamed districts and survives save/load |
| P1 | Interiors | PARTIAL | Replace isolated prototype rooms with building-linked gameplay interiors |
| P1 | Wroclaw landmarks | PARTIAL | Official GUGiK import for six central landmarks is wired; run download + UE bake, verify LoD actually returned for Wrocław, then perform hero visual/LOD/collision QA |
| P1 | City audio | PARTIAL | Exterior/interior ambience, traffic/trams, occlusion/reverb and event audio |
| P1 | Dialogue | NOT STARTED | Data-driven conversations with state/quest consequences |
| P1 | Cutscenes | NOT STARTED | Intro + key discovery + chapter ending sequences |
| P2 | Public transport | NOT STARTED | Tram/bus routes and stops driven from geographic data |
| P2 | Street races | PARTIAL | Complete pursuit/escape/follow-clues variants and campaign rewards |
| P2 | Garage/progression | NOT STARTED | Ownership, repair and selected customization loop |
| P2 | Multiple driveable cars | NOT STARTED | Several data-driven vehicle definitions with distinct handling |
| P2 | Dynamic world events | PARTIAL | Physical encounters, not only notification/flag events |
| P2 | Weather gameplay | PARTIAL | Wetness/visibility/AI/vehicle handling effects |
| P2 | Day/night simulation | PARTIAL | Time affects population, lighting, shops/events/quests |
| P2 | Wroclaw secrets | PARTIAL | Dense exploration rewards tied to actual city geometry |
| P2 | Reputation/factions | PARTIAL | Persistent faction relation changes with gameplay consequences |
| P2 | Economy | NOT STARTED | Rewards, purchases, repairs and resource sinks |
| P2 | Chapter 2 | NOT STARTED | Content after chapter-1 vertical slice passes acceptance |
| P2 | Further districts | PARTIAL | Expand only after existing sectors reach Playable/Detailed |
| P2 | Accessibility/settings | PARTIAL | Rebinding, FOV/sensitivity, subtitle/UI scaling and graphics options |

## Required order for subsequent passes

1. Keep the branch/PR buildable and update this file every pass.
2. Finish P0 build/toolchain gates before declaring gameplay systems complete.
3. Migrate the chapter onto real GIS before expanding the city footprint further.
4. Implement traffic/pursuit and population LOD before increasing population density.
5. Complete phone GIS/GPS and save integration before adding more map-dependent quests.
6. Only after the vertical slice is verified in a packaged Windows build, add Chapter 2 and large new district waves.

## Handoff protocol

At the end of every pass:
1. update checkboxes/statuses above;
2. write the exact last completed code change under **Current pass**;
3. record what was not verifiable without Unreal/Windows;
4. update the open PR description with the same summary;
5. leave the PR open/draft while the global backlog remains unfinished;
6. do not promote GIS coverage status without evidence matching the current fingerprint.
