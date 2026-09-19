# Building HLOD profiles

HLOD planning now combines selected building quality with proxy budgets, streaming priority and Data Layer identity.

## Runtime/bake contract

Each assignment contains:
- HLOD layer, cluster and transition distances;
- Nanite/material merge/reduction settings;
- UV and silhouette protection policy;
- estimated proxy triangle/material counts when source metrics are known;
- per-building memory budget and streaming priority;
- Data Layer binding;
- deterministic `rebuild_key` derived from building ID + quality + profile.

Hero buildings keep the largest transition distance, highest budget and silhouette protection. Background buildings receive aggressive simplification and the smallest budget.

The rebuild key makes source/profile changes detectable without relying on transient editor state, while aggregate totals allow the world validation step to enforce proxy budgets.
