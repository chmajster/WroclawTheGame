#!/usr/bin/env python3
import argparse,heapq,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CITY=ROOT/"Saved/CityData/sector.json"
def adjacency(graph,mode="car"):
 out={n["id"]:[] for n in graph["nodes"]}
 for e in graph["edges"]:
  if mode=="car":
   if e.get("car_forward"):out[e["from"]].append((e["to"],e))
   if e.get("car_backward"):out[e["to"]].append((e["from"],e))
  elif e.get("foot"):
   out[e["from"]].append((e["to"],e));out[e["to"]].append((e["from"],e))
 return out
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
 edges=[];node=end
 while node!=start:
  p,e,last=prev[node];edges.append(e);node=p
 edges.reverse();return {"distance_cm":dist[end],"edges":edges}
def instructions(result):
 if not result:return []
 out=[]
 for i,e in enumerate(result["edges"]):
  name=e.get("name") or "droga bez nazwy"
  if i==0 or name!=(result["edges"][i-1].get("name") or "droga bez nazwy"):out.append({"type":"continue","road":name,"edge":e["id"]})
 out.append({"type":"arrive"}) if result["edges"] else None
 return out
def main():
 p=argparse.ArgumentParser();p.add_argument("start");p.add_argument("end");p.add_argument("--city",type=Path,default=CITY);p.add_argument("--mode",choices=["car","foot"],default="car");a=p.parse_args();city=json.loads(a.city.read_text());r=route(city["road_graph"],a.start,a.end,a.mode);print(json.dumps({"route":r,"instructions":instructions(r)},ensure_ascii=False))
if __name__=="__main__":main()
