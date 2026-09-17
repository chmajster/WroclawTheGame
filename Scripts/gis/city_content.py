"""Place stable activity IDs and bake legal ambient loops from the global OSM graph."""
import math
from road_routes import adjacency, route

def generate_content(data, catalog, gameplay, owners):
    nodes={n['id']:n for n in data['road_graph']['nodes']}
    buildings={f['id']:f for f in data['features'] if f['kind']=='building'}
    actors=[]
    for action in gameplay['actions']:
        if action['node'] not in nodes or action['building'] not in buildings:
            raise ValueError('Authored activity source disappeared: '+action['id'])
        node=nodes[action['node']]
        if owners[action['node']]!=action['sector'] or buildings[action['building']]['sector']!=action['sector']:
            raise ValueError('Activity changed production sector')
        actors.append({'id':action['id'],'building':action['building'],'sector':action['sector'],
                       'position':[node['position'][0],node['position'][1],node['position'][2]+70]})
    interiors=[]
    for actor in actors:
        if actor['id'].startswith('city.huby.clue') or actor['id']=='city.borek.finish':
            entrance=actor['position'][:]
            center=[entrance[0],entrance[1],-20000]
            interiors.append({'id':'interior.'+actor['id'],'building':actor['building'],
                              'entrance':entrance,'center':center,'title':'Klatka i mieszkanie' if 'huby' in actor['id'] else 'Kryjówka na Borku'})
            actor['position']=[center[0]+220,center[1]+160,center[2]+280]
    loops=[]
    graph=data['road_graph']
    for sector in sorted({a['sector'] for a in gameplay['actions']}):
        anchor=nodes[next(a for a in gameplay['actions'] if a['sector']==sector)['node']]['position']
        for mode in ('car','foot'):
            links=adjacency(graph,mode)
            candidates=[key for key in nodes if owners[key]==sector and links[key] and math.dist(nodes[key]['position'],anchor)<30000]
            candidates.sort(key=lambda key:(math.dist(nodes[key]['position'],anchor),key))
            if not candidates:raise ValueError('No local population route: '+sector)
            start=candidates[0];loop=None
            ends=sorted(candidates,key=lambda key:abs(math.dist(nodes[key]['position'],nodes[start]['position'])-10000))[:30]
            for end in ends:
                if end==start:continue
                try:
                    outward=route(graph,start,end,mode);home=route(graph,end,start,mode)
                except ValueError:continue
                proposed=outward+home[1:]
                length=sum(math.dist(nodes[a]['position'],nodes[b]['position']) for a,b in zip(proposed,proposed[1:]))
                if 10000<=length<=100000 and all(owners[n]==sector for n in proposed):
                    loop=proposed;break
            if loop is None:raise ValueError('No directed return loop: '+sector+'/'+mode)
            loops.append({'id':sector+'.'+mode,'sector':sector,'vehicle':mode=='car',
                          'source_nodes':loop,'points':[nodes[n]['position'] for n in loop]})
    return {'schema_version':1,'activities':actors,'population_routes':loops,'interiors':interiors}
