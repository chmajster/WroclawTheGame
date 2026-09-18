"""Run in UnrealEditor-Cmd with -ExecutePythonScript. Does not regenerate maps.

Creates missing assets only: authored replacements and their parameters are preserved.
Catalog validation failures prevent the success marker used by packaging.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = '/Game/CharacterCreator'
MARKER = ROOT / 'Saved/CharacterCreatorReady.ok'
FREE_MODEL_IMPORT_MAP = ROOT / 'Saved/FreeModelImportMap.json'


def prepare():
    MARKER.unlink(missing_ok=True)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    catalog = unreal.load_asset(DEST + '/DA_CharacterAppearanceCatalog')
    if catalog is None:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.CharacterAppearanceCatalog)
        catalog = tools.create_asset('DA_CharacterAppearanceCatalog', DEST,
                                     unreal.CharacterAppearanceCatalog, factory)
        if not catalog:
            raise RuntimeError('Cannot create appearance catalog')
        catalog.build_fallback_catalog()
        unreal.EditorAssetLibrary.save_loaded_asset(catalog, False)

    if not FREE_MODEL_IMPORT_MAP.is_file():
        raise RuntimeError('Free model import map missing; character bodies cannot be resolved')
    imported_models = json.loads(FREE_MODEL_IMPORT_MAP.read_text(encoding='utf-8'))
    body_sources = {
        'Male.Standard': 'quaternius-ubc-superhero-male',
        'Female.Standard': 'quaternius-ubc-superhero-female',
    }
    bodies = list(catalog.get_editor_property('bodies'))
    for body in bodies:
        body_id = str(body.get_editor_property('id'))
        model_id = body_sources.get(body_id)
        if not model_id:
            continue
        metadata = imported_models.get(model_id)
        object_path = metadata.get('primary_object') if metadata else None
        mesh = unreal.load_asset(object_path) if object_path else None
        if not mesh or not isinstance(mesh, unreal.SkeletalMesh):
            raise RuntimeError(f'Character body is not a SkeletalMesh: {model_id} -> {object_path}')
        body.set_editor_property('mesh', mesh)
        body.set_editor_property('mesh_rotation', unreal.Rotator(0, -90, 0))
    catalog.set_editor_property('bodies', bodies)
    unreal.EditorAssetLibrary.save_loaded_asset(catalog, False)

    material = unreal.load_asset(DEST + '/M_CharacterPlaceholder')
    if material is None:
        material = tools.create_asset('M_CharacterPlaceholder', DEST, unreal.Material,
                                      unreal.MaterialFactoryNew())
        edit = unreal.MaterialEditingLibrary
        color = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter)
        color.set_editor_property('parameter_name', 'BaseColor')
        color.set_editor_property('default_value', unreal.LinearColor(.4, .3, .2, 1))
        edit.connect_material_property(color, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = edit.create_material_expression(material, unreal.MaterialExpressionScalarParameter)
        roughness.set_editor_property('parameter_name', 'Roughness')
        roughness.set_editor_property('default_value', .5)
        edit.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
        edit.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material, False)

    preview = unreal.load_asset(DEST + '/M_CharacterPreview')
    if preview is None:
        preview = tools.create_asset('M_CharacterPreview', DEST, unreal.Material,
                                    unreal.MaterialFactoryNew())
        preview.set_editor_property('material_domain', unreal.MaterialDomain.MD_UI)
        preview.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
        edit = unreal.MaterialEditingLibrary
        texture = edit.create_material_expression(preview, unreal.MaterialExpressionTextureSampleParameter2D)
        texture.set_editor_property('parameter_name', 'PreviewTexture')
        texture.set_editor_property('texture', unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
        edit.connect_material_property(texture, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        edit.recompile_material(preview)
        unreal.EditorAssetLibrary.save_loaded_asset(preview, False)

    for name, parent, widget in [
        ('BP_WTG_PlayerCharacter', unreal.SliceCharacter, False),
        ('BP_WTG_CharacterCreator', unreal.WTG_CharacterCreator, False),
        ('WBP_CharacterCreator', unreal.CharacterCreatorWidget, True),
    ]:
        asset = unreal.load_asset(DEST + '/' + name)
        if asset is None:
            factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
            factory.set_editor_property('parent_class', parent)
            asset = tools.create_asset(name, DEST, None, factory)
        if not asset:
            raise RuntimeError('Cannot create ' + name)
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
            raise RuntimeError('Cannot save ' + name)

    errors = unreal.CharacterCreatorValidator.validate_catalog(catalog, True)
    if errors:
        raise RuntimeError('\n'.join(errors))
    MARKER.write_text('Character Creator catalog and Blueprint assets validated.\n', encoding='utf-8')
    unreal.log('WTG_CHARACTER_CREATOR_READY')


if __name__ == '__main__':
    prepare()
