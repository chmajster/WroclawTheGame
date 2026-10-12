# Kreator postaci — bieżący stan

## Wykonane
2/3: UI i studio

Trzy panele16/56/28,13 kategorii z ikonami i animacją, kontekstowe edytory, palety, działające podsumowanie i istniejący zapis/menu. Usunięto przyczynę białego targetu: izolacja kanałów widoku/światła i atmosfery, fizyczna ekspozycja. Skalowanie i kadrowanie zależne od panelu, płynny obrót/zoom i animacja kości przy pauzie.

Sprawdzenia i ograniczenia: UE native build PASS. Rzeczywisty CreatorSmoke1280x720 i1920x1080 exit0, szkic zgodny, model widoczny, idle działa; obrazy obejrzane. Obraz1080 archiwizowany. Baza i garderoba przed strumieniem3 mają dotychczasowe ograniczenia; nie deklarujemy gotowej personalizacji.

## Do zrobienia
W strumieniu2 nic. Całe zadanie: strumień3 siatki/morphy/ubiór/materiały i kontrola zapisu/gry.

## Następny krok
Po scaleniu utworzyć gałąź z main dla strumienia3. Pozyskać przypięte CC0 MakeHuman base/targets; Blender generator dopasowany do istniejącego riggu, import i funkcjonalna personalizacja.

## Stan
UI i studio zakończone; całe zadanie nadal w toku. Nie uruchamiano ani nie oczekiwano na CI.

Pełny raport: docs/CHARACTER-CREATOR-REBUILD.md.

# Earlier implementation records
# Start location production — 2026-10-12

Primary live log: current start-location PR, stream 10/10.
See docs/START-LOCATION-PRODUCTION.md. Scope: existing Pomorska/Paulińska start, 100 m.
Start XY, mission, vehicle and gameplay systems retained.

## Wykonane
Stream 10/10. Completed all10 streams for existing100m start. Final report documents13models72UEassets7CC0sets3reused prop types,2272instances,source-backed placement,actual collision/walking/full460m mission QA and observed59.65FPS at60cap. Editor startup matches game map. Nine prior PRs merged; final delivery ready to merge.

Validation and limitations: Final source/native/screenshot hash audit PASS,171C++ files match compiled copy after CRLF normalization. All ten dedicated PRs targetmain and contain required live sections. Native asset72count/catalog/defaultmap and actual QA report consistency PASS. No CI manually run or awaited. Original raw hashes preserved; documented roofs/heights/background/default-character/one-hardware/locked-main-DLL limitations remain.

## Do zrobienia
Nothing remains in the implemented start-location scope.

## Następny krok
No required implementation continuation. Merge this completed delivery PR into main; optional future work is more precise roofs/background facade/character art, as separate requests.

## Stan
Complete, streams1-10 implemented and verified; final PR ready to merge. No CI manually run.

# Earlier city asset production — completed first batch

Primary live log: PR #151 (stream 6/6 complete); #146-150 merged. Source decisions,
license and evidence: docs/CITY-ASSET-PRODUCTION.md and CITY-ASSET-CREDITS.md.
Existing gameplay/menu work below is complete.

Wykonane: five original Blender meshes with UVs, three FBX LODs, editable
.blend/.glb, native UE assets and manifests. Railing has three open UCX guards,
balcony slab one hull, portal/cornice/static decorative door no collision.
Door binds versioned project Wood/Metal/Glass materials. Actual native import
and saved placement: 904 instances / 44 buildings / 28 HISM clusters;
120 railings + 120 slabs + 600 cornices + 32 portals + 32 doors.
Portal candidates intersecting upper balconies are rejected. FBX handedness
accounted for in placement; front-facing handles/guards verified in captures.

Acquisition: real CC0 Sandstone Cracks 2k by Rob Tuytel downloaded and locked
with provider MD5 and independent SHA256; offline reuse PASS. Existing 66-model
library reused instead of duplicate props. All five new meshes actually imported.
Native WTG.Assets.TenementKit PASS for all five, including full stone fallback,
three LODs, material slots and open railing rear. 16 focused tests PASS;
27 broader city tests PASS with correct PYTHONPATH. 15 manifests PASS.

Do zrobienia: nothing within the implemented first batch. Hero landmarks and
wider city remain future production, explicitly listed in Data/city_asset_needs.json.
The generated district has not been converted to World Partition; no real
large-world HLOD/FPS acceptance claim. Existing facade/window/ground art and
lighting contrast/temporal noise remain prototype-level limitations.

