"""Three implemented driving trials; no fake pursuit or tailing opponents."""
import json,math
from pathlib import Path
from road_routes import route
from import_sector import ROOT

def generate():
    sector=json.loads((ROOT/'Data/processed/wroclaw/sector.json').read_text());graph=sector['road_graph'];nodes={n['id']:n for n in graph['nodes']}
    candidates=[e for e in graph['edges'] if e['name']=='Ludwika Rydygiera' and e['car_forward'] and e['length_cm']>1500 and e['bridge']=='no']
    start=max(candidates,key=lambda e:e['length_cm'])['from'];results=[]
    for identifier,title,street,delivery,seconds in [('night_sprint','Nocny sprint — Pomorska','Pomorska',False,150),('time_trial','Próba czasu — Trzebnicka','Trzebnicka',False,120),('delivery','Przesyłka — Jagiellończyka','Kazimierza Jagiellończyka',True,180)]:
        routes=[]
        for end in sorted({e['to'] for e in graph['edges'] if e['name']==street and e['car_forward']}):
            try:
                path=route(graph,start,end)
                if len(path)>2:routes.append(path)
            except ValueError:pass
        if not routes:raise ValueError('No legal route: '+street)
        path=max(routes,key=lambda p:sum(math.dist(nodes[a]['position'],nodes[b]['position']) for a,b in zip(p,p[1:])))
        positions=[nodes[n]['position'] for n in path];a,b=positions[:2]
        results.append({'id':identifier,'title':title,'mode':'Delivery' if delivery else 'TimeTrial','time_limit':seconds,'source_nodes':path,
                        'start':[a[0],a[1],a[2]+85],'yaw':math.degrees(math.atan2(b[1]-a[1],b[0]-a[0])),
                        'checkpoints':[[p[0],p[1],p[2]+65] for p in positions[1:]],'street':street})
    (ROOT/'Data/processed/wroclaw/races.json').write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    return results
if __name__=='__main__':print('Real road routes:',len(generate()))
