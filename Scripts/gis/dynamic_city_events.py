#!/usr/bin/env python3
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CAT=ROOT/"Data/dynamic_city_events.json"
def in_window(hour,start,end):
 return start<=hour<end if start<=end else hour>=start or hour<end
def eligible(event,hour,weather,heat):
 return heat>=event.get("min_heat",0) and in_window(hour%24,event["start_hour"],event["end_hour"]) and weather in event.get("weathers",[weather])
def choose(cat,hour,weather,heat,seed):
 pool=[e for e in cat["events"] if eligible(e,hour,weather,heat)]
 if not pool:return None
 ranked=sorted(pool,key=lambda e:hashlib.sha256(f"{seed}:{e['id']}".encode()).hexdigest())
 return ranked[0]
def main():
 p=argparse.ArgumentParser();p.add_argument("--hour",type=float,required=True);p.add_argument("--weather",required=True);p.add_argument("--heat",type=float,default=0);p.add_argument("--seed",default="default");p.add_argument("--catalog",type=Path,default=CAT);a=p.parse_args();print(json.dumps(choose(json.loads(a.catalog.read_text()),a.hour,a.weather,a.heat,a.seed)))
if __name__=="__main__":main()
