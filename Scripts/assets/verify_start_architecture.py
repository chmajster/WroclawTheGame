"""Verify source-driven architecture in the actual playable level."""
import json
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
marker=ROOT/'Saved/StartLocation/architecture.ok'
marker.unlink(missing_ok=True)
try:
    source=json.loads((ROOT/'Data/start_location_buildings.json').read_text(encoding='utf-8'))
    metadata=json.loads((ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json').read_text(encoding='utf-8'))
    records={r['asset']:r for r in metadata['assets']}
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):raise RuntimeError('Playable level missing')
    buildings=[a for a in actors.get_all_level_actors() if a.get_actor_label().startswith(('StartLocation_Building_','StartLocation_Context_'))]
    configs=source['buildings']+source.get('context_buildings',[])
    if len(buildings)!=len(configs):raise RuntimeError('Duplicate/missing architecture actors')
    by_label={a.get_actor_label():a for a in buildings}
    report=[]
    for c in configs:
        label=('StartLocation_Context_' if c.get('context') else 'StartLocation_Building_')+c['asset']
        actor=by_label[label];record=records[c['asset']]
        origin=c['origin_cm'];centre=record['source_center_m']
        expected=[origin[0]+centre[0]*100,origin[1]-centre[1]*100,origin[2]+centre[2]*100]
        actual=actor.get_actor_location();position=[actual.x,actual.y,actual.z]
        if any(abs(a-b)>.02 for a,b in zip(position,expected)):raise RuntimeError('Source placement mismatch '+label)
        scale=actor.get_actor_scale3d();rotation=actor.get_actor_rotation()
        if any(abs(v-1)>.0001 for v in (scale.x,scale.y,scale.z)) or any(abs(v)>.0001 for v in (rotation.pitch,rotation.yaw,rotation.roll)):
            raise RuntimeError('Building footprint was scaled/rotated '+label)
        component=actor.get_component_by_class(unreal.StaticMeshComponent)
        mesh=component.get_editor_property('static_mesh')
        if mesh.get_name()!='SM_wtg_start_'+c['asset'].replace('-','_'):raise RuntimeError('Placeholder restored '+label)
        if not meshes.get_nanite_settings(mesh).enabled:raise RuntimeError('Architecture Nanite missing '+label)
        bounds=mesh.get_bounds();top=actual.z+bounds.origin.z+bounds.box_extent.z
        maximum=origin[2]+c['height_m']*100
        if top>maximum+.2 or top<maximum-10:raise RuntimeError('Source roof height changed '+label)
        hulls=meshes.get_simple_collision_count(mesh)+meshes.get_convex_collision_count(mesh)
        if hulls!=record['collision_shapes']:raise RuntimeError('Wall collision missing '+label)
        report.append(dict(actor=label,mesh=mesh.get_path_name(),position_cm=position,
            source_roof_max_cm=maximum,actual_roof_top_cm=top,nanite=True,collision_hulls=hulls))
    result=dict(status='PASS',map='/Game/Maps/Nadodrze_Playable',primary=len(source['buildings']),
        context=len(source.get('context_buildings',[])),actors=report,
        limits='Source geometry/placement contract; facade interpretation is not a surveyed reconstruction. Visual/runtime evidence is separate.')
    (marker.parent/'architecture-report.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    marker.write_text('PASS\n',encoding='utf-8')
    unreal.log('WTG_START_ARCHITECTURE_PASS '+str(len(report)))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
