#!/usr/bin/env python3
import argparse,gzip,json,math,re,xml.etree.ElementTree as ET
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

def tags_of(element):return {t.attrib["k"]:t.attrib["v"] for t in element.findall("tag")}

def search_tokens(name,tags):
 raw=" ".join(x for x in [name or "",tags.get("brand",""),tags.get("operator",""),tags.get("shop",""),tags.get("amenity","")] if x)
 return sorted(set(re.findall(r"[0-9A-Za-zÀ-ž]+",raw.lower())))

def poi_record(identifier,cat,tags,world,geo_pos,kind):
 return {
  "id":identifier,"source_kind":kind,"category":cat,"name":tags.get("name"),"position":world,"geographic":geo_pos,"tags":tags,
  "opening_hours_raw":tags.get("opening_hours"),"wheelchair":tags.get("wheelchair","unknown"),"access":tags.get("access","unknown"),
  "address":{"street":tags.get("addr:street"),"housenumber":tags.get("addr:housenumber"),"city":tags.get("addr:city")},
  "search_tokens":search_tokens(tags.get("name"),tags),
  "gameplay_service":cat in {"food","retail","health","finance","fuel","parking","services"}
 }

def centroid(coords):
 if not coords:return None
 return [sum(p[0] for p in coords)/len(coords),sum(p[1] for p in coords)/len(coords)]

def deduplicate(pois,radius_cm):
 kept=[];aliases={}
 for poi in sorted(pois,key=lambda x:(x["category"],(x["name"] or "").lower(),x["id"])):
  name=(poi["name"] or "").strip().lower()
  dup=None
  if name:
   for existing in kept:
    if existing["category"]==poi["category"] and (existing["name"] or "").strip().lower()==name and math.dist(existing["position"][:2],poi["position"][:2])<=radius_cm:
     dup=existing;break
  if dup:
   aliases[poi["id"]]=dup["id"];dup.setdefault("source_aliases",[]).append(poi["id"])
  else:
   poi["source_aliases"]=[];kept.append(poi)
 return kept,aliases

def build(source=SOURCE/"osm/wave1.osm.gz",terrain=SOURCE/"terrain/wave1.csv",policy=None):
 info=metadata(source);geo=Geography(info["BoundingBox"],terrain)
 root=ET.fromstring(gzip.decompress(source_bytes(source)))
 nodes={n.attrib["id"]:(float(n.attrib["lon"]),float(n.attrib["lat"])) for n in root.findall("node")}
 ways={w.attrib["id"]:[nodes[x.attrib["ref"]] for x in w.findall("nd") if x.attrib["ref"] in nodes] for w in root.findall("way")}
 out=[]
 for n in root.findall("node"):
  tags=tags_of(n);cat=category(tags)
  if not cat:continue
  lon,lat=nodes[n.attrib["id"]];out.append(poi_record("node/"+n.attrib["id"],cat,tags,geo.world(lon,lat),[lon,lat],"node"))
 for w in root.findall("way"):
  tags=tags_of(w);cat=category(tags);c=centroid(ways.get(w.attrib["id"],[]))
  if not cat or not c:continue
  out.append(poi_record("way/"+w.attrib["id"],cat,tags,geo.world(*c),c,"way"))
 for r in root.findall("relation"):
  tags=tags_of(r);cat=category(tags)
  if not cat:continue
  coords=[]
  for m in r.findall("member"):
   if m.attrib.get("type")=="node" and m.attrib.get("ref") in nodes:coords.append(nodes[m.attrib["ref"]])
   elif m.attrib.get("type")=="way":coords.extend(ways.get(m.attrib.get("ref"),[]))
  c=centroid(coords)
  if c:out.append(poi_record("relation/"+r.attrib["id"],cat,tags,geo.world(*c),c,"relation"))
 radius=float((policy or {}).get("dedupe_radius_m",20))*100
 deduped,aliases=deduplicate(out,radius)
 return {"schema_version":2,"count":len(deduped),"source_count":len(out),"pois":deduped,"aliases":aliases}

def main():
 p=argparse.ArgumentParser();p.add_argument("--source",type=Path,default=SOURCE/"osm/wave1.osm.gz");p.add_argument("--terrain",type=Path,default=SOURCE/"terrain/wave1.csv");p.add_argument("--policy",type=Path,default=Path(__file__).resolve().parents[2]/"Data/poi_policy.json");p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=build(a.source,a.terrain,json.loads(a.policy.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,ensure_ascii=False,separators=(",",":"))+"\n");print(r["count"],"from",r["source_count"])
if __name__=="__main__":main()
