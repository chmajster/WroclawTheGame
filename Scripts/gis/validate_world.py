#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];POLICY=ROOT/"Data/world_validation_policy.json";SECTOR=ROOT/"Saved/CityData/sector.json";CITY=ROOT/"Saved/CityData/city.json"
def load(path):return json.loads(path.read_text()) if path and path.is_file() else None
def validate(policy,sector,city,entrances=None,quality=None,quests=None,streaming=None,landmarks=None):
 issues=[]
 def add(severity,code,message):issues.append({"severity":severity,"code":code,"message":message})
 ids=set()
 for f in sector.get("features",[]):
  fid=f.get("id")
  if fid in ids:add("error","duplicate_feature_id",fid)
  ids.add(fid)
  coords=(f.get("rings",[[]])[0] if f.get("rings") else f.get("points",[]))
  for p in coords:
   if not all(math.isfinite(float(v)) for v in p):add("error","nonfinite_coordinate",fid);break
   if any(abs(float(v))>policy["max_abs_coordinate_cm"] for v in p):add("error","coordinate_outlier",fid);break
 nodes={n["id"] for n in sector["road_graph"]["nodes"]}
 edge_ids=set()
 for e in sector["road_graph"]["edges"]:
  if e["id"] in edge_ids:add("error","duplicate_edge_id",e["id"])
  edge_ids.add(e["id"])
  if e["from"] not in nodes or e["to"] not in nodes:add("error","missing_edge_node",e["id"])
  length=float(e["length_cm"])
  if not policy["min_road_edge_cm"]<=length<=policy["max_road_edge_cm"]:add("error","edge_length",e["id"])
 building_ids={f["id"] for f in sector.get("features",[]) if f.get("kind")=="building"}
 for e in (entrances or {}).get("entrances",[]):
  if e.get("building_id") not in building_ids:add("error","orphan_entrance",e.get("id","?"))
 seen_quality=set()
 for q in (quality or {}).get("decisions",[]):
  bid=q.get("building_id")
  if bid in seen_quality:add("error","duplicate_quality",str(bid))
  seen_quality.add(bid)
 districts={s["district"] for s in city.get("sectors",[])}
 for q in (quests or {}).get("quests",[]):
  for stage in q.get("stages",[]):
   if stage.get("district") not in districts:add("error","unknown_quest_district",q.get("id","?"))
 city_sectors={s["id"] for s in city.get("sectors",[])}
 for s in (streaming or {}).get("sectors",[]):
  if s.get("sector") not in city_sectors:add("error","unknown_streaming_sector",str(s.get("sector")))
 if policy.get("unresolved_hero_is_error"):
  for l in (landmarks or {}).get("records",[]):
   if l.get("quality")=="hero" and l.get("status")=="unresolved":add("error","unresolved_hero_landmark",l.get("id","?"))
 errors=sum(x["severity"]=="error" for x in issues);warnings=sum(x["severity"]=="warning" for x in issues)
 return {"schema_version":1,"status":"PASS" if errors==0 else "FAIL","errors":errors,"warnings":warnings,"issues":issues,"counts":{"features":len(sector.get("features",[])),"road_nodes":len(nodes),"road_edges":len(edge_ids),"buildings":len(building_ids)}}
def main():
 p=argparse.ArgumentParser();p.add_argument("--policy",type=Path,default=POLICY);p.add_argument("--sector",type=Path,default=SECTOR);p.add_argument("--city",type=Path,default=CITY);p.add_argument("--entrances",type=Path);p.add_argument("--quality",type=Path);p.add_argument("--quests",type=Path);p.add_argument("--streaming",type=Path);p.add_argument("--landmarks",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 r=validate(load(a.policy),load(a.sector),load(a.city),load(a.entrances),load(a.quality),load(a.quests),load(a.streaming),load(a.landmarks));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"],r["errors"],"errors");return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
