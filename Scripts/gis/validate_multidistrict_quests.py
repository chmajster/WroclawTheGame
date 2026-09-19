#!/usr/bin/env python3
import argparse,json
from collections import deque
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];QUESTS=ROOT/"Data/multidistrict_quests.json";CITY=ROOT/"Saved/CityData/city.json"

def district_graph(city):
 sector_to_district={s["id"]:s["district"] for s in city.get("sectors",[])};g={d:set() for d in sector_to_district.values()}
 for link in city.get("links",[]):
  a=sector_to_district.get(link["from"]);b=sector_to_district.get(link["to"])
  if a and b and a!=b and (link.get("car") or link.get("foot")):g.setdefault(a,set()).add(b)
 return g

def shortest_path(g,a,b):
 q=deque([a]);prev={a:None}
 while q:
  x=q.popleft()
  if x==b:
   path=[];cur=b
   while cur is not None:path.append(cur);cur=prev[cur]
   return list(reversed(path))
  for y in sorted(g.get(x,())):
   if y not in prev:prev[y]=x;q.append(y)
 return None

def reachable(g,a,b):return shortest_path(g,a,b) is not None

def catalog_ids(catalog,key):
 return {x["id"] for x in (catalog or {}).get(key,[])}

def compile_quests(quests,city,pois=None,landmarks=None,entrances=None):
 g=district_graph(city);poi_ids=catalog_ids(pois,"pois");landmark_ids=catalog_ids(landmarks,"records");entrance_ids=catalog_ids(entrances,"entrances");issues=[];compiled=[]
 for q in quests["quests"]:
  stages=q.get("stages",[]);compiled_stages=[]
  if len({s["district"] for s in stages})<2:issues.append({"quest":q["id"],"code":"single_district"})
  for i,s in enumerate(stages):
   target=s.get("target",{"type":"district_anchor","id":s["district"]});typ=target.get("type");tid=target.get("id")
   known=True
   if typ=="poi" and pois is not None:known=tid in poi_ids
   elif typ=="landmark" and landmarks is not None:known=tid in landmark_ids
   elif typ=="entrance" and entrances is not None:known=tid in entrance_ids
   elif typ=="district_anchor":known=tid in g
   if not known:issues.append({"quest":q["id"],"stage":s.get("id"),"code":"unknown_target","target":target})
   compiled_stages.append({**s,"objective_id":f"{q['id']}:{s.get('id',i)}","target":target,"gps_enabled":bool(s.get("gps_enabled",True)),"recovery":s.get("recovery","district_anchor")})
  legs=[]
  for a,b in zip(stages,stages[1:]):
   if a["district"] not in g or b["district"] not in g:
    issues.append({"quest":q["id"],"code":"unknown_district","from":a["district"],"to":b["district"]});path=None
   else:
    path=shortest_path(g,a["district"],b["district"])
    if not path:issues.append({"quest":q["id"],"code":"unreachable","from":a["district"],"to":b["district"]})
   legs.append({"from_stage":a.get("id"),"to_stage":b.get("id"),"district_path":path})
  compiled.append({"id":q["id"],"title":q.get("title"),"save_checkpoint_each_stage":q.get("save_checkpoint_each_stage",True),"stages":compiled_stages,"route_legs":legs})
 return {"schema_version":2,"status":"PASS" if not issues else "FAIL","issues":issues,"compiled_quests":compiled}

def validate(quests,city,pois=None,landmarks=None,entrances=None):
 return compile_quests(quests,city,pois,landmarks,entrances)

def save_state(quest_id,stage_id,world_mutations=None):
 return {"quest_id":quest_id,"stage_id":stage_id,"world_mutations":world_mutations or [],"schema_version":2}

def main():
 p=argparse.ArgumentParser();p.add_argument("--quests",type=Path,default=QUESTS);p.add_argument("--city",type=Path,default=CITY);p.add_argument("--pois",type=Path);p.add_argument("--landmarks",type=Path);p.add_argument("--entrances",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 load=lambda x:json.loads(x.read_text()) if x else None;r=validate(load(a.quests),load(a.city),load(a.pois),load(a.landmarks),load(a.entrances));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2,ensure_ascii=False)+"\n");print(r["status"]);return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
