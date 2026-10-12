"""Make small production inputs from authoritative GIS, never from binary maps."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def build():
    policy = json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))
    folder = ROOT/'Saved/PlayableDistrict'
    district = json.loads((folder/'district.json').read_text(encoding='utf-8'))
    if district['anchors']['player'] != policy['player_anchor_cm']:
        raise ValueError('Source start changed; review the location policy before rebuilding')
    sector_file = folder/'sector.json'
    sector = json.loads(sector_file.read_text(encoding='utf-8'))
    features = {f['id']:f for f in sector['features']}
    buildings = []
    for config in policy['buildings']:
        feature = features[config['id']]
        if feature['kind'] != 'building':
            raise ValueError('Building policy references a non-building')
        rings = feature['rings']
        base = max(p[2] for p in rings[0])
        xs, ys = [p[0] for p in rings[0]], [p[1] for p in rings[0]]
        origin = [(min(xs)+max(xs))/2, (min(ys)+max(ys))/2, base]
        buildings.append(dict(config, origin_cm=origin, tags=feature['tags'],
            rings_m=[[[round((p[i]-origin[i])/100,5) for i in range(3)] for p in r] for r in rings]))
    output = ROOT/'Data/start_location_buildings.json'
    output.write_text(json.dumps(dict(schema_version=1,
        source_sha256=hashlib.sha256(sector_file.read_bytes()).hexdigest(),
        source_license='OpenStreetMap ODbL 1.0; DEM Copernicus/USGS existing project attribution',
        buildings=buildings), indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'START_INPUTS_PASS {len(buildings)} exact source footprints; anchors retained')


if __name__ == '__main__':
    build()
