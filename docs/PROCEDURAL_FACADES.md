# Procedural facade layouts

The facade generator converts real building perimeter edges into deterministic bays/floors and produces window, balcony and door-candidate descriptors.

Profiles differ for tenements, estates, villas and mixed development. This prevents every district from receiving the same facade rhythm.

All non-source facade details are explicit procedural fallbacks. The generator does not claim the output matches a photographed facade.

## Remaining
1. bind layouts to modular production window/door/balcony meshes;
2. choose street-facing entrance from the entrance/address pipeline instead of the longest edge;
3. implement corner handling and facade occlusion;
4. generate material instances/variation without texture repetition;
5. add UE instancing/HISM/Nanite strategy and visual QA.
