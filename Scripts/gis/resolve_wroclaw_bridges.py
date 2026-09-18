#!/usr/bin/env python3
"""Resolve curated Wrocław bridges against OSM GIS features and audit engineering readiness."""
from __future__ import annotations
import argparse,json,re,unicodedata
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_CATALOG=ROOT/"Data"/"wroclaw_bridges.json"
DEFAULT_CITY=ROOT/"Saved"/"CityData"/"sector.json"
DEFAULT_AUTHORED=ROOT/"Data"/"city_structures.json"

def norm(v):
    v=unicodedata.normalize("NFKD",str(v).casefold())
    v="".join(c for c in v if not unicodedata.combining(c))
    return " ".join(re.sub(r"[^a-z0-9]+"," ",v).split())

def resolve(catalog,city,authored=None):
    bridge_features=[]
    for f in city.get("features",[]):
        tags=f.get("tags",{})
        if f.get("kind")=="road" and tags.get("bridge","no")!="no":
            bridge_features.append(f)
    authored_by_name={}
    for item in (authored or {}).get("bridges",[]):
        authored_by_name.setdefault(norm(item["name"]),[]).append(item)
    records=[];counts={"resolved":0,"unresolved":0,"authored_prototype":0}
    for bridge in catalog["bridges"]:
        names={norm(bridge["name"]),*(norm(x) for x in bridge.get("aliases",[]))}
        matched=[]
        for f in bridge_features:
            candidate=norm(f.get("name") or f.get("tags",{}).get("name",""))
            if candidate and any(candidate==name or candidate in name or name in candidate for name in names):
                matched.append(f)
        authored_items=[]
        for name in names:authored_items.extend(authored_by_name.get(name,[]))
        status="resolved" if matched else "unresolved"
        counts[status]+=1
        if authored_items:counts["authored_prototype"]+=1
        records.append({
          **bridge,"status":status,
          "osm_features":[{"id":f["id"],"name":f.get("name"),"points":f.get("points"),"tags":f.get("tags",{})} for f in matched],
          "authored_prototype_ways":[x["way"] for x in authored_items],
          "geometry_source":"osm" if matched else None,
          "elevation_source":"prototype_interpolation" if authored_items else None,
          "requires_surveyed_elevation":True,
          "engine_verified":False
        })
    return {"schema_version":1,"counts":counts,"bridges":records}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument("--catalog",type=Path,default=DEFAULT_CATALOG);p.add_argument("--city",type=Path,default=DEFAULT_CITY);p.add_argument("--authored",type=Path,default=DEFAULT_AUTHORED);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
    authored=json.loads(a.authored.read_text()) if a.authored.is_file() else {}
    result=resolve(json.loads(a.catalog.read_text()),json.loads(a.city.read_text()),authored)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+"\n",encoding="utf-8");print(json.dumps(result["counts"]))
if __name__=="__main__":main()
