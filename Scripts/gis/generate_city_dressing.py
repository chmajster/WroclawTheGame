#!/usr/bin/env python3
import argparse,hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CFG=ROOT/"Data"/"city_dressing_rules.json";CITY=ROOT/"Saved"/"CityData"/"sector.json"
def unit(key): return int(hashlib.sha256(key.encode()).hexdigest()[:8],16)/0xffffffff
def road_points(points,spacing_cm,offset_cm,key):
 out=[]
 carry=0.0
 for a,b in zip(points,points[1:]):
  dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
  if length<1:continue
  nx,ny=-dy/length,dx/length
  d=max(spacing_cm-carry,0)
  while d<length:
   side=-1 if unit(f"{key}:{d}")<.5 else 1
   out.append([round(a[0]+dx*d/length+nx*offset_cm*side,2),round(a[1]+dy*d/length+ny*offset_cm*side,2),round(a[2],2)])
   d+=spacing_cm
  carry=max(0,d-length)
 return out
def generate(cfg,city):
 out=[];seen=set()
 for f in city.get("features",[]):
  tags=f.get("tags",{});kind=f.get("kind")
  for role,rule in cfg["rules"].items():
   pts=[]
   if kind=="road" and tags.get("highway") in rule.get("road_kinds",[]):
    pts=road_points(f.get("points",[]),rule["spacing_m"]*100,rule.get("offset_m",0)*100,f["id"]+role)
   elif kind in rule.get("feature_kinds",[]) and f.get("rings"):
    ring=f["rings"][0];step=max(1,int(rule["spacing_m"]*100))
    for i,p in enumerate(ring[:-1]):
     if i%max(1,len(ring)//4)==0:pts.append(p)
   for i,p in enumerate(pts):
    cell=(math.floor(p[0]/(cfg["cell_size_m"]*100)),math.floor(p[1]/(cfg["cell_size_m"]*100)))
    ident=f"{role}:{f['id']}:{i}"
    if ident in seen:continue
    seen.add(ident);out.append({"id":ident,"role":rule["asset_role"],"source_feature":f["id"],"position":p,"cell":cell,"procedural":True})
 return {"schema_version":1,"count":len(out),"placements":out}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
