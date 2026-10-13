"""Inspect actual saved actors, shader/mesh flags and the resolved player catalog."""
from pathlib import Path
import json
import sys
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup


def inspect():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):
        raise RuntimeError('Real start map unavailable')
    catalog = unreal.load_asset('/Game/CharacterCreator/DA_CharacterAppearanceCatalog')
    records = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh = component.get_editor_property('static_mesh')
            if not mesh: continue
            bounds = mesh.get_bounds()
            records.append(dict(actor=actor.get_actor_label(), mesh=mesh.get_path_name(),
                class_name=component.get_class().get_name(),
                extent_cm=[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],
                position_cm=[getattr(actor.get_actor_location(),key) for key in ('x','y','z')],
                scale=[getattr(actor.get_actor_scale3d(),key) for key in ('x','y','z')],
                nanite=mesh.get_editor_property('nanite_settings').get_editor_property('enabled'),
                cast_shadow=component.get_editor_property('cast_shadow'),
                distance_fields=component.get_editor_property('affect_distance_field_lighting'),
                materials=[str(component.get_material(i).get_path_name()) if component.get_material(i) else None
                           for i in range(component.get_num_materials())]))
    bodies = []
    def describe(value):
        return value.get_path_name() if isinstance(value,unreal.Object) else str(value)
    for body in catalog.get_editor_property('bodies'):
        bodies.append({key:describe(body.get_editor_property(key)) for key in
            ['id','mesh','animation_class','creator_idle','locomotion','skin_material','mesh_rotation']})
    report = dict(schema_version=1, map='/Game/Maps/Nadodrze_Playable', actors=records,
                  bodies=bodies, world_partition=False,
                  note='Saved map inspection, not a runtime or visual quality verdict.')
    out=ROOT/'Saved/Realism/native-audit.json';out.parent.mkdir(parents=True,exist_ok=True)
    out.write_text(json.dumps(report,indent=2),encoding='utf-8')
    (out.parent/'native-audit.ok').write_text('PASS\n')
    unreal.log('WTG_REALISM_NATIVE_AUDIT '+str(len(records)))


try: inspect()
finally: quit_after_editor_startup()
