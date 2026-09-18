"""Execute inside Unreal Editor 5.6 via -ExecutePythonScript. Never reports success on a partial import."""
from pathlib import Path
import sys
import json
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Scripts'))
from make_source_assets import generate, MATERIALS, SOUNDS, LOOPS
MARKER = ROOT / 'Saved/GeneratedContent.ok'
FREE_MODEL_IMPORT_MAP = ROOT / 'Saved/FreeModelImportMap.json'
OPENING_MODELS = ROOT / 'Data/opening_scene_models.json'
PROP_MODEL_BINDINGS = ROOT / 'Data/prop_model_bindings.json'
MODEL_BINDINGS = ROOT / 'Data/model_bindings.json'


def import_file(file, destination):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(file))
    task.set_editor_property('destination_path', destination)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = task.get_editor_property('imported_object_paths')
    if not paths:
        raise RuntimeError(f'Import failed: {file}')
    asset = unreal.load_asset(paths[0])
    if not asset:
        raise RuntimeError(f'Imported asset not readable: {paths[0]}')
    return asset


def load_free_models(required_ids):
    if not FREE_MODEL_IMPORT_MAP.is_file():
        raise RuntimeError('Free model import map missing; run import_free_models.py before prepare_content.py')
    imported = json.loads(FREE_MODEL_IMPORT_MAP.read_text(encoding='utf-8'))
    resolved = {}
    for model_id in sorted(required_ids):
        metadata = imported.get(model_id)
        object_path = metadata.get('primary_object') if metadata else None
        if not object_path:
            raise RuntimeError(f'Free model not imported: {model_id}')
        asset = unreal.load_asset(object_path)
        if not asset or not isinstance(asset, (unreal.StaticMesh, unreal.SkeletalMesh)):
            raise RuntimeError(f'Free model is not a mesh: {model_id} -> {object_path}')
        resolved[model_id] = asset
    return resolved


def apply_static_mesh(actor, mesh, scale):
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not component:
        raise RuntimeError(f'StaticMeshComponent missing on {actor.get_actor_label()}')
    component.set_static_mesh(mesh)
    component.set_editor_property('cast_shadow', True)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return component


def hide_static_visuals(actor):
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        component.set_visibility(False, True)
        component.set_editor_property('cast_shadow', False)


def apply_skeletal_mesh(actor, mesh, scale=(1, 1, 1), rotation=None):
    component = actor.get_component_by_class(unreal.SkeletalMeshComponent)
    if not component:
        raise RuntimeError(f'SkeletalMeshComponent missing on {actor.get_actor_label()}')
    component.set_skeletal_mesh_asset(mesh)
    component.set_visibility(True, True)
    component.set_editor_property('cast_shadow', True)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if rotation is not None:
        component.set_relative_rotation(unreal.Rotator(*rotation))
    hide_static_visuals(actor)
    return component


