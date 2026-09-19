#!/usr/bin/env python3
import argparse,datetime,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/city_seasons.json"

def season_id(date):
 m=date.month
 if m in {12,1,2}:return "winter"
 if m in {3,4,5}:return "spring"
 if m in {6,7,8}:return "summer"
 return "autumn"

def season(cfg,date):
 sid=season_id(date);return next(x for x in cfg["seasons"] if x["id"]==sid)

def next_season_id(sid):
 order=["winter","spring","summer","autumn"];return order[(order.index(sid)+1)%len(order)]

def next_boundary(date):
 y=date.year
 if date.month in {12,1,2}:return datetime.date(y if date.month<3 else y+1,3,1)
 if date.month in {3,4,5}:return datetime.date(y,6,1)
 if date.month in {6,7,8}:return datetime.date(y,9,1)
 return datetime.date(y,12,1)

def blend_value(a,b,t):return float(a)+(float(b)-float(a))*t

def state(cfg,date):
 current=season(cfg,date);next_id=next_season_id(current["id"]);nxt=next(x for x in cfg["seasons"] if x["id"]==next_id)
 transition_days=max(0,int(cfg.get("transition_days",14)));days=(next_boundary(date)-date).days
 alpha=max(0.0,min(1.0,1.0-days/max(1,transition_days))) if transition_days and days<=transition_days else 0.0
 numeric=("foliage","daylight_hours","crowd","wetness_bias","snow_accumulation_rate")
 out={"date":date.isoformat(),"season":current["id"],"blend_to":next_id,"transition_alpha":round(alpha,4)}
 for key in numeric:out[key]=round(blend_value(current.get(key,0),nxt.get(key,current.get(key,0)),alpha),4)
 out["crowd_multiplier"]=out.pop("crowd")
 out["snow_allowed"]=bool(current.get("snow_allowed",False) or (alpha>0.5 and nxt.get("snow_allowed",False)))
 out["foliage_variant"]=current.get("foliage_variant",current["id"])
 out["material_variant"]=current.get("material_variant",current["id"])
 out["npc_clothing_profile"]=current.get("npc_clothing_profile",current["id"])
 out["decoration_layer"]=current.get("decoration_layer")
 out["pcg_tags"]=list(current.get("pcg_tags",[current["id"]]))
 daylight=out["daylight_hours"];out["sunrise_hour"]=round(12-daylight/2,3);out["sunset_hour"]=round(12+daylight/2,3)
 out["save_state"]={"date":out["date"],"season":out["season"],"transition_alpha":out["transition_alpha"]}
 return out

def main():
 p=argparse.ArgumentParser();p.add_argument("--date",default=datetime.date.today().isoformat());p.add_argument("--config",type=Path,default=CFG);a=p.parse_args();print(json.dumps(state(json.loads(a.config.read_text()),datetime.date.fromisoformat(a.date))))
if __name__=="__main__":main()
