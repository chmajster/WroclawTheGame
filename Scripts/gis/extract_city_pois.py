#!/usr/bin/env python3
import argparse,gzip,json,xml.etree.ElementTree as ET
from pathlib import Path
from import_sector import SOURCE,source_bytes,metadata,Geography
def category(tags):
 if tags.get("shop"):return "retail"
 if tags.get("amenity") in {"restaurant","cafe","fast_food"}:return "food"
 if tags.get("amenity") in {"pharmacy","clinic","hospital"}:return "health"
 if tags.get("amenity") in {"atm","bank"}:return "finance"
 if tags.get("amenity")=="fuel":return "fuel"
 if tags.get("amenity")=="parking":return "parking"
 if tags.get("tourism")=="museum" or tags.get("amenity") in {"theatre","cinema"}:return "culture"
 if tags.get("office") or tags.get("craft"):return "services"
 return None
def build(source=SOURCE/"osm/wave1.osm.gz",terrain=SOURCE/"terrain/wave1.csv"):
 info=metadata(source);geo=Geography(info["BoundingBox"],terrain)
 root=ET.fromstring(gzip.decompress(source_bytes(source)));out=[]
 for n in root.findall("node"):
  tags={t.attrib["k"]:t.attrib["v"] for t in n.findall("tag")};cat=category(tags)
  if not cat:continue
  lon,lat=float(n.attrib["lon"]),float(n.attrib["lat"]);out.append({"id":"node/"+n.attrib["id"],"category":cat,"name":tags.get("name"),"position":geo.world(lon,lat),"geographic":[lon,lat],"tags":tags})
 return {"schema_version":1,"count":len(out),"pois":out}
def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--terrain",type=Path,default=SOURCE/"terrain/wave1.csv");p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=build(a.source,a.terrain);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(",",":"))+"\n");print(r["count"])
if __name__=="__main__":main()
