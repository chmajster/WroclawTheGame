# Building quality hierarchy

The resolver selects exactly one representation for each building and records why higher-quality candidates were rejected.

Order:

`textured_mesh → lod2 → lod1 → osm`

A textured mesh is intentionally stricter: it must have geometry, texture data and a recorded QA PASS. A failed or unreviewed hero candidate never hides a valid lower-quality representation.

Example:

```bash
python Scripts/gis/building_quality_resolver.py \
  --textured-mesh Saved/GUGiKMesh/hero.json \
  --lod2 Saved/CityData/lod2.json \
  --lod1 Saved/OfficialBuildings3D/catalog.json \
  --osm Saved/CityData/osm_buildings.json \
  --output Saved/CityData/building_quality.json
```

## Remaining integration gate

The resolver is deliberately data-source agnostic so this PR can target `main` independently. After source-specific PRs land, connect their generated catalogs to the city build and make `building_quality.json` the authoritative placement choice. Then verify that no building disappears when a higher-quality source is missing or fails QA.
