#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_audio_profiles.json";CITY=ROOT/"Saved/CityData/city.json"
def generate(cfg,city):
 out=[]
 for s in city.get("sectors",[]):
  c=s.get("counts",{});layers=[]
  if c.get("roads",0)>0:layers.append({"id":"traffic","gain":cfg["layers"]["traffic"]["base_gain"]})
  if c.get("rail_lines",0)>0:layers.append({"id":"tram","gain":cfg["layers"]["tram"]["base_gain"]})
  if s.get("density") in {"High","VeryHigh"}:layers.append({"id":"crowd","gain":cfg["layers"]["crowd"]["base_gain"]})
  out.append({"sector":s["id"],"district":s.get("district"),"layers":layers})
 return {"schema_version":1,"sectors":out,"weather_gain":cfg["weather_gain"]}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["sectors"]))
if __name__=="__main__":main()
