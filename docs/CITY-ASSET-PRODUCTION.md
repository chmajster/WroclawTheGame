# City asset production — 2026-10-12

The city is a development prototype. This pass adds reusable production assets
and an executable authoring/acquisition/import/placement workflow. It does not
claim an AAA city, surveyed facade reconstruction, or completed hero landmarks.

## Ordered change streams

1. Blender: authored ornamented iron balcony railing, entrance surround and
   cornice; editable sources, UVs, PBR maps and production manifests.
2. Legal acquisition: a bounded, checksum-verified CC0 acquisition registry,
   source/author/license records and no duplicate downloads.
3. Pipeline: geometry/material/collision validation and repeatable batch import.
4. Wroclaw: source-data placement of the new architecture kit, priority landmark
   coverage records; unresolved landmark geometry stays explicitly unresolved.
5. Graphics: measured asset budgets, renderer/streaming compatibility and
   scalable instances, reusing existing Lumen, weather and lighting systems.
6. Execution: one command for the implemented workflow, real test/import evidence
   and precise continuation for the remaining city/hero art.

## Sources and fidelity

Existing OSM/GIS footprint and facade descriptors remain authoritative for
placement. Repeated generic architectural details are stylistic enrichment,
not verified reconstruction of a named building. We retain the existing playable
district, missions, vehicles and materials.

Architectural reference (viewing only, photographs are not redistributed or
used as textures): Wroclaw municipality, photographs by Oleksandr Poliakovsky,
https://www.wroclaw.pl/dla-mieszkanca/nadodrze-krolestwem-kamienic-z-pieknymi-detalami-zdjecia.
Poly Haven architecture catalog was searched; existing imported props are
reused, while no suitable verified free Nadodrze ornamental kit was found.
https://polyhaven.com/models/architecture.

Geoportal mesh candidates are not production assets: free download alone is
insufficient evidence of redistribution rights. Keep them out of the production
registry until exact license and model suitability are verified.

## Authoring evidence (stream 1)

Blender 5.2.2 LTS executed both authoring and the repository FBX pipeline.
All 13 production manifests passed validation. Each new source has a UV layer
and a single opaque PBR material. Source Cycles previews were visually inspected;
the arch remains open and the railing is geometry with welded ornament supports.

| Asset | Size in metres | LOD0 / LOD1 / LOD2 triangles |
|---|---|---|
| Ornamented balcony railing | 2.50 × 1.10 × 1.107 | 25,912 / 15,546 / 7,773 |
| Entrance surround | 2.80 × 0.48 × 3.89 | 6,508 / 3,904 / 1,952 |
| Dentil cornice | 3.00 × 0.49 × 0.44 | 4,700 / 2,820 / 1,410 |

Rebuild: install Scripts/assets/requirements.txt into a project-local Python
environment, run Scripts/assets/make_kit_textures.py, then run Blender with
`--background --python-exit-code 1 --python Scripts/assets/build_tenement_kit.py -- --render`.
The production manifests select three FBX LODs in Pipeline/blender/build_asset.py.
Generated reports/previews live in Saved/Pipeline; editable sources and GLB
exports are versioned in SourceAssets/Models/Wroclaw/TenementKit.
These source checks do not establish successful Unreal import or runtime QA.

## District integration (stream 4)

Five authored sources now include a supported balcony floor (2256/1352/676
triangles) and panelled double door (4412/2646/1322, three material slots).
The door reuses versioned project Wood/Metal/Glass materials and is decorative;
existing entrances/interactions remain authoritative. Native placement saved
904 instances on 44 GIS building descriptors in 28 HISM clusters, preserving
other district actors. Generated map remains a derivative, not the source of truth.

Data/city_detail_policy.json limits the batch; build_city_detail_plan.py derives
transforms and SHA256 source locks. place_city_details.py checks source locks
and replaces only CityKit_ actors. Portals too tall for a balcony above are
excluded, cornices use repeated modules instead of stretched dentil spacing.
Legacy FBX changes Blender +Y to native -Y; placement rotates authored fronts
towards facade normals. prepare_city_asset_qa.py recreates studio/street cameras.

Four transform/overlap tests PASS. Native captured views found handedness,
exposure and fallback-detail issues; fixes are tracked separately from capture
success. Screenshot tasks have timeout and fresh-file/hash checks, with an
explicit NOT_REVIEWED state. No claim of a reconstructed historic facade or
an interactive interior follows from these details.

## Graphics policy (stream 5)

Data/city_asset_quality.json defines full conventional Nanite fallback for small
stone profiles, three LOD thresholds (1/.22/.06) and HISM culling at 180–240 m.
Pipeline/unreal/city_quality.py applies it on import and placement, as does
configure_city_asset_quality.py for existing native products. PBR textures keep
mips/streaming, max 2k, DirectX normals, sRGB base color and linear packed ORM
with Masks compression/sampler. HISM/Nanite material usage is explicitly saved.
Three existing door instances receive HISM usage overrides; shared masters stay
unchanged. Existing renderer Lumen/lighting/weather systems are reused.

Native settings saved/verified on all five assets and 28 components. Budget PASS:
6,549,600 instanced LOD0 triangles; six unique source maps (26,077,960 bytes).
Conservative uncompressed full-mip RGBA upper bound 83,886,084 bytes; this is
not measured resident VRAM. Two limit/accounting tests PASS. This district is
not World Partition and these checks do not establish whole-city FPS or HLOD
streaming. Those require a separate source-driven large-world production stream.

