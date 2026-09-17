"""Authored prototype bridge surfaces. Geometry and graph receive identical elevations."""
import math


def apply_structures(data, definitions):
    if definitions.get('schema_version') != 1: raise ValueError('Unsupported structure schema')
    nodes = {n['id']: n for n in data['road_graph']['nodes']}
    records = []
    for item in definitions['bridges']:
        way = item['way']
        features = [f for f in data['features'] if f['id'].startswith('way/'+way+':line:')]
        edges = [e for e in data['road_graph']['edges'] if e['way'] == way]
        if len(features) != 1 or not edges: raise ValueError('Bridge source changed: '+way)
        feature = features[0]; points = feature['points']
        if feature['tags'].get('bridge') != 'yes': raise ValueError('Expected bridge: '+way)
        distances = [0.0]
        for a,b in zip(points,points[1:]): distances.append(distances[-1]+math.dist(a[:2],b[:2]))
        length = distances[-1]
        if length < 100 or abs(points[-1][2]-points[0][2])/length > item['max_grade']:
            raise ValueError('Unsafe bridge grade: '+way)
        start,end = points[0][2],points[-1][2]
        coordinates = {}
        for point,distance in zip(points,distances):
            point[2] = round(start+(end-start)*distance/length,3)
            coordinates[tuple(point[:2])] = point[2]
        for edge in edges:
            for key in ('from','to'):
                point = nodes[edge[key]]['position']
                if tuple(point[:2]) not in coordinates: raise ValueError('Bridge graph/render mismatch')
                point[2] = coordinates[tuple(point[:2])]
            edge['length_cm'] = math.dist(nodes[edge['from']]['position'],nodes[edge['to']]['position'])
            edge['surface_built'] = True
        feature['surface_built'] = True
        records.append({'way':way,'name':item['name'],'length_cm':length,'surveyed':False,'engine_verified':False})
    data['authored_structures'] = records
