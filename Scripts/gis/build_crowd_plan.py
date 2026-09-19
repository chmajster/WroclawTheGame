#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/crowd_simulation_policy.json"

def clamp(v,a,b):return max(a,min(b,v))

def edge_density(edge,cfg):
 klass=edge.get("highway","default")
 return float(cfg["density_by_road_class"].get(klass,cfg["density_by_road_class"]["default"]))

def activity_multiplier(day_night=None,weather=None):
 mult=1.0
 if day_night:mult*=float(day_night.get("crowd_multiplier",1.0))
 if weather:
  mult*=float(weather.get("crowd_multiplier",weather.get("crowd",1.0)))
 return max(0.0,mult)

def generate(cfg,city,day_night=None,weather=None):
 nodes={n["id"]:n for n in city["road_graph"]["nodes"]};corridors=[]
 activity=activity_multiplier(day_night,weather)
 for e in city["road_graph"]["edges"]:
  if not e.get("foot"):continue
  points=[nodes[e["from"]]["position"],nodes[e["to"]]["position"]]
  length_m=float(e.get("length_cm",math.dist(points[0],points[1])))/100.0
  base=edge_density(e,cfg);effective=round(base*activity,3)
  capacity=max(1,int(length_m/max(1.0,float(cfg["personal_spacing_m"]))*max(0.1,effective)))
  corridors.append({
   "id":"foot:"+e["id"],"edge":e["id"],"points":points,"road_name":e.get("name",""),
   "road_class":e.get("highway","road"),"base_density":base,"effective_density":effective,
   "capacity":capacity,"walk_speed_mps":cfg["walk_speed_mps"],
   "avoidance_radius_m":cfg["avoidance_radius_m"],"representation_hysteresis_m":cfg["handoff_hysteresis_m"]
  })
 crossings=[]
 for f in city.get("features",[]):
  tags=f.get("tags",{})
  if tags.get("highway")=="crossing" and f.get("points"):
   crossings.append({"id":f["id"],"position":f["points"][0],"signalized":tags.get("crossing")=="traffic_signals" or tags.get("traffic_signals") is not None,"reservation_radius_m":cfg["crossing_radius_m"]})
 return {
  "schema_version":2,
  "limits":{"physical":cfg["max_physical"],"mass":cfg["max_mass"]},
  "representations":cfg["representations"],
  "activity_multiplier":round(activity,3),
  "corridors":corridors,"crossings":crossings,
  "capacity_total":sum(x["capacity"] for x in corridors)
 }

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--day-night-state",type=Path);p.add_argument("--weather-state",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 load=lambda x:json.loads(x.read_text()) if x else None
 r=generate(load(a.config),load(a.city),load(a.day_night_state),load(a.weather_state));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["corridors"]),len(r["crossings"]),r["capacity_total"])
if __name__=="__main__":main()
