#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_data_layers.json";CITY=ROOT/"Saved/CityData/sector.json"

def lookup(cfg,kind=None,role=None,system=None):
 for l in cfg["layers"]:
  if kind and kind in l.get("kinds",[]):return l["id"]
  if role and role in l.get("roles",[]):return l["id"]
  if system and system in l.get("systems",[]):return l["id"]
 return None

def validate_dependencies(cfg):
 layers={l["id"]:l for l in cfg["layers"]};issues=[];visiting=set();visited=set()
 def visit(layer_id,path):
  if layer_id in visiting:
   issues.append({"code":"dependency_cycle","path":path+[layer_id]});return
  if layer_id in visited:return
  if layer_id not in layers:
   issues.append({"code":"unknown_layer_dependency","layer":path[-1] if path else None,"dependency":layer_id});return
  visiting.add(layer_id)
  for dep in layers[layer_id].get("depends_on",[]):visit(dep,path+[layer_id])
  visiting.remove(layer_id);visited.add(layer_id)
 for lid in layers:visit(lid,[])
 for set_id,ids in cfg.get("activation_sets",{}).items():
  for lid in ids:
   if lid not in layers:issues.append({"code":"unknown_activation_layer","set":set_id,"layer":lid})
 return issues

def activation_closure(cfg,layer_ids):
 layers={l["id"]:l for l in cfg["layers"]};out=set()
 def add(lid):
  if lid in out:return
  out.add(lid)
  for dep in layers.get(lid,{}).get("depends_on",[]):add(dep)
 for lid in layer_ids:add(lid)
 return sorted(out)

def generate(cfg,city,role_items=None,system_items=None):
 assignments=[]
 for f in city.get("features",[]):
  layer=lookup(cfg,kind=f.get("kind"))
  if layer:assignments.append({"id":f["id"],"layer":layer,"source":"city_feature"})
 for item in role_items or []:
  layer=lookup(cfg,role=item.get("role"))
  if layer:assignments.append({"id":item["id"],"layer":layer,"source":"role","role":item.get("role")})
 for item in system_items or []:
  layer=lookup(cfg,system=item.get("system"))
  if layer:assignments.append({"id":item["id"],"layer":layer,"source":"system","system":item.get("system")})
 dedup={(x["source"],x["id"]):x for x in assignments}
 issues=validate_dependencies(cfg)
 activation={k:activation_closure(cfg,v) for k,v in cfg.get("activation_sets",{}).items()}
 return {"schema_version":2,"status":"PASS" if not issues else "FAIL","issues":issues,"layers":cfg["layers"],"activation_sets":activation,"assignments":sorted(dedup.values(),key=lambda x:(x["layer"],x["id"]))}

def main():
 p=argparse.ArgumentParser();p.add_argument("--config",type=Path,default=CFG);p.add_argument("--city",type=Path,default=CITY);p.add_argument("--roles",type=Path);p.add_argument("--systems",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 load=lambda x:json.loads(x.read_text()) if x else None
 roles=(load(a.roles) or {}).get("items",[]);systems=(load(a.systems) or {}).get("items",[])
 r=generate(load(a.config),load(a.city),roles,systems);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["status"],len(r["assignments"]));return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
