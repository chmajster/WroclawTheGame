#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];WORLD=ROOT/"Data/openworld.json"
def active_slot(schedule,hour):
 if not schedule:return None
 entries=sorted(schedule,key=lambda x:x["hour"]);chosen=entries[-1]
 for e in entries:
  if hour>=e["hour"]:chosen=e
  else:break
 return chosen
def state(npc,hour,player_position=None,physicalize_radius=18000):
 slot=active_slot(npc.get("schedule",[]),hour)
 location=(slot or {}).get("location",npc.get("position"))
 physical=False
 if player_position and location:
  physical=sum((location[i]-player_position[i])**2 for i in range(3))<=physicalize_radius**2
 return {"npc_id":npc["id"],"hour":hour%24,"logical_location":location,"physicalize":physical,"schedule_hour":(slot or {}).get("hour")}
def simulate(world,hour,player_position=None,physicalize_radius=18000):
 return {"schema_version":1,"hour":hour%24,"npcs":[state(n,hour%24,player_position,physicalize_radius) for n in world.get("npc",[])]}
def main():
 p=argparse.ArgumentParser();p.add_argument("--world",type=Path,default=WORLD);p.add_argument("--hour",type=float,required=True);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=simulate(json.loads(a.world.read_text()),a.hour);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["npcs"]))
if __name__=="__main__":main()
