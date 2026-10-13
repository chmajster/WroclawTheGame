"""Resolve source/manifests/provenance without claiming editor import or QA."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]


def main():
    source=ROOT/'SourceAssets/Models/Wroclaw/StartLocation'
    metadata=json.loads((source/'geometry.json').read_text(encoding='utf-8'))
    assets=[]
    for record in metadata['assets']:
        manifest_path=Path('Pipeline/assets/start_location')/(record['asset']+'.asset.json')
        manifest=json.loads((ROOT/manifest_path).read_text(encoding='utf-8'))
        model=Path(manifest['source']['model']);glb=model.with_suffix('.glb')
        for path in [model,glb]:
            if not (ROOT/path).is_file():raise FileNotFoundError(str(path))
        expected='/Game/Wroclaw/StartLocation/'+record['area']+'/SM_wtg_start_'+record['asset'].replace('-','_')
        if manifest['unreal']['destination']!=expected.rsplit('/',1)[0]:raise ValueError('Destination mismatch '+record['asset'])
        if len(manifest['materials'])!=len(record['materials']):raise ValueError('Material count mismatch')
        native=Path('Content')/(expected.removeprefix('/Game/')+'.uasset')
        assets.append(dict(id=manifest['id'],category=record['area'],source_model=model.as_posix(),
            exchange_model=glb.as_posix(),manifest=manifest_path.as_posix(),native_path=expected,
            native_file_present=(ROOT/native).is_file(),author='WroclawTheGame original procedural geometry',
            license=manifest['source']['license'],generator='Scripts/assets/build_start_location_models.py',
            triangles_lod0=record['triangles'],bounds_m=record['bounds_m'],source_material_slots=record['materials']))
    approved=json.loads((ROOT/'Data/city_asset_acquisition.json').read_text(encoding='utf-8'))
    textures=[]
    for record in approved['assets']:
        if record['license']!='CC0-1.0':raise ValueError('Unreviewed texture license')
        for channel,file in record['files'].items():
            if not (ROOT/file['path']).is_file() or len(file.get('sha256',''))!=64:raise ValueError('Missing pinned source texture')
        textures.append({key:record[key] for key in ['id','name','authors','source_url','license','license_url','resolution','files']})
    reused=[]
    for record in json.loads((ROOT/'Data/free_model_catalog.json').read_text(encoding='utf-8')):
        if record['slug'] in ['street_lamp_01','modular_street_seating','metal_trash_can']:reused.append(record)
    if len(reused)!=3:raise ValueError('Required existing props missing from source catalog')
    policy=json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))
    catalog=dict(schema_version=1,scope=f"Existing Pomorska/Paulinska start,{policy['radius_m']}m; bounded architecture context {policy.get('architecture_context',{}).get('radius_m',0)}m",assets=assets,
        textures=textures,reused_models=reused,credit=approved['credit'],
        placement_sources=['Data/start_location.json','Data/start_location_buildings.json','Data/playable_district.json',
                           'Data/source/wroclaw/osm/wave1.osm.gz (part-aware source_bytes reader)'],
        verification='Source paths/metadata only. File presence does not certify native import, collision or visual QA; see archived native reports.')
    (ROOT/'Data/start_location_asset_catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
    print('PASS source catalog',len(assets),'original meshes;',len(textures),'CC0 sets;3 reused prop types')


if __name__=='__main__':main()
