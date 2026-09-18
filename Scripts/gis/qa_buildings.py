#!/usr/bin/env python3
"""Static QA gates for building catalogs before Unreal visual/runtime QA."""
from __future__ import annotations
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_POLICY=ROOT/"Data"/"building_qa_policy.json"

def load(path):
    if not path:return None
    return json.loads(path.read_text(encoding="utf-8"))

def records(data):
    if data is None:return []
    if isinstance(data,list):return data
    for key in ("buildings","records","decisions","entrances","bridges"):
        if isinstance(data.get(key),list):return data[key]
    return []

def issue(bucket,severity,code,message,record=None):
    item={"severity":severity,"code":code,"message":message}
    if record:
        item["id"]=record.get("id") or record.get("building_id") or record.get("gml_id")
    bucket.append(item)

def qa(policy,official=None,quality=None,mesh_catalog=None):
    issues=[];thresholds=policy["thresholds"]
    official_records=records(official);quality_records=records(quality);mesh_records=records(mesh_catalog)
    seen_replacements={}
    for r in official_records:
        for field in policy["required_fields"]["official"]:
            if r.get(field) in (None,""):issue(issues,"error","official_missing_field",f"Missing {field}",r)
        replaced=r.get("replaced_feature_id")
        if replaced:
            if replaced in seen_replacements:
                issue(issues,"error","duplicate_replacement",f"OSM feature {replaced} replaced more than once",r)
            seen_replacements[replaced]=r.get("id")
        distance=r.get("distance_to_osm_reference_m")
        if distance is not None and float(distance)>thresholds["max_osm_match_distance_m"]:
            issue(issues,"error","match_distance",f"OSM match distance {distance} m exceeds limit",r)
        overlap=r.get("footprint_overlap_ratio")
        if overlap is not None and float(overlap)<thresholds["min_footprint_overlap_ratio"]:
            issue(issues,"warning","low_overlap",f"Footprint overlap {overlap} below preferred threshold",r)

    seen_quality=set()
    for r in quality_records:
        for field in policy["required_fields"]["quality"]:
            if r.get(field) in (None,""):issue(issues,"error","quality_missing_field",f"Missing {field}",r)
        bid=r.get("building_id")
        if bid in seen_quality:issue(issues,"error","duplicate_quality_decision",f"Duplicate quality decision for {bid}",r)
        seen_quality.add(bid)
        if policy.get("hero_requires_visual_qa") and r.get("quality")=="hero":
            selected=r.get("selected") or r
            if str(selected.get("qa_status","")).upper()!="PASS":
                issue(issues,"error","hero_without_visual_qa","Hero representation has no QA PASS",r)

    for r in mesh_records:
        quality_name=r.get("quality","background")
        triangle_limit=thresholds.get("max_triangles_"+quality_name)
        triangles=r.get("triangles") or r.get("triangle_count")
        if triangle_limit is not None and triangles is not None and int(triangles)>triangle_limit:
            issue(issues,"error","triangle_budget",f"{triangles} triangles exceed {quality_name} budget {triangle_limit}",r)
        vertices=r.get("vertices") or r.get("vertex_count")
        if isinstance(vertices,int) and vertices>thresholds["max_vertices_per_mesh_group"]:
            issue(issues,"error","vertex_budget",f"{vertices} vertices exceed mesh-group budget",r)

    errors=sum(i["severity"]=="error" for i in issues)
    warnings=sum(i["severity"]=="warning" for i in issues)
    return {"schema_version":1,"status":"PASS" if errors==0 else "FAIL","errors":errors,"warnings":warnings,"issues":issues,
            "counts":{"official":len(official_records),"quality":len(quality_records),"meshes":len(mesh_records)}}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--policy",type=Path,default=DEFAULT_POLICY);p.add_argument("--official",type=Path);p.add_argument("--quality",type=Path);p.add_argument("--meshes",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    result=qa(load(a.policy),load(a.official),load(a.quality),load(a.meshes));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n",encoding="utf-8");print(result["status"],result["errors"],"errors",result["warnings"],"warnings");return 0 if result["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
