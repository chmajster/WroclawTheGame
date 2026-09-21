#!/usr/bin/env python3
"""Generate deterministic roof/detail descriptors from real building footprints."""
from __future__ import annotations

import argparse, hashlib, json, math
from pathlib import Path
from shapely.geometry import Point, Polygon

ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CONFIG=ROOT/"Data"/"roof_detail_profiles.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"
DEFAULT_BINDINGS=ROOT/"Data"/"roof_asset_bindings.json"


def number(value, default=None):
    try:
        if value is None: return default
        return float(str(value).replace(" m","").replace(",","."))
    except ValueError:
        return default


def deterministic_choice(identifier, values):
    digest=int(hashlib.sha256(identifier.encode()).hexdigest()[:8],16)
    return values[digest%len(values)]


def deterministic_unit(identifier):
    return int(hashlib.sha256(identifier.encode()).hexdigest()[:8],16)/0xffffffff


def interior_point(polygon,identifier,index,total):
    minx,miny,maxx,maxy=polygon.bounds
    for attempt in range(12):
        ux=.15+.7*deterministic_unit(f"{identifier}:x:{index}:{attempt}")
        uy=.15+.7*deterministic_unit(f"{identifier}:y:{index}:{attempt}")
        point=Point(minx+(maxx-minx)*ux,miny+(maxy-miny)*uy)
        if polygon.covers(point):
            return [point.x,point.y]
    point=polygon.representative_point()
    return [point.x,point.y]


def dominant_edge(ring):
    best=None
    for a,b in zip(ring,ring[1:]):
        dx=b[0]-a[0];dy=b[1]-a[1];length=math.hypot(dx,dy)
        if best is None or length>best[0]: best=(length,a,b)
    if not best:return 0.0,0.0
    length,a,b=best
    return length,math.degrees(math.atan2(b[1]-a[1],b[0]-a[0]))


def generate(config,city,bindings):
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
        minx,miny,maxx,maxy=polygon.bounds
        ground_z=max(p[2] for p in ring)
        eave_z=ground_z+total_height*100
        roof_asset=bindings["shapes"].get(shape)
        if not roof_asset:
            raise ValueError(f"No roof asset binding for {shape}")
        chimney_asset=bindings["details"]["chimney"]
        dormer_asset=bindings["details"]["dormer"]
        chimneys=[]
        for index in range(chimney_count):
            x,y=interior_point(polygon,feature["id"]+":chimney",index,chimney_count)
            chimneys.append({
                "id":f"{feature['id']}:chimney:{index}",
                "asset_id":chimney_asset,
                "position":[round(x,2),round(y,2),round(eave_z+roof_height*55,2)],
                "yaw_deg":round(yaw,2),
                "scale":[1.0,1.0,round(max(.7,min(1.8,roof_height/2.0)),3)]
            })
        dormers=[]
        if dormer_count:
            edge=max(zip(ring,ring[1:]),key=lambda pair:math.hypot(pair[1][0]-pair[0][0],pair[1][1]-pair[0][1]))
            a,b=edge
            for index in range(dormer_count):
                t=(index+1)/(dormer_count+1)
                ex=a[0]+(b[0]-a[0])*t;ey=a[1]+(b[1]-a[1])*t
                x=ex*.72+centroid.x*.28;y=ey*.72+centroid.y*.28
                point=Point(x,y)
                if not polygon.covers(point):
                    x,y=interior_point(polygon,feature["id"]+":dormer",index,dormer_count)
                dormers.append({
                    "id":f"{feature['id']}:dormer:{index}",
                    "asset_id":dormer_asset,
                    "position":[round(x,2),round(y,2),round(eave_z+roof_height*30,2)],
                    "yaw_deg":round(yaw,2),
                    "scale":[1.0,1.0,1.0]
                })
        details.append({
            "building_id":feature["id"],
            "sector":feature.get("sector"),
            "architecture_profile":profile_name,
            "roof":{
                "shape":shape,
                "shape_source":"osm" if source_shape==shape else "procedural_fallback",
                "asset_id":roof_asset,
                "height_m":round(roof_height,2),
                "ridge_yaw_deg":round(yaw,2),
                "material_hint":tags.get("roof:material"),
                "colour_hint":tags.get("roof:colour"),
                "position":[round((minx+maxx)/2,2),round((miny+maxy)/2,2),round(eave_z,2)],
                "width_m":round((maxx-minx)/100,3),
                "depth_m":round((maxy-miny)/100,3)
            },
            "details":{
                "chimneys":chimney_count,
                "dormers":dormer_count,
                "anchor":[round(centroid.x,2),round(centroid.y,2),round(eave_z,2)],
                "chimney_instances":chimneys,
                "dormer_instances":dormers
            }
        })
    return {"schema_version":2,"count":len(details),"buildings":details}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--config",type=Path,default=DEFAULT_CONFIG)
    p.add_argument("--city",type=Path,default=DEFAULT_CITY)
    p.add_argument("--bindings",type=Path,default=DEFAULT_BINDINGS)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    result=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()),json.loads(a.bindings.read_text()))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8")
    print("Roof descriptors:",result["count"])
if __name__=="__main__":main()
