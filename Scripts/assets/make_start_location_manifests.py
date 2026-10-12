"""Declare each original production mesh and its shared material bindings."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]


def main():
    source=ROOT/'SourceAssets/Models/Wroclaw/StartLocation'
    data=json.loads((source/'geometry.json').read_text(encoding='utf-8'))
    output=ROOT/'Pipeline/assets/start_location';output.mkdir(parents=True,exist_ok=True)
    for asset in data['assets']:
        building=asset['area']=='Buildings'
        manifest=dict(schema_version=1,id='wtg-start-'+asset['asset'],display_name=asset['asset'],
            area='start_location',type='static_mesh',quality='hero' if building else 'standard',
            source=dict(model=(source/asset['area']/(asset['asset']+'.blend')).relative_to(ROOT).as_posix(),
                        license=data['license'],reference_directory=None),
            mesh=dict(max_triangles=120000 if building else 18000,combine=True,generate_uv_if_missing=False),
            lod=dict(ratios=[1,.55,.22]),
            collision=dict(mode='authored' if asset['collision_shapes'] else ('simple' if asset['asset'] in ['curb','bike-rack','parked-car'] else 'none'),max_hulls=32 if building else 1),
            textures={},materials=[dict(slot=i,asset='/Game/Wroclaw/StartLocation/Materials/MI_'+name) for i,name in enumerate(asset['materials'])],
            unreal=dict(destination='/Game/Wroclaw/StartLocation/'+asset['area'],nanite=building,qa_map='/Game/Maps/Nadodrze_Playable'),
            qa=dict(automation_filter='WTG.District',cameras=['StartLocation_Start_Player','StartLocation_Start_Street','StartLocation_Start_Park'],screenshot_width=1280,screenshot_height=720))
        (output/(asset['asset']+'.asset.json')).write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('START_MANIFESTS_PASS',len(data['assets']))


if __name__=='__main__':main()
