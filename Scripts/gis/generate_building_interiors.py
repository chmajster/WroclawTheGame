#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
from shapely.geometry import Polygon
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/interior_generation_profiles.json"
def levels(f,profile,max_floors):
 tags=f.get("tags",{})
 try:
  if tags.get("building:levels"):return max(1,min(max_floors,int(float(tags["building:levels"]))))
 except ValueError:pass
 return max(1,min(max_floors,round(float(f.get("height_m",12))/3.1)))
def generate(cfg,city,entrances=None):
 e_by_building={}
 for e in (entrances or {}).get("entrances",[]):e_by_building.setdefault(e["building_id"],[]).append(e)
 out=[]
 for f in city.get("features",[]):
  if f.get("kind")!="building" or not f.get("rings"):continue
  poly=Polygon([(p[0],p[1]) for p in f["rings"][0]])
  if poly.is_empty or poly.area<800000:continue
  profile_name=f.get("architecture_profile","mixed");profile=cfg["profiles"].get(profile_name,cfg["profiles"]["mixed"]);floor_count=levels(f,profile,cfg["max_generated_floors"])
  minx,miny,maxx,maxy=poly.bounds;centroid=poly.centroid;interior_id="interior/"+f["id"]
  rooms=[]
  for floor in range(floor_count):
   for unit in range(profile["units_per_floor"]):
    angle=2*math.pi*unit/profile["units_per_floor"];r=min(maxx-minx,maxy-miny)*.25
    rooms.append({"id":f"{interior_id}/f{floor}/u{unit}","floor":floor,"unit":unit,"center":[round(centroid.x+math.cos(angle)*r,2),round(centroid.y+math.sin(angle)*r,2),floor*310],"kind":"unit"})
  out.append({"id":interior_id,"building_id":f["id"],"profile":profile_name,"floors":floor_count,"rooms":rooms,"entrance_ids":[e["id"] for e in e_by_building.get(f["id"],[])],"generated":True})
 return {"schema_version":1,"count":len(out),"interiors":out}
def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--entrances",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args();ents=json.loads(a.entrances.read_text()) if a.entrances else None;r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()),ents);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
