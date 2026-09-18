#!/usr/bin/env python3
"""Generate deterministic facade-layout descriptors for GIS buildings."""
from __future__ import annotations
import argparse,hashlib,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CONFIG=ROOT/"Data"/"facade_profiles.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"

def deterministic_unit(key):
    return int(hashlib.sha256(key.encode()).hexdigest()[:8],16)/0xffffffff

def floors(feature,profile):
    tags=feature.get("tags",{})
    try:
        if tags.get("building:levels"):return max(1,min(30,int(float(tags["building:levels"]))))
    except ValueError:pass
    return max(1,min(30,round(float(feature.get("height_m",12))/profile["floor_height_m"])))

def edges(ring):
    result=[]
    for index,(a,b) in enumerate(zip(ring,ring[1:])):
        dx=b[0]-a[0];dy=b[1]-a[1];length=math.hypot(dx,dy)/100
        if length<1.5:continue
        result.append((index,a,b,length,math.degrees(math.atan2(dy,dx))))
    return result

def generate(config,city):
    records=[]
    for feature in city.get("features",[]):
        if feature.get("kind")!="building" or not feature.get("rings"):continue
        profile_name=feature.get("architecture_profile","mixed")
        profile=config["profiles"].get(profile_name,config["profiles"]["mixed"])
        level_count=floors(feature,profile)
        facade_edges=[]
        for index,a,b,length,yaw in edges(feature["rings"][0]):
            bays=max(1,round(length/profile["bay_width_m"]))
            bay_width=length/bays
            elements=[]
            for floor in range(level_count):
                for bay in range(bays):
                    key=f"{feature['id']}:{index}:{floor}:{bay}"
                    balcony=floor>0 and deterministic_unit(key+":balcony")<profile["balcony_probability"]
                    elements.append({
                        "floor":floor,"bay":bay,"kind":"window",
                        "offset_m":round((bay+0.5)*bay_width,3),
                        "width_m":profile["window_width_m"],"height_m":profile["window_height_m"],
                        "balcony":balcony
                    })
            facade_edges.append({
                "edge_index":index,"start":a[:3],"end":b[:3],"length_m":round(length,3),
                "yaw_deg":round(yaw,2),"bays":bays,"elements":elements
            })
        if not facade_edges:continue
        longest=max(facade_edges,key=lambda e:e["length_m"])
        door_candidate={
            "edge_index":longest["edge_index"],
            "offset_m":round(longest["length_m"]/2,3),
            "width_m":profile["door_width_m"],
            "source":"procedural_candidate"
        }
        material=profile["materials"][int(deterministic_unit(feature["id"]+":material")*len(profile["materials"]))%len(profile["materials"])]
        records.append({
            "building_id":feature["id"],"sector":feature.get("sector"),"architecture_profile":profile_name,
            "floors":level_count,"material":material,"material_source":"profile_fallback",
            "door_candidate":door_candidate,"facades":facade_edges
        })
    return {"schema_version":1,"count":len(records),"buildings":records}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--config",type=Path,default=DEFAULT_CONFIG);p.add_argument("--city",type=Path,default=DEFAULT_CITY);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    result=generate(json.loads(a.config.read_text()),json.loads(a.city.read_text()))
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8")
    print("Facade layouts:",result["count"])
if __name__=="__main__":main()
