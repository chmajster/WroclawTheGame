# City Data Layers

The plan separates base geometry, buildings, street furniture, traffic, crowd, interiors, quests and debug content. Runtime/default-loading intent is explicit.

## Remaining
1. create DataLayer assets in UE 5.8;
2. assign generated actors during editor bake;
3. ensure PCG output inherits intended Data/HLOD Layers;
4. stream interiors only while entered/needed by quests;
5. verify quest actors survive layer changes correctly;
6. test cook/runtime layer activation and memory impact.
