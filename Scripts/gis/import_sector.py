"""Reproducible OSM -> projected UE sector. No network requests during a build.

UE coordinates: X east, Y south, Z up, centimetres. Heights remain EGM96
orthometric; they must not be described as WGS84 ellipsoidal heights.
"""
import argparse
import csv
import hashlib
import gzip
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET
from pyproj import Transformer
from shapely.geometry import LineString, Polygon, box, mapping
from shapely.ops import polygonize, unary_union
from shapely.validation import make_valid

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'Data/source/wroclaw'
OUTPUT = ROOT / 'Data/processed/wroclaw'
DRIVE = {'motorway', 'trunk', 'primary', 'secondary', 'tertiary', 'unclassified',
         'residential', 'living_street', 'service', 'motorway_link', 'trunk_link',
         'primary_link', 'secondary_link', 'tertiary_link'}
WALK_EXCLUDED = {'motorway', 'motorway_link', 'trunk', 'trunk_link', 'construction', 'proposed'}


def metadata(path):
    info = json.loads(path.with_name(path.name.split('.')[0]+'.metadata.json').read_text())
    for key in ('SourceName', 'SourceURL', 'SourceDate', 'SourceLicense', 'CRS',
                'ImportDate', 'ProcessingVersion', 'SHA256'):
        if not info.get(key):
            raise ValueError(f'Missing provenance: {key}')
    if hashlib.sha256(path.read_bytes()).hexdigest() != info['SHA256']:
        raise ValueError(f'Source checksum mismatch: {path.name}')
    return info


class Geography:
    def __init__(self, bbox, terrain):
        self.forward = Transformer.from_crs(4326, 32633, always_xy=True)
        self.inverse = Transformer.from_crs(32633, 4326, always_xy=True)
        e, n = self.forward.transform(bbox[0], bbox[3])
        self.origin = (round(e), round(n), 115.0)
        with terrain.open() as stream:
            rows = list(csv.DictReader(stream))
        self.elevations = {(float(r['longitude']), float(r['latitude'])): float(r['elevation']) for r in rows}
        self.lons = sorted({p[0] for p in self.elevations})
        self.lats = sorted({p[1] for p in self.elevations})

    def height(self, lon, lat):
        import bisect
        i = max(1, min(len(self.lons)-1, bisect.bisect_left(self.lons, lon)))
        j = max(1, min(len(self.lats)-1, bisect.bisect_left(self.lats, lat)))
        x0, x1 = self.lons[i-1:i+1]; y0, y1 = self.lats[j-1:j+1]
        u = max(0, min(1, (lon-x0)/(x1-x0))); v = max(0, min(1, (lat-y0)/(y1-y0)))
        z = lambda x,y: self.elevations[x,y]
        return (1-v)*((1-u)*z(x0,y0)+u*z(x1,y0))+v*((1-u)*z(x0,y1)+u*z(x1,y1))

    def world(self, lon, lat, height=None):
        e,n = self.forward.transform(lon,lat)
        z = self.height(lon,lat) if height is None else height
        return [round((e-self.origin[0])*100,3), round((self.origin[1]-n)*100,3), round((z-self.origin[2])*100,3)]

    def geographic(self, world):
        lon,lat = self.inverse.transform(self.origin[0]+world[0]/100,self.origin[1]-world[1]/100)
        return [lon,lat,self.origin[2]+world[2]/100]


def parts(geometry, kind):
    if geometry.is_empty:
        return
    if geometry.geom_type == kind:
        yield geometry
    elif hasattr(geometry, 'geoms'):
        for child in geometry.geoms:
            yield from parts(child, kind)


