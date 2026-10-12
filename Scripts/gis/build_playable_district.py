"""Bounded, reproducible first session from the existing Wrocław GIS sources.

No downloads, invented street connectivity or administrative-boundary claims.
Authored gameplay anchors are snapped to unobstructed sidewalks/road lanes.
"""
from __future__ import annotations
import argparse
import hashlib
import heapq
import json
import math
from pathlib import Path
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import unary_union, triangulate
from import_sector import ROOT, build, DRIVE
from build_meshes import generate as generate_meshes
from generate_facades import generate as generate_facades

CONFIG = ROOT/'Data/playable_district.json'


def read(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def route_between(graph, start, goal):
    queue, distances, previous = [(0,start)], {start:0}, {}
    while queue:
        cost,node = heapq.heappop(queue)
        if cost != distances.get(node):
            continue
        if node == goal:
            result = [node]
            while node != start:
                node = previous[node]
                result.append(node)
            return list(reversed(result))
        for target,length in graph.get(node,()):
            updated = cost+length
            if updated < distances.get(target,math.inf):
                distances[target],previous[target] = updated,node
                heapq.heappush(queue,(updated,target))
    raise ValueError('Authored locations are not connected by legal source roads')


def attach_car_anchor(graph, nodes, edges, position, elevation):
    eligible=[e for e in edges if e['bridge']=='no' and e['tunnel']=='no' and e['layer']=='0'
              and (e['car_forward'] or e['car_backward'])]
    point=Point(position[:2])
    edge=min(eligible,key=lambda e:LineString((nodes[e['from']][:2],nodes[e['to']][:2])).distance(point))
    a,b=edge['from'],edge['to'];line=LineString((nodes[a][:2],nodes[b][:2]))
    q=line.interpolate(line.project(point));key='district.car.anchor'
    nodes[key]=[q.x,q.y,elevation(q.x,q.y)];graph[key]=[]
    before,after=Point(nodes[a][:2]).distance(q),Point(nodes[b][:2]).distance(q)
    if edge['car_forward']:
        graph.setdefault(a,[]).append((key,before));graph[key].append((b,after))
    if edge['car_backward']:
        graph.setdefault(b,[]).append((key,after));graph[key].append((a,before))
    return key


def build_district(output):
    config = read(CONFIG)
    if config['schema_version'] != 1:
        raise ValueError('Unsupported district schema')
    data,geo = build(name=config['source_dataset'],output=output,
                     origin=config['origin_projected_m'],bbox_override=config['bbox'])
    buildings = unary_union([Polygon([p[:2] for p in f['rings'][0]],
                                      [[p[:2] for p in ring] for ring in f['rings'][1:]])
                             for f in data['features'] if f['kind']=='building'])
    water = unary_union([Polygon([p[:2] for p in f['rings'][0]],
                                  [[p[:2] for p in r] for r in f['rings'][1:]])
                         for f in data['features'] if f['kind']=='water'])
    obstacles = buildings.buffer(65).union(water)
    roads = [f for f in data['features'] if f['kind']=='road'
             and f['tags'].get('highway') in DRIVE
             and f['tags'].get('bridge','no')=='no'
             and f['tags'].get('tunnel','no')=='no'
             and f['tags'].get('access') not in ('private','no')]

    def elevation(x,y):
        lon,lat,_ = geo.geographic([x,y,0])
        return geo.world(lon,lat)[2]

    def snap(location, walking):
        authored = geo.world(*location)
        candidates = []
        for road in roads:
            for a,b in zip(road['points'],road['points'][1:]):
                line = LineString((a[:2],b[:2]))
                if line.length < 200:
                    continue
                q = line.interpolate(max(90,min(line.length-90,line.project(Point(authored[:2])))))
                dx,dy = b[0]-a[0],b[1]-a[1]
                length = math.hypot(dx,dy)
                offset = road['width_m']*50+90 if walking else min(140,road['width_m']*25)
                for side in (1,-1):
                    p = Point(q.x-dy/length*offset*side,q.y+dx/length*offset*side)
                    lon,lat,_=geo.geographic([p.x,p.y,0])
                    if obstacles.covers(p) or not (config['bbox'][0] <= lon <= config['bbox'][2]
                                                  and config['bbox'][1] <= lat <= config['bbox'][3]):
                        continue
                    candidates.append((p.distance(Point(authored[:2])),
                                       [round(p.x,2),round(p.y,2),round(elevation(p.x,p.y)+(22 if walking else 100),2)],
                                       math.degrees(math.atan2(dy,dx))))
        if not candidates:
            raise ValueError('No safe source-road anchor')
        return min(candidates,key=lambda item:item[0])[1:]

    anchors = {name:snap(ll,name!='vehicle')[0] for name,ll in config['anchors'].items()}
    yaw = snap(config['anchors']['vehicle'],False)[1]
    graph,nodes,degree = {},{n['id']:n['position'] for n in data['road_graph']['nodes']},{}
    for edge in data['road_graph']['edges']:
        if edge['bridge']!='no' or edge['tunnel']!='no' or edge['layer']!='0':
            continue
        for enabled,a,b in ((edge['car_forward'],edge['from'],edge['to']),
                            (edge['car_backward'],edge['to'],edge['from'])):
            if enabled:
                graph.setdefault(a,[]).append((b,edge['length_cm']))
                degree.setdefault(a,set()).add(b)
                degree.setdefault(b,set()).add(a)
    def nearest(p):
        return min(graph,key=lambda k:math.dist(nodes[k][:2],p[:2]))
    target_node=nearest(anchors['delivery'])
    car_node=attach_car_anchor(graph,nodes,data['road_graph']['edges'],anchors['vehicle'],elevation)
    outbound = route_between(graph,car_node,target_node)
    inbound = route_between(graph,target_node,car_node)
    loop = outbound+inbound[1:]

    def lane_path(ids):
        points=[]
        for index,node in enumerate(ids):
            p=nodes[node]
            prev=nodes[ids[max(0,index-1)]]
            next_point=nodes[ids[min(len(ids)-1,index+1)]]
            dx,dy=next_point[0]-prev[0],next_point[1]-prev[1]
            length=max(1,math.hypot(dx,dy))
            points.append([round(p[0]-dy/length*125,2),round(p[1]+dx/length*125,2),p[2]+8])
        return points
    first_drive=lane_path(outbound)
    goal=first_drive[0]
    dx,dy=goal[0]-anchors['vehicle'][0],goal[1]-anchors['vehicle'][1]
    if math.hypot(dx,dy)<150:
        dx,dy=first_drive[1][0]-goal[0],first_drive[1][1]-goal[1]
    yaw=math.degrees(math.atan2(dy,dx))
    traffic = lane_path(loop)
    if len(traffic)<4:
        raise ValueError('District needs a connected driving circuit')
    traffic[-1]=traffic[0]
    pedestrian_routes=[]
    for road in sorted(roads,key=lambda f:f['id']):
        if len(pedestrian_routes)>=16:
            break
        line=LineString([p[:2] for p in road['points']])
        if line.length<3500 or line.length>16000:
            continue
        offset=road['width_m']*50+90
        shifted=line.offset_curve(offset)
        if shifted.geom_type!='LineString' or shifted.intersects(obstacles):
            continue
        points=[[round(x,2),round(y,2),round(elevation(x,y)+20,2)] for x,y in shifted.coords]
        pedestrian_routes.append({'id':'walk.'+road['id'],'vehicle':False,'points':points+list(reversed(points[:-1]))})
    for feature in data['features']:
        if feature['kind']=='building':
            index=int(hashlib.sha256(feature['id'].encode()).hexdigest()[:8],16)
            feature['material']=('Plaster','Brick','Concrete')[index%3]
        elif feature['kind']=='road':
            feature['material']='Stone' if feature['tags'].get('surface') in ('paving_stones','cobblestone','sett') else (
                'Concrete' if feature['tags'].get('highway') in ('footway','path','pedestrian','steps') else 'Asphalt')
            if feature['tags'].get('bridge','no')!='no' and feature['tags'].get('tunnel','no')=='no':
                feature['surface_built']=True
    meshes=output/'Meshes'
    start_policy=read(ROOT/'Data/start_location.json')
    replacement_ids=[b['id'] for b in start_policy['buildings']]
    generate_meshes(meshes,data,geo,exclude_feature_ids=replacement_ids)
    # Keep the river genuinely open: no collision terrain immediately under water.
    for filename in read(meshes/'meshes.json'):
        record=read(meshes/filename)
        if record['kind']=='terrain':
            valid=[]
            for i in range(0,len(record['triangles']),3):
                ids=record['triangles'][i:i+3]
                x=sum(record['vertices'][j][0] for j in ids)/3+record['origin'][0]
                y=sum(record['vertices'][j][1] for j in ids)/3+record['origin'][1]
                if not water.covers(Point(x,y)):
                    valid.extend(ids)
            record['triangles']=valid
            (meshes/filename).write_text(json.dumps(record,separators=(',',':')),encoding='utf-8')
    # Sidewalk polygons respect building footprints rather than passing through walls.
    sidewalks=unary_union([LineString([p[:2] for p in road['points']]).buffer(road['width_m']*50+160,
                         cap_style=2,join_style=2).difference(LineString([p[:2] for p in road['points']]).buffer(road['width_m']*50,
                         cap_style=2,join_style=2)) for road in roads]).difference(buildings).difference(water)
    cells={}
    for polygon in getattr(sidewalks,'geoms',(sidewalks,)):
        if polygon.geom_type!='Polygon':
            continue
        for triangle in triangulate(polygon):
            if not polygon.covers(triangle):
                continue
            coords=list(triangle.exterior.coords)[:3]
            key=(math.floor(triangle.centroid.x/12800),math.floor(triangle.centroid.y/12800))
            record=cells.setdefault(key,{'kind':'sidewalk','material':'Concrete','origin':[key[0]*12800,key[1]*12800,0],
                                        'vertices':[],'triangles':[]})
            base=len(record['vertices'])
            record['vertices'] += [[x-record['origin'][0],y-record['origin'][1],elevation(x,y)+20] for x,y in coords]
            record['triangles'] += [base,base+1,base+2]
    filenames=read(meshes/'meshes.json')
    for key,record in sorted(cells.items()):
        name=f'sidewalk_{key[0]}_{key[1]}.json'
        (meshes/name).write_text(json.dumps(record,separators=(',',':')),encoding='utf-8')
        filenames.append(name)
    (meshes/'meshes.json').write_text(json.dumps(filenames),encoding='utf-8')
    facades=generate_facades(read(ROOT/'Data/facade_profiles.json'),data,bindings=read(ROOT/'Data/facade_asset_bindings.json'))
    (output/'facades.json').write_text(json.dumps(facades,ensure_ascii=False),encoding='utf-8')
    corners=[geo.world(config['bbox'][i],config['bbox'][j]) for i in (0,2) for j in (1,3)]
    plan={'schema_version':1,'id':config['id'],'title':config['title'],'map':config['map'],'anchors':anchors,
          'vehicle_yaw':yaw,'delivery_route':lane_path(outbound),
          'bounds_min':[min(p[i] for p in corners) for i in range(3)],
          'bounds_max':[max(p[i] for p in corners) for i in range(3)],
          'intersections':[nodes[k] for k,neighbors in degree.items() if len(neighbors)>=3],
          'population_routes':pedestrian_routes+[{'id':'traffic.delivery','vehicle':True,'points':traffic}],
          'streets':[{'name':f['name'],'points':f['points'],'walkway':f['tags'].get('highway') in ('footway','path','pedestrian')}
                     for f in data['features'] if f['kind']=='road' and f['name']],
          'mission':config['mission'],'budgets':config['budgets']}
    dressing=[]
    for route in pedestrian_routes:
        points=route['points'];middle=len(points)//4
        p=points[middle];q=points[max(0,middle-1)]
        dx,dy=p[0]-q[0],p[1]-q[1];length=math.hypot(dx,dy)
        if length<1:continue
        yaw=math.degrees(math.atan2(dy,dx))
        for name,offset,radius in (('street_lamp_01',110,35),('metal_trash_can',160,55),('modular_street_seating',220,150)):
            x,y=p[0]-dy/length*offset,p[1]+dx/length*offset
            candidate=Point(x,y).buffer(radius)
            if not obstacles.intersects(candidate) and not candidate.intersects(LineString([a[:2] for a in points]).buffer(45)):
                dressing.append({'model':name,'position':[x,y,elevation(x,y)+20],'yaw':yaw,'radius':radius})
    plan['dressing']=dressing
    plan['start_location_replacement_ids']=replacement_ids
    (output/'district.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2),encoding='utf-8')
    triangles=sum(len(read(meshes/n)['triangles'])//3 for n in filenames)
    if triangles>config['budgets']['mesh_triangles']:
        raise ValueError(f'Mesh budget exceeded: {triangles}')
    report={'features':len(data['features']),'mesh_cells':len(filenames),'triangles':triangles,
            'drive_route_m':round(sum(math.dist(nodes[a],nodes[b]) for a,b in zip(outbound,outbound[1:]))/100,1),
            'pedestrian_routes':len(pedestrian_routes),'status':'SourceValidated','source_dataset':config['source_dataset'],
            'anchors':anchors,'source_sha256':data['sources'][0]['SHA256']}
    (output/'source-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=ROOT/'Saved/PlayableDistrict')
    args=parser.parse_args()
    print(json.dumps(build_district(args.output),ensure_ascii=True))
