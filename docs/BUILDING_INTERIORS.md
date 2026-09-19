# Building-linked interiors

The generator creates deterministic gameplay interiors keyed to real GIS building IDs and optional resolved entrances. Generated rooms are explicitly fictional gameplay layouts; `factual_layout=false` prevents them from being presented as real private floor plans.

## Runtime layout

- buildings are classified as public, private, quest or mixed from source tags/policy;
- unit polygons are clipped to an inset of the actual building footprint;
- every floor receives a stable vertical core descriptor;
- stairs are always represented and elevator availability is profile-driven;
- a lightweight nav graph connects the vertical core to every unit and between floors;
- `layout_revision` is stable for a building/profile/floor-count combination.

This provides entrance→interior streaming and save systems with stable IDs without baking private real-world interiors into the project.