Następny krok: merge complete PR #151. Next production batch starts
market_and_town_hall from Data/city_asset_needs.json: inspect existing GIS/catalog
and legal references, author actual Town Hall geometry, create
Pipeline/assets/rynek/ratusz.asset.json, then import and source-driven placement.
Do not use an empty or generic box as a completed landmark.

Integrated run 20261012-201713 EXECUTION_PASS: five exports/imports, quality,
placement, native Automation and five fresh rendered views. District rules/save,
full delivery (147.35 s / 459.49 m) and separate-process resume (5.01 s) PASS.
Explicit inspection of five PNGs PASS_WITH_LIMITATIONS, archived with SHA256
and native kit/map fingerprints in docs/qa/city-assets-2026-10-12/review.json.
See that directory README for evidence/logs; reproduce with
Scripts/Invoke-CityAssetProduction.ps1 -Python Saved/GISPython/Scripts/python.exe
-GameplayChecks. Parser defect in Assert-UnrealVersion fixed; delayed native
shutdown uses real settling time rather than just fast NullRHI frame count.
Budget PASS: 6,549,600 instanced LOD0 triangles, six unique textures, 26,077,960
source bytes and 83,886,084 bytes conservative RGBA mip upper bound.
Source quality policy applied on every future import/placement. Screenshots are freshly generated
with hashes; capture success alone is not visual acceptance.

Stan: first batch complete. Generic GIS facade enrichment, no surveyed reconstruction
or AAA claim. Existing missions/vehicles/building openings retained. No CI run.

# Menu rebuild - stages 1-7 complete

Implemented in PRs #139-145, each on its own branch from main:
1. Architecture audit/integration contracts.
2. Shared original Slate visual system and UMG viewport wrapper.
3. Cinematic frontend with seven working actions and validated save selection.
4. Pause with six tabs, real map/character/session data and save confirmation.
5. Persistent graphics/audio/input configuration and full conflict-aware remap.
6. Camera drift, transitions, loading, focus-loss pause and menu frame cap.
7. Native interaction/render/regression tests and fixes found during acceptance.

Final editor build PASS (119.18 s). Test-Menu.ps1 -Layouts PASS:
settings + separate-process replay, all 33 UI stages (41.96 s), actual
quit, video cancel/15 s timeout/confirm, keyboard/mouse/pad Slate routing,
frozen world/input, stats save/load, corrupt-save fallback, NPC visibility.
20 native PNGs have verified dimensions at 1080p/1440p/4K/3440x1440.
Reviewed main/pause/settings/controls/loading; ultrawide background fills
viewport. Mean frame time 33.33-33.39 ms; p95 33.33-33.36 ms with menu cap 30.

Test-PlayableDistrict.ps1 PASS: four project rules/geometry/save tests,
foot/vehicle persistence, real physical session 147.29 s/459.60 m, separate
process replay 5.02 s. Runtime/menu logs have no Error/Fatal. Engine startup
smoke still logs 17 generic Condition failed lines before the requested
editor test queue; script reports these and rejects other startup errors
and all errors in requested tests/runtime. No engine modification or CI.

QA fixes: blur/resize geometry and video rollback snapshot; raw Slate
accept vs CommonUI virtual cursor; safe save-header check; map labels;
ultrawide camera constraint; frontend mission actors restored in gameplay;
reentrant pursuit destruction caused by inherited RaceSession EndPlay.
All corrected paths have native evidence. Input tests use automation;
physical controller hardware was not tested manually. Existing city art
remains prototype quality. Details/results: docs/MENU-REBUILD.md.

Visible game launched with Scripts/Launch-District.ps1. Fresh default log
shows real Nadodrze map load, gameplay readiness and WTG_MENU_SCREEN main
at 18:36:40 UTC on 2026-10-12. Keep the game open for the user.

Remaining implementation: none in the requested menu scope.
Next action: no implementation continuation required. A later fresh game
start uses Scripts/Launch-District.ps1; further changes start from main
on a new dedicated branch. Final delivery PR: #145.

Prior open-world implementation history follows.

# Full game expansion — implementation state

## Current state - complete first playable slice (streams 1-7)

All streams 1-7 / PRs #132-138 are merged.
All requested implementation streams for this first bounded slice are done.
No remaining implementation work in this pass. This is a functional prototype,
not the final GTA-VI-level visual target or a full Wroclaw city simulation.

