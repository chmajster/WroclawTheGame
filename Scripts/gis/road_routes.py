"""Routes for vehicles and walking use the same OSM topology as the rendered map."""
import heapq,json,math
from pathlib import Path

def adjacency(graph,mode='car'):
    if mode not in ('car', 'foot'):raise ValueError('Unknown routing mode')
    result={n['id']:[] for n in graph['nodes']}
    for e in graph['edges']:
        # Bridge/tunnel heights require an authored elevation pass before drive testing.
        if not e.get('surface_built',False) and (e['bridge']!='no' or e['layer']!='0' or e.get('tunnel','no')!='no'):continue
        if (mode=='foot' and e['foot']) or (mode=='car' and e['car_forward']):result[e['from']].append((e['to'],e['length_cm'],e['id']))
        if (mode=='foot' and e['foot']) or (mode=='car' and e['car_backward']):result[e['to']].append((e['from'],e['length_cm'],e['id']))
    return result

def route(graph,start,end,mode='car'):
    links=adjacency(graph,mode)
    if start not in links or end not in links:raise ValueError('Unknown route endpoint')
    queue=[(0,start)];distance={start:0};previous={}
    while queue:
        cost,node=heapq.heappop(queue)
        if cost!=distance[node]:continue
        if node==end:
            path=[end]
            while path[-1]!=start:path.append(previous[path[-1]])
            return list(reversed(path))
        for target,length,_ in links[node]:
            candidate=cost+length
            if candidate<distance.get(target,math.inf):
                distance[target]=candidate;previous[target]=node;heapq.heappush(queue,(candidate,target))
    raise ValueError('No legal route between endpoints')

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('start');p.add_argument('end');p.add_argument('--mode',choices=['car','foot'],default='car');a=p.parse_args()
    root=Path(__file__).resolve().parents[2]
    data=json.loads((root/'Data/processed/wroclaw/sector.json').read_text())
    print(json.dumps(route(data['road_graph'],a.start,a.end,a.mode)))
