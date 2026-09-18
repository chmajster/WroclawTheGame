"""Generate six data-driven driving scenarios on legal OSM road-graph paths."""
import json,math
from pathlib import Path
from road_routes import route
from import_sector import ROOT

SPECS=[
    ('night_sprint','Nocny sprint — Pomorska','Pomorska','TimeTrial',150),
    ('time_trial','Próba czasu — Trzebnicka','Trzebnicka','TimeTrial',120),
    ('delivery','Przesyłka — Jagiellończyka','Kazimierza Jagiellończyka','Delivery',180),
    ('escape','Ucieczka — Pomorska','Pomorska','Escape',180),
    ('follow','Czarny samochód — Trzebnicka','Trzebnicka','Follow',210),
    ('navigation','Bez mapy — Jagiellończyka','Kazimierza Jagiellończyka','Navigation',220),
]

def _edge_names(graph,path):
    by_pair={}
    for edge in graph['edges']:
        if edge.get('car_forward'):
            by_pair[(edge['from'],edge['to'])]=edge
    names=[]
    for a,b in zip(path,path[1:]):
        name=by_pair.get((a,b),{}).get('name','').strip()
        if name and (not names or names[-1]!=name):
            names.append(name)
    return names

def _navigation_hints(graph,path):
    names=_edge_names(graph,path)
    if not names:
        return ['Jedź zgodnie z rozpoznanymi ulicami do kolejnego charakterystycznego punktu.']
    indexes=sorted({0,len(names)//2,len(names)-1})
    return [f'Kieruj się na: {names[i]}' for i in indexes]

def generate():
    sector=json.loads((ROOT/'Data/processed/wroclaw/sector.json').read_text())
    graph=sector['road_graph'];nodes={n['id']:n for n in graph['nodes']}
    candidates=[e for e in graph['edges'] if e['name']=='Ludwika Rydygiera' and e['car_forward'] and e['length_cm']>1500 and e['bridge']=='no']
    if not candidates:raise ValueError('No verified start segment: Ludwika Rydygiera')
    start=max(candidates,key=lambda e:e['length_cm'])['from']
    route_cache={}
    results=[]
    for identifier,title,street,mode,seconds in SPECS:
        if street not in route_cache:
            routes=[]
            for end in sorted({e['to'] for e in graph['edges'] if e['name']==street and e['car_forward']}):
                try:
                    path=route(graph,start,end)
                    if len(path)>2:routes.append(path)
                except ValueError:pass
            if not routes:raise ValueError('No legal route: '+street)
            route_cache[street]=max(
                routes,key=lambda p:sum(math.dist(nodes[a]['position'],nodes[b]['position']) for a,b in zip(p,p[1:])))
        path=route_cache[street]
        positions=[nodes[n]['position'] for n in path];a,b=positions[:2]
        record={
            'id':identifier,'title':title,'mode':mode,'time_limit':seconds,'source_nodes':path,
            'start':[a[0],a[1],a[2]+85],
            'yaw':math.degrees(math.atan2(b[1]-a[1],b[0]-a[0])),
            'checkpoints':[[p[0],p[1],p[2]+65] for p in positions[1:]],
            'street':street,
        }
        if mode=='Escape':
            record['pursuer_count']=2
        elif mode=='Follow':
            record.update(min_follow_distance=700,max_follow_distance=2600,lost_target_time=6)
        elif mode=='Navigation':
            record['navigation_hints']=_navigation_hints(graph,path)
        results.append(record)
    (ROOT/'Data/processed/wroclaw/races.json').write_text(
        json.dumps(results,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    return results
if __name__=='__main__':print('Real road scenarios:',len(generate()))
