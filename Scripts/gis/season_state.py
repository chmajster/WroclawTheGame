#!/usr/bin/env python3
import argparse,datetime,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_seasons.json"
def season(cfg,date):
 m=date.month
 if m in {12,1,2}:sid="winter"
 elif m in {3,4,5}:sid="spring"
 elif m in {6,7,8}:sid="summer"
 else:sid="autumn"
 return next(x for x in cfg["seasons"] if x["id"]==sid)
def state(cfg,date):
 s=season(cfg,date);return {"date":date.isoformat(),**s}
def main():
 p=argparse.ArgumentParser();p.add_argument("--date",default=datetime.date.today().isoformat());p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();print(json.dumps(state(json.loads(a.config.read_text()),datetime.date.fromisoformat(a.date))))
if __name__=="__main__":main()
