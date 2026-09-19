# Building-linked interiors

This pass creates deterministic interior descriptors keyed to the real GIS building ID and optional resolved entrance IDs. It is a gameplay layout seed, not a claim about the real private interior.

## Remaining
1. classify public/quest/private buildings and never expose private real layouts as factual;
2. generate valid room polygons within irregular footprints;
3. stairs/elevators/fire exits and nav connectivity;
4. entrance → interior teleport/streaming actor;
5. authored hero interiors for story locations;
6. save/load interior IDs across GIS revisions;
7. UE collision/nav/light/occlusion QA.
