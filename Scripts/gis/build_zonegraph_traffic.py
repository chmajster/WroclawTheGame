#!/usr/bin/env python3
import argparse,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/traffic_simulation_policy.json"

def parse_speed(value,highway,cfg):
 if value:
  m=re.search(r"(\d+(?:\.\d+)?)",str(value))
  if m:
   speed=float(m.group(1))
   if "mph" in str(value).lower():speed*=1.609344
   return round(speed,1)
 return float(cfg["default_speed_kph"].get(highway,cfg["default_speed_kph"]["default"]))

def parse_lanes(value,max_lanes):
 try:return max(1,min(max_lanes,int(float(value))))
 except (TypeError,ValueError):return 1

def lane_descriptor(e,points,direction,cfg):
 speed=parse_speed(e.get("maxspeed"),e.get("highway","default"),cfg)
 total_lanes=parse_lanes(e.get("lanes"),cfg["max_lanes_per_edge"])
 directions=int(bool(e.get("car_forward")))+int(bool(e.get("car_backward")))
 directional_lanes=max(1,round(total_lanes/max(1,directions)))
 length_m=float(e.get("length_cm",0))/100.0
 spacing=max(4.0,float(cfg["vehicle_spacing_m"]))
 capacity=max(1,int(length_m/spacing)*directional_lanes)
 return {
  "id":e["id"]+(":f" if direction=="forward" else ":b"),
  "edge":e["id"],"way":str(e.get("way","")),"points":points,"direction":direction,
  "road_class":e.get("highway","road"),"junction":e.get("junction"),"surface":e.get("surface"),
  "width_m":cfg["zonegraph_lane_width_m"],"directional_lane_count":directional_lanes,
  "speed_limit_kph":speed,"capacity":capacity
 }

def generate(cfg,city):
 nodes={n["id"]:n for n in city["road_graph"]["nodes"]};lanes=[]
 for e in city["road_graph"]["edges"]:
  if not (e.get("car_forward") or e.get("car_backward")):continue
  a,b=nodes[e["from"]]["position"],nodes[e["to"]]["position"]
  if e.get("car_forward"):lanes.append(lane_descriptor(e,[a,b],"forward",cfg))
  if e.get("car_backward"):lanes.append(lane_descriptor(e,[b,a],"backward",cfg))
 return {
  "schema_version":2,
  "policy":cfg["representations"],
  "handoff":{"hysteresis_m":cfg["handoff_hysteresis_m"],"despawn_grace_seconds":cfg["despawn_grace_seconds"]},
  "limits":{"physical":cfg["max_physical"],"mass":cfg["max_mass"]},
  "lanes":lanes,
  "capacity_total":sum(x["capacity"] for x in lanes)
 }

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print("lanes",len(r["lanes"]),"capacity",r["capacity_total"])
if __name__=="__main__":main()
