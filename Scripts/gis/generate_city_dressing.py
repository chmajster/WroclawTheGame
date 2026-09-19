#!/usr/bin/env python3
import argparse,hashlib,json,math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
CFG=ROOT/"Data"/"city_dressing_rules.json"
CITY=ROOT/"Saved"/"CityData"/"sector.json"

def unit(key):
 return int(hashlib.sha256(key.encode()).hexdigest()[:8],16)/0xffffffff

def choose_variant(rule,key):
 variants=rule.get("variants") or [rule["asset_role"]]
 index=min(len(variants)-1,int(unit(key)*len(variants)))
 return variants[index]

def scale_for(rule,key):
 lo,hi=rule.get("scale_range",[1.0,1.0])
 return round(lo+(hi-lo)*unit(key),3)

def road_points(points,spacing_cm,offset_cm,key):
 out=[];carry=0.0
 for segment,(a,b) in enumerate(zip(points,points[1:])):
  dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
  if length<1:continue
  nx,ny=-dy/length,dx/length
  road_yaw=math.degrees(math.atan2(dy,dx))
  d=max(spacing_cm-carry,0)
  while d<length:
   side=-1 if unit(f"{key}:{segment}:{d}:side")<.5 else 1
   pos=[round(a[0]+dx*d/length+nx*offset_cm*side,2),round(a[1]+dy*d/length+ny*offset_cm*side,2),round(a[2],2)]
   out.append({"position":pos,"yaw_deg":round((road_yaw+90*side)%360,2),"side":side})
   d+=spacing_cm
  carry=max(0,d-length)
 return out

def ring_points(ring,rule,key):
 if len(ring)<2:return []
 vertices=ring[:-1] if ring[0]==ring[-1] else ring
 if not vertices:return []
 cx=sum(p[0] for p in vertices)/len(vertices);cy=sum(p[1] for p in vertices)/len(vertices)
 target=max(1,int(round(max(1,len(vertices))*min(1.0,20/max(float(rule.get("spacing_m",20)),1)))))
 stride=max(1,len(vertices)//target)
 out=[]
 for i,p in enumerate(vertices):
  if i%stride:continue
  yaw=math.degrees(math.atan2(cy-p[1],cx-p[0]))%360
  out.append({"position":[round(p[0],2),round(p[1],2),round(p[2],2)],"yaw_deg":round(yaw,2),"side":0})
 return out

def generate(cfg,city):
 out=[];seen=set();cell_size_cm=cfg["cell_size_m"]*100
 for f in city.get("features",[]):
  tags=f.get("tags",{});kind=f.get("kind")
  for role,rule in cfg["rules"].items():
   candidates=[]
   if kind=="road" and tags.get("highway") in rule.get("road_kinds",[]):
    candidates=road_points(f.get("points",[]),rule["spacing_m"]*100,rule.get("offset_m",0)*100,f["id"]+role)
   elif kind in rule.get("feature_kinds",[]) and f.get("rings"):
    candidates=ring_points(f["rings"][0],rule,f["id"]+role)
   probability=float(rule.get("probability",1.0))
   for i,candidate in enumerate(candidates):
    ident=f"{role}:{f['id']}:{i}"
    if ident in seen or unit(ident+":density")>probability:continue
    seen.add(ident);p=candidate["position"]
    cell=[math.floor(p[0]/cell_size_cm),math.floor(p[1]/cell_size_cm)]
    out.append({
     "id":ident,
     "role":rule["asset_role"],
     "variant":choose_variant(rule,ident+":variant"),
     "source_feature":f["id"],
     "position":p,
     "cell":cell,
     "rotation_yaw_deg":candidate["yaw_deg"],
     "road_side":candidate["side"],
     "uniform_scale":scale_for(rule,ident+":scale"),
     "collision_profile":rule.get("collision_profile","WorldStatic"),
     "nav_behavior":rule.get("nav_behavior","obstacle"),
     "data_layer":rule.get("data_layer","StreetFurniture"),
     "procedural":True
    })
 return {"schema_version":2,"cell_size_m":cfg["cell_size_m"],"count":len(out),"placements":out}

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
