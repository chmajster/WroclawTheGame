#!/usr/bin/env python3
import argparse,gzip,json,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import ROOT,SOURCE,source_bytes
CFG=ROOT/"Data/public_transport_policy.json"

def build(source=SOURCE/"osm/wave1.osm.gz",policy=None):
 policy=policy or {"default_headway_minutes":{"tram":10,"bus":15},"logical_update_seconds":15,"physicalize_radius_m":220,"max_physical_vehicles":8}
 root=ET.fromstring(gzip.decompress(source_bytes(source)))
 nodes={n.attrib["id"]:{"lon":float(n.attrib["lon"]),"lat":float(n.attrib["lat"]),"tags":{t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")}} for n in root.findall("node")}
 ways={w.attrib["id"]:{"nodes":[n.attrib["ref"] for n in w.findall("nd")],"tags":{t.attrib["k"]:t.attrib["v"] for t in w.findall("tag")}} for w in root.findall("way")}
 routes=[];stop_ids=set()
 for r in root.findall("relation"):
  tags={t.attrib["k"]:t.attrib["v"] for t in r.findall("tag")}
  mode=tags.get("route")
  if tags.get("type")!="route" or mode not in {"tram","bus"}:continue
  members=[];stop_sequence=[];way_refs=[]
  for index,m in enumerate(r.findall("member")):
   role=m.attrib.get("role","");typ=m.attrib["type"];ref=m.attrib["ref"]
   members.append({"type":typ,"ref":ref,"role":role,"order":index})
   if typ=="node" and role in {"stop","platform","stop_entry_only","stop_exit_only"}:
    stop_ids.add(ref);stop_sequence.append({"stop_id":ref,"role":role,"order":len(stop_sequence)})
   if typ=="way":way_refs.append(ref)
  breaks=[]
  for a,b in zip(way_refs,way_refs[1:]):
   an=ways.get(a,{}).get("nodes",[]);bn=ways.get(b,{}).get("nodes",[])
   if not an or not bn or not set((an[0],an[-1]))&set((bn[0],bn[-1])):
    breaks.append({"from_way":a,"to_way":b})
  route_id=r.attrib["id"];ref=tags.get("ref") or route_id
  routes.append({
   "id":route_id,"service_id":f"{mode}:{ref}:{route_id}","mode":mode,"ref":tags.get("ref"),"name":tags.get("name"),
   "from":tags.get("from"),"to":tags.get("to"),"members":members,"way_refs":way_refs,"stop_sequence":stop_sequence,
   "continuity":{"continuous":not breaks,"breaks":breaks},
   "simulation":{"headway_minutes":policy["default_headway_minutes"].get(mode,15),"logical_update_seconds":policy["logical_update_seconds"],"physicalize_radius_m":policy["physicalize_radius_m"]}
  })
 stops=[]
 for nid in sorted(stop_ids):
  if nid not in nodes:continue
  tags=nodes[nid]["tags"]
  stops.append({"id":nid,**nodes[nid],"name":tags.get("name"),"wheelchair":tags.get("wheelchair","unknown"),"shelter":tags.get("shelter","unknown")})
 return {"schema_version":2,"routes":routes,"stops":stops,"limits":{"max_physical_vehicles":policy["max_physical_vehicles"]},"continuity_breaks":sum(len(x["continuity"]["breaks"]) for x in routes)}

def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();policy=json.loads(a.config.read_text());r=build(a.source,policy);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(",",":"))+"\n");print(len(r["routes"]),len(r["stops"]),r["continuity_breaks"])
if __name__=="__main__":main()
