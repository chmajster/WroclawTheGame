"""Bind audited Quaternius clips after checking named rig hierarchies in C++."""
import importlib.util
import json
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
MARKER=ROOT/'Saved/DistrictLocomotionReady.ok'

def prepare():
    MARKER.unlink(missing_ok=True)
    spec=importlib.util.spec_from_file_location('wtg_creator',ROOT/'Scripts/prepare_character_creator.py')
    creator=importlib.util.module_from_spec(spec);spec.loader.exec_module(creator)
    creator.prepare()
    catalog=unreal.load_asset('/Game/CharacterCreator/DA_CharacterAppearanceCatalog')
    imported=json.loads((ROOT/'Saved/FreeAnimationImportMap.json').read_text(encoding='utf-8'))
    paths=imported['quaternius-ual1-standard']['animations']
    def clip(name):
        matches=[p for p in paths if p.rsplit('.',1)[-1]=='UAL1_Standard'+name]
        if len(matches)!=1: raise RuntimeError('Ambiguous or missing clip '+name)
        return unreal.load_asset(matches[0])
    clips={k:clip(v) for k,v in {'idle':'Idle_Loop','walk':'Walk_Loop','jog':'Jog_Fwd_Loop','sprint':'Sprint_Loop','crouch':'Crouch_Fwd_Loop','jump':'Jump_Loop'}.items()}
    bodies=list(catalog.get_editor_property('bodies'))
    prepared=[]
    female_mesh=None
    for body in bodies:
        identifier=str(body.get_editor_property('id'))
        if identifier not in ('Male.Standard','Female.Standard'): continue
        mesh=body.get_editor_property('mesh').load_synchronous() if hasattr(body.get_editor_property('mesh'),'load_synchronous') else body.get_editor_property('mesh')
        if not isinstance(mesh,unreal.SkeletalMesh): raise RuntimeError('Body missing '+identifier)
        if identifier=='Female.Standard': female_mesh=mesh
        for anim in clips.values():
            if not unreal.LocomotionAssetLibrary.link_compatible_rig(mesh,anim): raise RuntimeError('Retarget required for '+identifier+' / '+anim.get_name())
        space=unreal.LocomotionAssetLibrary.bake_locomotion('/Game/CharacterCreator/BS_'+identifier.replace('.','_'),mesh,clips['idle'],clips['walk'],clips['jog'],clips['sprint'])
        if not space: raise RuntimeError('Cannot build locomotion '+identifier)
        for field,key in [('creator_idle','idle'),('walk','walk'),('jog','jog'),('crouch','crouch'),('jump_loop','jump')]: body.set_editor_property(field,clips[key])
        body.set_editor_property('locomotion',space)
        body.set_editor_property('animation_class',None)
        body.set_editor_property('placeholder',True)
        for asset in [space,mesh.get_editor_property('skeleton')]+[a.get_editor_property('skeleton') for a in clips.values()]:
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset,False): raise RuntimeError('Cannot save '+asset.get_name())
        prepared.append(identifier)
    if len(prepared)!=2: raise RuntimeError('Both player bodies must have locomotion')
    catalog.set_editor_property('bodies',bodies)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog,False)
    # Store the prepared walk clip and body in the generated source map population.
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'): raise RuntimeError('District map unavailable')
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor,unreal.CityPopulation):
            actor.set_editor_property('pedestrian_mesh',female_mesh)
            actor.set_editor_property('pedestrian_walk',clips['walk'])
    if not levels.save_current_level(): raise RuntimeError('Cannot save population locomotion')
    MARKER.write_text(json.dumps({'bodies':prepared,'clips':{k:a.get_path_name() for k,a in clips.items()},'status':'PASS'}),encoding='utf-8')
    unreal.log('WTG_DISTRICT_LOCOMOTION_READY')

if __name__=='__main__':
    try: prepare()
    except Exception:
        unreal.log_error(traceback.format_exc());raise
    finally: unreal.SystemLibrary.quit_editor()
