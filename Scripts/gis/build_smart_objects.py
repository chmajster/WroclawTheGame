#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/smart_object_roles.json"

def role_for(tags):
 if tags.get("entrance"):return "building_entrance"
 if tags.get("shop"):return "shop"
 if tags.get("public_transport") or tags.get("highway")=="bus_stop" or tags.get("railway")=="tram_stop":return "transit_stop"
 if tags.get("amenity")=="parking":return "parking"
 if tags.get("amenity"):return "amenity"
 return None

def feature_position(f):
 if f.get("points"):return f["points"][0]
 if f.get("rings") and f["rings"][0]:
  ring=f["rings"][0]
  pts=ring[:-1] if len(ring)>1 and ring[0]==ring[-1] else ring
  if pts:
   return [round(sum(p[i] for p in pts)/len(pts),2) for i in range(3)]
 return None

def make_slots(base_id,pos,role_cfg):
 count=max(1,int(role_cfg.get("slot_count",1)));radius=float(role_cfg.get("slot_radius_m",0))*100
 slots=[]
 for i in range(count):
  angle=(360.0*i/count)+float(role_cfg.get("slot_yaw_offset_deg",0))
  rad=math.radians(angle)
  slot_pos=[round(pos[0]+math.cos(rad)*radius,2),round(pos[1]+math.sin(rad)*radius,2),round(pos[2],2)]
  face=(angle+180.0)%360.0 if radius>0 else float(role_cfg.get("default_yaw_deg",0))
  slots.append({"id":f"{base_id}/slot/{i}","index":i,"position":slot_pos,"yaw_deg":round(face,2),"enabled":True})
 return slots

def generate(cfg,city):
 out=[]
 for f in city.get("features",[]):
  tags=f.get("tags",{});role=role_for(tags)
  if not role:continue
  pos=feature_position(f)
  if not pos:continue
  rcfg=cfg["roles"][role];ident="smart:"+f["id"];slots=make_slots(ident,pos,rcfg)
  access=(tags.get("wheelchair") or tags.get("access") or "unknown")
  out.append({
   "id":ident,"role":role,"source_feature":f["id"],"position":pos,"tags":tags,
   "reservation":rcfg["reservation"],"capacity":len(slots),"slots":slots,
   "interaction_radius_m":rcfg.get("interaction_radius_m",1.5),
   "approach_distance_m":rcfg.get("approach_distance_m",1.0),
   "behavior":rcfg.get("behavior",role),
   "queue_spacing_m":rcfg.get("queue_spacing_m"),
   "accessibility":access,
   "data_layer":rcfg.get("data_layer","Gameplay")
  })
 return {"schema_version":2,"count":len(out),"slot_count":sum(len(x["slots"]) for x in out),"objects":out}

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"],r["slot_count"])
if __name__=="__main__":main()
