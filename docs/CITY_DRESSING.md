# PCG-ready city dressing

The generator emits deterministic, spatially partitioned placement candidates for lamps, bollards, benches, bins and bicycle racks from real GIS features. It does not pretend these are surveyed individual objects.

## Runtime-ready placement contract

Each generated placement now contains a stable ID, asset role, deterministic visual variant, world position, spatial cell, yaw, road side, uniform scale, collision policy, navigation behavior and Data Layer assignment. Road furniture is oriented from the road tangent; park/green-area furniture faces toward the feature interior.

The catalogue is therefore directly suitable as an input to a UE PCG/Data Asset importer without another geometry inference pass. Asset names remain semantic roles/variants rather than claims that a specific surveyed prop exists at a location.

## Validation

- identical GIS/config input produces byte-stable placement descriptors;
- variant and scale selection are deterministic;
- road-side orientation is explicit;
- every item carries collision/nav/Data Layer metadata.

## Follow-up

Verified OSM/BDOT10k furniture can override procedural candidates when available. Production mesh binding remains data-driven so models can be replaced without regenerating GIS placement IDs.
