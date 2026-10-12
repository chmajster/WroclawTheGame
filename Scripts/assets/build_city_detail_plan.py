"""Derive repeatable architectural enrichment from existing GIS facade descriptors."""
import argparse
import hashlib
import json
import math
from collections import Counter
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def build(facades,district,policy):
    route=district["delivery_route"]+[district["anchors"]["meeting"]]
    items=[]; counts=Counter()
    def distance(position): return min(math.hypot(position[0]-p[0],position[1]-p[1]) for p in route)
    def add(building,source,role,position,yaw,scale,suffix=""):
        if any(not math.isfinite(v) for v in position+scale+[yaw]) or min(scale)<=0: raise ValueError("Invalid placement transform")
        items.append({"id":source["id"]+":"+role+suffix,"building_id":building["building_id"],
            "source_descriptor":source["id"],"role":role,"position_cm":[round(v,3) for v in position],
            "yaw_deg":round(yaw,3),"scale":[round(v,6) for v in scale]})
        counts[role]+=1
    candidates=[]
    for building in facades["buildings"]:
        for source in building["details"]+building["openings"]:
            if source["kind"] in {"balcony","cornice","door"} and distance(source["world_position"])<=policy["route_radius_cm"]:
                candidates.append((distance(source["world_position"]),source["id"],building,source))
    for _,_,building,source in sorted(candidates,key=lambda x:(x[0],x[1])):
        p=list(source["world_position"]); yaw=source["yaw_deg"]
        if source["kind"]=="balcony" and counts["railing"]<policy["limits"]["balconies"]:
            # The GIS point is slab center at its upper surface; render pivots are base-center.
            width=source["width_m"]*100; depth=source["depth_m"]*100
            # Legacy FBX converts Blender +Y to native -Y (handedness).
            add(building,source,"railing",p,yaw+90,[width/250,depth/110,source["railing_height_m"]*100/110.6847])
            slab_scale=[(width+12)/270,(depth+10)/120,1.]
            slab_p=p.copy(); slab_p[2]-=policy["slab_top_cm"]
            add(building,source,"slab",slab_p,yaw+90,slab_scale)
        elif source["kind"]=="door" and counts["entrance"]<policy["limits"]["entrances"]:
            height=source["height_m"]*100/policy["entrance_spring_cm"]
            width=source["width_m"]*100/policy["entrance_clear_width_cm"]
            base=p[2]-source["height_m"]*50
            # Do not bury the arch crown/entablature inside a balcony above it.
            if any(detail["kind"]=="balcony" and
                math.hypot(detail["world_position"][0]-p[0],detail["world_position"][1]-p[1])<(source["width_m"]+detail["width_m"])*50 and
                base+389*height>detail["world_position"][2]-policy["slab_top_cm"]-5
                for detail in building["details"]): continue
            # Rectangular door fits below the arch spring, with clear width.
            q=p.copy(); q[2]-=source["height_m"]*50
            radians=math.radians(yaw)
            q[0]+=math.cos(radians)*17; q[1]+=math.sin(radians)*17
            add(building,source,"entrance",q,yaw+90,[source["width_m"]*100/policy["entrance_clear_width_cm"],.6,
                source["height_m"]*100/policy["entrance_spring_cm"]])
            q=q.copy(); q[0]+=math.cos(radians)*8; q[1]+=math.sin(radians)*8
            add(building,source,"door_leaf",q,yaw+90,[width,1.,height])
        elif source["kind"]=="cornice":
            length=source["length_m"]*100-2*policy["corner_clearance_cm"]
            if length<100: continue
            segments=math.ceil(length/300)
            if counts["cornice"]+segments>policy["limits"]["cornice_segments"]: continue
            radians=math.radians(yaw)
            for index in range(segments):
                offset=(index+.5)*length/segments-length/2
                q=[p[0]+math.cos(radians)*offset,p[1]+math.sin(radians)*offset,p[2]-22]
                add(building,source,"cornice",q,yaw+180,[length/segments/300,source["depth_m"]*100/49,1.],":"+str(index))
    if not items: raise ValueError("No valid source placements within route radius")
    return {"schema_version":1,"map":policy["map"],"fidelity":policy["fidelity"],"items":items,
            "counts":dict(counts),"building_count":len({item["building_id"] for item in items})}


def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--input",default="Saved/PlayableDistrict")
    parser.add_argument("--output",default="Saved/CityDetails/placement.json")
    args=parser.parse_args(); source=ROOT/args.input
    policy_path=ROOT/"Data/city_detail_policy.json"
    paths=[source/"facades.json",source/"district.json",policy_path]
    facades,district,policy=[json.loads(path.read_text(encoding="utf-8")) for path in paths]
    report=build(facades,district,policy)
    report["source_files"]={str(path.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    output=ROOT/args.output; output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2),encoding="utf-8")
    print("PASS GIS plan "+json.dumps({"counts":report["counts"],"buildings":report["building_count"]}))


if __name__=="__main__": main()