## Reproduce this batch (stream 6)

From the repository on this machine:

```powershell
./Scripts/Invoke-CityAssetProduction.ps1 `
  -Python './Saved/GISPython/Scripts/python.exe' -GameplayChecks
```

Use -EngineRoot/-Blender for other installations. The Python environment needs
Scripts/assets/requirements.txt; the existing district source generator also
needs Scripts/gis/requirements.txt when Saved/PlayableDistrict is missing.
The default verifies/reuses reviewed sources, exports five FBX products, builds
the editor module, imports, places, applies quality, creates QA scenes, runs the
five-asset native test and captures five views. -GameplayChecks adds district
rules/save/full-session/replay regression tests. No CI is invoked.

- -SourcesOnly runs source/license/manifest/tests/GIS budget checks, records
  SOURCE_CHECKS_PASS and explicitly leaves native work NOT_RUN.
- -DownloadReviewed downloads only missing files from the approved CC0 registry.
- -RebuildSources regenerates original textures and all five editable sources;
  use it deliberately because source .blend edits are otherwise retained.
- -PrepareDistrict uses the existing district builder. Its imported free-model
  catalog (Saved/FreeModelImportMap.json) is an explicit prerequisite; on a fresh
  checkout import the audited catalog with Scripts/import_free_models.py first.
  Rebuild the district when base GIS inputs change; default mode enriches the
  existing baked district using its associated facade descriptors.

Saved/CityDetails/production.json records the current stage, real success/failure
and continuation. Native markers are deleted before every stage, exit status,
freshness and Python/material errors checked; empty Automation reports fail.
QA output is NOT_REVIEWED until explicit inspection. After viewing all five:

```powershell
python Scripts/assets/record_city_visual_review.py `
  --reviewer 'reviewer name' --status PASS_WITH_LIMITATIONS `
  --notes 'observed geometry, materials, placement and remaining limitations'
```

The recorder rejects changed/missing PNGs or changed native map/asset fingerprints.
It records an explicit review rather than pretending captures alone validate art.
Generated .umap/FBX/reports remain derived outputs; source GIS, manifests,
editable originals and native mesh/material/texture products are versioned.

Integrated run 20261012-201713 completed: five exports/imports, saved native
placement/quality, five-asset Automation, five rendered views and district
rules/save/delivery-session/replay all PASS. Explicit five-view inspection is
PASS_WITH_LIMITATIONS; see [archived native evidence](qa/city-assets-2026-10-12/README.md).
The first batch is complete. Broader priority landmarks, detailed facade art,
World Partition/HLOD conversion and whole-city performance remain future work.
Next start market_and_town_hall in Data/city_asset_needs.json, using real geometry,
legal references and a production manifest rather than a placeholder landmark.

| Stream | PR |
|---|---|
| Blender original sources | [#146](https://github.com/chmajster/WroclawTheGame/pull/146) |
| Audited CC0 acquisition | [#147](https://github.com/chmajster/WroclawTheGame/pull/147) |
| Native pipeline import | [#148](https://github.com/chmajster/WroclawTheGame/pull/148) |
| GIS district placement | [#149](https://github.com/chmajster/WroclawTheGame/pull/149) |
| Native quality and budgets | [#150](https://github.com/chmajster/WroclawTheGame/pull/150) |
| Executable production and validation | [#151](https://github.com/chmajster/WroclawTheGame/pull/151) |

## Acquisition evidence (stream 2)

Acquired Sandstone Cracks by Rob Tuytel under verified Poly Haven CC0-1.0:
three 2048² maps, 24,148,414 bytes total, 2 m physical tile. Provider MD5 and
independent SHA-256 checks passed, followed by a fully offline run reusing all
three files. Five infrastructure/prop roles reuse the existing library of 66
Poly Haven models; the audit checks actual source files. Exact credits and API
terms: docs/CITY-ASSET-CREDITS.md. Priority hero gaps remain explicit in
Data/city_asset_needs.json. No Geoportal candidate was downloaded or relabeled.

## Import evidence (stream 3)

Three meshes and six shared textures were actually imported into
`/Game/Wroclaw/TenementKit`. Native bounds match metre sources converted to cm;
three LODs and one PBR slot per mesh are retained. Stone details use Nanite;
thin ironwork uses conventional LODs. Three authored UCX guard panels leave
the rear and center of the balcony open. Stone trim has no collision.

Two legacy FBX problems were found: reimport ignored unit conversion and
temporary LOD defaults appended material slots. The importer now creates a
fresh product in an isolated temporary package, replaces the manifest's stable
path and matches material interfaces before attaching LODs. Artist edits to
pipeline products belong in the source/manifest. Start on /Engine/Maps/Entry
and delay shutdown until tool contexts settle; Python commandlet mode lacks
the required StaticMeshEditorSubsystem.

Two consecutive passes completed in one process (six imports). Native
WTG.Assets.TenementKit PASS; final editor build PASS (7.01 s). The test checks
source LODs separately from Nanite's derived fallback, scale, PBR slots, Nanite
and open collision space. Five pipeline validation tests PASS. Automation
report validation now rejects empty, failed and unfinished test queues.
17 generic engine startup smoke errors precede the requested test; its result
is Success without errors. Screenshots/appearance verification follows later.
