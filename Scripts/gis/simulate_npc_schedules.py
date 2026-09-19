#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];WORLD=ROOT/"Data/openworld.json";CFG=ROOT/"Data/npc_schedule_policy.json"

def normalized_entries(schedule):
 return sorted(schedule,key=lambda x:float(x["hour"])%24)

def active_and_next(schedule,hour):
 entries=normalized_entries(schedule)
 if not entries:return None,None
 h=hour%24;active=entries[-1];active_hour=float(active["hour"])-24
 nxt=entries[0];next_hour=float(nxt["hour"])+24
 for i,e in enumerate(entries):
  eh=float(e["hour"])
  if eh<=h:
   active=e;active_hour=eh
   if i+1<len(entries):nxt=entries[i+1];next_hour=float(nxt["hour"])
   else:nxt=entries[0];next_hour=float(nxt["hour"])+24
  elif active_hour<0:
   nxt=e;next_hour=eh;break
 return (active,active_hour),(nxt,next_hour)

def context_schedule(npc,context=None):
 context=context or {};schedule=npc.get("schedule",[])
 for o in npc.get("overrides",[]):
  cond=o.get("when",{})
  if cond.get("weather") and context.get("weather") not in cond["weather"]:continue
  if cond.get("weekdays") and context.get("weekday") not in cond["weekdays"]:continue
  if cond.get("quest") and context.get("quest")!=cond["quest"]:continue
  schedule=o.get("schedule",schedule)
 return schedule

def lerp(a,b,t):return [round(a[i]+(b[i]-a[i])*t,2) for i in range(min(len(a),len(b)))]

def representation(location,player_position,policy):
 if not player_position or not location:return "logical"
 d=math.dist(location[:3],player_position[:3])/100.0
 if d<=policy["physicalize_radius_m"]:return "physical"
 if d<=policy["simplified_radius_m"]:return "simplified"
 return "logical"

def state(npc,hour,player_position=None,physicalize_radius=None,policy=None,context=None):
 policy=dict(policy or {})
 if physicalize_radius is not None:policy["physicalize_radius_m"]=physicalize_radius
 policy.setdefault("physicalize_radius_m",180);policy.setdefault("simplified_radius_m",650);policy.setdefault("travel_speed_mps",1.35);policy.setdefault("minimum_travel_minutes",2)
 schedule=context_schedule(npc,context);pair,next_pair=active_and_next(schedule,hour)
 if not pair:
  location=npc.get("position");return {"npc_id":npc["id"],"hour":hour%24,"logical_location":location,"physicalize":representation(location,player_position,policy)=="physical","representation":representation(location,player_position,policy),"schedule_hour":None,"traveling":False,"travel_progress":0}
 active,active_hour=pair;nxt,next_hour=next_pair;start=active.get("location",npc.get("position"));target=nxt.get("location",start)
 distance_m=math.dist(start[:3],target[:3])/100.0 if start and target else 0
 travel_hours=max(policy["minimum_travel_minutes"]/60.0,(distance_m/max(0.1,policy["travel_speed_mps"]))/3600.0)
 depart=max(active_hour,next_hour-travel_hours);h=hour%24
 if active_hour<0 and h<float(normalized_entries(schedule)[0]["hour"]):h+=24
 traveling=bool(start and target and h>=depart and h<next_hour and distance_m>0)
 progress=max(0.0,min(1.0,(h-depart)/max(1e-6,next_hour-depart))) if traveling else 0.0
 location=lerp(start,target,progress) if traveling else start
 rep=representation(location,player_position,policy)
 return {"npc_id":npc["id"],"hour":hour%24,"logical_location":location,"physicalize":rep=="physical","representation":rep,"schedule_hour":active.get("hour"),"next_schedule_hour":nxt.get("hour"),"traveling":traveling,"travel_progress":round(progress,4),"from_slot":active.get("id"),"to_slot":nxt.get("id")}

def simulate(world,hour,player_position=None,physicalize_radius=None,policy=None,context=None):
 return {"schema_version":2,"hour":hour%24,"npcs":[state(n,hour,player_position,physicalize_radius,policy,context) for n in world.get("npc",[])]}

def main():
 p=argparse.ArgumentParser();p.add_argument("--world",type=Path,default=WORLD);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--hour",type=float,required=True);p.add_argument("--weather");p.add_argument("--weekday");p.add_argument("--quest");p.add_argument("--output",type=Path,required=True);a=p.parse_args();context={"weather":a.weather,"weekday":a.weekday,"quest":a.quest};r=simulate(json.loads(a.world.read_text()),a.hour,policy=json.loads(a.config.read_text()),context=context);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["npcs"]))
if __name__=="__main__":main()
