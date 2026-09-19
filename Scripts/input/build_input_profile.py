#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ACTIONS=ROOT/"Data/input_actions.json";DEFAULTS=ROOT/"Data/input_profile_defaults.json"

def action_index(catalog):
 out={}
 for context in catalog["contexts"]:
  for action in context["actions"]:out[action["id"]]={"context":context["id"],"priority":context["priority"],**action}
 return out

def build_profile(catalog,defaults,overrides=None):
 profile={**defaults,"rebindings":dict(defaults.get("rebindings",{}))}
 profile["rebindings"].update(overrides or {})
 idx=action_index(catalog);issues=[]
 for action_id,binding in profile["rebindings"].items():
  if action_id not in idx:issues.append({"code":"unknown_action","action":action_id});continue
  if not idx[action_id].get("rebindable",False):issues.append({"code":"not_rebindable","action":action_id})
 for device in ("keyboard","gamepad"):
  claimed={}
  for action_id,a in idx.items():
   bindings=profile["rebindings"].get(action_id,{}).get(device,a.get(device,[]))
   if isinstance(bindings,str):bindings=[bindings]
   for key in bindings:
    scope=(a["context"],device,key)
    if scope in claimed and claimed[scope]!=action_id:issues.append({"code":"binding_conflict","context":a["context"],"device":device,"key":key,"actions":sorted([claimed[scope],action_id])})
    claimed[scope]=action_id
 return {"schema_version":2,"settings":{k:v for k,v in profile.items() if k!="rebindings"},"rebindings":profile["rebindings"],"issues":issues,"status":"PASS" if not issues else "FAIL"}

def main():
 p=argparse.ArgumentParser();p.add_argument("--actions",type=Path,default=ACTIONS);p.add_argument("--defaults",type=Path,default=DEFAULTS);p.add_argument("--overrides",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 overrides=json.loads(a.overrides.read_text()) if a.overrides else None;r=build_profile(json.loads(a.actions.read_text()),json.loads(a.defaults.read_text()),overrides);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"]);return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
