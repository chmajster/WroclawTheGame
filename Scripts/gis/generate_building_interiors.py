#!/usr/bin/env python3
import argparse,hashlib,json,math
from pathlib import Path
from shapely.geometry import Polygon,box
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json";CFG=ROOT/"Data/interior_generation_profiles.json"

def levels(f,profile,max_floors):
 tags=f.get("tags",{})
 try:
  if tags.get("building:levels"):return max(1,min(max_floors,int(float(tags["building:levels"]))))
 except ValueError:pass
 return max(1,min(max_floors,round(float(f.get("height_m",12))/3.1)))

def classification(tags,building_id,cfg):
 if building_id in set(cfg.get("quest_building_ids",[])):return "quest"
 if tags.get("shop") or tags.get("tourism") or tags.get("office") or tags.get("amenity") in set(cfg.get("public_amenities",[])):return "public"
 if tags.get("building") in {"apartments","house","residential","detached","terrace"}:return "private"
 return "mixed"

def largest_polygon(geom):
 if geom.is_empty:return None
 if geom.geom_type=="Polygon":return geom
 polys=[g for g in getattr(geom,"geoms",[]) if g.geom_type=="Polygon"]
 return max(polys,key=lambda p:p.area) if polys else None

def floor_units(poly,count,inset_cm):
 usable=largest_polygon(poly.buffer(-inset_cm)) or poly
 minx,miny,maxx,maxy=usable.bounds;units=[]
 for i in range(count):
  x0=minx+(maxx-minx)*i/count;x1=minx+(maxx-minx)*(i+1)/count
  part=largest_polygon(usable.intersection(box(x0,miny,x1,maxy)))
  if not part or part.area<=1:continue
  rp=part.representative_point();coords=[[round(x,2),round(y,2)] for x,y in part.exterior.coords]
  units.append({"unit":i,"center":[round(rp.x,2),round(rp.y,2)],"polygon_xy":coords,"area_m2":round(part.area/10000,2)})
 return units

def generate(cfg,city,entrances=None):
 e_by_building={}
 for e in (entrances or {}).get("entrances",[]):e_by_building.setdefault(e["building_id"],[]).append(e)
 out=[]
 for f in city.get("features",[]):
  if f.get("kind")!="building" or not f.get("rings"):continue
  poly=Polygon([(p[0],p[1]) for p in f["rings"][0]])
  if poly.is_empty or poly.area<800000:continue
  profile_name=f.get("architecture_profile","mixed");profile=cfg["profiles"].get(profile_name,cfg["profiles"]["mixed"]);floor_count=levels(f,profile,cfg["max_generated_floors"])
  interior_id="interior/"+f["id"];floor_height_cm=int(cfg.get("floor_height_m",3.1)*100);rooms=[];connectors=[];nav_edges=[]
  base_units=floor_units(poly,profile["units_per_floor"],float(cfg.get("wall_inset_m",0.25))*100)
  core=poly.representative_point()
  for floor in range(floor_count):
   z=floor*floor_height_cm
   connector_id=f"{interior_id}/core/f{floor}";connectors.append({"id":connector_id,"floor":floor,"position":[round(core.x,2),round(core.y,2),z],"stairs":True,"elevator":bool(profile.get("elevator",floor_count>=4))})
   if floor>0:nav_edges.append({"from":f"{interior_id}/core/f{floor-1}","to":connector_id,"kind":"vertical"})
   for u in base_units:
    rid=f"{interior_id}/f{floor}/u{u['unit']}";center=[u["center"][0],u["center"][1],z]
    rooms.append({"id":rid,"floor":floor,"unit":u["unit"],"center":center,"polygon_xy":u["polygon_xy"],"area_m2":u["area_m2"],"kind":"unit"})
    nav_edges.append({"from":connector_id,"to":rid,"kind":"door"})
  cls=classification(f.get("tags",{}),f["id"],cfg);revision=hashlib.sha256((f["id"]+":"+str(floor_count)+":"+profile_name).encode()).hexdigest()[:12]
  out.append({"id":interior_id,"building_id":f["id"],"classification":cls,"profile":profile_name,"floors":floor_count,"rooms":rooms,"vertical_connectors":connectors,"nav_edges":nav_edges,"entrance_ids":[e["id"] for e in e_by_building.get(f["id"],[])],"layout_revision":revision,"generated":True,"factual_layout":False})
 return {"schema_version":2,"count":len(out),"interiors":out}

def main():
 p=argparse.ArgumentParser();p.add_argument("--city",type=Path,default=CITY);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--entrances",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args();ents=json.loads(a.entrances.read_text()) if a.entrances else None;r=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()),ents);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
