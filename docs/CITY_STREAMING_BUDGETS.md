# City streaming budgets

This PR defines **initial planning budgets**, not measured performance claims.

Each city sector already has a ring. The planner maps that ring to:
- full-detail activation radius;
- HLOD radius;
- background-proxy radius;
- maximum estimated visible triangles;
- maximum full-detail and hero building counts;
- HLOD cluster target size.

If a building-quality catalogue is available, the planner estimates cost by selected representation. Without it, it falls back to the sector's building count and explicitly records that weaker source.

```bash
python Scripts/gis/build_streaming_plan.py \
  --city Saved/CityData/city.json \
  --quality Saved/CityData/building_quality.json \
  --output Saved/CityData/streaming_plan.json
```

## Remaining
1. connect the plan to actual World Partition runtime grid/HLOD layer generation;
2. export real triangle/material/texture-memory costs from Unreal;
3. replace estimates with measured UE 5.8 budgets on target hardware;
4. test walking and fast driving, including predictive streaming;
5. tune per district and weather/time-of-day population load;
6. make QA fail only after measured budgets are established.
