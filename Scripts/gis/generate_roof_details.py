#!/usr/bin/env python3
"""Generate deterministic roof/detail descriptors from real building footprints."""
from __future__ import annotations

import argparse, hashlib, json, math
from pathlib import Path
from shapely.geometry import Polygon

ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CONFIG=ROOT/"Data"/"roof_detail_profiles.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"


def number(value, default=None):
    try:
        if value is None: return default
        return float(str(value).replace(" m","").replace(",","."))
    except ValueError:
        return default


def deterministic_choice(identifier, values):
    digest=int(hashlib.sha256(identifier.encode()).hexdigest()[:8],16)
    return values[digest%len(values)]


def dominant_edge(ring):
    best=None
    for a,b in zip(ring,ring[1:]):
        dx=b[0]-a[0];dy=b[1]-a[1];length=math.hypot(dx,dy)
        if best is None or length>best[0]: best=(length,a,b)
    if not best:return 0.0,0.0
    length,a,b=best
    return length,math.degrees(math.atan2(b[1]-a[1],b[0]-a[0]))


def generate(config,city):
    details=[]
    for feature in city.get("features",[]):
        if feature.get("kind")!="building" or not feature.get("rings"):continue
        ring=feature["rings"][0]
        if len(ring)<4:continue
        polygon=Polygon([(p[0],p[1]) for p in ring])
        if polygon.is_empty or polygon.area<=1:continue
        tags=feature.get("tags",{})
        profile_name=feature.get("architecture_profile","mixed")
        profile=config["profiles"].get(profile_name,config["profiles"]["mixed"])
        source_shape=tags.get("roof:shape")
        shape=source_shape if source_shape in {"flat","gabled","hipped","pyramidal","mansard","dome","onion"} else deterministic_choice(feature["id"],profile["fallback_shapes"])
        total_height=float(feature.get("height_m",12))
        roof_height=number(tags.get("roof:height"))
        if roof_height is None:
            levels=number(tags.get("roof:levels"))
            roof_height=levels*3.0 if levels else max(0.6,total_height*profile["roof_height_ratio"])
        edge_length_cm,yaw=dominant_edge(ring)
        area_m2=polygon.area/10000.0
        chimney_count=0 if shape=="flat" and profile_name=="estate" else min(config["max_chimneys"],max(1,round(area_m2/profile["chimney_area_m2"])))
        dormer_count=0
        if shape in {"gabled","hipped","mansard"} and profile["dormer_spacing_m"]>0:
            dormer_count=min(config["max_dormers_per_edge"],int((edge_length_cm/100)/profile["dormer_spacing_m"]))
        centroid=polygon.centroid
        details.append({
            "building_id":feature["id"],
            "sector":feature.get("sector"),
            "architecture_profile":profile_name,
            "roof":{
                "shape":shape,
                "shape_source":"osm" if source_shape==shape else "procedural_fallback",
                "height_m":round(roof_height,2),
                "ridge_yaw_deg":round(yaw,2),
                "material_hint":tags.get("roof:material"),
                "colour_hint":tags.get("roof:colour")
            },
            "details":{
                "chimneys":chimney_count,
                "dormers":dormer_count,
                "anchor":[round(centroid.x,2),round(centroid.y,2),max(p[2] for p in ring)]
            }
        })
    return {"schema_version":1,"count":len(details),"buildings":details}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--config",type=Path,default=DEFAULT_CONFIG)
    p.add_argument("--city",type=Path,default=DEFAULT_CITY)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    result=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8")
    print("Roof descriptors:",result["count"])
if __name__=="__main__":main()