Final physical SessionSmoke PASS 16:01:06 UTC: 199.91 seconds, 463.06 m.
Walking/wall/floor/jump/camera, giver/failure/restart/prompt/entry/exit,
real delivery, wanted decay after losing contact, one reward saved, new
police dispatch and saved progress restored. New-process replay PASS
16:01:54 UTC: complete mission, 430 PLN and exactly one ledger entry.
Test save slots were removed after replay; player saves remain separate.

Traffic cleared its mutual stop through checked yield/retry; the long delay
is a known limitation of the basic AI. Cover search was added to the probe,
but not selected in the final passing run: wanted cleared before the receiver.
The production Lost redispatch bug and stale pursuit after loading are fixed.

Native build PASS; rules4 and SaveSmoke PASS after production pursuit fixes;
eight GIS PASS. Final UE renderer screenshots inspected: player/back/car/
street/river; 1310 frames mean16.78ms/p9516.67ms with cap60. No CI invoked.
Visible Launch-District started PID44308 at16:02 UTC, window responding,
Load map complete /Game/Maps/Nadodrze_Playable. Leave this game window open.

Next step: no implementation remains in scope. Finalize PR #138 merge and
return to main. For another play session use Scripts/Launch-District.ps1;
controls and reproduction instructions are in docs/PLAYABLE-DISTRICT.md.

Historical notes below are prior batches, not current acceptance status.
Presentation remains a CC0/procedural prototype without full wardrobe/hero
art or a full-city simulation.

## Sesja otwartego świata — zlecenie 12.10.2026

Ten zakres rozwija istniejącą grę w pierwszą kompletną sesję pieszo/samochodem
na zwartym fragmencie Wrocławia. Plan i audyt: `docs/OPEN-WORLD-SLICE.md`.

- Punkt 1: rozpoznanie wykonane, UE 5.8.2 i działający start kampanii potwierdzone.
- Punkt 2: PR #133, `feat/02-playable-wroclaw-district`. Źródło i generator
  istnieją; 439 komórek, 75 364 trójkąty, legalna trasa 477,9 m, 16 tras pieszych.
  UHT/UBT i pięć testów geometrii przeszły. Balustrada ma pipeline OBJ → Blender
  (LOD 108/54/26) → FBX → UE; liczenie UCX poprawiono na osobne convex collision.
  Pełny bake PASS (439 komórek, 12 000 fasad, 244 balustrady, 740 aktorów).
  Proces generatora zakończył się kodem 0. Mapa uruchomiona w oknie `-game`.
- Punkt 3: wykonany, PR #134. Punkty 4–7 pozostają w zakresie.
- Punkt 3 scalony do main; punkt 4 na `feat/04-odra-delivery-mission`.
  DeliveryState / DistrictDeliverySubsystem / DistrictDeliveryPoint zawierają
  etapy, czas, utratę auta/śmierć, restart R i canonical ledger reward 180 zł.
  HUD pokazuje cel, czas i znacznik; auto-exit nie gubi dostawy. Wprowadzono
  osobną kotwicę startu gracza, aby nie kolidował ze zleceniodawczynią.
- Decyzja: wykorzystać istniejące GIS, postać, pojazd, populację, ekonomię i menu;
  nie zmieniać silnika ani wdrażać zastępczego frameworka gry.
- Punkt 2 scalony do `main` przez PR #133. Punkt 3 rozpoczęty na
  `feat/03-district-gameplay`, draft PR #134. Pierwsza partia C++ kompiluje się.
  Wdrożono wykroczenia/decay wanted, widoczne auta policji z dojazdem do ostatniej
  pozycji również pieszo i kontrolą widoczności spawnu; skrzyżowania mają
  deterministyczne pierwszeństwo. Bezpieczne wyjście uwzględnia kapsułę i ściany,
  auto włącza silnik przy wejściu. Piesi mają reakcję na obrażenia i omijanie.
  LocomotionAssetLibrary tworzy BlendSpace po kontroli nazw/hierarchii kości,
  CharacterAppearanceComponent odtwarza go w runtime. Przygotowanie obu rigów
  i sześciu klipów PASS (`Saved/DistrictLocomotionReady.ok`). Test native
  `WTG.District.WantedDecay` PASS, końcowy UHT/UBT PASS. Poprawiono restart
  animacji co klatkę (GetCurrentAsset zamiast serializowanego AnimationData)
  i pozostawioną pauzę menu kampanii przy starcie GeoPreviewGameMode.
