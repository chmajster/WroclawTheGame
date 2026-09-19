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

def route_distance_cm(graph,nodes):
 total=0.0
 for a,b in zip(nodes,nodes[1:]):
  candidates=[]
  for e in graph["edges"]:
   if e["from"]==a and e["to"]==b and e.get("car_forward"):candidates.append(float(e["length_cm"]))
   if e["to"]==a and e["from"]==b and e.get("car_backward"):candidates.append(float(e["length_cm"]))
  if not candidates:return None
  total+=min(candidates)
 return total

def required_units(policy,service,incident):
 cfg=policy["services"][service]
 if service=="police":
  heat=float(incident.get("heat",0));units=cfg["units"][0]
  for threshold,count in zip(cfg.get("heat_thresholds",[]),cfg["units"]):
   if heat>=threshold:units=count
  return units
 severity=max(1,int(incident.get("severity",1)))
 return cfg["units"][min(severity-1,len(cfg["units"])-1)]

def service_accepts(policy,service,incident):
 cfg=policy["services"].get(service)
 if not cfg:return False
 types=cfg.get("incident_types")
 return types is None or incident.get("type") in types

def dispatch(city,depots,incident,service="police",policy=None):
 policy=policy or json.loads(CFG.read_text())
 if not service_accepts(policy,service,incident):
  return {"status":"rejected","service":service,"reason":"incident_type_not_supported","incident":incident}
 graph=city["road_graph"];target=nearest_node(graph,incident["position"]);units=required_units(policy,service,incident);candidates=[]
 for d in depots:
  if d.get("service")!=service or int(d.get("available_units",units))<units:continue
  start=nearest_node(graph,d["position"])
  try:path=route(graph,start,target,"car")
  except ValueError:continue
  distance_cm=route_distance_cm(graph,path)
  if distance_cm is None or distance_cm/100>policy["max_route_distance_m"]:continue
  speed_kph=float(policy["response_speed_kph"].get(service,50));eta=(distance_cm/100)/(speed_kph/3.6)
  candidates.append((eta,d["id"],d,start,path,distance_cm))
 if not candidates:
  return {"status":"unavailable","service":service,"reason":"no_reachable_depot","required_units":units,"incident":incident}
 eta,_,depot,start,path,distance_cm=min(candidates,key=lambda x:(x[0],x[1]))
 priority=policy["services"][service].get("priority","normal")
 return {
  "status":"dispatched","service":service,"depot_id":depot["id"],"start_node":start,"target_node":target,"route_nodes":path,
  "route_distance_m":round(distance_cm/100,1),"eta_seconds":round(eta,1),"units":units,"priority":priority,
  "traffic_yield":priority in {"high","critical"},"physicalize_radius_m":policy["physicalize_radius_m"],"incident":incident
 }

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--depots",type=Path,required=True);p.add_argument("--incident",type=Path,required=True);p.add_argument("--service",default="police");p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();r=dispatch(json.loads(a.city.read_text()),json.loads(a.depots.read_text())["depots"],json.loads(a.incident.read_text()),a.service,json.loads(a.config.read_text()));print(json.dumps(r,ensure_ascii=False))
if __name__=="__main__":main()
