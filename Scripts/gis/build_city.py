"""Build Wave 1 into the existing GIS world. Offline and deterministic."""
import argparse
import hashlib
import json
import math
from collections import deque
from pathlib import Path
from shapely.geometry import Polygon, box
from import_sector import ROOT, SOURCE, build
from road_routes import adjacency
from city_coverage import evaluate, fingerprint

DEFAULT_OUTPUT = ROOT/'Saved/CityData'


def validate_catalog(catalog):
    if catalog.get('schema_version') != 1:
        raise ValueError('Unsupported city schema')
    districts = {d['id'] for d in catalog['districts']}
    if len(districts) != len(catalog['districts']): raise ValueError('Duplicate district ID')
    ids = set(); polygons = []
    for sector in catalog['sectors']:
        sid = sector['id']; bbox = sector['bbox']
        if not sid or sid in ids: raise ValueError('Duplicate or empty sector ID')
        ids.add(sid)
        if sector['district'] not in districts or sector['profile'] not in catalog['profiles']:
            raise ValueError('Unknown district or architecture profile')
        if len(bbox) != 4 or not all(math.isfinite(v) for v in bbox) or not (-180 <= bbox[0] < bbox[2] <= 180 and -90 <= bbox[1] < bbox[3] <= 90):
            raise ValueError('Invalid sector bounds')
        polygon = box(*bbox)
        if any(polygon.intersection(other).area > 1e-12 for other in polygons):
            raise ValueError('Production sectors overlap')
        polygons.append(polygon)
    if not ids: raise ValueError('Empty city')
    if not set(catalog.get('content_targets', {})) <= districts:
        raise ValueError('Unknown content district')
    for profile in catalog['profiles'].values():
        if profile['density'] not in ('VeryHigh', 'High', 'Medium', 'Low') or not profile['materials']:
            raise ValueError('Invalid architecture profile')


def owner(catalog, lon, lat):
    # Boundaries have deterministic ownership independent of array order.
    matches = [s['id'] for s in catalog['sectors'] if s['bbox'][0]-1e-10 <= lon <= s['bbox'][2]+1e-10 and s['bbox'][1]-1e-10 <= lat <= s['bbox'][3]+1e-10]
    if not matches: raise ValueError(f'GIS coordinate outside production sectors: {lon},{lat}')
    return min(matches)


def reachable(links, seed, reverse=False):
    if reverse:
        rev = {node: [] for node in links}
        for a, edges in links.items():
            for b, *_ in edges: rev[b].append((a,))
        links = rev
    seen = {seed}; queue = deque([seed])
    while queue:
        for target, *_ in links[queue.popleft()]:
            if target not in seen: seen.add(target); queue.append(target)
    return seen


def connectivity(data, catalog, owners):
    graph = data['road_graph']; nodes = {n['id']: n for n in graph['nodes']}
    links = {mode: adjacency(graph, mode) for mode in ('car', 'foot')}
    # Preserve the actual laboratory street as the connection point.
    seeds = [e['from'] for e in graph['edges'] if e['name'] == 'Ludwika Rydygiera' and links['car'][e['from']] and links['foot'][e['from']]]
    if not seeds: raise ValueError('Existing Nadodrze connection is absent')
    seed = min(seeds)
    connected = {mode: reachable(table, seed) & reachable(table, seed, True) for mode, table in links.items()}
    result = {}; anchors = {}
    for sector in catalog['sectors']:
        sid = sector['id']; w, s, e, n = sector['bbox']
        result[sid] = {}; anchors[sid] = {}
        for mode in links:
            candidates = [node for node in nodes if owners[node] == sid and node in connected[mode]]
            if candidates:
                target = min(candidates, key=lambda key: ((nodes[key]['geographic'][0]-(w+e)/2)**2 + (nodes[key]['geographic'][1]-(s+n)/2)**2, key))
                anchors[sid][mode] = target
            result[sid][mode] = bool(candidates)
    return result, anchors, seed


