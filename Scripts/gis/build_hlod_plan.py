#!/usr/bin/env python3
import argparse,hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/building_hlod_profiles.json"

def estimate_proxy(profile,decision):
 triangles=max(0,int(decision.get("triangles",0) or 0));materials=max(1,int(decision.get("materials",1) or 1))
 ratio=float(profile["target_triangle_ratio"])
 return {
  "estimated_triangles":max(1,int(round(triangles*ratio))) if triangles else None,
  "estimated_materials":min(materials,int(profile.get("max_materials_per_proxy",materials))),
  "memory_budget_mb":float(profile.get("memory_budget_mb",16))
 }

def stable_key(building_id,quality,profile):
 payload=json.dumps({"building_id":building_id,"quality":quality,"profile":profile},sort_keys=True,separators=(",",":"))
 return hashlib.sha256(payload.encode()).hexdigest()[:16]

def generate(cfg,quality,data_layers=None):
 out=[];counts={};layer_index={}
 for a in (data_layers or {}).get("assignments",[]):layer_index[a["id"]]=a.get("layer")
 for d in quality.get("decisions",[]):
  q=d.get("quality","background")
  if q=="missing":q="background"
  profile=dict(cfg["profiles"].get(q,cfg["profiles"]["background"]))
  profile["max_materials_per_proxy"]=cfg["max_materials_per_proxy"]
  counts[q]=counts.get(q,0)+1
  assignment={"building_id":d["building_id"],"quality":q,**profile,**estimate_proxy(profile,d)}
  assignment["data_layer"]=layer_index.get(d["building_id"],"DL_Buildings")
  assignment["rebuild_key"]=stable_key(d["building_id"],q,profile)
  assignment["streaming_priority"]=cfg["quality_priority"].get(q,cfg["quality_priority"]["background"])
  assignment["silhouette_protected"]=q=="hero"
  out.append(assignment)
 totals={
  "estimated_triangles":sum(x["estimated_triangles"] or 0 for x in out),
  "memory_budget_mb":round(sum(x["memory_budget_mb"] for x in out),2)
 }
 return {"schema_version":2,"counts":counts,"totals":totals,"assignments":out}

def main():
 p=argparse.ArgumentParser();p.add_argument("--quality",type=Path,required=True);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--data-layers",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 dl=json.loads(a.data_layers.read_text()) if a.data_layers else None;r=generate(json.loads(a.config.read_text()),json.loads(a.quality.read_text()),dl);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["counts"],r["totals"])
if __name__=="__main__":main()
