#!/usr/bin/env python3
import argparse,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];POLICY=ROOT/"Data/world_validation_policy.json";SECTOR=ROOT/"Saved/CityData/sector.json";CITY=ROOT/"Saved/CityData/city.json"

def load(path):return json.loads(path.read_text()) if path and path.is_file() else None

def finite_position(pos):
 try:return bool(pos) and all(math.isfinite(float(v)) for v in pos[:3])
 except (TypeError,ValueError):return False

def validate(policy,sector,city,entrances=None,quality=None,quests=None,streaming=None,landmarks=None,
             parking=None,smart_objects=None,crowd=None,traffic=None,hlod=None,data_layers=None,
             terrain_checks=None,service_probes=None,benchmark_report=None,migration_registry=None):
 issues=[]
 def add(severity,code,message):issues.append({"severity":severity,"code":code,"message":str(message)})
 def upstream(name,data):
  if data and data.get("status")=="FAIL":add("error","upstream_failed",name)

 feature_ids=set();building_ids=set()
 for f in sector.get("features",[]):
  fid=f.get("id")
  if fid in feature_ids:add("error","duplicate_feature_id",fid)
  feature_ids.add(fid)
  if f.get("kind")=="building":building_ids.add(fid)
  coords=(f.get("rings",[[]])[0] if f.get("rings") else f.get("points",[]))
  for pos in coords:
   if not finite_position(pos):add("error","nonfinite_coordinate",fid);break
   if any(abs(float(v))>policy["max_abs_coordinate_cm"] for v in pos[:3]):add("error","coordinate_outlier",fid);break

 graph=sector.get("road_graph",{});nodes={n["id"] for n in graph.get("nodes",[])};edge_ids=set()
 for e in graph.get("edges",[]):
  if e["id"] in edge_ids:add("error","duplicate_edge_id",e["id"])
  edge_ids.add(e["id"])
  if e.get("from") not in nodes or e.get("to") not in nodes:add("error","missing_edge_node",e["id"])
  try:length=float(e["length_cm"])
  except (TypeError,ValueError,KeyError):length=-1
  if not policy["min_road_edge_cm"]<=length<=policy["max_road_edge_cm"]:add("error","edge_length",e.get("id","?"))

 for e in (entrances or {}).get("entrances",[]):
  if e.get("building_id") not in building_ids:add("error","orphan_entrance",e.get("id","?"))

 seen_quality=set()
 for q in (quality or {}).get("decisions",[]):
  bid=q.get("building_id")
  if bid in seen_quality:add("error","duplicate_quality",bid)
  seen_quality.add(bid)
  if bid and bid not in building_ids:add("error","quality_unknown_building",bid)

 districts={s["district"] for s in city.get("sectors",[]) if s.get("district")}
 city_sectors={s["id"] for s in city.get("sectors",[]) if s.get("id")}
 quest_items=(quests or {}).get("compiled_quests") or (quests or {}).get("quests",[])
 for q in quest_items:
  for stage in q.get("stages",[]):
   if stage.get("district") not in districts:add("error","unknown_quest_district",q.get("id","?"))
  for leg in q.get("route_legs",[]):
   if leg.get("district_path") is None:add("error","unroutable_quest_leg",f"{q.get('id','?')}:{leg.get('from_stage')}->{leg.get('to_stage')}")

 for s in (streaming or {}).get("sectors",[]):
  if s.get("sector") not in city_sectors:add("error","unknown_streaming_sector",s.get("sector"))

 if policy.get("unresolved_hero_is_error"):
  for l in (landmarks or {}).get("records",[]):
   if l.get("quality")=="hero" and l.get("status")=="unresolved":add("error","unresolved_hero_landmark",l.get("id","?"))

 parking_ids=set();parking_slot_ids=set()
 for p in (parking or {}).get("parking",[]):
  pid=p.get("id")
  if pid in parking_ids:add("error","duplicate_parking_id",pid)
  parking_ids.add(pid)
  slots=p.get("slots",[])
  if int(p.get("capacity",len(slots)))!=len(slots):add("error","parking_capacity_mismatch",pid)
  if len(slots)>policy["max_parking_slots_per_location"]:add("error","parking_capacity_limit",pid)
  for slot in slots:
   sid=slot.get("id")
   if sid in parking_slot_ids:add("error","duplicate_parking_slot",sid)
   parking_slot_ids.add(sid)
   if not finite_position(slot.get("position")):add("error","parking_slot_position",sid)

 smart_ids=set();smart_slot_ids=set()
 for obj in (smart_objects or {}).get("objects",[]):
  oid=obj.get("id")
  if oid in smart_ids:add("error","duplicate_smart_object",oid)
  smart_ids.add(oid)
  source=obj.get("source_feature")
  if source and source not in feature_ids:add("error","smart_object_unknown_source",oid)
  slots=obj.get("slots",[])
  if int(obj.get("capacity",len(slots)))!=len(slots):add("error","smart_object_capacity_mismatch",oid)
  if len(slots)>policy["max_smart_object_slots"]:add("error","smart_object_slot_limit",oid)
  for slot in slots:
   sid=slot.get("id")
   if sid in smart_slot_ids:add("error","duplicate_smart_object_slot",sid)
   smart_slot_ids.add(sid)
   if not finite_position(slot.get("position")):add("error","smart_object_slot_position",sid)

 for corridor in (crowd or {}).get("corridors",[]):
  if corridor.get("edge") not in edge_ids:add("error","crowd_unknown_edge",corridor.get("id"))
  if float(corridor.get("capacity",0))<=0:add("error","crowd_invalid_capacity",corridor.get("id"))

 lane_ids=set()
 for lane in (traffic or {}).get("lanes",[]):
  lid=lane.get("id")
  if lid in lane_ids:add("error","duplicate_traffic_lane",lid)
  lane_ids.add(lid)
  if lane.get("edge") not in edge_ids:add("error","traffic_unknown_edge",lid)
  if float(lane.get("speed_limit_kph",0))<policy["min_traffic_speed_kph"]:add("error","traffic_invalid_speed",lid)
  if int(lane.get("capacity",0))<=0:add("error","traffic_invalid_capacity",lid)

 data_layer_ids={x.get("id") for x in (data_layers or {}).get("layers",[]) if x.get("id")}
 upstream("data_layers",data_layers)
 for a in (data_layers or {}).get("assignments",[]):
  if a.get("layer") not in data_layer_ids:add("error","unknown_data_layer_assignment",a.get("id"))
 for name,layers in (data_layers or {}).get("activation_sets",{}).items():
  for layer in layers:
   if layer not in data_layer_ids:add("error","unknown_activation_layer",f"{name}:{layer}")

 for h in (hlod or {}).get("assignments",[]):
  bid=h.get("building_id")
  if bid not in building_ids:add("error","hlod_unknown_building",bid)
  if data_layer_ids and h.get("data_layer") not in data_layer_ids:add("error","hlod_unknown_data_layer",bid)
  if float(h.get("memory_budget_mb",0))>policy["max_hlod_memory_mb_per_building"]:add("error","hlod_memory_budget",bid)
  if h.get("quality")=="hero" and not h.get("silhouette_protected",False):add("error","hero_hlod_not_protected",bid)

 for check in terrain_checks or []:
  if not math.isfinite(float(check.get("object_z_cm",float("nan")))) or not math.isfinite(float(check.get("terrain_z_cm",float("nan")))):
   add("error","terrain_check_nonfinite",check.get("id","?"));continue
  offset=abs(float(check["object_z_cm"])-float(check["terrain_z_cm"]))
  if offset>policy["max_floating_offset_cm"] and not check.get("allow_elevated",False):add("error","floating_object",check.get("id","?"))

 for probe in service_probes or []:
  if probe.get("critical",True) and probe.get("status")!="dispatched":add("error","critical_service_unroutable",probe.get("id","?"))
  if probe.get("status")=="dispatched" and not probe.get("route_nodes"):add("error","service_missing_route",probe.get("id","?"))

 upstream("benchmark",benchmark_report)
 if benchmark_report and benchmark_report.get("status")!="PASS":add("error","benchmark_failed","benchmark report")

 if migration_registry:
  required_systems={"economy","parking","npc_schedule","quests","season"}
  missing=required_systems-set(migration_registry.get("system_payloads",{}))
  for name in sorted(missing):add("error","missing_save_migration_system",name)
  if not migration_registry.get("rules",{}).get("backup_before_migration",False):add("error","migration_backup_disabled","save migration registry")

 errors=sum(x["severity"]=="error" for x in issues);warnings=sum(x["severity"]=="warning" for x in issues)
 return {
  "schema_version":2,"status":"PASS" if errors==0 else "FAIL","errors":errors,"warnings":warnings,"issues":issues,
  "counts":{"features":len(feature_ids),"road_nodes":len(nodes),"road_edges":len(edge_ids),"buildings":len(building_ids),
            "parking":len(parking_ids),"parking_slots":len(parking_slot_ids),"smart_objects":len(smart_ids),"smart_slots":len(smart_slot_ids),
            "crowd_corridors":len((crowd or {}).get("corridors",[])),"traffic_lanes":len(lane_ids),"hlod_assignments":len((hlod or {}).get("assignments",[]))}
 }

def main():
 p=argparse.ArgumentParser();p.add_argument("--policy",type=Path,default=POLICY);p.add_argument("--sector",type=Path,default=SECTOR);p.add_argument("--city",type=Path,default=CITY)
 for name in ("entrances","quality","quests","streaming","landmarks","parking","smart_objects","crowd","traffic","hlod","data_layers","service_probes","benchmark_report","migration_registry"):
  p.add_argument("--"+name.replace("_","-"),dest=name,type=Path)
 p.add_argument("--terrain-checks",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 r=validate(load(a.policy),load(a.sector),load(a.city),load(a.entrances),load(a.quality),load(a.quests),load(a.streaming),load(a.landmarks),
            load(a.parking),load(a.smart_objects),load(a.crowd),load(a.traffic),load(a.hlod),load(a.data_layers),
            (load(a.terrain_checks) or {}).get("checks",[]) if a.terrain_checks else None,
            (load(a.service_probes) or {}).get("probes",[]) if a.service_probes else None,
            load(a.benchmark_report),load(a.migration_registry))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"],r["errors"],"errors",r["warnings"],"warnings");return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