def spawn_skeletal_visual(spawn, mesh, position, label, rotation=None, scale=(1, 1, 1)):
    visual = spawn(unreal.SkeletalMeshActor, position, label, rotation or [0, 0, 0])
    component = visual.get_component_by_class(unreal.SkeletalMeshComponent)
    if not component:
        raise RuntimeError(f'SkeletalMeshComponent missing on {label}')
    component.set_skeletal_mesh_asset(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_editor_property('cast_shadow', True)
    visual.set_actor_scale3d(unreal.Vector(*scale))
    return visual

def material(name, texture, roughness, metallic):
    path = f'/Game/Generated/M_{name}'
    asset = unreal.load_asset(path)
    if asset is None:
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            f'M_{name}', '/Game/Generated', unreal.Material, unreal.MaterialFactoryNew())
    if asset is None:
        raise RuntimeError(f'Material creation failed: {path}')
    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(asset)
    sample = edit.create_material_expression(asset, unreal.MaterialExpressionTextureSample, -450, 0)
    sample.set_editor_property('texture', texture)
    edit.connect_material_property(sample, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    for y, value, prop in [(180, roughness, unreal.MaterialProperty.MP_ROUGHNESS), (280, metallic, unreal.MaterialProperty.MP_METALLIC)]:
        constant = edit.create_material_expression(asset, unreal.MaterialExpressionConstant, -200, y)
        constant.set_editor_property('r', value)
        edit.connect_material_property(constant, '', prop)
    edit.recompile_material(asset)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError(f'Material save failed: {path}')


def prepare():
    MARKER.unlink(missing_ok=True)
    directory = generate(ROOT / 'Saved/SourceAssets')
    for name, (_, roughness, metallic) in MATERIALS.items():
        texture = import_file(directory / f'T_{name}.tga', '/Game/Generated/Textures')
        material(name, texture, roughness, metallic)
    for name in SOUNDS:
        sound = import_file(directory / f'{name}.wav', '/Game/Generated/Audio')
        sound.set_editor_property('looping', name in LOOPS)
        if not unreal.EditorAssetLibrary.save_loaded_asset(sound, False):
            raise RuntimeError(f'Audio save failed: {name}')
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    map_path = '/Game/Maps/Przebudzenie_Source'
    # This package is generated exclusively from Data/environment.json, never an authored user map.
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        if not levels.new_level('/Game/Maps/WTG_BakeScratch'):raise RuntimeError('Cannot switch away from generated map')
        if not unreal.EditorAssetLibrary.delete_asset(map_path):raise RuntimeError('Cannot replace generated map')
        for directory in ('/Game/__ExternalActors__/Maps/Przebudzenie_Source','/Game/__ExternalObjects__/Maps/Przebudzenie_Source'):
            if unreal.EditorAssetLibrary.does_directory_exist(directory) and not unreal.EditorAssetLibrary.delete_directory(directory):raise RuntimeError('Cannot clear generated external actors')
    if not levels.new_level(map_path):raise RuntimeError('Cannot create generated map')
    hlod=unreal.load_asset('/Game/Generated/HLOD_Instancing')
    if not hlod:hlod=unreal.AssetToolsHelpers.get_asset_tools().create_asset('HLOD_Instancing','/Game/Generated',unreal.HLODLayer,unreal.HLODLayerFactory())
    if not hlod:raise RuntimeError('HLOD layer creation failed')
    hlod.set_editor_property('layer_type',unreal.HLODLayerType.INSTANCING)
    hlod.set_editor_property('cell_size',6400);hlod.set_editor_property('loading_range',14000)
    unreal.EditorAssetLibrary.save_loaded_asset(hlod,False)
    layer_assets={}
    for layer_name in ('Architecture','Gameplay'):
        asset=unreal.load_asset('/Game/Generated/DL_'+layer_name)
        if not asset:asset=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DL_'+layer_name,'/Game/Generated',unreal.DataLayerAsset,unreal.DataLayerFactory())
        if not asset:raise RuntimeError('Data Layer creation failed')
        unreal.EditorAssetLibrary.save_loaded_asset(asset,False);layer_assets[layer_name]=asset
    def spawn(cls, position, label, rotation=None):
        actor=actors.spawn_actor_from_class(cls,unreal.Vector(*position),unreal.Rotator(*(rotation or [0,0,0])))
        if not actor:raise RuntimeError(f'Cannot spawn {label}')
        actor.set_actor_label('SliceBootstrap_'+label)
        actor.set_editor_property('data_layer_assets',[layer_assets['Architecture' if label.startswith('environment_') else 'Gameplay']])
        return actor
    cube=unreal.load_asset('/Engine/BasicShapes/Cube')
    for record in json.loads((ROOT/'Data/environment.json').read_text(encoding='utf-8')):
        kind=record['type']
        if kind=='box':
            actor=spawn(unreal.StaticMeshActor,record['position'],record['id'])
            component=actor.get_component_by_class(unreal.StaticMeshComponent)
            component.set_static_mesh(cube)
            surface=unreal.load_asset('/Game/SurfaceQuality/Instances/MI_'+record['material'])
            component.set_material(0,surface or unreal.load_asset('/Game/Generated/M_'+record['material']))
            actor.set_editor_property('hlod_layer',hlod)
            actor.set_actor_scale3d(unreal.Vector(*[v/100 for v in record['size']]))
        elif kind=='light':
            actor=spawn(unreal.PointLight,record['position'],record['id'])
            component=actor.get_component_by_class(unreal.PointLightComponent)
            component.set_editor_property('intensity',record['intensity'])
            component.set_editor_property('attenuation_radius',record['radius'])
            component.set_editor_property('cast_shadows',record.get('cast_shadows', True))
            component.set_editor_property('source_radius',record.get('source_radius', 4.0))
            component.set_editor_property('soft_source_radius',record.get('soft_source_radius', 12.0))
            component.set_editor_property('light_color',unreal.Color(*[int(v*255) for v in record['color']],255))
        elif kind=='sign':
            actor=spawn(unreal.TextRenderActor,record['position'],record['id'],record['rotation'])
            component=actor.get_component_by_class(unreal.TextRenderComponent)
            component.set_text(record['text']);component.set_world_size(record['size'])
        else:raise RuntimeError('Unknown environment record')
    opening_records=json.loads(OPENING_MODELS.read_text(encoding='utf-8'))
    binding_records=json.loads(PROP_MODEL_BINDINGS.read_text(encoding='utf-8'))
    model_bindings=json.loads(MODEL_BINDINGS.read_text(encoding='utf-8'))
    prop_bindings={record['action']:record for record in binding_records}
    required_models={record['model'] for record in opening_records}
    required_models.update(record['model'] for record in binding_records)
    required_models.update(model_bindings['actions'].values())
    required_models.update(model_bindings['systems'].values())
    free_models=load_free_models(required_models)
    for record in opening_records:
        actor=spawn(unreal.StaticMeshActor,record['position'],record['id'],record.get('rotation',[0,0,0]))
        apply_static_mesh(actor,free_models[record['model']],record.get('scale',[1,1,1]))
        actor.set_editor_property('hlod_layer',hlod)

    content=json.loads((ROOT/'Data/chapter1.json').read_text(encoding='utf-8'))
    for action in content['actions']:
        if action['kind'] in ('virtual','zone') or action['position'][:2]==[0,0]:continue
        authored=prop_bindings.get(action['id'],{})
        model_id=authored.get('model') or model_bindings['actions'].get(action['id'])
        if not model_id:
            raise RuntimeError('Physical action has no model binding: '+action['id'])
        position=list(action['position'])
        offset=authored.get('offset',[0,0,0])
        position=[position[i]+offset[i] for i in range(3)]
        rotation=authored.get('rotation',[0,0,0])
        scale=authored.get('scale',[1,1,1])
        actor=spawn(unreal.SliceProp,position,'action_'+action['id'],rotation)
        actor.set_editor_property('action_id',action['id'])
        mesh=free_models[model_id]
        if isinstance(mesh,unreal.StaticMesh):
            apply_static_mesh(actor,mesh,scale)
        else:
            hide_static_visuals(actor)
            spawn_skeletal_visual(spawn,mesh,position,'action_visual_'+action['id'],rotation,scale)
    world=json.loads((ROOT/'Data/openworld.json').read_text(encoding='utf-8'))
    guard_mesh=free_models[model_bindings['systems']['enemy_guard']]
    resident_mesh=free_models[model_bindings['systems']['resident_npc']]
    camera_mesh=free_models[model_bindings['systems']['surveillance_camera']]
    monitor_mesh=free_models[model_bindings['systems']['cctv_monitor']]
    hide_models={
        'container':model_bindings['systems']['hide_container'],
        'garage_hiding':model_bindings['systems']['hide_shelf'],
    }
    for guard in world['guards']:
        actor=spawn(unreal.SliceEnemy,guard['position'],'guard_'+guard['id'])
        actor.set_editor_property('guard_id',guard['id'])
        if isinstance(guard_mesh,unreal.SkeletalMesh):
            apply_skeletal_mesh(actor,guard_mesh)
    for hiding in world['hides']:
        actor=spawn(unreal.WorldInteraction,hiding['position'],'hide_'+hiding['id'])
        actor.set_editor_property('definition_id',hiding['id']);actor.set_editor_property('kind','Hide')
        model_id=hide_models.get(hiding['id'])
        if model_id:
            mesh=free_models[model_id]
            if isinstance(mesh,unreal.StaticMesh):
                apply_static_mesh(actor,mesh,[1,1,1])
        else:
            hide_static_visuals(actor)
    for camera in world['cameras']:
        actor=spawn(unreal.SurveillanceCamera,camera['position'],'camera_'+camera['id'],camera['rotation'])
        actor.set_editor_property('definition_id',camera['id'])
        if isinstance(camera_mesh,unreal.StaticMesh):
            visual=spawn(unreal.StaticMeshActor,camera['position'],'camera_visual_'+camera['id'],camera['rotation'])
            apply_static_mesh(visual,camera_mesh,[1,1,1])
            visual.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor=spawn(unreal.WorldInteraction,[camera['position'][0]-180,camera['position'][1],100],'monitor_'+camera['id'])
        actor.set_editor_property('definition_id',camera['id']);actor.set_editor_property('kind','CCTV')
        if isinstance(monitor_mesh,unreal.StaticMesh):
            apply_static_mesh(actor,monitor_mesh,[0.55,0.55,0.55])
    for npc in world['npc']:
        actor=spawn(unreal.ResidentNPC,npc['position'],'npc_'+npc['id']);actor.set_editor_property('definition_id',npc['id'])
        if isinstance(resident_mesh,unreal.SkeletalMesh):
            apply_skeletal_mesh(actor,resident_mesh)
    definition=unreal.load_asset('/Game/Generated/ChapterDefinition')
    if not definition:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.ChapterDefinition)
        definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset('ChapterDefinition','/Game/Generated',unreal.ChapterDefinition,factory)
    if not definition:raise RuntimeError('ChapterDefinition creation failed')
    definition.import_generated_catalog()
    if not unreal.EditorAssetLibrary.save_loaded_asset(definition,False):raise RuntimeError('ChapterDefinition save failed')
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(250, 400, 456))
    if not start:
        raise RuntimeError('PlayerStart spawn failed')
    start.set_actor_label('SliceBootstrap_PlayerStart')
    # Actor factory creates the bounds volume brush in an editor world.
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(7900, 4600, 200))
    if not nav:
        raise RuntimeError('Navigation volume spawn failed')
    nav.set_actor_label('SliceBootstrap_Navigation')
    _, extent = nav.get_actor_bounds(False)
    if min(extent.x, extent.y, extent.z) <= 0:
        raise RuntimeError('Navigation brush has no bounds; generation aborted before packaging')
    nav.set_actor_scale3d(unreal.Vector(8100 / extent.x, 4800 / extent.y, 1500 / extent.z))
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    unreal.EditorAssetLibrary.save_directory('/Game/Generated', False, True)
    MARKER.parent.mkdir(parents=True, exist_ok=True)
    MARKER.write_text('Przebudzenie content v3\n', encoding='utf-8')
    unreal.log('WROCLAW_CONTENT_READY')


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
