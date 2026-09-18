# Building QA gates

`qa_buildings.py` is a static pre-Unreal gate. It catches repository/data failures before expensive editor QA:

- duplicate OSM replacements;
- excessive OSM match distance;
- suspicious footprint overlap;
- duplicate quality decisions;
- hero assets without explicit QA PASS;
- triangle/vertex budget overruns when mesh stats are available.

A PASS here does **not** replace UE 5.8 collision, rendering, screenshot or performance validation.

## Remaining
- emit real mesh triangle/vertex stats from all generators;
- add ground-distance/floating checks using terrain samples;
- add 2D overlap/duplicate geometry checks at sector scale;
- ingest Unreal Automation and screenshot-review reports;
- make full build refuse promotion when this gate fails.
