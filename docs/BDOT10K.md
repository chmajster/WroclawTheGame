# BDOT10k supplemental source

BDOT10k is used as a supplemental authoritative vector source, not as an automatic replacement for OSM.

Geoportal exposes class-level GPKG/GeoParquet downloads and WMS package discovery. The default first class is `OT_BUBD_A` (buildings). Additional classes can be passed explicitly without hard-coding unverified class codes.

```bash
python Scripts/gis/fetch_bdot10k.py --class OT_BUBD_A --format geoparquet
python Scripts/gis/fetch_bdot10k.py --regional-package
```

Every download is cached with URL, byte size and SHA-256.

## Remaining integration

1. choose exact classes for roads, engineering structures, land cover and utilities from the current 2021 BDOT10k schema;
2. add a GeoParquet/GPKG reader dependency or an external GDAL conversion step;
3. clip records to active Wrocław sectors;
4. map attributes into project semantics;
5. compare geometry with OSM and define conflict precedence;
6. add regression tests on a small committed fixture;
7. only after that use BDOT10k to replace or augment world geometry.
