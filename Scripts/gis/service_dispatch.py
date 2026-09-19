#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
from road_routes import route
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/emergency_response_policy.json"
def nearest_node(graph,pos):
 best=None
 for n in graph["nodes"]:
  d=math.dist(n["position"][:2],pos[:2])
  if best is None or d<best[0]:best=(d,n["id"])
 return best[1]
def dispatch(city,depots,incident,service="police"):
 graph=city["road_graph"];target=nearest_node(graph,incident["position"]);candidates=[]
 for d in depots:
  if d.get("service")!=service:continue
  start=nearest_node(graph,d["position"])
  try:path=route(graph,start,target,"car")
  except ValueError:continue
  candidates.append((len(path),d,start,path))
 if not candidates:return None
 _,depot,start,path=min(candidates,key=lambda x:(x[0],x[1]["id"]))
 return {"service":service,"depot_id":depot["id"],"start_node":start,"target_node":target,"route_nodes":path,"incident":incident}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--depots",type=Path,required=True);p.add_argument("--incident",type=Path,required=True);p.add_argument("--service",default="police");a=p.parse_args();r=dispatch(json.loads(a.city.read_text()),json.loads(a.depots.read_text())["depots"],json.loads(a.incident.read_text()),a.service);print(json.dumps(r,ensure_ascii=False))
if __name__=="__main__":main()
