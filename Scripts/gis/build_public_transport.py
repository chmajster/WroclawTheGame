#!/usr/bin/env python3
import argparse,gzip,json,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import ROOT,SOURCE,source_bytes
def build(source=SOURCE/"osm/wave1.osm.gz"):
 root=ET.fromstring(gzip.decompress(source_bytes(source)))
 nodes={n.attrib["id"]:{"lon":float(n.attrib["lon"]),"lat":float(n.attrib["lat"]),"tags":{t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")}} for n in root.findall("node")}
 routes=[];stop_ids=set()
 for r in root.findall("relation"):
  tags={t.attrib["k"]:t.attrib["v"] for t in r.findall("tag")}
  if tags.get("type")!="route" or tags.get("route") not in {"tram","bus"}:continue
  members=[]
  for m in r.findall("member"):
   role=m.attrib.get("role","");members.append({"type":m.attrib["type"],"ref":m.attrib["ref"],"role":role})
   if m.attrib["type"]=="node" and role in {"stop","platform","stop_entry_only","stop_exit_only"}:stop_ids.add(m.attrib["ref"])
  routes.append({"id":r.attrib["id"],"mode":tags.get("route"),"ref":tags.get("ref"),"name":tags.get("name"),"from":tags.get("from"),"to":tags.get("to"),"members":members})
 stops=[{"id":nid,**nodes[nid]} for nid in sorted(stop_ids) if nid in nodes]
 return {"schema_version":1,"routes":routes,"stops":stops}
def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=build(a.source);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(",",":"))+"\n");print(len(r["routes"]),len(r["stops"]))
if __name__=="__main__":main()
