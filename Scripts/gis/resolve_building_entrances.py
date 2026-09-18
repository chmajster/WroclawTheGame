#!/usr/bin/env python3
"""Link real OSM entrance/address nodes to GIS buildings and emit gameplay entrances."""
from __future__ import annotations
import argparse,json,math
from pathlib import Path
from shapely.geometry import Point,Polygon
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"

def building_records(city):
    result=[]
    for f in city.get("features",[]):
        if f.get("kind")!="building" or not f.get("rings"):continue
        poly=Polygon([(p[0],p[1]) for p in f["rings"][0]])
        if poly.is_empty:continue
        result.append((f,poly))
    return result

def nearest_building(point,buildings,max_cm=500):
    best=None
    for feature,poly in buildings:
        d=poly.distance(point)
        if d<=max_cm and (best is None or d<best[0]):best=(d,feature,poly)
    return best

def nearest_edge_point(poly,point):
    boundary=poly.exterior
    distance=boundary.project(point)
    p=boundary.interpolate(distance)
    return [round(p.x,3),round(p.y,3)]

def generate(city):
    buildings=building_records(city);entrances=[];used=set()
    for f in city.get("features",[]):
        if f.get("kind")!="entrance" or not f.get("points"):continue
        p=Point(f["points"][0][0],f["points"][0][1])
        match=nearest_building(p,buildings)
        if not match:continue
        distance,building,poly=match
        tags=f.get("tags",{})
        entry={
          "id":f["id"],"building_id":building["id"],"position":f["points"][0],
          "source":"osm_node","entrance":tags.get("entrance"),
          "street":tags.get("addr:street") or building.get("tags",{}).get("addr:street"),
          "housenumber":tags.get("addr:housenumber") or building.get("tags",{}).get("addr:housenumber"),
          "distance_to_building_cm":round(distance,2)
        }
        entrances.append(entry);used.add(building["id"])
    # Addressed buildings without entrance nodes get an explicit fallback candidate.
    for building,poly in buildings:
        if building["id"] in used:continue
        tags=building.get("tags",{})
        if not (tags.get("addr:housenumber") or tags.get("addr:street")):continue
        centroid=poly.centroid
        pos=nearest_edge_point(poly,centroid)
        base=max(p[2] for p in building["rings"][0])
        entrances.append({
          "id":"derived/"+building["id"],"building_id":building["id"],
          "position":[pos[0],pos[1],base],"source":"procedural_edge_candidate",
          "entrance":None,"street":tags.get("addr:street"),"housenumber":tags.get("addr:housenumber"),
          "distance_to_building_cm":0
        })
    return {"schema_version":1,"count":len(entrances),"entrances":entrances}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--city",type=Path,default=DEFAULT_CITY);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    result=generate(json.loads(a.city.read_text(encoding="utf-8")));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8");print("Entrances:",result["count"])
if __name__=="__main__":main()
