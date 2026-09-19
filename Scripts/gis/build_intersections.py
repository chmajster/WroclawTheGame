#!/usr/bin/env python3
import argparse,gzip,json,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import ROOT,SOURCE,source_bytes
def build(source=SOURCE/"osm/wave1.osm.gz"):
 root=ET.fromstring(gzip.decompress(source_bytes(source)))
 nodes={n.attrib["id"]:{t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")} for n in root.findall("node")}
 junctions=[]
 for nid,t in nodes.items():
  if t.get("highway") in {"traffic_signals","stop","give_way","crossing"}:
   junctions.append({"node":nid,"control":t.get("highway"),"crossing":t.get("crossing"),"traffic_signals":t.get("traffic_signals")})
 restrictions=[]
 for r in root.findall("relation"):
  tags={t.attrib["k"]:t.attrib["v"] for t in r.findall("tag")}
  if tags.get("type")!="restriction":continue
  members={m.attrib.get("role"):(m.attrib.get("type"),m.attrib.get("ref")) for m in r.findall("member") if m.attrib.get("role") in {"from","to","via"}}
  if not {"from","to","via"}<=members.keys():continue
  restrictions.append({"id":r.attrib["id"],"restriction":tags.get("restriction"),"except":tags.get("except"),"members":members})
 return {"schema_version":1,"junctions":junctions,"turn_restrictions":restrictions}
def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=build(a.source);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,separators=(",",":"))+"\n");print(len(r["junctions"]),len(r["turn_restrictions"]))
if __name__=="__main__":main()