- Kontrola runtime wykazała konkretny brak kolizji podłoża: animacja załadowana,
  GameMode tick i streaming aktywne, ale floor trace nie trafia (postać MOVE_None).
  `GeoMeshLibrary::BakeMesh` budował/cookował body przed ustawieniem
  complex-as-simple; dodano jawne flagi sekcji i invalidację/re-cook body.
  Natywny `WTG.District.GeneratedMeshCollision` potwierdził trójkąty renderu,
  dane kolizji i gotowy cooked triangle mesh. Oba testy WTG.District PASS.
  Mapa przebudowana z kolizją; assety animacji ponowione, dwa LOD-y postaci.
  GameplaySmoke.log 12:07:35 UTC potwierdza WTG_DISTRICT_GAMEPLAY_READY:
  floor=walking, locomotion=BlendSpace, cameraCollision=1, registry=nadodrze_odra.
  Skrypt postaci jest powtarzalny i zachowuje oznaczenie CC0 modeli jako
  prototypów kreatora (nie mają pełnego kompletu morfów twarzy).
- Punkt 4 ukończony: build, trzy testy native i pięć testów danych PASS.
  DeliverySmoke.log: giver=1 receiver=1 reward=18000, walking/BlendSpace.
- Punkt 5 rozpoczęty: `feat/05-district-presentation`, oprawa i audio miasta.
- Punkt 5: PBR director z ekspozycją outdoor EV 4–14, ciągłe audio miasta,
  przestrzenne silniki zależne od prędkości i syreny. Naprawiony kierunek słońca.
  Generator detali sprawdza source buildings/water i trasy piesze; 44 props,
  pięć oryginalnie syntezowanych/importowanych dźwięków, sześć testów GIS PASS.
- Następny krok: końcowy build oprawy, WTG.DistrictViewSmoke i ogląd zrzutów
  Saved/Tests/DistrictStreet.png oraz DistrictRiver.png; finalizacja PR #136.
- Render wykazał niewidoczne podłoże: GIS CCW nie odpowiada clockwise UE
  MeshDescription (Engine normal = Cross(C-A, B-A)). BakeMesh odwraca teraz
  indeksy, test sprawdza normalną w górę. Źródłowe ściany normalizują ring winding.
  Kenney windows mają normalną X (Y/Z plane), usunięto błędne +90 yaw,
  dopasowano wymiary/pivot i wysunięto poza pełną geometrię ścian.
  Następne: ponowny bake po zakończeniu build, locomotion/presentation, render.
- Potwierdzony UE Python Rotator positional (0,90,0) = pitch90/yaw0.
  Generatory używają jawnych pól; naprawiono też mesh_rotation postaci.
  Autorska rama okienna w Pipeline/assets/nadodrze/window-frame.asset.json:
  OBJ/MTL → Blender LOD 96/48/24 → UE trzy LOD, NoCollision (source walls blokują).
  Okna mają osobne szkło PBR i ramę, instancing i skale z source openings.
- Naprawiono jednostki FBX: convert_scene_unit=True (wcześniej 1:100).
  Wysokość ramy UE 150 cm, balustrady 110 cm; importer i presentation sprawdzają
  wysokości deklarowane w manifestach. Końcowy bake po konwersji PASS.
- Ogląd renderu: powierzchnie i okna poprawne, ale klipy zgodnej hierarchii
  mają różne translacje/proporcje; pełna postać zapadała się do niskiej pozy.
  LinkCompatibleRig zachowuje teraz translacje skeleton body i animuje rotacje
  dla in-place clips; zapisuje oba skeletons. Dodano runtime bone-height log.
  Lumen cached preexposure EV6 obejmuje wymagane EV4–14 bez clipping warning.
- Następny krok: po locomotion uruchomić render, sprawdzić bone_height_cm
  i pełną sylwetkę; zakończyć PR #136, przejść do HUD/zapisu (punkt 6).
- Bone-height161.8 cm był poprawny, ale import primary_object wskazywał
  Eyebrows z FullBody.gltf. Importer i creator wybierają teraz największą
  siatkę ciała z walidacją importowanych bounds140–220 cm. Populacja używa
  poprawionego ciała po prepare_district_locomotion. Ponowić i obejrzeć render.
  Zapis nagrody i pełna sesja należą do punktów 6–7.
