#!/usr/bin/env python3
import argparse,gzip,json,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import ROOT,SOURCE,source_bytes

CITY=ROOT/"Saved/CityData/sector.json"
POLICY=ROOT/"Data/intersection_policy.json"

def directed_edges(graph):
 out=[]
 for e in graph.get("edges",[]):
  if e.get("car_forward"):out.append({"edge":e["id"],"way":str(e.get("way","")),"from":str(e["from"]),"to":str(e["to"])})
  if e.get("car_backward"):out.append({"edge":e["id"],"way":str(e.get("way","")),"from":str(e["to"]),"to":str(e["from"])})
 return out

def resolve_restriction(restriction,graph):
 members=restriction["members"];via=members.get("via")
 if not via:return {"status":"unresolved","reason":"missing_via","pairs":[]}
 if via[0]!="node":return {"status":"unresolved","reason":"via_way_not_yet_supported","pairs":[]}
 via_node=str(via[1]);from_way=str(members["from"][1]);to_way=str(members["to"][1]);directed=directed_edges(graph)
 incoming=[d for d in directed if d["way"]==from_way and d["to"]==via_node]
 outgoing=[d for d in directed if d["way"]==to_way and d["from"]==via_node]
 pairs=[{"from_edge":a["edge"],"to_edge":b["edge"],"via_node":via_node} for a in incoming for b in outgoing]
 return {"status":"resolved" if pairs else "unresolved","reason":None if pairs else "edge_mapping_failed","pairs":pairs}

def junction_descriptor(nid,tags,policy):
 control=tags.get("highway")
 cfg=policy.get("controls",{}).get(control,{})
 return {
  "node":str(nid),
  "control":control,
  "crossing":tags.get("crossing"),
  "traffic_signals":tags.get("traffic_signals"),
  "priority":cfg.get("priority","normal"),
  "stop_line_offset_m":cfg.get("stop_line_offset_m",0),
  "pedestrian_conflict":bool(control=="crossing" or tags.get("crossing") or cfg.get("pedestrian_conflict",False)),
  "requires_reservation":bool(cfg.get("requires_reservation",control in {"traffic_signals","stop","give_way"}))
 }

def build(source=SOURCE/"osm/wave1.osm.gz",city=None,policy=None):
 root=ET.fromstring(gzip.decompress(source_bytes(source)))
 policy=policy or {"controls":{}}
 nodes={n.attrib["id"]:{t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")} for n in root.findall("node")}
 junctions=[]
 for nid,t in nodes.items():
  if t.get("highway") in {"traffic_signals","stop","give_way","crossing"}:
   junctions.append(junction_descriptor(nid,t,policy))
 restrictions=[]
 for r in root.findall("relation"):
  tags={t.attrib["k"]:t.attrib["v"] for t in r.findall("tag")}
  if tags.get("type")!="restriction":continue
  members={m.attrib.get("role"):(m.attrib.get("type"),m.attrib.get("ref")) for m in r.findall("member") if m.attrib.get("role") in {"from","to","via"}}
  if not {"from","to","via"}<=members.keys():continue
  rec={"id":r.attrib["id"],"restriction":tags.get("restriction"),"except":tags.get("except"),"members":members}
  if city:
   rec.update(resolve_restriction(rec,city.get("road_graph",{})))
  else:
   rec.update({"status":"unresolved","reason":"road_graph_not_supplied","pairs":[]})
  restrictions.append(rec)
 unresolved=[r for r in restrictions if r["status"]!="resolved"]
 return {
  "schema_version":2,
  "junctions":junctions,
  "turn_restrictions":restrictions,
  "resolved_pairs":[{"restriction_id":r["id"],**pair,"restriction":r["restriction"],"except":r.get("except")} for r in restrictions for pair in r["pairs"]],
  "unresolved_restrictions":[{"id":r["id"],"reason":r["reason"]} for r in unresolved]
 }

def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--city",type=Path,default=CITY);p.add_argument("--policy",type=Path,default=POLICY);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 city=json.loads(a.city.read_text()) if a.city and a.city.is_file() else None
 policy=json.loads(a.policy.read_text()) if a.policy and a.policy.is_file() else None
 r=build(a.source,city,policy);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n")
 print(len(r["junctions"]),len(r["turn_restrictions"]),len(r["resolved_pairs"]),len(r["unresolved_restrictions"]))
if __name__=="__main__":main()
