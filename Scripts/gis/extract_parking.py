#!/usr/bin/env python3
import argparse,gzip,hashlib,json,math,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import SOURCE,source_bytes,metadata,Geography

def parse_capacity(tags,default_value):
 try:return max(1,int(tags.get("capacity",default_value)))
 except (TypeError,ValueError):return default_value

def orientation(points):
 if len(points)<2:return 0.0
 best=(0.0,0.0)
 for a,b in zip(points,points[1:]):
  dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
  if length>best[0]:best=(length,math.degrees(math.atan2(dy,dx))%360)
 return round(best[1],2)

def make_slots(pid,kind,points,position,tags,policy):
 configured=parse_capacity(tags,1 if kind=="point" else policy["default_area_capacity"])
 if kind=="garage":configured=parse_capacity(tags,policy["default_garage_capacity"])
 configured=min(configured,policy["max_generated_slots_per_parking"])
 if position:base=position
 else:
  xs=[p[0] for p in points];ys=[p[1] for p in points];zs=[p[2] for p in points];base=[sum(xs)/len(xs),sum(ys)/len(ys),sum(zs)/len(zs)]
 yaw=orientation(points) if points else 0.0;spacing=policy["slot_width_m"]*100;slots=[]
 cols=max(1,int(math.sqrt(configured)))
 for i in range(configured):
  row=i//cols;col=i%cols;rad=math.radians(yaw)
  lateral=(col-(cols-1)/2)*spacing;forward=row*policy["slot_length_m"]*100
  x=base[0]+math.cos(rad)*forward-math.sin(rad)*lateral;y=base[1]+math.sin(rad)*forward+math.cos(rad)*lateral
  slots.append({"id":f"{pid}/slot/{i}","index":i,"position":[round(x,2),round(y,2),round(base[2],2)],"yaw_deg":yaw,"vehicle_class":"car"})
 return slots

def occupied(slot_id,seed,ratio):
 value=int(hashlib.sha256(f"{seed}:{slot_id}".encode()).hexdigest()[:8],16)/0xffffffff
 return value<ratio

def legality(tags):
 access=tags.get("access","yes");parking=tags.get("parking") or tags.get("parking:lane")
 if access in {"no","private"}:return {"legal":False,"reason":"restricted_access"}
 if tags.get("parking:condition") in {"no_parking","no_stopping"}:return {"legal":False,"reason":"parking_restricted"}
 return {"legal":True,"reason":"designated" if parking or tags.get("amenity") in {"parking","parking_space"} else "unknown"}

def record(pid,kind,points,position,tags,policy,seed):
 slots=make_slots(pid,kind,points,position,tags,policy);ratio=float(policy["default_occupancy_ratio"])
 for s in slots:s["occupied_procedural"]=occupied(s["id"],seed,ratio)
 legal=legality(tags)
 return {"id":pid,"kind":kind,"position":position,"points":points,"tags":tags,"slots":slots,"capacity":len(slots),"access":tags.get("access","unknown"),"fee":tags.get("fee","unknown"),"maxstay":tags.get("maxstay"),"legal":legal["legal"],"legality_reason":legal["reason"],"economy":{"hourly_price_key":policy["economy"]["hourly_price_key"],"tow_release_price_key":policy["economy"]["tow_release_price_key"]},"garage":kind=="garage"}

def build(source=SOURCE/"osm/wave1.osm.gz",terrain=SOURCE/"terrain/wave1.csv",policy=None,seed="parking"):
 policy=policy or {"default_area_capacity":12,"default_garage_capacity":40,"max_generated_slots_per_parking":120,"slot_width_m":2.6,"slot_length_m":5.2,"default_occupancy_ratio":0.55,"economy":{"hourly_price_key":"parking_hour","tow_release_price_key":"tow_release"}}
 info=metadata(source);geo=Geography(info["BoundingBox"],terrain);root=ET.fromstring(gzip.decompress(source_bytes(source)));nodes={n.attrib["id"]:(float(n.attrib["lon"]),float(n.attrib["lat"])) for n in root.findall("node")};out=[]
 for n in root.findall("node"):
  tags={t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")}
  if tags.get("amenity") not in {"parking","parking_space","motorcycle_parking"}:continue
  lon,lat=nodes[n.attrib["id"]];out.append(record("node/"+n.attrib["id"],"point",[],geo.world(lon,lat),tags,policy,seed))
 for w in root.findall("way"):
  tags={t.attrib["k"]:t.attrib["v"] for t in w.findall("tag")}
  if tags.get("amenity")!="parking" and tags.get("building") not in {"garage","garages","parking"}:continue
  pts=[geo.world(*nodes[nd.attrib["ref"]]) for nd in w.findall("nd") if nd.attrib["ref"] in nodes]
  if len(pts)>=2:out.append(record("way/"+w.attrib["id"],"garage" if tags.get("building") else "area",pts,None,tags,policy,seed))
 return {"schema_version":2,"count":len(out),"slot_count":sum(x["capacity"] for x in out),"parking":out}

def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--terrain",type=Path,default=SOURCE/"terrain/wave1.csv");p.add_argument("--policy",type=Path,default=Path(__file__).resolve().parents[2]/"Data/parking_policy.json");p.add_argument("--seed",default="parking");p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=build(a.source,a.terrain,json.loads(a.policy.read_text()),a.seed);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(r["count"],r["slot_count"])
if __name__=="__main__":main()
