"""Repeatable district dressing and original synthesized audio, no campaign changes."""
import json
import math
from pathlib import Path
import sys
import traceback
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Scripts'))
from make_source_assets import make_sound, SOUNDS, LOOPS
MARKER=ROOT/'Saved/DistrictPresentationReady.ok'

def prepare():
    MARKER.unlink(missing_ok=True)
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    for name in ('EngineStart','EngineIdle','Siren','Street','Footstep'):
        source=ROOT/'Saved/SourceAudio'/f'{name}.wav'
        source.parent.mkdir(parents=True,exist_ok=True)
        make_sound(source,name,SOUNDS[name])
        task=unreal.AssetImportTask()
        for key,value in {'filename':str(source),'destination_path':'/Game/Generated/Audio',
                          'automated':True,'replace_existing':True,'save':False}.items():
            task.set_editor_property(key,value)
        tools.import_asset_tasks([task])
        sound=unreal.load_asset('/Game/Generated/Audio/'+name)
        if not isinstance(sound,unreal.SoundWave):raise RuntimeError('Audio import failed: '+name)
        sound.set_editor_property('looping',name in LOOPS)
        if not unreal.EditorAssetLibrary.save_loaded_asset(sound,False):raise RuntimeError('Audio save failed')
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):raise RuntimeError('District missing')
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith('District_Detail_'):actors.destroy_actor(actor)
    director=actors.spawn_actor_from_class(unreal.SurfaceQualityDirector,unreal.Vector(),unreal.Rotator())
    director.set_actor_label('District_Detail_SurfaceDirector')
    director.set_editor_property('district_outdoor',True)
    director.set_editor_property('follow_weather',False)
    plan=json.loads((ROOT/'Saved/PlayableDistrict/district.json').read_text(encoding='utf-8'))
    imports=json.loads((ROOT/'Saved/FreeModelImportMap.json').read_text(encoding='utf-8'))
    # Reuse licensed meshes already imported by the repository asset catalog.
    # Their real bounds define metre dimensions rather than guessed unit scales.
    meshes={name:unreal.load_asset(imports[name]['primary_object']) for name in
            ('street_lamp_01','modular_street_seating','metal_trash_can')}
    placed=[]
    for index,item in enumerate(plan['dressing']):
        name=item['model'];p=item['position'];yaw=item['yaw']
        if any(name==q['model'] and math.dist(p,q['position'])<700 for q in placed):continue
        position=unreal.Vector(*p);mesh=meshes[name];bounds=mesh.get_bounds()
        height={'street_lamp_01':420,'metal_trash_can':85,'modular_street_seating':50}[name]
        scale=min(height/max(bounds.box_extent.z*2,1),item['radius']/max(bounds.box_extent.x,bounds.box_extent.y,1))
        position.z-=bounds.origin.z*scale-bounds.box_extent.z*scale
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,position,unreal.Rotator(0,yaw,0))
        actor.set_actor_label(f'District_Detail_Prop_{index}')
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(mesh);component.set_collision_profile_name('BlockAll')
        actor.set_actor_scale3d(unreal.Vector(scale,scale,scale));placed.append(item)
    if not levels.save_current_level():raise RuntimeError('Cannot save dressed map')
    report={'status':'Prepared','audio':5,'props':len(placed),
            'exposure_ev':[4,14],'pbr':'Existing cm-scale triplanar CC0/procedural profiles',
            'nanite':'Not needed for the bounded low-poly slice; HISM facade batches retained'}
    MARKER.write_text(json.dumps(report),encoding='utf-8')
    unreal.log('WTG_DISTRICT_PRESENTATION_READY '+json.dumps(report))

try:prepare()
except Exception:
    MARKER.unlink(missing_ok=True)
    unreal.log_error(traceback.format_exc())
    raise
finally:unreal.SystemLibrary.quit_editor()
