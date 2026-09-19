#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/smart_object_roles.json"
def role_for(tags):
 if tags.get("entrance"):return "building_entrance"
 if tags.get("shop"):return "shop"
 if tags.get("public_transport") or tags.get("highway")=="bus_stop" or tags.get("railway")=="tram_stop":return "transit_stop"
 if tags.get("amenity")=="parking":return "parking"
 if tags.get("amenity"):return "amenity"
 return None
def generate(cfg,city):
 out=[]
 for f in city.get("features",[]):
  tags=f.get("tags",{});role=role_for(tags)
  if not role:continue
  if f.get("points"):pos=f["points"][0]
  elif f.get("rings") and f["rings"][0]:pos=f["rings"][0][0]
  else:continue
  out.append({"id":"smart:"+f["id"],"role":role,"source_feature":f["id"],"position":pos,"tags":tags,"reservation":cfg["roles"][role]["reservation"]})
 return {"schema_version":1,"count":len(out),"objects":out}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
