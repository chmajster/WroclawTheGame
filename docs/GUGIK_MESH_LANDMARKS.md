# GUGiK textured 3D mesh — landmark pipeline

This pipeline prepares high-detail, textured landmark candidates from the official GUGiK 3D-mesh index. GUGiK distributes mesh data as OBJ with image textures. The Wrocław photogrammetric dataset published in 2026 is based on 2025 data.

## Scope

Configured hero candidates are listed in `Data/gugik_mesh_landmarks.json`. The first pass covers ten landmarks: Old Town Hall, Centennial Hall, Cathedral, University main building, Market Hall, National Museum, Main Station, Opera, NFM and Hydropolis.

## Command

```powershell
.\Scripts\Prepare-GUGiKMeshLandmarks.ps1
.\Scripts\Prepare-GUGiKMeshLandmarks.ps1 -Target old_town_hall,centennial_hall
```

The script queries the official WMS index, downloads intersecting ZIP tiles, records SHA-256, extracts OBJ/MTL/textures and crops geometry in PL-2000 zone 6 (EPSG:2177).

Products are kept under `Saved/GUGiKMeshLandmarks` because a raw source tile is not a production-ready Unreal asset.

## Required continuation gates

A candidate is not complete until all of the following are done:

1. verify the downloaded tile metadata/date/CRS;
2. merge overlapping candidate OBJ tiles and remove unrelated terrain/trees;
3. reduce geometry without destroying silhouette/UVs;
4. preserve or rebuild material/texture bindings;
5. create a versioned `Pipeline/assets/WroclawHeroLandmarks/<asset>.asset.json`;
6. run `Pipeline/Invoke-WTGAssetPipeline.ps1`;
7. verify Nanite/LOD/collision in UE 5.8;
8. capture required QA screenshots;
9. compare against real reference imagery and record visual PASS;
10. only then publish the asset and wire it into the city quality resolver.

Do not commit raw downloaded city tiles as if they were final art assets.
