"""Source-placed street production. Traffic controls require explicit OSM tags."""
import gzip
import hashlib
import json
import math
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import unary_union

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Scripts/gis'))
from import_sector import Geography,SOURCE,source_bytes,DRIVE,metadata


def build():
    policy_path=ROOT/'Data/start_location.json';policy=json.loads(policy_path.read_text(encoding='utf-8'))
    district_path=ROOT/'Saved/PlayableDistrict/district.json';district=json.loads(district_path.read_text(encoding='utf-8'))
    sector_path=ROOT/'Saved/PlayableDistrict/sector.json';sector=json.loads(sector_path.read_text(encoding='utf-8'))
    if district['anchors']['player']!=policy['player_anchor_cm']:raise ValueError('Start moved without policy review')
    config=json.loads((ROOT/'Data/playable_district.json').read_text(encoding='utf-8'))
    geometry_path=ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json'
    geometry=json.loads(geometry_path.read_text(encoding='utf-8'))
    drain_height=next(asset['bounds_m'][2]*100 for asset in geometry['assets'] if asset['asset']=='drain')
    geo=Geography(config['bbox'],SOURCE/'terrain'/f"{config['source_dataset']}.csv",config['origin_projected_m'])
    osm_path=SOURCE/'osm'/f"{config['source_dataset']}.osm.gz";provenance=metadata(osm_path)
    xml=gzip.decompress(source_bytes(osm_path))
    if hashlib.sha256(xml).hexdigest()!=provenance['UncompressedSHA256']:raise ValueError('OSM source changed')
    center=Point(policy['player_anchor_cm'][:2]);region=center.buffer(policy['radius_m']*100)
    roads=[f for f in sector['features'] if f['kind']=='road' and f['tags'].get('highway') in DRIVE and LineString([p[:2] for p in f['points']]).intersects(region)]
    road_shapes=[LineString([p[:2] for p in f['points']]).buffer(f['width_m']*50,cap_style=2,join_style=2) for f in roads]
    paved=unary_union(road_shapes)
    buildings=unary_union([Polygon([p[:2] for p in f['rings'][0]]) for f in sector['features'] if f['kind']=='building'])
    greens=unary_union([Polygon([p[:2] for p in f['rings'][0]]) for f in sector['features'] if f['kind']=='green'])
    paths=[f for f in sector['features'] if f['kind']=='road' and f['tags'].get('highway') in ['footway','path','cycleway'] and LineString([p[:2] for p in f['points']]).intersects(region)]
    path_shapes=unary_union([LineString([p[:2] for p in f['points']]).buffer(f['width_m']*50+50) for f in paths])
    vehicle_paths=unary_union([LineString([p[:2] for p in district['delivery_route']]).buffer(100)]+
        [LineString([p[:2] for p in r['points']]).buffer(100) for r in district['population_routes'] if r['vehicle']])
    tram_clearance=unary_union([LineString([p[:2] for p in f['points']]).buffer(180) for f in sector['features'] if f['kind']=='rail'])
    nodes=[]
    for node in ET.fromstring(xml).findall('node'):
        tags={t.attrib['k']:t.attrib['v'] for t in node.findall('tag')}
        if tags.get('highway') not in ['crossing','traffic_signals','bus_stop'] and tags.get('natural')!='tree' and tags.get('amenity') not in ['bench','bicycle_parking','waste_basket']:continue
        position=geo.world(float(node.attrib['lon']),float(node.attrib['lat']))
        if region.covers(Point(position[:2])):nodes.append(dict(id='node/'+node.attrib['id'],tags=tags,position_cm=position))
    items=[];occupied=[];counts=Counter()
    def ground(x,y):
        lon,lat,_=geo.geographic([x,y,0]);return geo.world(lon,lat)[2]
    def add(role,p,yaw=0,scale=(1,1,1),source='source-road-boundary',radius=0):
        point=Point(p[:2])
        if point.distance(center)<policy['street_rules']['keep_player_clear_m']*100:return
        if any(point.distance(Point(district['anchors'][key][:2]))<300 for key in ['meeting','vehicle','delivery']):return
        if radius and (buildings.buffer(radius).covers(point) or any(point.distance(q)<radius+r for q,r in occupied)):return
        if radius:occupied.append((point,radius))
        items.append(dict(role=role,position_cm=[round(v,3) for v in p],yaw_deg=round(yaw,3),scale=list(scale),source=source));counts[role]+=1
    crossings=[Point(n['position_cm'][:2]) for n in nodes if n['tags'].get('highway')=='crossing']
    curb_building_clearance=buildings.buffer(30)
    for line in getattr(paved.boundary.intersection(region),'geoms',[paved.boundary.intersection(region)]):
        if line.geom_type!='LineString':continue
        for step in range(math.ceil(line.length/200)):
            span=min(200,line.length-step*200)
            distance=step*200+span*.5;q=line.interpolate(distance);a=line.interpolate(max(0,distance-25));b=line.interpolate(min(line.length,distance+25))
            if curb_building_clearance.covers(q) or q.distance(region.boundary)<80:continue
            lowered=any(q.distance(c)<220 for c in crossings)
            add('curb',[q.x,q.y,ground(q.x,q.y)-(12 if lowered else 1)],math.degrees(math.atan2(b.y-a.y,b.x-a.x)),(span/200,1,1))
            if step%12==6 and not lowered:
                dx,dy=b.x-a.x,b.y-a.y;norm=max(1,math.hypot(dx,dy))
                nx,ny=-dy/norm,dx/norm
                side=1 if paved.covers(Point(q.x+nx*30,q.y+ny*30)) else -1
                x,y=q.x+nx*30*side,q.y+ny*30*side
                add('drain',[x,y,ground(x,y)+8-drain_height+.2],math.degrees(math.atan2(dy,dx)))
    def nearest_road(p):return min(roads,key=lambda f:LineString([v[:2] for v in f['points']]).distance(Point(p[:2])))
    def tangent(f,p):
        line=LineString([v[:2] for v in f['points']]);d=line.project(Point(p[:2]));a=line.interpolate(max(0,d-50));b=line.interpolate(min(line.length,d+50))
        return math.degrees(math.atan2(b.y-a.y,b.x-a.x))
    def add_tree(p,source,radius=60):
        point=Point(p[:2])
        if paved.covers(point) or tram_clearance.covers(point) or buildings.buffer(80).covers(point):return
        seed=int(hashlib.sha256(source.encode()).hexdigest()[:8],16)
        green=greens.covers(point) and not path_shapes.covers(point)
        role=['young-tree','young-maple','park-lime'][seed%3 if green else seed%2]
        scale=.93+(seed%15)/100
        pos=[p[0],p[1],ground(*p[:2])+(4 if green else 20)]
        before=len(items);add(role,pos,seed%360,(scale,scale,scale),source,radius)
        if len(items)>before and not green:add('tree-pit',[pos[0],pos[1],pos[2]+.2],source=source+':soil-pit')
    for node in nodes:
        tags=node['tags'];p=node['position_cm'];yaw=tangent(nearest_road(p),p)
        role={'bench':'seating','bicycle_parking':'bike-rack','waste_basket':'bin'}.get(tags.get('amenity'))
        if role:add(role,[p[0],p[1],p[2]+20],yaw,source=node['id'],radius={'seating':110,'bike-rack':60,'bin':40}[role])
        if tags.get('natural')=='tree':add_tree(p,node['id'])
        marking=tags.get('crossing:markings',tags.get('footway:crossing:markings'))
        if marking=='zebra':
            road=nearest_road(p)
            if LineString([v[:2] for v in road['points']]).distance(Point(p[:2]))>180:continue
            # Zebra stripes run parallel to traffic, span the mapped road width.
            for i in range(max(2,int(road['width_m']))):
                angle=math.radians(yaw+90);offset=(i-(int(road['width_m'])-1)/2)*100
                add('paint',[p[0]+math.cos(angle)*offset,p[1]+math.sin(angle)*offset,p[2]+8.04],yaw,(3,.5,.0004),node['id'])
    # Supplement vegetation only inside mapped green land, clear of foot/cycle paths.
    for path in sorted(paths,key=lambda f:f['id']):
        if sum(counts[k] for k in ['young-tree','young-maple','park-lime'])>=18:break
        line=LineString([p[:2] for p in path['points']])
        for d in range(900,int(line.length),1800):
            q=line.interpolate(d);a=line.interpolate(max(0,d-50));b=line.interpolate(min(line.length,d+50));dx,dy=b.x-a.x,b.y-a.y;length=max(1,math.hypot(dx,dy))
            for side in (-1,1):
                x,y=q.x-dy/length*350*side,q.y+dx/length*350*side;point=Point(x,y)
                if region.covers(point) and greens.covers(point) and not path_shapes.covers(point) and sum(counts[k] for k in ['young-tree','young-maple','park-lime'])<18:
                    add_tree([x,y,ground(x,y)],path['id']+f':green-strip-estimate:{d}:{side}',radius=120)
    for road in sorted(roads,key=lambda f:f['id']):
        line=LineString([p[:2] for p in road['points']])
        if road['tags'].get('lit')=='yes':
            for d in range(1200,int(line.length),2800):
                q=line.interpolate(d);yaw=tangent(road,[q.x,q.y]);angle=math.radians(yaw+90);offset=road['width_m']*50+75
                x,y=q.x+math.cos(angle)*offset,q.y+math.sin(angle)*offset
                if region.covers(Point(x,y)):add('lamp',[x,y,ground(x,y)+20],yaw,source=road['id']+':lighting-estimate',radius=45)
        if road['tags'].get('highway')=='residential' and line.length>2200:
            for d in range(1100,int(line.length)-500,1600):
                if counts['parked-car']>=6:break
                q=line.interpolate(d);yaw=tangent(road,[q.x,q.y]);angle=math.radians(yaw+90);offset=-(road['width_m']*50-100)
                x,y=q.x+math.cos(angle)*offset,q.y+math.sin(angle)*offset
                c,s=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
                footprint=Polygon([(x+u*c-v*s,y+u*s+v*c) for u,v in [(-225,-105),(225,-105),(225,105),(-225,105)]])
                if region.covers(Point(x,y)) and not footprint.intersects(vehicle_paths) and not any(Point(x,y).distance(cross)<800 for cross in crossings):add('parked-car',[x,y,ground(x,y)+8],yaw,source=road['id']+':curbside-parking-estimate',radius=250)
    # The production profile has a real head/web/flange. Gauge is measured
    # between inner rail-head faces, not between the two profile centres.
    strips=[]
    for f in sector['features']:
        if f['kind']!='rail' or f['tags'].get('railway')!='tram':continue
        line=LineString([p[:2] for p in f['points']]).intersection(region)
        lines=list(getattr(line,'geoms',[line]))
        for segment in lines:
          if segment.geom_type!='LineString':continue
          for a,b in zip(segment.coords,list(segment.coords)[1:]):
            dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
            if length<1:continue
            try:gauge=float(f['tags'].get('gauge','1435').split(';')[0])/10
            except ValueError:gauge=143.5
            steps=math.ceil(length/200)
            for side in (-1,1):
                offset=(gauge+6.8)*.5*side
                for step in range(steps):
                    t=(step+.5)/steps;x=a[0]+dx*t-dy/length*offset;y=a[1]+dy*t+dx/length*offset
                    ax=a[0]+dx*step/steps-dy/length*offset;ay=a[1]+dy*step/steps+dx/length*offset
                    bx=ax+dx/steps;by=ay+dy/steps;za=ground(ax,ay);zb=ground(bx,by)
                    # 18 cm profile is embedded below paving; head is 3 mm proud.
                    items.append(dict(role='embedded-tram-rail',position_cm=[x,y,(za+zb)*.5+8.3-18],
                        yaw_deg=math.degrees(math.atan2(dy,dx)),pitch_deg=math.degrees(math.atan2(zb-za,length/steps)),
                        scale=[math.hypot(length/steps,zb-za)/100,1,1],source=f['id'],gauge_cm=gauge))
                    counts['embedded-tram-rail']+=1
    # Continuous panel order: a-left, a-right, b-right, b-left.
    output=dict(schema_version=1,map=policy['map'],items=items,counts=dict(counts),rail_strips=strips,
        source_nodes=nodes,source_files={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in [policy_path,district_path,sector_path,ROOT/'Data/start_location_buildings.json',geometry_path]},
        placement_accuracy='OSM nodes exact; curb follows source widths; supplemental vegetation/lighting/parking explicitly estimated; no invented traffic signals')
    if len(items)>policy['quality']['max_instances']:raise ValueError('Start instance budget exceeded')
    folder=ROOT/'Saved/StartLocation';folder.mkdir(parents=True,exist_ok=True)
    (folder/'placement.json').write_text(json.dumps(output,indent=2),encoding='utf-8')
    print('START_PLAN_PASS '+json.dumps(dict(counts=counts,rail_strips=len(strips))))


if __name__=='__main__':build()