def generate(output=DEFAULT_OUTPUT, catalog_path=ROOT/'Data/city.json', evidence_path=None):
    catalog = json.loads(catalog_path.read_text(encoding='utf-8')); validate_catalog(catalog)
    data, geo = build(output=output, name=catalog['dataset'], origin=catalog['origin_projected_m'])
    sectors = {s['id']: s for s in catalog['sectors']}
    owners = {n['id']: owner(catalog, *n['geographic']) for n in data['road_graph']['nodes']}
    stats = {sid: {'buildings': 0, 'roads': 0, 'foot_edges': 0, 'rail_lines': 0, 'courtyards': 0} for sid in sectors}
    streets = {}; addresses = []; courtyards = []
    for feature in data['features']:
        coords = feature['rings'][0] if 'rings' in feature else feature['points']
        representative = coords[len(coords)//2]
        if 'rings' in feature:
            shape = Polygon([p[:2] for p in coords], [[p[:2] for p in r] for r in feature['rings'][1:]])
            point = shape.representative_point(); representative = [point.x, point.y, coords[0][2]]
        lon, lat, _ = geo.geographic(representative)
        sid = owner(catalog, lon, lat); feature['sector'] = sid
        profile = catalog['profiles'][sectors[sid]['profile']]
        feature['architecture_profile'] = sectors[sid]['profile']
        feature['fidelity'] = 'Background'  # GIS geometry has not passed manual facade review.
        tags = feature['tags']; kind = feature['kind']
        if kind == 'building':
            stats[sid]['buildings'] += 1
            palette = profile['materials']
            feature['material'] = palette[int(hashlib.sha256(feature['id'].encode()).hexdigest()[:8], 16) % len(palette)]
            for index, ring in enumerate(feature['rings'][1:]):
                courtyards.append({'id': feature['id']+f'/courtyard/{index}', 'building_id': feature['id'],
                                  'sector': sid, 'ring': ring, 'accessible': False})
                stats[sid]['courtyards'] += 1
            if tags.get('addr:housenumber') and tags.get('addr:street'):
                # Address street labels may precede a matching OSM way in this extract.
                street_id = 'street/'+hashlib.sha256(tags['addr:street'].encode()).hexdigest()[:20]
                streets.setdefault(street_id, {'id': street_id, 'name': tags['addr:street'], 'ways': []})
                addresses.append({'id': 'address/'+feature['id'], 'street_id': street_id,
                                  'building_number': tags['addr:housenumber'], 'building_id': feature['id'],
                                  'geographic': [lon,lat], 'sector': sid})
        if kind == 'road':
            stats[sid]['roads'] += 1
            if feature['name']:
                street_id = 'street/'+hashlib.sha256(feature['name'].encode()).hexdigest()[:20]
                record = streets.setdefault(street_id, {'id': street_id, 'name': feature['name'], 'ways': []})
                record['ways'].append(feature['id'])
        if kind == 'rail': stats[sid]['rail_lines'] += 1
    for edge in data['road_graph']['edges']:
        if edge['foot']: stats[owners[edge['from']]]['foot_edges'] += 1
    connections, anchors, seed = connectivity(data, catalog, owners)
    pipeline = hashlib.sha256()
    paths = list((ROOT/'Scripts/gis').glob('*.py')) + list((ROOT/'Source').rglob('*.cpp')) + list((ROOT/'Source').rglob('*.h'))
    paths += list((ROOT/'Config').glob('*.ini')) + [ROOT/'Scripts/prepare_geography.py', ROOT/'WroclawTheGame.uproject']
    for path in sorted(paths):
        pipeline.update(path.relative_to(ROOT).as_posix().encode()); pipeline.update(path.read_bytes())
    digest = fingerprint(catalog, [*data['sources'], {'pipeline_sha256': pipeline.hexdigest()}])
    evidence = json.loads(evidence_path.read_text()) if evidence_path else None
    report = evaluate(catalog, stats, connections, digest, evidence)
    runtime = {'schema_version': 1, 'world_package': catalog['world_package'], 'fingerprint': digest, 'sectors': []}
    for sector, assessed in zip(catalog['sectors'], report['sectors']):
        w,s,e,n = sector['bbox']; corners = [geo.world(lon,lat,115) for lon,lat in ((w,s),(e,s),(e,n),(w,n))]
        runtime['sectors'].append({**sector, 'name': assessed['name'], 'status': assessed['status'],
                                   'blockers': assessed['blockers'], 'counts': stats[sector['id']],
                                   'density': catalog['profiles'][sector['profile']]['density'],
                                   'boundary': [p[:2] for p in corners],
                                   'min': [min(p[i] for p in corners) for i in range(2)],
                                   'max': [max(p[i] for p in corners) for i in range(2)]})
    products = {'sector.json': data, 'city.json': runtime, 'coverage.json': report,
                'routing.json': {'owners': owners, 'anchors': anchors, 'origin_node': seed, 'connectivity': connections},
                'streets.json': sorted(streets.values(), key=lambda s:s['id']), 'addresses.json': addresses,
                'courtyards.json': courtyards}
    for name, product in products.items():
        (output/name).write_text(json.dumps(product, ensure_ascii=False, separators=(',',':'))+'\n', encoding='utf-8')
    return data, geo, report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--evidence', type=Path)
    parser.add_argument('--meshes', action='store_true')
    parser.add_argument('--require-playable', action='store_true')
    args = parser.parse_args(); data, geo, report = generate(args.output, evidence_path=args.evidence)
    print(json.dumps({k:v for k,v in report.items() if k != 'sectors'}, indent=2))
    if args.require_playable and any(s['wave'] == 1 and s['status'] not in ('Playable','Detailed','Final') for s in report['sectors']):
        raise SystemExit('Wave 1 is not playable: inspect coverage.json blockers')
    if args.meshes:
        from build_meshes import generate as meshes
        print('City mesh cells:', meshes(args.output/'Meshes', data, geo))
