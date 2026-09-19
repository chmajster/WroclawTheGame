#!/usr/bin/env python3
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CFG=ROOT/"Data/building_hlod_profiles.json"
def generate(cfg,quality):
 out=[];counts={}
 for d in quality.get("decisions",[]):
  q=d.get("quality","background")
  if q=="missing":q="background"
  profile=cfg["profiles"].get(q,cfg["profiles"]["background"]);counts[q]=counts.get(q,0)+1
  out.append({"building_id":d["building_id"],"quality":q,**profile})
 return {"schema_version":1,"counts":counts,"assignments":out}
def main():
 p=argparse.ArgumentParser();p.add_argument("--quality",type=Path,required=True);p.add_argument("--config",type=Path,default=CFG);p.add_argument("--output",type=Path,required=True);a=p.parse_args();r=generate(json.loads(a.config.read_text()),json.loads(a.quality.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["counts"])
if __name__=="__main__":main()
