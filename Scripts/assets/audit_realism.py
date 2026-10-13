"""Read-only source diagnosis for the real start; never infer visual success."""
from pathlib import Path
import argparse
import hashlib
import json
import math
import re

ROOT = Path(__file__).resolve().parents[2]


def audit(engine_root=None):
    project = json.loads((ROOT / 'WroclawTheGame.uproject').read_text(encoding='utf-8-sig'))
    config = (ROOT / 'Config/DefaultEngine.ini').read_text(encoding='utf-8-sig')
    policy = json.loads((ROOT / 'Data/start_location.json').read_text(encoding='utf-8'))
    report = {
        'schema_version': 1, 'engine_association': project['EngineAssociation'],
        'map': re.search(r'^GameDefaultMap=(.+)$', config, re.M).group(1).strip(),
        'scope_radius_m': policy['radius_m'], 'player_anchor_cm': policy['player_anchor_cm'],
        'source_sha256': {}, 'surface_candidates': [],
        'findings': [
            {'id': 'character', 'evidence': 'Scripts/prepare_character_creator.py: body_sources',
             'cause': 'The repeatable importer assigns Quaternius Superhero full-body prototypes, not a realistic civilian body.'},
            {'id': 'regeneration', 'evidence': 'Scripts/prepare_character_creator.py: prepare',
             'cause': 'Every preparation overwrites Male.Standard/Female.Standard and hair references, even when a better catalog exists.'},
            {'id': 'green-height', 'evidence': 'Scripts/gis/build_meshes.py: generate',
             'cause': 'Green polygons use the maximum perimeter elevation for every triangle rather than the DEM at each vertex; verify intersecting candidates below.'},
            {'id': 'cell-bounds', 'evidence': 'Scripts/gis/build_meshes.py: triangle; Scripts/gis/build_playable_district.py: sidewalk triangulation',
             'cause': 'Triangles are assigned by centroid without clipping to cells, so actual bounds can exceed the claimed 128 m sector.'},
            {'id': 'trees', 'evidence': 'Scripts/assets/build_start_location_models.py: tree',
             'cause': 'Leaf faces are rectangular quads with generic planar UVs and no leaf silhouette mask.'},
        ],
        'unverified': ['No original screenshot was attached; the tube cannot be uniquely identified from its description.',
                       'Source findings do not establish the cause of every dark pixel. Native diagnostic views are separate evidence.'],
    }
    for path in ['WroclawTheGame.uproject', 'Config/DefaultEngine.ini',
                 'Scripts/gis/build_meshes.py', 'Scripts/gis/build_playable_district.py',
                 'Scripts/prepare_character_creator.py', 'Scripts/assets/build_start_location_models.py']:
        report['source_sha256'][path] = hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
    if engine_root:
        engine = Path(engine_root) / 'Engine'
        report['engine_build'] = json.loads((engine/'Build/Build.version').read_text())
        source = engine/'Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapCacheManager.cpp'
        lines = source.read_text(encoding='utf-8').splitlines()
        report['vsm'] = {'source': str(source), 'diagnostics': [
            {'line': i+1, 'text': line.strip()} for i,line in enumerate(lines)
            if 'case VSM_STAT_OVERFLOW_FLAG_' in line],
            'interpretation': 'MARKING_JOB_QUEUE and PAGE_POOL are different overflow flags. Do not change page-pool size to fix the marking queue.'}
    meshes = ROOT/'Saved/PlayableDistrict/Meshes'
    if (meshes/'meshes.json').is_file():
        for filename in json.loads((meshes/'meshes.json').read_text()):
            data = json.loads((meshes/filename).read_text())
            if not data['vertices'] or not data['triangles']: continue
            points = data['vertices']; origin = data['origin']
            minimum = [min(p[i] for p in points)+origin[i] for i in range(3)]
            maximum = [max(p[i] for p in points)+origin[i] for i in range(3)]
            distance = math.hypot(*[max(minimum[i]-policy['player_anchor_cm'][i],0,
                                       policy['player_anchor_cm'][i]-maximum[i]) for i in range(2)])
            if distance <= policy['radius_m']*100:
                report['surface_candidates'].append({'mesh': filename, 'kind': data['kind'],
                    'bounds_min_cm': minimum, 'bounds_max_cm': maximum,
                    'oversized_cell': any(maximum[i]-minimum[i] > 12801 for i in range(2)),
                    'triangles': len(data['triangles'])//3})
    output = ROOT/'Saved/Realism/source-audit.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f"REALISM_AUDIT {report['engine_association']} {report['map']} {len(report['surface_candidates'])} start cells")
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--engine-root')
    audit(parser.parse_args().engine_root)
