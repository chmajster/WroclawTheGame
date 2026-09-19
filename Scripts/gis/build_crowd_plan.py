#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/crowd_simulation_policy.json"
def generate(cfg,city):
 nodes={n["id"]:n for n in city["road_graph"]["nodes"]};corridors=[]
 for e in city["road_graph"]["edges"]:
  if not e.get("foot"):continue
  corridors.append({"id":"foot:"+e["id"],"edge":e["id"],"points":[nodes[e["from"]]["position"],nodes[e["to"]]["position"]],"road_name":e.get("name","")})
 crossings=[]
 for f in city.get("features",[]):
  if f.get("kind")=="entrance":continue
  tags=f.get("tags",{})
  if tags.get("highway")=="crossing" and f.get("points"):
   crossings.append({"id":f["id"],"position":f["points"][0]})
 return {"schema_version":1,"limits":{"physical":cfg["max_physical"],"mass":cfg["max_mass"]},"representations":cfg["representations"],"corridors":corridors,"crossings":crossings}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["corridors"]),len(r["crossings"]))
if __name__=="__main__":main()
