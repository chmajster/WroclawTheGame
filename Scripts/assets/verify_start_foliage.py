"""Certify real meshes/shader/textures and instancing on the playable map."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup


def verify():
    dest='/Game/Wroclaw/StartLocation';edit=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mat=unreal.load_asset(dest+'/Materials/M_Start_Leaf')
    if not mat.get_editor_property('two_sided') or mat.get_editor_property('blend_mode')!=unreal.BlendMode.BLEND_MASKED or mat.get_editor_property('shading_model')!=unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE:raise RuntimeError('Wrong foliage shader')
    reports=[]
    for name in ['young_tree','young_maple','park_lime']:
        mesh=unreal.load_asset(dest+'/Vegetation/SM_wtg_start_'+name)
        if edit.get_lod_count(mesh)!=3 or edit.get_nanite_settings(mesh).enabled:raise RuntimeError('Foliage LOD contract')
        counts=[mesh.get_num_triangles(i) for i in range(3)]
        if not counts[0]>counts[1]>counts[2]>0:raise RuntimeError('LOD did not reduce leaves')
        if edit.get_convex_collision_count(mesh)!=1:raise RuntimeError('Trunk-only collision missing')
        for i in range(3):
            basis=unreal.GeoMeshLibrary.get_authored_mesh_basis_stats(mesh,i)
            if basis.y or basis.z:raise RuntimeError('Invalid foliage basis')
        reports.append(dict(mesh=mesh.get_path_name(),triangles=counts,lod_sizes=list(edit.get_lod_screen_sizes(mesh))))
    textures=[]
    for stem in ['leaf','bark']:
        for kind in ['base_color','normal']:
            t=unreal.load_asset(dest+'/Materials/Textures/'+stem+'_'+kind)
            if not t or bool(t.get_editor_property('srgb'))!=(kind=='base_color'):raise RuntimeError('Texture interpretation')
            textures.append(t.get_path_name())
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);levels.load_level('/Game/Maps/Nadodrze_Playable')
    counts={}
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if not actor.get_actor_label().startswith('StartLocation_Instances_'):continue
        for c in actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            name=c.get_editor_property('static_mesh').get_name();counts[name]=counts.get(name,0)+c.get_instance_count()
    if sum(counts.get('SM_wtg_start_'+n,0) for n in ['young_tree','young_maple','park_lime'])!=18 or counts.get('SM_wtg_start_modern_lamp')!=8:raise RuntimeError('Placement mismatch')
    out=ROOT/'Saved/StartLocation/foliage-report.json';out.write_text(json.dumps(dict(status='PASS',assets=reports,texture_paths=textures,instance_counts=counts),indent=2),encoding='utf-8')
    (out.parent/'foliage-qa.ok').write_text('PASS\n');unreal.log('WTG_START_FOLIAGE_PASS')


try:verify()
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
