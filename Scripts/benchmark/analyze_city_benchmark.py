#!/usr/bin/env python3
import argparse,csv,json,statistics
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];POLICY=ROOT/"Data/benchmark_policy.json"
def pct(values,p):
 if not values:return None
 v=sorted(values);return v[min(len(v)-1,max(0,int(round((len(v)-1)*p))))]
def analyze(rows,policy):
 numeric=["fps","frame_ms","cpu_ms","gpu_ms","ram_mb","vram_mb","streaming_misses","draw_calls"];summary={}
 for k in numeric:
  vals=[float(r[k]) for r in rows if r.get(k) not in (None,"")]
  if vals:summary[k]={"min":min(vals),"mean":statistics.fmean(vals),"p95":pct(vals,.95),"max":max(vals)}
 issues=[];t=policy["targets"]
 if summary.get("fps",{}).get("min",999)<t["min_fps"]:issues.append("min_fps")
 if summary.get("frame_ms",{}).get("p95",0)>t["max_p95_frame_ms"]:issues.append("p95_frame_ms")
 if summary.get("cpu_ms",{}).get("p95",0)>t["max_p95_cpu_ms"]:issues.append("p95_cpu_ms")
 if summary.get("gpu_ms",{}).get("p95",0)>t["max_p95_gpu_ms"]:issues.append("p95_gpu_ms")
 if summary.get("ram_mb",{}).get("max",0)>t["max_ram_mb"]:issues.append("ram")
 if summary.get("vram_mb",{}).get("max",0)>t["max_vram_mb"]:issues.append("vram")
 if summary.get("streaming_misses",{}).get("max",0)>t["max_streaming_misses"]:issues.append("streaming")
 return {"status":"PASS" if not issues else "FAIL","issues":issues,"samples":len(rows),"summary":summary}
def main():
 p=argparse.ArgumentParser();p.add_argument("csv",type=Path);p.add_argument("--policy",type=Path,default=POLICY);p.add_argument("--output",type=Path,required=True);a=p.parse_args();rows=list(csv.DictReader(a.csv.open(newline="",encoding="utf-8")));r=analyze(rows,json.loads(a.policy.read_text()));a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"]);return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
