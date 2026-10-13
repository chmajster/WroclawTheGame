"""Persist production LOD/fallback and measure actual native scene budgets."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
from city_quality import configure_mesh,configure_instances
marker=ROOT/'Saved/StartLocation/quality.ok';marker.unlink(missing_ok=True)
try:
    policy=json.loads((ROOT/'Data/start_location.json').read_text(encoding='utf-8'))['quality']
    metadata=json.loads((ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json').read_text(encoding='utf-8'))
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    records=[]
    for record in metadata['assets']:
        manifest=json.loads((ROOT/'Pipeline/assets/start_location'/(record['asset']+'.asset.json')).read_text(encoding='utf-8'))
        mesh=unreal.load_asset(manifest['unreal']['destination']+'/SM_wtg_start_'+record['asset'].replace('-','_'))
        configure_mesh(mesh,manifest)
        sizes=list(editor.get_lod_screen_sizes(mesh))
        category='building' if record['area']=='Buildings' else ('vegetation' if record['area']=='Vegetation' else 'prop')
        if any(abs(a-b)>1e-5 for a,b in zip(sizes,policy['lod_screen_sizes'][category])):raise RuntimeError('LOD thresholds not retained')
        records.append(dict(asset=record['asset'],lod_screen_sizes=sizes,nanite=bool(editor.get_nanite_settings(mesh).enabled),lod0_triangles=mesh.get_num_triangles(0)))
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):raise RuntimeError('Map missing')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    instance_count=0;clusters=0;scene_triangles=0;collision_shapes=0
    for actor in actors.get_all_level_actors():
        if not actor.get_actor_label().startswith('StartLocation_'):continue
        for component in actor.get_components_by_class(unreal.StaticMeshComponent):
            mesh=component.get_editor_property('static_mesh')
            if not mesh:continue
            count=component.get_instance_count() if isinstance(component,unreal.HierarchicalInstancedStaticMeshComponent) else 1
            if isinstance(component,unreal.HierarchicalInstancedStaticMeshComponent):
                configure_instances(component,policy);instance_count+=count;clusters+=1
            scene_triangles+=count*mesh.get_num_triangles(0)
            collision_shapes+=count*(editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh))
    if instance_count>policy['max_instances'] or scene_triangles>policy['max_new_mesh_triangles']:raise RuntimeError('Native scene budget exceeded')
    textures=unreal.EditorAssetLibrary.list_assets('/Game/Wroclaw/StartLocation/Materials/Textures',True,False)
    for path in textures:
        texture=unreal.load_asset(path)
        if isinstance(texture,unreal.Texture2D):
            texture.set_editor_property('max_texture_size',policy['texture_max_size']);texture.set_editor_property('never_stream',False)
            unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
    if not levels.save_current_level():raise RuntimeError('Save failed')
    report=dict(status='PASS',instances=instance_count,hism_components=clusters,instanced_lod0_triangles=scene_triangles,
        native_collision_shapes=collision_shapes,shared_texture_count=len(textures),texture_max_size=policy['texture_max_size'],
        instance_cull_cm=[policy['instance_cull_start_cm'],policy['instance_cull_end_cm']],assets=records,
        limits='Engineering budget, not measured GPU memory or FPS; map uses source cells/HISM, not World Partition.')
    (marker.parent/'quality-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    marker.write_text('PASS\n',encoding='utf-8');unreal.log('WTG_START_QUALITY_PASS '+json.dumps(report))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
