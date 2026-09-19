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
def reachable(g,a,b):
 q=deque([a]);seen={a}
 while q:
  x=q.popleft()
  if x==b:return True
  for y in g.get(x,()):
   if y not in seen:seen.add(y);q.append(y)
 return False
def validate(quests,city):
 g=district_graph(city);issues=[]
 for q in quests["quests"]:
  stages=q.get("stages",[])
  if len({s["district"] for s in stages})<2:issues.append({"quest":q["id"],"code":"single_district"})
  for a,b in zip(stages,stages[1:]):
   if a["district"] not in g or b["district"] not in g:issues.append({"quest":q["id"],"code":"unknown_district","from":a["district"],"to":b["district"]})
   elif not reachable(g,a["district"],b["district"]):issues.append({"quest":q["id"],"code":"unreachable","from":a["district"],"to":b["district"]})
 return {"schema_version":1,"status":"PASS" if not issues else "FAIL","issues":issues}
def main():
 p=argparse.ArgumentParser();p.add_argument("--quests",type=Path,default=QUESTS);p.add_argument("--city",type=Path,default=CITY);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=validate(json.loads(a.quests.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"]);return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
