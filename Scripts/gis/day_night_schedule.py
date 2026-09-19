#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/day_night_policy.json"
def phase(cfg,hour):
 hour=hour%24
 for p in cfg["phases"]:
  if p["start"]<=hour<p["end"]:return p
 return cfg["phases"][0]
def state(cfg,hour):
 p=phase(cfg,hour);return {"hour":hour%24,"phase":p["id"],"street_light":p["street_light"],"traffic_multiplier":p["traffic"],"crowd_multiplier":p["crowd"],"shop_activity":p["shops"]}
def main():
 p=argparse.ArgumentParser();p.add_argument("--config",type=Path,default=CFG);p.add_argument("--hour",type=float,required=True);a=p.parse_args();print(json.dumps(state(json.loads(a.config.read_text()),a.hour)))
if __name__=="__main__":main()
