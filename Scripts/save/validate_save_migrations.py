#!/usr/bin/env python3
import argparse,json
from collections import deque
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
REGISTRY=ROOT/"Data/save_migration_registry.json"

def state_key(version,space):return (int(version),str(space))

def build_plan(registry,save_version,save_space,legacy=False):
 campaign=registry["campaign"];target=state_key(campaign["current_version"],campaign["current_coordinate_space"]);start=state_key(save_version,save_space)
 if start[0]>target[0] or start[0]<=0:return None
 if start==target:return []
 q=deque([(start,[])]);seen={start}
 while q:
  state,steps=q.popleft()
  for step in campaign["supported"]:
   if state!=state_key(step["from_version"],step["from_space"]):continue
   if step.get("legacy_slot_only") and not legacy:continue
   nxt=state_key(step["to_version"],step["to_space"]);path=steps+[step["id"]]
   if nxt==target:return path
   if nxt not in seen:seen.add(nxt);q.append((nxt,path))
 return None

def validate_registry(registry):
 issues=[];seen=set()
 for step in registry["campaign"]["supported"]:
  key=(step["from_version"],step["to_version"],step["from_space"],step["to_space"],bool(step.get("legacy_slot_only")))
  if key in seen:issues.append({"code":"duplicate_step","step":step.get("id")})
  seen.add(key)
 for name,spec in registry.get("system_payloads",{}).items():
  if int(spec.get("current_version",0))<1:issues.append({"code":"invalid_system_version","system":name})
 for category,mapping in registry.get("id_remaps",{}).items():
  if len(mapping)!=len(set(mapping)):issues.append({"code":"duplicate_remap","category":category})
 return issues

def migrate_payloads(registry,payloads):
 result=json.loads(json.dumps(payloads or {}))
 for name,spec in registry.get("system_payloads",{}).items():
  if spec.get("storage","").startswith("SystemPayloads."):
   current=int(spec["current_version"]);existing=result.get(name)
   if existing is None:
    result[name]={"schema_version":current,"json":spec["default_json"]}
   elif int(existing.get("schema_version",0))>current:
    raise ValueError(f"newer system payload: {name}")
   else:
    existing["schema_version"]=current
    if not existing.get("json"):existing["json"]=spec["default_json"]
 return result

def remap_id(registry,category,value):
 return registry.get("id_remaps",{}).get(category,{}).get(value,value)

def backup_slot(registry,slot):
 return slot+registry["rules"].get("backup_suffix","_pre_migration_backup")

def main():
 p=argparse.ArgumentParser();p.add_argument("--registry",type=Path,default=REGISTRY);p.add_argument("--fixture",type=Path);a=p.parse_args();r=json.loads(a.registry.read_text());issues=validate_registry(r)
 if a.fixture:
  f=json.loads(a.fixture.read_text());plan=build_plan(r,f["version"],f["coordinate_space"],f.get("legacy",False))
  if plan!=f["expected_steps"]:issues.append({"code":"fixture_plan_mismatch","expected":f["expected_steps"],"actual":plan})
 print(json.dumps({"status":"PASS" if not issues else "FAIL","issues":issues},indent=2));return 0 if not issues else 1
if __name__=="__main__":raise SystemExit(main())
