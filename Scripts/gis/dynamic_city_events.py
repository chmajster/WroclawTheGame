#!/usr/bin/env python3
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CAT=ROOT/"Data/dynamic_city_events.json"

def in_window(hour,start,end):
 return start<=hour<end if start<=end else hour>=start or hour<end

def eligible(event,hour,weather,heat,last_triggered_minutes=None,now_minutes=None):
 if heat<event.get("min_heat",0) or not in_window(hour%24,event["start_hour"],event["end_hour"]) or weather not in event.get("weathers",[weather]):return False
 if last_triggered_minutes is not None and now_minutes is not None and now_minutes-last_triggered_minutes<float(event.get("cooldown_minutes",0)):return False
 return True

def unit(key):return int(hashlib.sha256(key.encode()).hexdigest()[:12],16)/float(0xffffffffffff)

def choose(cat,hour,weather,heat,seed,state=None,now_minutes=None):
 state=state or {};pool=[]
 for e in cat["events"]:
  if eligible(e,hour,weather,heat,state.get("last_triggered",{}).get(e["id"]),now_minutes):pool.append(e)
 if not pool:return None
 total=sum(max(0,float(e.get("weight",1))) for e in pool)
 if total<=0:return None
 roll=unit(f"{seed}:{hour%24:.3f}:{weather}:{heat}")*total
 cursor=0.0
 for e in sorted(pool,key=lambda x:x["id"]):
  cursor+=max(0,float(e.get("weight",1)))
  if roll<=cursor:return e
 return sorted(pool,key=lambda x:x["id"])[-1]

def choose_location(event,city,seed):
 if not city:return {"kind":"unbound","id":None}
 source=event.get("location_source","road_edge")
 if source=="road_edge":
  edges=[e for e in city.get("road_graph",{}).get("edges",[]) if e.get("car_forward") or e.get("car_backward")]
  if not edges:return {"kind":"unbound","id":None}
  index=min(len(edges)-1,int(unit(f"{seed}:{event['id']}:edge")*len(edges)));e=sorted(edges,key=lambda x:x["id"])[index]
  return {"kind":"road_edge","id":e["id"],"way":e.get("way")}
 sectors=city.get("sectors",[])
 if sectors:
  index=min(len(sectors)-1,int(unit(f"{seed}:{event['id']}:sector")*len(sectors)));s=sorted(sectors,key=lambda x:x["id"])[index];return {"kind":"sector","id":s["id"]}
 return {"kind":"unbound","id":None}

def mutations_for(event,location):
 out=[]
 for effect in event.get("effects",[]):
  if effect in {"close_edge","roadblock"} and location.get("kind")=="road_edge":out.append({"type":"blocked_edge","edge_id":location["id"],"reason":event["id"]})
  elif effect=="slow_traffic":out.append({"type":"traffic_speed_multiplier","target":location,"value":0.45})
  elif effect=="reroute":out.append({"type":"request_reroute","target":location})
  elif effect=="service_dispatch":out.append({"type":"service_dispatch","services":event.get("services",["police","ambulance"])})
  elif effect=="heat_response":out.append({"type":"heat_response","value":event.get("heat_delta",5)})
  elif effect=="disable_lights":out.append({"type":"power_lighting_enabled","target":location,"value":False})
  elif effect=="disable_camera":out.append({"type":"security_camera_enabled","target":location,"value":False})
 return out

def instantiate(event,city,seed,now_minutes):
 location=choose_location(event,city,seed);instance_id=f"event:{event['id']}:{int(now_minutes)}:{hashlib.sha1(seed.encode()).hexdigest()[:8]}"
 return {"instance_id":instance_id,"event_id":event["id"],"kind":event["kind"],"started_minutes":float(now_minutes),"expires_minutes":float(now_minutes)+float(event["duration_minutes"]),"location":location,"effects":event["effects"],"mutations":mutations_for(event,location),"actor_role":event.get("actor_role",event["kind"]),"physicalize_radius_m":event.get("physicalize_radius_m",300),"cleanup_on_unload":True}

def trigger(cat,hour,weather,heat,seed,city=None,state=None,now_minutes=0):
 state=json.loads(json.dumps(state or {"active":[],"last_triggered":{}}));event=choose(cat,hour,weather,heat,seed,state,now_minutes)
 if not event:return state,None
 inst=instantiate(event,city,seed,now_minutes);state.setdefault("active",[]).append(inst);state.setdefault("last_triggered",{})[event["id"]]=float(now_minutes);return state,inst

def advance(state,now_minutes,unloaded_location_ids=None):
 unloaded=set(unloaded_location_ids or []);active=[];removed=[]
 for inst in state.get("active",[]):
  expired=float(now_minutes)>=float(inst["expires_minutes"]);unloaded_match=inst.get("cleanup_on_unload") and inst.get("location",{}).get("id") in unloaded
  (removed if expired or unloaded_match else active).append(inst)
 return {**state,"active":active},{"removed_instance_ids":[x["instance_id"] for x in removed],"reverted_mutations":[m for x in removed for m in x.get("mutations",[])]}

def main():
 p=argparse.ArgumentParser();p.add_argument("--hour",type=float,required=True);p.add_argument("--weather",required=True);p.add_argument("--heat",type=float,default=0);p.add_argument("--seed",default="default");p.add_argument("--now-minutes",type=float,default=0);p.add_argument("--catalog",type=Path,default=CAT);p.add_argument("--city",type=Path);a=p.parse_args();city=json.loads(a.city.read_text()) if a.city else None;state,inst=trigger(json.loads(a.catalog.read_text()),a.hour,a.weather,a.heat,a.seed,city,now_minutes=a.now_minutes);print(json.dumps({"state":state,"instance":inst}))
if __name__=="__main__":main()
