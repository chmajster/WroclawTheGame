#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/traffic_simulation_policy.json"
def generate(cfg,city):
 nodes={n["id"]:n for n in city["road_graph"]["nodes"]};lanes=[]
 for e in city["road_graph"]["edges"]:
  if not (e.get("car_forward") or e.get("car_backward")):continue
  a,b=nodes[e["from"]]["position"],nodes[e["to"]]["position"]
  if e.get("car_forward"):lanes.append({"id":e["id"]+":f","edge":e["id"],"points":[a,b],"direction":"forward","width_m":cfg["zonegraph_lane_width_m"]})
  if e.get("car_backward"):lanes.append({"id":e["id"]+":b","edge":e["id"],"points":[b,a],"direction":"backward","width_m":cfg["zonegraph_lane_width_m"]})
 return {"schema_version":1,"policy":cfg["representations"],"limits":{"physical":cfg["max_physical"],"mass":cfg["max_mass"]},"lanes":lanes}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print("lanes",len(r["lanes"]))
if __name__=="__main__":main()
