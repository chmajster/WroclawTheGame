#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/weather_gameplay_policy.json"
def state(cfg,weather):
 if weather not in cfg["weathers"]:raise ValueError("Unknown weather")
 return {"weather":weather,**cfg["weathers"][weather],"transition_seconds":cfg["transition_seconds"]}
def main():
 p=argparse.ArgumentParser();p.add_argument("weather");p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();print(json.dumps(state(json.loads(a.config.read_text()),a.weather)))
if __name__=="__main__":main()
