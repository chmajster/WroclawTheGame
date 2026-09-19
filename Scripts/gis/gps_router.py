#!/usr/bin/env python3
import argparse,heapq,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json"

DEFAULT_SPEED_KPH={"motorway":120,"trunk":100,"primary":70,"secondary":60,"tertiary":50,"residential":30,"living_street":20,"service":20,"default":50}

def adjacency(graph,mode="car"):
 out={n["id"]:[] for n in graph["nodes"]}
 for e in graph["edges"]:
  if mode=="car":
   if e.get("car_forward"):out[e["from"]].append((e["to"],e))
   if e.get("car_backward"):out[e["to"]].append((e["from"],e))
  elif e.get("foot"):
   out[e["from"]].append((e["to"],e));out[e["to"]].append((e["from"],e))
 return out

def nearest_node(graph,pos,mode="car"):
 allowed=set(adjacency(graph,mode))
 best=None
 for n in graph["nodes"]:
  if n["id"] not in allowed or not n.get("position"):continue
  d=math.dist(n["position"][:2],pos[:2])
  if best is None or d<best[0]:best=(d,n["id"])
 return None if best is None else {"node":best[1],"distance_cm":round(best[0],2)}

def blocked_pairs(intersections):
 return {(x["from_edge"],x["to_edge"]) for x in (intersections or {}).get("resolved_pairs",[]) if str(x.get("restriction","")).startswith("no_")}

def route(graph,start,end,mode="car",blocked_turns=None):
 adj=adjacency(graph,mode);dist={start:0};prev={};q=[(0,start,None)];blocked_turns=set(blocked_turns or [])
 while q:
  cost,node,last_edge=heapq.heappop(q)
  if cost!=dist.get(node):continue
  if node==end:break
  for nxt,e in adj.get(node,[]):
   if last_edge and (last_edge,e["id"]) in blocked_turns:continue
   nc=cost+float(e["length_cm"])
   if nc<dist.get(nxt,float("inf")):dist[nxt]=nc;prev[nxt]=(node,e,last_edge);heapq.heappush(q,(nc,nxt,e["id"]))
 if end not in dist:return None
 steps=[];node=end
 while node!=start:
  p,e,last=prev[node];steps.append({"from":p,"to":node,"edge":e});node=p
 steps.reverse()
 return {"distance_cm":dist[end],"edges":[s["edge"] for s in steps],"steps":steps,"start_node":start,"end_node":end}

def edge_speed_kph(edge):
 raw=edge.get("maxspeed")
 if raw:
  import re
  m=re.search(r"(\d+(?:\.\d+)?)",str(raw))
  if m:
   v=float(m.group(1))
   return v*1.609344 if "mph" in str(raw).lower() else v
 return float(DEFAULT_SPEED_KPH.get(edge.get("highway"),DEFAULT_SPEED_KPH["default"]))

def eta_seconds(result,mode="car",traffic_multiplier=1.0):
 if not result:return None
 if mode=="foot":speed_mps=1.4;return round((result["distance_cm"]/100)/speed_mps,1)
 total=0.0;traffic=max(0.15,float(traffic_multiplier))
 for step in result["steps"]:
  e=step["edge"];speed_mps=(edge_speed_kph(e)/3.6)*traffic
  total+=(float(e["length_cm"])/100)/max(0.1,speed_mps)
 return round(total,1)

def signed_turn(graph,a,b,c):
 nodes={n["id"]:n for n in graph["nodes"]}
 if any(x not in nodes or not nodes[x].get("position") for x in (a,b,c)):return None
 pa,pb,pc=[nodes[x]["position"] for x in (a,b,c)]
 v1=(pb[0]-pa[0],pb[1]-pa[1]);v2=(pc[0]-pb[0],pc[1]-pb[1])
 dot=v1[0]*v2[0]+v1[1]*v2[1];cross=v1[0]*v2[1]-v1[1]*v2[0]
 return math.degrees(math.atan2(cross,dot))

def instructions(result,graph=None):
 if not result:return []
 out=[];steps=result.get("steps",[])
 for i,s in enumerate(steps):
  e=s["edge"];name=e.get("name") or "droga bez nazwy"
  if i==0:
   out.append({"type":"depart","road":name,"edge":e["id"]});continue
  prev=steps[i-1];angle=signed_turn(graph,prev["from"],prev["to"],s["to"]) if graph else None
  if e.get("junction")=="roundabout":kind="roundabout"
  elif angle is None:kind="continue" if name==(prev["edge"].get("name") or "droga bez nazwy") else "turn"
  elif abs(angle)<25:kind="continue"
  elif abs(angle)>155:kind="u_turn"
  elif angle<0:kind="left"
  else:kind="right"
  if kind!="continue" or name!=(prev["edge"].get("name") or "droga bez nazwy"):out.append({"type":kind,"road":name,"edge":e["id"],"angle_deg":None if angle is None else round(angle,1)})
 if steps:out.append({"type":"arrive"})
 return out

def navigate(graph,start=None,end=None,start_position=None,end_position=None,mode="car",intersections=None,traffic_multiplier=1.0):
 start_snap=nearest_node(graph,start_position,mode) if start_position is not None else None
 end_snap=nearest_node(graph,end_position,mode) if end_position is not None else None
 start=start or (start_snap and start_snap["node"]);end=end or (end_snap and end_snap["node"])
 if not start or not end:return {"route":None,"instructions":[],"eta_seconds":None,"start_snap":start_snap,"end_snap":end_snap}
 r=route(graph,start,end,mode,blocked_pairs(intersections));return {"route":r,"instructions":instructions(r,graph),"eta_seconds":eta_seconds(r,mode,traffic_multiplier),"start_snap":start_snap,"end_snap":end_snap}

def reroute(graph,current_position,end,mode="car",intersections=None,traffic_multiplier=1.0):
 snap=nearest_node(graph,current_position,mode)
 return navigate(graph,start=snap["node"] if snap else None,end=end,mode=mode,intersections=intersections,traffic_multiplier=traffic_multiplier)

def main():
 p=argparse.ArgumentParser();p.add_argument("start");p.add_argument("end");p.add_argument("--city",type=Path,default=CITY);p.add_argument("--mode",choices=["car","foot"],default="car");p.add_argument("--intersections",type=Path);p.add_argument("--traffic-multiplier",type=float,default=1.0);a=p.parse_args();city=json.loads(a.city.read_text());ints=json.loads(a.intersections.read_text()) if a.intersections else None;print(json.dumps(navigate(city["road_graph"],start=a.start,end=a.end,mode=a.mode,intersections=ints,traffic_multiplier=a.traffic_multiplier),ensure_ascii=False))
if __name__=="__main__":main()
