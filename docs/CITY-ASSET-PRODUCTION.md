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
