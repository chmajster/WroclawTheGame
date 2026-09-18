# Wrocław landmark catalogue

`Data/wroclaw_landmark_catalog.json` is the curated list of recognizable city anchors used by asset and gameplay pipelines.

The catalogue intentionally stores names/aliases rather than invented coordinates. `resolve_landmark_catalog.py` resolves a record only when the current GIS extract contains an exact or partial OSM name match. Unresolved objects stay explicit in the report.

## Continuation

Run after the city GIS has been generated:

```bash
python Scripts/gis/resolve_landmark_catalog.py --output Saved/CityData/landmarks.json
```

Then:
1. review unresolved records and add authoritative aliases/coordinates only from verified sources;
2. attach a preferred geometry source through the building-quality resolver;
3. bind relevant landmarks to quests/map markers;
4. run visual QA for every `quality=hero` object;
5. expand the list only after current hero landmarks have a verified representation.
