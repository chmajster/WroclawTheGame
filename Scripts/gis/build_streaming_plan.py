#!/usr/bin/env python3
"""Build and audit a sector-level World Partition/HLOD budget plan."""
from __future__ import annotations
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_BUDGETS=ROOT/"Data"/"city_streaming_budgets.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"city.json"

def quality_counts(quality):
    result={}
    if not quality:return result
    for decision in quality.get("decisions",[]):
        selected=decision.get("selected") or {}
        sector=decision.get("sector") or selected.get("sector")
        if not sector:continue
        q=decision.get("quality","background")
        result.setdefault(sector,{}).setdefault(q,0);result[sector][q]+=1
    return result

def plan(config,city,quality=None):
    qcounts=quality_counts(quality);plans=[];issues=[]
    for sector in city.get("sectors",[]):
        ring=str(sector.get("ring",3));budget=config["rings"].get(ring,config["rings"]["3"])
        counts=qcounts.get(sector["id"],{})
        if counts:
            estimated=sum(count*config["quality_cost_triangles"].get(q,config["quality_cost_triangles"]["background"]) for q,count in counts.items())
            full=sum(count for q,count in counts.items() if q in {"hero","detailed","standard"})
            heroes=counts.get("hero",0)
            source="quality_catalog"
        else:
            building_count=int(sector.get("counts",{}).get("buildings",0))
            estimated=building_count*config["quality_cost_triangles"]["background"]
            full=min(building_count,budget["max_full_detail_buildings"])
            heroes=0;source="sector_building_count_fallback"
        status="within_budget"
        if estimated>budget["max_estimated_visible_triangles"]:
            status="over_budget";issues.append({"sector":sector["id"],"code":"triangle_budget","estimated":estimated,"limit":budget["max_estimated_visible_triangles"]})
        if full>budget["max_full_detail_buildings"]:
            status="over_budget";issues.append({"sector":sector["id"],"code":"full_detail_count","estimated":full,"limit":budget["max_full_detail_buildings"]})
        if heroes>budget["max_hero_buildings"]:
            status="over_budget";issues.append({"sector":sector["id"],"code":"hero_count","estimated":heroes,"limit":budget["max_hero_buildings"]})
        plans.append({
          "sector":sector["id"],"district":sector.get("district"),"ring":int(ring),"status":status,"count_source":source,
          "quality_counts":counts,"estimated_visible_triangles":estimated,
          "streaming":{"cell_size_m":config["cell_size_m"],**budget},
          "representation_rules":config["representation_by_distance"]
        })
    return {"schema_version":1,"status":"PASS" if not issues else "REQUIRES_TUNING","issues":issues,"sectors":plans}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--budgets",type=Path,default=DEFAULT_BUDGETS);p.add_argument("--city",type=Path,default=DEFAULT_CITY);p.add_argument("--quality",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    cfg=json.loads(a.budgets.read_text());city=json.loads(a.city.read_text());quality=json.loads(a.quality.read_text()) if a.quality else None
    result=plan(cfg,city,quality);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n",encoding="utf-8");print(result["status"],len(result["issues"]),"issues")
if __name__=="__main__":main()