- Weryfikacja: lokalny UE jest dostępny; natywny Computer Use nie działa (pipe,
  os error 2). Nie deklarować wizualnego QA bez rzeczywistego dowodu.
- Stan: w trakcie całego zakresu, punkty 1–4 wykonane; dalsze sekcje poniżej to historyczny
  zapis wcześniejszych zakresów i nie stanowią odbioru nowego zadania.

Updated: 2026-09-18
Working branch: `codex/official-wroclaw-buildings-3d`

This file is the hand-off point for long-running implementation of the full WroclawTheGame expansion. Update it before ending every implementation pass. Do not mark runtime work as complete without an Unreal/Windows verification artifact.

## Menu polish — PR #65 — 2026-09-19

Status: IN PROGRESS (draft PR).

Wykonane w tej fali:
- context chrome: numer strony 01–07, wskaźnik sesji, zegar/data i separator stopki,
- spójne keycapy w stopce, podglądzie postaci, modalu potwierdzenia i sterowaniu mapą,
- numerowane zakładki, mocniejsza hierarchia aktywnej sekcji i wyraźniejszy focus klawiatury/gamepada,
- dynamiczne nagłówki i separatory trzech kolumn,
- accent rail we wspólnych kartach statusowych,
- numerowana nawigacja ustawień z paskiem postępu 1/4–4/4,
- szczegóły jakości obrazu jako karty zamiast surowego tekstowego dumpu,
- precyzyjne suwaki audio 0–100% z krokiem 5%, miernikami, zapisem po zakończeniu regulacji i poprawną obsługą D-pada,
- toast ustawień z accent rail i reduced-motion-safe slide/fade-in,
- zachowanie focusu na aktywnej kategorii ustawień po Refresh(),
- rzeczywisty stan wybranego kadru i oświetlenia w podglądzie postaci,
- obrót/zoom podglądu postaci ograniczony do środkowego panelu,
- garderoba/fryzura/głos używają player-facing DisplayName z katalogu; usunięto PresetID i RandomSeed z profilu,
- Dziennik/Mapa/Statystyki mają karty i keycapy zamiast surowych statusów/debugowych etykiet,
- stany awaryjne menu nie pokazują nazw klas Unreal,
- rozszerzone regresje Tests/test_player_menu.py.

Walidacja wykonana:
- statyczny smoke-check connectora: PASS,
- automatyczny audyt 377 prostych assertIn/assertNotIn: PASS,
- 55 testów regresyjnych wykrytych,
- 53 deklaracje UFUNCTION mają odpowiadające definicje,
- definicje siedmiu Build*Tab występują dokładnie po jednym razie,
- bilans klamer PlayerMenuWidget.cpp: 255/255,
- bilans nawiasów PlayerMenuWidget.cpp: 3637/3637,
- API USlider sprawdzone z dokumentacją Unreal Engine 5.8: OnValueChanged, OnMouseCaptureEnd, OnControllerCaptureEnd i SetStepSize są dostępne,
- PR #65 jest mergeable; branch jest 0 commitów za main.

Następny konkretny krok:
1. uruchomić Scripts/test.sh w środowisku z lokalnym checkoutem,
2. uruchomić UE 5.8 UHT/UBT,
3. wykonać wizualny odbiór 16:9 / 16:10 / ultrawide / 4K, safe zone, mouse/keyboard/gamepad, reduced motion i blur off,
4. po zaliczeniu odbioru oznaczyć PR jako ready i zmergować zgodnie z AGENTS.md.

Luki weryfikacyjne:
- kontener tej sesji nie ma DNS do github.com, więc lokalny checkout/test.sh nie został uruchomiony,
- GitHub Actions nie były uruchamiane zgodnie z AGENTS.md (brak osobnego polecenia),
- runtime UE 5.8 nie został uruchomiony dla tego PR,
- brak wizualnego QA toastów, modali, suwaków audio, focusu i trzech kolumn.

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
- Expanded official GUGiK CityGML ingestion from six named landmarks to every safely footprint-matched building inside active game sectors. Results are grouped into 128 m mesh cells, matching OSM blockouts are excluded, and six central landmarks remain the manual hero-QA set. Unreal visual/performance QA remains pending.
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
| P1 | Wroclaw landmarks | PARTIAL | City-wide official GUGiK LoD1 import is wired for active sectors and six central landmarks are tracked for hero QA; run real download + UE bake, measure match coverage, then perform visual/LOD/collision QA |
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
