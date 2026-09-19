# Procedural facade layouts

The facade pipeline turns real GIS building perimeter edges into deterministic facade bays, openings and micro-architecture. It is a visual/gameplay fallback for buildings that do not have an authored or official 3D facade; generated detail is explicitly marked with `factual_facade=false`.

## Openings

The generator now creates stable descriptors for:

- entrance doors linked to `resolve_building_entrances.py`;
- procedural fallback doors only when no resolved entrance is supplied;
- windows on every valid floor/bay;
- commercial storefronts;
- garage/shop roller shutters where the source tags support that use.

A real OSM/address entrance is projected to the nearest facade edge and becomes an interactive door descriptor with the same stable entrance ID. The ground-floor window in the selected bay is removed so the door does not overlap a window.

Existing audited repository assets are used when available:

- Kenney square/round door variants;
- Kenney detailed window modules;
- Poly Haven roller-shutter variants;
- Poly Haven exterior air-conditioner;
- project-owned wall intercom.

`Data/facade_asset_bindings.json` is the single mapping between semantic facade roles and these existing asset IDs.

## Micro-architecture

Per-profile generation also adds:

- balconies with slab + railing geometry;
- window sills and lintel descriptors;
- gutters and vertical downspouts;
- cornices;
- exterior air-conditioning units;
- intercoms;
- door steps;
- address plaques;
- storefront signs and awnings.

Profiles differ for tenements, estates, villas and mixed development, including floor rhythm, opening dimensions, balcony frequency, storefront treatment, drainage spacing and decorative probabilities.

## Day/night

Window/storefront descriptors expose the global `WindowEmissive` material parameter. This binds the facade layer to `AWTGDayNightEnvironment`, so building windows can participate in the existing night-lighting system without individual Actor ticks.

## Unreal bake

`Build-Geography.ps1 -City` now generates:

1. `Saved/CityData/entrances.json`;
2. `Saved/CityData/facades.json`;
3. `Saved/CityData/roof_details.json`.

`prepare_geography.py` reads `facades.json` and places existing mesh-backed details plus simple procedural detail geometry through `AWTGFacadeInstanceCluster`.

The cluster actor uses `UHierarchicalInstancedStaticMeshComponent` and groups instances by 128 m world cells and mesh. This avoids one-Actor-per-window/door overhead and remains compatible with World Partition/HLOD. Buildings replaced by official GUGiK 3D models are excluded from procedural facade placement.

## Source-of-truth rule

OSM/GIS data determines the building footprint and resolved entrances. The remaining facade rhythm and decorative details are deterministic procedural fallbacks and must not be described as an exact reconstruction of a photographed/private facade.