def build(source=SOURCE, output=OUTPUT):
    osm = source/'osm/nadodrze.osm.gz'; terrain = source/'terrain/nadodrze.csv'
    sources = [metadata(osm), metadata(terrain)]
    bbox = sources[0]['BoundingBox']; clip = box(*bbox)
    geo = Geography(bbox, terrain)
    with gzip.open(osm, 'rb') as stream:
        xml=stream.read()
    if hashlib.sha256(xml).hexdigest()!=sources[0]['UncompressedSHA256']:
        raise ValueError('Uncompressed OSM checksum mismatch')
    root = ET.fromstring(xml)
    nodes = {n.attrib['id']: (float(n.attrib['lon']),float(n.attrib['lat'])) for n in root.findall('node')}
    ways = {w.attrib['id']: ([n.attrib['ref'] for n in w.findall('nd')],
                            {t.attrib['k']:t.attrib['v'] for t in w.findall('tag')}) for w in root.findall('way')}
    features=[]; graph_nodes={}; edges=[]; unsupported=[]; building_members=set()
    def polygon_feature(identifier, geometry, tags):
        if tags.get('building') not in (None,'no'): kind='building'
        elif tags.get('natural')=='water' or tags.get('waterway')=='riverbank': kind='water'
        elif tags.get('leisure') in ('park','garden','playground') or tags.get('landuse') in ('grass','forest','recreation_ground'): kind='green'
        else:return
        for index, polygon in enumerate(parts(make_valid(geometry).intersection(clip), 'Polygon')):
            if polygon.area < 1e-12:continue
            def ring(r):return [geo.world(*p) for p in r.coords]
            height=12.0; estimated=True
            try:
                if 'height' in tags:height=float(tags['height'].replace(' m',''));estimated=False
                elif 'building:levels' in tags:height=float(tags['building:levels'])*3.2
            except ValueError:pass
            height=max(2,min(150,height))
            features.append({'id':f'{identifier}:{index}','kind':kind,'name':tags.get('name',''),
                             'rings':[ring(polygon.exterior)]+[ring(r) for r in polygon.interiors],
                             'height_m':height,'height_estimated':estimated,'tags':tags})
    # Assemble relation outers and holes before way processing; do not fill courtyards.
    for relation in root.findall('relation'):
        tags={t.attrib['k']:t.attrib['v'] for t in relation.findall('tag')}
        if tags.get('type')!='multipolygon':continue
        lines={'outer':[],'inner':[]}; members=[]
        for m in relation.findall('member'):
            if m.attrib['type']!='way' or m.attrib['ref'] not in ways:continue
            refs,_=ways[m.attrib['ref']]; coords=[nodes[r] for r in refs if r in nodes]
            if len(coords)>1:lines['inner' if m.attrib.get('role')=='inner' else 'outer'].append(LineString(coords))
            members.append(m.attrib['ref'])
        outer=unary_union(list(polygonize(lines['outer'])))
        inner=unary_union(list(polygonize(lines['inner'])))
        polygon_feature('relation/'+relation.attrib['id'],outer.difference(inner),tags)
        if tags.get('building') not in (None,'no'):building_members.update(members)
    for identifier,(refs,tags) in ways.items():
        coords=[nodes[r] for r in refs if r in nodes]
        if len(coords)<2:continue
        if coords[0]==coords[-1] and len(coords)>3 and identifier not in building_members:
            polygon_feature('way/'+identifier,Polygon(coords),tags)
        highway=tags.get('highway'); rail=tags.get('railway')
        if not highway and not rail:continue
        full=LineString(coords)
        for i,line in enumerate(parts(full.intersection(clip),'LineString')):
            features.append({'id':f'way/{identifier}:line:{i}','kind':'road' if highway else 'rail',
                             'name':tags.get('name',''),'points':[geo.world(*p) for p in line.coords],'tags':tags,
                             'width_m':{'primary':9,'secondary':8,'tertiary':7,'residential':6,'service':3.5,'footway':2,'path':1.5}.get(highway,3)})
        if not highway:continue
        access=tags.get('access') not in ('no','private')
        car=highway in DRIVE and tags.get('motor_vehicle',tags.get('vehicle','yes')) not in ('no','private') and access
        foot=highway not in WALK_EXCLUDED and tags.get('foot','yes') not in ('no','private') and access
        if not(car or foot):continue
        # Only source node IDs connect intersections. Geometric crossings are never welded.
        for a,b in zip(refs,refs[1:]):
            if a not in nodes or b not in nodes:continue
            segment=LineString([nodes[a],nodes[b]])
            for j,line in enumerate(parts(segment.intersection(clip),'LineString')):
                p,q=list(line.coords)[0],list(line.coords)[-1]
                if p==q:continue
                na=a if p==nodes[a] else f'boundary:{identifier}:{a}:{b}:{j}:a'
                nb=b if q==nodes[b] else f'boundary:{identifier}:{a}:{b}:{j}:b'
                for key,coord in ((na,p),(nb,q)):
                    graph_nodes[key]={'id':key,'position':geo.world(*coord),'geographic':list(coord),'boundary':key.startswith('boundary:')}
                oneway=tags.get('oneway','yes' if tags.get('junction')=='roundabout' else 'no')
                edges.append({'id':f'{identifier}:{a}:{b}:{j}','from':na,'to':nb,'way':identifier,
                              'name':tags.get('name',''),'car_forward':car and oneway!='-1',
                              'car_backward':car and oneway not in ('yes','1','true'),
                              'foot':foot,'layer':tags.get('layer','0'),'bridge':tags.get('bridge','no'),
                              'length_cm':math.dist(geo.world(*p),geo.world(*q))})
                if tags.get('bridge','no')!='no' or tags.get('tunnel','no')!='no':unsupported.append(identifier)
    result={'version':1,'bbox':bbox,'projected_crs':'EPSG:32633','geographic_crs':'EPSG:4326',
            'vertical_crs':'EGM96 orthometric metres','origin_projected_m':geo.origin,'units_per_metre':100,
            'axes':'X=east,Y=south,Z=up','sources':sources,'features':features,
            'road_graph':{'nodes':list(graph_nodes.values()),'edges':edges},
            'requires_height_review':sorted(set(unsupported))}
    validate(result)
    output.mkdir(parents=True,exist_ok=True)
    (output/'sector.json').write_text(json.dumps(result,ensure_ascii=False,separators=(',',':'))+'\n',encoding='utf-8')
    manifest={k:v for k,v in result.items() if k not in ('features','road_graph')}
    manifest['counts']={'features':len(features),'nodes':len(graph_nodes),'edges':len(edges)}
    (output/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    return result,geo


def validate(data):
    graph=data['road_graph'];nodes={n['id']:n for n in graph['nodes']}
    if len(nodes)!=len(graph['nodes']):raise ValueError('Duplicate road node')
    for n in nodes.values():
        if not all(math.isfinite(v) for v in n['position']):raise ValueError('Nonfinite coordinates')
    for e in graph['edges']:
        if e['from'] not in nodes or e['to'] not in nodes:raise ValueError('Missing road node')
        if e['from']==e['to'] or not math.isfinite(e['length_cm']) or e['length_cm']<=0:raise ValueError('Invalid road edge')
    if not any(f['kind']=='building' for f in data['features']):raise ValueError('No real buildings')
    if not any(e['car_forward'] for e in graph['edges']):raise ValueError('No driveable road')


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=OUTPUT)
    args=parser.parse_args();data,_=build(output=args.output)
    print(f"Imported {len(data['features'])} features, {len(data['road_graph']['nodes'])} road nodes")
