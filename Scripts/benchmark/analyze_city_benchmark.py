#!/usr/bin/env python3
import argparse,csv,json,statistics
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];POLICY=ROOT/"Data/benchmark_policy.json";ROUTES=ROOT/"Data/benchmark_routes.json"
NUMERIC=["fps","frame_ms","cpu_ms","gpu_ms","ram_mb","vram_mb","streaming_misses","draw_calls"]

def pct(values,p):
 if not values:return None
 v=sorted(values);return v[min(len(v)-1,max(0,int(round((len(v)-1)*p))))]

def summarize(rows):
 summary={}
 for k in NUMERIC:
  vals=[float(r[k]) for r in rows if r.get(k) not in (None,"")]
  if vals:summary[k]={"min":min(vals),"mean":round(statistics.fmean(vals),4),"p95":pct(vals,.95),"max":max(vals)}
 return summary

def threshold_issues(summary,policy):
 issues=[];t=policy["targets"]
 if summary.get("fps",{}).get("min",999)<t["min_fps"]:issues.append("min_fps")
 if summary.get("frame_ms",{}).get("p95",0)>t["max_p95_frame_ms"]:issues.append("p95_frame_ms")
 if summary.get("cpu_ms",{}).get("p95",0)>t["max_p95_cpu_ms"]:issues.append("p95_cpu_ms")
 if summary.get("gpu_ms",{}).get("p95",0)>t["max_p95_gpu_ms"]:issues.append("p95_gpu_ms")
 if summary.get("ram_mb",{}).get("max",0)>t["max_ram_mb"]:issues.append("ram")
 if summary.get("vram_mb",{}).get("max",0)>t["max_vram_mb"]:issues.append("vram")
 if summary.get("streaming_misses",{}).get("max",0)>t["max_streaming_misses"]:issues.append("streaming")
 if summary.get("draw_calls",{}).get("max",0)>t.get("max_draw_calls",float("inf")):issues.append("draw_calls")
 return issues

def compare_baseline(summary,baseline,policy):
 if not baseline:return []
 issues=[];limit=float(policy.get("max_regression_pct",10))
 checks={"frame_ms":"p95","cpu_ms":"p95","gpu_ms":"p95","ram_mb":"max","vram_mb":"max","draw_calls":"max"}
 for metric,stat in checks.items():
  current=summary.get(metric,{}).get(stat);base=baseline.get(metric,{}).get(stat)
  if current is None or base in (None,0):continue
  regression=(float(current)-float(base))/float(base)*100
  if regression>limit:issues.append({"metric":metric,"stat":stat,"regression_pct":round(regression,2),"limit_pct":limit})
 fps=summary.get("fps",{}).get("min");base_fps=baseline.get("fps",{}).get("min")
 if fps is not None and base_fps not in (None,0):
  regression=(float(base_fps)-float(fps))/float(base_fps)*100
  if regression>limit:issues.append({"metric":"fps","stat":"min","regression_pct":round(regression,2),"limit_pct":limit})
 return issues

def analyze(rows,policy,baseline=None):
 summary=summarize(rows);issues=threshold_issues(summary,policy);regressions=compare_baseline(summary,baseline,policy)
 return {"status":"PASS" if not issues and not regressions else "FAIL","issues":issues,"regressions":regressions,"samples":len(rows),"summary":summary}

def analyze_routes(rows,policy,routes,baseline=None):
 required={r["id"] for r in routes["routes"]};grouped={}
 for row in rows:
  route=row.get("route_id")
  if route:grouped.setdefault(route,[]).append(row)
 missing=sorted(required-set(grouped));route_results={}
 for route_id,items in sorted(grouped.items()):
  route_results[route_id]=analyze(items,policy,(baseline or {}).get("routes",{}).get(route_id))
 issues=[{"code":"missing_route","route_id":x} for x in missing]
 for route_id,result in route_results.items():
  if result["status"]!="PASS":issues.append({"code":"route_failed","route_id":route_id})
 metadata_fields=policy.get("required_metadata",[])
 metadata={}
 for key in metadata_fields:
  values=sorted({r.get(key) for r in rows if r.get(key)})
  metadata[key]=values
  if not values:issues.append({"code":"missing_metadata","field":key})
 return {"schema_version":2,"status":"PASS" if not issues else "FAIL","issues":issues,"routes":route_results,"metadata":metadata,"samples":len(rows)}

def main():
 p=argparse.ArgumentParser();p.add_argument("csv",type=Path);p.add_argument("--policy",type=Path,default=POLICY);p.add_argument("--routes",type=Path,default=ROUTES);p.add_argument("--baseline",type=Path);p.add_argument("--output",type=Path,required=True);a=p.parse_args()
 rows=list(csv.DictReader(a.csv.open(newline="",encoding="utf-8")));baseline=json.loads(a.baseline.read_text()) if a.baseline else None;r=analyze_routes(rows,json.loads(a.policy.read_text()),json.loads(a.routes.read_text()),baseline);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+"\n");print(r["status"]);return 0 if r["status"]=="PASS" else 1
if __name__=="__main__":raise SystemExit(main())
