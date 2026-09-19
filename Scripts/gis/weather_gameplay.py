#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/weather_gameplay_policy.json"
NUMERIC=("traction","visibility","ai_sight","crowd","traffic_speed","street_wetness","cloud_coverage","precipitation","fog_multiplier","wind","lightning")

def state(cfg,weather):
 if weather not in cfg["weathers"]:raise ValueError("Unknown weather")
 return {"weather":weather,**cfg["weathers"][weather],"transition_seconds":cfg["transition_seconds"]}

def transition(cfg,current,target,alpha):
 a=state(cfg,current);b=state(cfg,target);t=max(0.0,min(1.0,float(alpha)));out={"weather":current,"target_weather":target,"transition_alpha":round(t,4),"transition_seconds":cfg["transition_seconds"]}
 for key in NUMERIC:out[key]=round(float(a.get(key,0))+(float(b.get(key,0))-float(a.get(key,0)))*t,5)
 out["crowd_multiplier"]=out["crowd"];out["traffic_speed_multiplier"]=out["traffic_speed"]
 return out

def save_state(current,target=None,alpha=1.0):
 return {"weather":current,"target_weather":target or current,"transition_alpha":round(float(alpha),4)}

def main():
 p=argparse.ArgumentParser();p.add_argument("weather");p.add_argument("--target");p.add_argument("--alpha",type=float,default=1);p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();cfg=json.loads(a.config.read_text());print(json.dumps(transition(cfg,a.weather,a.target,a.alpha) if a.target else state(cfg,a.weather)))
if __name__=="__main__":main()
