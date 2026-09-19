#!/usr/bin/env python3
"""Generate deterministic facade/opening/detail descriptors for GIS buildings."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CONFIG=ROOT/"Data"/"facade_profiles.json"
DEFAULT_BINDINGS=ROOT/"Data"/"facade_asset_bindings.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"


def deterministic_unit(key):
    return int(hashlib.sha256(key.encode()).hexdigest()[:8],16)/0xffffffff


def deterministic_choice(key,values):
    if not values:return None
    return values[min(len(values)-1,int(deterministic_unit(key)*len(values)))]


def floors(feature,profile):
    tags=feature.get("tags",{})
    try:
        if tags.get("building:levels"):
            return max(1,min(30,int(float(tags["building:levels"]))))
    except ValueError:
        pass
    return max(1,min(30,round(float(feature.get("height_m",12))/profile["floor_height_m"])))


def signed_area(ring):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(ring,ring[1:]))/2.0


def edges(ring):
    ccw=signed_area(ring)>0
    result=[]
    for index,(a,b) in enumerate(zip(ring,ring[1:])):
        dx=b[0]-a[0];dy=b[1]-a[1];length_cm=math.hypot(dx,dy)
        if length_cm<150:continue
        tangent_yaw=math.degrees(math.atan2(dy,dx))%360
        normal=(dy,-dx) if ccw else (-dy,dx)
        outward_yaw=math.degrees(math.atan2(normal[1],normal[0]))%360
        result.append({
            "edge_index":index,"a":a,"b":b,
            "length_m":length_cm/100.0,
            "tangent_yaw_deg":tangent_yaw,
            "outward_yaw_deg":outward_yaw
        })
    return result


def project_point_to_edge(point,edge):
    a=edge["a"];b=edge["b"];dx=b[0]-a[0];dy=b[1]-a[1]
    length_sq=dx*dx+dy*dy
    if length_sq<=0:return None
    t=max(0.0,min(1.0,((point[0]-a[0])*dx+(point[1]-a[1])*dy)/length_sq))
    x=a[0]+dx*t;y=a[1]+dy*t
    return {
        "distance_cm":math.hypot(point[0]-x,point[1]-y),
        "offset_m":t*edge["length_m"],
        "t":t,
        "point":[round(x,2),round(y,2),round(a[2]+(b[2]-a[2])*t,2)]
    }


def point_on_edge(edge,offset_m,z_offset_m=0.0,outward_offset_m=0.0):
    t=max(0.0,min(1.0,offset_m/max(edge["length_m"],1e-6)))
    a=edge["a"];b=edge["b"]
    x=a[0]+(b[0]-a[0])*t;y=a[1]+(b[1]-a[1])*t;z=a[2]+(b[2]-a[2])*t+z_offset_m*100
    rad=math.radians(edge["outward_yaw_deg"])
    x+=math.cos(rad)*outward_offset_m*100;y+=math.sin(rad)*outward_offset_m*100
    return [round(x,2),round(y,2),round(z,2)]


def floor_base_m(floor,profile):
    if floor<=0:return 0.0
    return float(profile["ground_floor_height_m"])+(floor-1)*float(profile["floor_height_m"])


def is_commercial(tags):
    return bool(
        tags.get("shop") or tags.get("office") or
        tags.get("building") in {"commercial","retail","supermarket","kiosk"} or
        tags.get("amenity") in {"restaurant","cafe","fast_food","bank","pharmacy","clinic"}
    )


def is_garage(tags):
    return tags.get("building") in {"garage","garages","parking"} or tags.get("amenity")=="parking"


def entrance_index(entrances):
    result={}
    for entrance in (entrances or {}).get("entrances",[]):
        result.setdefault(entrance.get("building_id"),[]).append(entrance)
    return result


def nearest_edge_for_entrance(entrance,edge_records,max_snap_m):
    point=entrance.get("position")
    if not point:return None
    best=None
    for edge in edge_records:
        projection=project_point_to_edge(point,edge)
        if projection and (best is None or projection["distance_cm"]<best[0]):
            best=(projection["distance_cm"],edge,projection)
    if not best or best[0]>max_snap_m*100:return None
    return best[1],best[2]


def opening_model(bindings,group,key):
    return deterministic_choice(key,bindings.get("groups",{}).get(group,[]))


def procedural_asset(bindings,key):
    return bindings.get("procedural",{}).get(key,f"procedural:{key}")


def door_records(feature,profile,edge_records,building_entrances,bindings,config):
    doors=[]
    candidates=building_entrances.get(feature["id"],[])
    for entrance in candidates:
        match=nearest_edge_for_entrance(entrance,edge_records,config["max_entrance_snap_m"])
        if not match:continue
        edge,projection=match
        door_id=f"{feature['id']}:door:{entrance['id']}"
        group=profile["door_asset_group"]
        doorway_group=profile["doorway_asset_group"]
        doors.append({
            "id":door_id,"kind":"door","entrance_id":entrance["id"],
            "source":entrance.get("source","entrance_pipeline"),
            "edge_index":edge["edge_index"],"offset_m":round(projection["offset_m"],3),
            "bay":None,"floor":0,
            "world_position":point_on_edge(edge,projection["offset_m"],profile["door_height_m"]*.5,-profile["door_recess_m"]),
            "yaw_deg":round(edge["outward_yaw_deg"],2),
            "width_m":profile["door_width_m"],"height_m":profile["door_height_m"],
            "asset_id":opening_model(bindings,group,door_id+":asset"),
            "frame_asset_id":opening_model(bindings,doorway_group,door_id+":frame"),
            "interactive":True,
            "smart_object_id":"smart:"+entrance["id"],
            "address":{"street":entrance.get("street"),"housenumber":entrance.get("housenumber")}
        })
    if doors or not edge_records:return doors
    edge=max(edge_records,key=lambda e:e["length_m"])
    offset=edge["length_m"]/2
    door_id=f"{feature['id']}:door:procedural"
    doors.append({
        "id":door_id,"kind":"door","entrance_id":None,"source":"procedural_candidate",
        "edge_index":edge["edge_index"],"offset_m":round(offset,3),"bay":None,"floor":0,
        "world_position":point_on_edge(edge,offset,profile["door_height_m"]*.5,-profile["door_recess_m"]),
        "yaw_deg":round(edge["outward_yaw_deg"],2),
        "width_m":profile["door_width_m"],"height_m":profile["door_height_m"],
        "asset_id":opening_model(bindings,profile["door_asset_group"],door_id+":asset"),
        "frame_asset_id":opening_model(bindings,profile["doorway_asset_group"],door_id+":frame"),
        "interactive":False,"smart_object_id":None,
        "address":{"street":feature.get("tags",{}).get("addr:street"),"housenumber":feature.get("tags",{}).get("addr:housenumber")}
    })
    return doors


def generate(config,city,entrances=None,bindings=None):
    bindings=bindings or {"groups":{},"procedural":{}}
    entrances_by_building=entrance_index(entrances)
    records=[];totals={}
    for feature in city.get("features",[]):
        if feature.get("kind")!="building" or not feature.get("rings"):continue
        ring=feature["rings"][0]
        edge_records=edges(ring)
        if not edge_records:continue
        profile_name=feature.get("architecture_profile","mixed")
        profile=config["profiles"].get(profile_name,config["profiles"]["mixed"])
        tags=feature.get("tags",{})
        level_count=floors(feature,profile)
        doors=door_records(feature,profile,edge_records,entrances_by_building,bindings,config)
        door_bays={}
        for door in doors:
            edge=next(e for e in edge_records if e["edge_index"]==door["edge_index"])
            bays=max(1,round(edge["length_m"]/profile["bay_width_m"]))
            bay_width=edge["length_m"]/bays
            door["bay"]=max(0,min(bays-1,int(door["offset_m"]/max(bay_width,1e-6))))
            door_bays.setdefault(edge["edge_index"],set()).add(door["bay"])

        facade_edges=[];details=[];openings=list(doors)
        commercial=is_commercial(tags);garage=is_garage(tags)
        building_height_m=float(feature.get("height_m",profile["ground_floor_height_m"]+max(0,level_count-1)*profile["floor_height_m"]))
        for edge in edge_records:
            index=edge["edge_index"];length=edge["length_m"]
            bays=max(1,round(length/profile["bay_width_m"]));bay_width=length/bays
            edge_openings=[];edge_details=[]
            for floor in range(level_count):
                base_m=floor_base_m(floor,profile)
                for bay in range(bays):
                    if floor==0 and bay in door_bays.get(index,set()):
                        continue
                    key=f"{feature['id']}:{index}:{floor}:{bay}"
                    offset=(bay+.5)*bay_width
                    kind="window"
                    asset_group=profile["window_asset_group"]
                    width=profile["window_width_m"];height=profile["window_height_m"]
                    if floor==0 and garage:
                        kind="garage_shutter";asset_group="storefront";width=min(bay_width*.9,profile.get("storefront_width_m",2.8));height=profile.get("storefront_height_m",2.5)
                    elif floor==0 and commercial:
                        kind="storefront";asset_group="storefront" if deterministic_unit(key+":shutter")<profile["storefront_shutter_probability"] else profile["storefront_window_asset_group"]
                        width=min(bay_width*.9,profile.get("storefront_width_m",2.8));height=profile.get("storefront_height_m",2.5)
                    sill=profile["ground_window_sill_m"] if floor==0 else profile["window_sill_m"]
                    if kind in {"storefront","garage_shutter"}:sill=0.05
                    center_z=base_m+sill+height*.5
                    opening={
                        "id":f"{feature['id']}:{kind}:{index}:{floor}:{bay}",
                        "floor":floor,"bay":bay,"kind":kind,"edge_index":index,
                        "offset_m":round(offset,3),"width_m":round(width,3),"height_m":round(height,3),
                        "world_position":point_on_edge(edge,offset,center_z,-profile["window_recess_m"]),
                        "yaw_deg":round(edge["outward_yaw_deg"],2),
                        "asset_id":opening_model(bindings,asset_group,key+":opening"),
                        "sill_asset_id":None if kind in {"storefront","garage_shutter"} else procedural_asset(bindings,"sill"),
                        "lintel_asset_id":procedural_asset(bindings,"lintel"),
                        "night_emissive_parameter":config["night_emissive_parameter"] if kind!="garage_shutter" else None,
                        "instance_group":f"{profile_name}:{kind}:{asset_group}"
                    }
                    balcony=floor>0 and kind=="window" and deterministic_unit(key+":balcony")<profile["balcony_probability"]
                    opening["balcony"]=balcony
                    edge_openings.append(opening);openings.append(opening)
                    if balcony:
                        balcony_detail={
                            "id":f"{feature['id']}:balcony:{index}:{floor}:{bay}","kind":"balcony","edge_index":index,
                            "floor":floor,"bay":bay,"world_position":point_on_edge(edge,offset,base_m+profile["window_sill_m"]-.12,profile["balcony_depth_m"]*.5),
                            "yaw_deg":round(edge["outward_yaw_deg"],2),"width_m":round(min(bay_width*.9,width+profile["balcony_width_extra_m"]),3),
                            "depth_m":profile["balcony_depth_m"],"railing_height_m":profile["balcony_railing_height_m"],
                            "asset_id":procedural_asset(bindings,"balcony"),"instance_group":f"{profile_name}:balcony"
                        }
                        edge_details.append(balcony_detail);details.append(balcony_detail)
                    if floor>0 and deterministic_unit(key+":aircon")<profile["aircon_probability"]:
                        aircon={
                            "id":f"{feature['id']}:aircon:{index}:{floor}:{bay}","kind":"aircon","edge_index":index,"floor":floor,"bay":bay,
                            "world_position":point_on_edge(edge,min(length-.35,offset+width*.55),base_m+sill+.45,.18),
                            "yaw_deg":round(edge["outward_yaw_deg"],2),"asset_id":opening_model(bindings,"facade_aircon",key+":aircon"),
                            "instance_group":"facade:aircon"
                        }
                        edge_details.append(aircon);details.append(aircon)
                    if floor==0 and kind=="storefront":
                        if deterministic_unit(key+":awning")<profile["awning_probability"]:
                            awning={
                                "id":f"{feature['id']}:awning:{index}:{bay}","kind":"awning","edge_index":index,"floor":0,"bay":bay,
                                "world_position":point_on_edge(edge,offset,height+.25,.45),"yaw_deg":round(edge["outward_yaw_deg"],2),
                                "width_m":round(width*1.05,3),"depth_m":profile["awning_depth_m"],"asset_id":procedural_asset(bindings,"awning")
                            }
                            edge_details.append(awning);details.append(awning)
                        sign={
                            "id":f"{feature['id']}:sign:{index}:{bay}","kind":"storefront_sign","edge_index":index,"floor":0,"bay":bay,
                            "world_position":point_on_edge(edge,offset,height+.55,.08),"yaw_deg":round(edge["outward_yaw_deg"],2),
                            "width_m":round(width*.85,3),"asset_id":procedural_asset(bindings,"sign")
                        }
                        edge_details.append(sign);details.append(sign)

            gutter={
                "id":f"{feature['id']}:gutter:{index}","kind":"gutter","edge_index":index,
                "world_position":point_on_edge(edge,length*.5,building_height_m,.08),
                "yaw_deg":round(edge["tangent_yaw_deg"],2),"length_m":round(length,3),
                "diameter_m":profile["gutter_diameter_m"],"asset_id":procedural_asset(bindings,"gutter")
            }
            edge_details.append(gutter);details.append(gutter)
            downspout_count=max(1,int(length/profile["downspout_spacing_m"]))
            for pipe_index in range(downspout_count):
                offset=(pipe_index+1)*length/(downspout_count+1)
                pipe={
                    "id":f"{feature['id']}:downspout:{index}:{pipe_index}","kind":"downspout","edge_index":index,
                    "world_position":point_on_edge(edge,offset,building_height_m*.5,.08),
                    "yaw_deg":round(edge["outward_yaw_deg"],2),"height_m":round(building_height_m,3),
                    "diameter_m":profile["downspout_diameter_m"],"asset_id":procedural_asset(bindings,"downspout")
                }
                edge_details.append(pipe);details.append(pipe)

            if profile.get("cornice",False):
                cornice={
                    "id":f"{feature['id']}:cornice:{index}","kind":"cornice","edge_index":index,
                    "world_position":point_on_edge(edge,length*.5,building_height_m-.18,.1),
                    "yaw_deg":round(edge["tangent_yaw_deg"],2),"length_m":round(length,3),
                    "depth_m":profile["cornice_depth_m"],"asset_id":procedural_asset(bindings,"cornice")
                }
                edge_details.append(cornice);details.append(cornice)

            facade_edges.append({
                "edge_index":index,"start":edge["a"][:3],"end":edge["b"][:3],"length_m":round(length,3),
                "yaw_deg":round(edge["tangent_yaw_deg"],2),"outward_yaw_deg":round(edge["outward_yaw_deg"],2),
                "bays":bays,"openings":edge_openings,"details":edge_details
            })

        for door in doors:
            edge=next(e for e in edge_records if e["edge_index"]==door["edge_index"])
            key=door["id"]
            step={
                "id":key+":step","kind":"door_step","edge_index":door["edge_index"],"floor":0,"bay":door["bay"],
                "world_position":point_on_edge(edge,door["offset_m"],.08,.18),"yaw_deg":door["yaw_deg"],
                "width_m":round(door["width_m"]*1.15,3),"depth_m":profile["door_step_depth_m"],"asset_id":procedural_asset(bindings,"door_step")
            }
            details.append(step)
            if door.get("address",{}).get("housenumber"):
                plaque={
                    "id":key+":address","kind":"address_plaque","edge_index":door["edge_index"],"floor":0,"bay":door["bay"],
                    "world_position":point_on_edge(edge,min(edge["length_m"]-.2,door["offset_m"]+door["width_m"]*.7),1.65,.05),
                    "yaw_deg":door["yaw_deg"],"text":str(door["address"]["housenumber"]),"asset_id":procedural_asset(bindings,"address_plaque")
                }
                details.append(plaque)
            if deterministic_unit(key+":intercom")<profile["intercom_probability"]:
                intercom={
                    "id":key+":intercom","kind":"intercom","edge_index":door["edge_index"],"floor":0,"bay":door["bay"],
                    "world_position":point_on_edge(edge,min(edge["length_m"]-.2,door["offset_m"]+door["width_m"]*.65),1.35,.08),
                    "yaw_deg":door["yaw_deg"],"asset_id":opening_model(bindings,"intercom",key+":intercom_asset")
                }
                details.append(intercom)

        material=deterministic_choice(feature["id"]+":material",profile["materials"])
        counts={}
        for item in openings+details:counts[item["kind"]]=counts.get(item["kind"],0)+1
        for kind,count in counts.items():totals[kind]=totals.get(kind,0)+count
        records.append({
            "building_id":feature["id"],"sector":feature.get("sector"),"architecture_profile":profile_name,
            "floors":level_count,"material":material,"material_source":"profile_fallback",
            "data_layer":"DL_Buildings","factual_facade":False,
            "doors":doors,"openings":openings,"details":details,"facades":facade_edges,"counts":counts
        })
    return {"schema_version":2,"count":len(records),"element_totals":totals,"buildings":records}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--config",type=Path,default=DEFAULT_CONFIG)
    p.add_argument("--bindings",type=Path,default=DEFAULT_BINDINGS)
    p.add_argument("--city",type=Path,default=DEFAULT_CITY)
    p.add_argument("--entrances",type=Path)
    p.add_argument("--output",type=Path,required=True)
    a=p.parse_args()
    entrance_data=json.loads(a.entrances.read_text(encoding="utf-8")) if a.entrances else None
    result=generate(
        json.loads(a.config.read_text(encoding="utf-8")),
        json.loads(a.city.read_text(encoding="utf-8")),
        entrance_data,
        json.loads(a.bindings.read_text(encoding="utf-8"))
    )
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,ensure_ascii=False,separators=(",",":"))+"\n",encoding="utf-8")
    print("Facade layouts:",result["count"],"elements:",sum(result["element_totals"].values()))


if __name__=="__main__":
    main()
