"""Inventory the existing start without moving gameplay anchors or reading saves."""
import json
from pathlib import Path
from shapely.geometry import LineString, Point, Polygon

ROOT = Path(__file__).resolve().parents[2]


def audit():
    folder = ROOT / 'Saved/PlayableDistrict'
    plan = json.loads((folder / 'district.json').read_text(encoding='utf-8'))
    sector = json.loads((folder / 'sector.json').read_text(encoding='utf-8'))
    start = Point(plan['anchors']['player'][:2])
    buildings, roads = [], []
    for feature in sector['features']:
        if feature['kind'] == 'building':
            shape = Polygon([p[:2] for p in feature['rings'][0]])
            distance = shape.distance(start) / 100
            if distance <= 100:
                tags = feature['tags']
                buildings.append(dict(id=feature['id'], distance_m=round(distance, 2),
                    height_m=feature['height_m'], height_measured=bool(tags.get('height')),
                    address=' '.join(filter(None, [tags.get('addr:street'), tags.get('addr:housenumber')])),
                    levels=tags.get('building:levels'), roof=tags.get('roof:shape')))
        elif feature['kind'] == 'road':
            distance = LineString([p[:2] for p in feature['points']]).distance(start) / 100
            if distance <= 100:
                roads.append(dict(id=feature['id'], distance_m=round(distance, 2),
                    name=feature.get('name'), width_m=feature['width_m'], tags=feature['tags']))
    report = dict(schema_version=1, map=plan['map'], radius_m=100,
        player_anchor_cm=plan['anchors']['player'], vehicle_anchor_cm=plan['anchors']['vehicle'],
        buildings=sorted(buildings, key=lambda b: b['distance_m']),
        roads=sorted(roads, key=lambda r: r['distance_m']),
        source_dataset=sector['sources'],
        limitations=['Fallback building heights are estimates, not surveyed measurements.',
                     'Map uses spatial mesh cells/HISM; it is not World Partition.',
                     'Inventory does not certify native collision, visuals or frame time.'])
    output = ROOT / 'Saved/StartLocation/audit.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f"AUDIT_PASS {len(buildings)} buildings / {len(roads)} roads; start unchanged")
    return report


if __name__ == '__main__':
    audit()
