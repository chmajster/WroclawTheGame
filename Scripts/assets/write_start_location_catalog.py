"""Resolve source/manifests/provenance without claiming editor import or QA."""
import json
import hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]


def main():
    source=ROOT/'SourceAssets/Models/Wroclaw/StartLocation'
    metadata=json.loads((source/'geometry.json').read_text(encoding='utf-8'))
    assets=[]
    infrastructure=json.loads((source/'infrastructure.json').read_text(encoding='utf-8'))['assets']
    infrastructure_ids={p['asset'] for p in infrastructure}
    records=metadata['assets']+[dict(asset=p['asset'],area=('Roads' if p['asset']=='embedded-tram-rail' else 'Props'),
        provenance=p) for p in infrastructure]
    for record in records:
        manifest_path=Path('Pipeline/assets/start_location')/(record['asset']+'.asset.json')
        manifest=json.loads((ROOT/manifest_path).read_text(encoding='utf-8'))
        model=Path(manifest['source']['model']);glb=model.with_suffix('.glb')
        exchange=glb if (ROOT/glb).is_file() else None
        if record['asset'] not in infrastructure_ids and exchange is None:raise FileNotFoundError(str(glb))
        for path in [model]+([exchange] if exchange else []):
            if not (ROOT/path).is_file():raise FileNotFoundError(str(path))
        expected='/Game/Wroclaw/StartLocation/'+record['area']+'/SM_wtg_start_'+record['asset'].replace('-','_')
        if manifest['unreal']['destination']!=expected.rsplit('/',1)[0]:raise ValueError('Destination mismatch '+record['asset'])
        if record.get('materials') and len(manifest['materials'])!=len(record['materials']):raise ValueError('Material count mismatch')
        native=Path('Content')/(expected.removeprefix('/Game/')+'.uasset')
        provenance=record.get('provenance',{})
        author=provenance.get('author','WroclawTheGame contributors')
        if provenance and (not provenance.get('game_use') or not provenance.get('public_source_redistribution')):raise ValueError('Rights missing '+record['asset'])
        if provenance.get('sha256') and hashlib.sha256((ROOT/provenance['source']).read_bytes()).hexdigest()!=provenance['sha256']:raise ValueError('Source checksum '+record['asset'])
        assets.append(dict(id=manifest['id'],category=record['area'],source_model=model.as_posix(),
            exchange_model=exchange.as_posix() if exchange else None,manifest=manifest_path.as_posix(),native_path=expected,
            native_file_present=(ROOT/native).is_file(),author=author,source_provenance=provenance,
            license=manifest['source']['license'],game_use=True,public_source_redistribution=True,
            source_sha256=hashlib.sha256((ROOT/model).read_bytes()).hexdigest(),
            generator=provenance.get('generator','Scripts/assets/build_start_location_models.py'),
            triangles_lod0=record.get('triangles'),bounds_m=record.get('bounds_m',provenance.get('dimensions_m')),
            source_material_slots=record.get('materials')))
    approved=json.loads((ROOT/'Data/city_asset_acquisition.json').read_text(encoding='utf-8'))
    textures=[]
    for record in approved['assets']:
        if record['license']!='CC0-1.0':raise ValueError('Unreviewed texture license')
        for channel,file in record['files'].items():
            if not (ROOT/file['path']).is_file() or len(file.get('sha256',''))!=64:raise ValueError('Missing pinned source texture')
        item={key:record[key] for key in ['id','name','authors','source_url','license','license_url','resolution','files']}
        for file in record['files'].values():
            if hashlib.sha256((ROOT/file['path']).read_bytes()).hexdigest()!=file['sha256']:raise ValueError('Texture checksum '+file['path'])
        item.update(game_use=True,public_source_redistribution=True);textures.append(item)
    reused=[]
    for record in json.loads((ROOT/'Data/free_model_catalog.json').read_text(encoding='utf-8')):
        if record['slug'] in ['street_lamp_01','modular_street_seating','metal_trash_can']:
            reused.append(dict(record,game_use=True,public_source_redistribution=True))
    if len(reused)!=3:raise ValueError('Required existing props missing from source catalog')
    policy=json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))
    hero=json.loads((ROOT/'Data/hero_source_catalog.json').read_text(encoding='utf-8'))
    for item in hero['assets']:
        if hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest()!=item['sha256']:raise ValueError('Hero checksum '+item['path'])
        if item['license']=='CC0-1.0' and (not item['game_use'] or not item['public_source_redistribution']):raise ValueError('Hero rights '+item['path'])
    foliage=json.loads((ROOT/'SourceAssets/Textures/Wroclaw/Foliage/provenance.json').read_text(encoding='utf-8'))
    for path,sha in foliage['files'].items():
        if hashlib.sha256((ROOT/'SourceAssets/Textures/Wroclaw/Foliage'/path).read_bytes()).hexdigest()!=sha:raise ValueError('Foliage checksum '+path)
    foliage.update(game_use=True,public_source_redistribution=True,generator='Scripts/assets/build_foliage_textures.py')
    catalog=dict(schema_version=2,scope=f"Existing Pomorska/Paulinska start,{policy['radius_m']}m; bounded architecture context {policy.get('architecture_context',{}).get('radius_m',0)}m",assets=assets,
        textures=textures,reused_models=reused,credit=approved['credit'],
        hero=dict(manifest='Pipeline/assets/player/civilian.asset.json',source_catalog='Data/hero_source_catalog.json',sources=hero),
        foliage=foliage,
        placement_sources=['Data/start_location.json','Data/start_location_buildings.json','Data/playable_district.json',
                           'Data/source/wroclaw/osm/wave1.osm.gz (part-aware source_bytes reader)'],
        verification='Source paths/metadata only. File presence does not certify native import, collision or visual QA; see archived native reports.')
    (ROOT/'Data/start_location_asset_catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
    print('PASS source catalog',len(assets),'environment meshes;',len(textures),'CC0 sets;',len(hero['assets']),'hero source records')


if __name__=='__main__':main()
