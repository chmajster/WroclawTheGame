"""Execute inside Unreal Editor 5.6 via -ExecutePythonScript. Never reports success on a partial import."""
from pathlib import Path
import sys
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0, str(ROOT / 'Scripts'))
from make_source_assets import generate, MATERIALS, SOUNDS, LOOPS
MARKER = ROOT / 'Saved/GeneratedContent.ok'


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
    map_path = '/Game/Maps/Przebudzenie'
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        if not levels.load_level(map_path):
            raise RuntimeError('Cannot load generated level')
        # Only the generated bootstrap actors are replaced. Runtime architecture is in SliceWorld.
        for actor in actors.get_all_level_actors():
            if actor.get_actor_label().startswith('SliceBootstrap_'):
                actors.destroy_actor(actor)
    elif not levels.new_level(map_path):
        raise RuntimeError('Cannot create Przebudzenie map')
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(250, 400, 456))
    if not start:
        raise RuntimeError('PlayerStart spawn failed')
    start.set_actor_label('SliceBootstrap_PlayerStart')
    # Actor factory creates the bounds volume brush in an editor world.
    nav = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(5600, 3000, 100))
    if not nav:
        raise RuntimeError('Navigation volume spawn failed')
    nav.set_actor_label('SliceBootstrap_Navigation')
    _, extent = nav.get_actor_bounds(False)
    if min(extent.x, extent.y, extent.z) <= 0:
        raise RuntimeError('Navigation brush has no bounds; generation aborted before packaging')
    nav.set_actor_scale3d(unreal.Vector(6000 / extent.x, 3400 / extent.y, 1000 / extent.z))
    if not levels.save_current_level():
        raise RuntimeError('Map save failed')
    unreal.EditorAssetLibrary.save_directory('/Game/Generated', False, True)
    MARKER.parent.mkdir(parents=True, exist_ok=True)
    MARKER.write_text('Przebudzenie content v2\n', encoding='utf-8')
    unreal.log('WROCLAW_CONTENT_READY')


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
