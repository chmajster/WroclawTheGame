#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/day_night_policy.json"
NUMERIC_FIELDS=("street_light","traffic","crowd","shops","sun_lux","moon_lux","sky_light","fog_density","exposure","window_emissive")

def phase_index(cfg,hour):
 hour=hour%24
 for i,p in enumerate(cfg["phases"]):
  if p["start"]<=hour<p["end"]:return i
 return 0

def phase(cfg,hour):return cfg["phases"][phase_index(cfg,hour)]

def state(cfg,hour):
 hour=hour%24;i=phase_index(cfg,hour);p=cfg["phases"][i];n=cfg["phases"][(i+1)%len(cfg["phases"])]
 transition=max(0.0,float(cfg.get("transition_minutes",0))/60.0)
 remaining=float(p["end"])-hour
 alpha=max(0.0,min(1.0,1.0-remaining/transition)) if transition>0 and remaining<=transition else 0.0
 out={"hour":hour,"phase":p["id"],"blend_to":n["id"],"transition_alpha":round(alpha,4)}
 for key in NUMERIC_FIELDS:
  a=float(p.get(key,0));b=float(n.get(key,a));out[key]=round(a+(b-a)*alpha,5)
 out["traffic_multiplier"]=out.pop("traffic");out["crowd_multiplier"]=out.pop("crowd");out["shop_activity"]=out.pop("shops")
 out["night_alpha"]=round(max(out["street_light"],out["window_emissive"]),5)
 return out

def main():
 p=argparse.ArgumentParser();p.add_argument("--config",type=Path,default=CFG);p.add_argument("--hour",type=float,required=True);a=p.parse_args();print(json.dumps(state(json.loads(a.config.read_text()),a.hour)))
if __name__=="__main__":main()
