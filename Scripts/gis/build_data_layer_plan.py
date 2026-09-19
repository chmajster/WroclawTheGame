#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_data_layers.json";CITY=ROOT/"Saved/CityData/sector.json"
def lookup(cfg,kind=None,role=None,system=None):
 for l in cfg["layers"]:
  if kind and kind in l.get("kinds",[]):return l["id"]
  if role and role in l.get("roles",[]):return l["id"]
  if system and system in l.get("systems",[]):return l["id"]
 return None
def generate(cfg,city):
 items=[]
 for f in city.get("features",[]):
  layer=lookup(cfg,kind=f.get("kind"))
  if layer:items.append({"id":f["id"],"layer":layer,"source":"city_feature"})
 return {"schema_version":1,"layers":cfg["layers"],"assignments":items}
def main():
 p=argparse.ArgumentParser();p.add_argument("--config",type=Path,default=CFG);p.add_argument("--city",type=Path,default=CITY);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["assignments"]))
if __name__=="__main__":main()
