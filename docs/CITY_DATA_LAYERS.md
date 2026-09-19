# City Data Layers

The Data Layer plan now models runtime dependencies and activation sets, not only labels.

## Runtime contract

Each layer declares whether it is runtime-capable, its default state and explicit dependencies. Dependency closure guarantees, for example, that activating interiors also activates buildings and base geometry.

Named activation sets cover normal gameplay, interior-focused streaming, low-population mode and debug content. The planner can assign:
- GIS features by `kind`;
- PCG/generated content by semantic `role`;
- simulation/runtime objects by `system`.

The plan validates unknown dependencies, activation-set references and dependency cycles before export. Stable layer IDs remain the contract for editor-generated DataLayer assets and runtime activation logic.
