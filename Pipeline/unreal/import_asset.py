"""Import a Blender pipeline product into Unreal Engine and configure PBR/LOD/Nanite."""
from __future__ import annotations

import json
import os
import re
import traceback
import uuid
import sys
import time
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
MARKER_ROOT = ROOT / "Saved" / "Pipeline" / "import"


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def clean_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_")


def import_task(filename: Path, destination: str, static_mesh: bool = False, name=None, explicit_tangents=False):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination)
    if name: task.set_editor_property("destination_name",name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    if static_mesh:
        if name:
            existing=unreal.load_asset(f"{destination}/{name}")
            if isinstance(existing,unreal.StaticMesh):
                data=existing.get_editor_property("asset_import_data")
                if isinstance(data,unreal.FbxStaticMeshImportData):
                    data.set_editor_property("convert_scene_unit",True)
                    data.set_editor_property("convert_scene",True)
                    data.set_editor_property("import_uniform_scale",1.)
        # The pipeline supplies FbxImportUI and UCX meshes. UE 5.8's default
        # Interchange importer ignores these legacy options; select their factory.
        task.set_editor_property("factory", unreal.FbxFactory())
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        static_options = options.get_editor_property("static_mesh_import_data")
        static_options.set_editor_property("combine_meshes", True)
        # Blender's metre scene is encoded in FBX unit metadata. Without this
        # UE treats metre coordinates as centimetres and produces 1:100 props.
        static_options.set_editor_property("convert_scene_unit", True)
        static_options.set_editor_property("generate_lightmap_u_vs", True)
        static_options.set_editor_property("auto_generate_collision", False)
        # Blender exports explicit per-corner normals and tangent space.
        # Preserve it instead of averaging opposing UV directions at shared
        # architectural seams (observed zero tangents in a real native buffer).
        method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS if explicit_tangents else unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        static_options.set_editor_property("normal_import_method",method)
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths") or [])
    if not imported:
        raise RuntimeError(f"Import failed: {filename}")
    return imported


def first_asset(paths, asset_type):
    for path in paths:
        asset = unreal.load_asset(path)
        if isinstance(asset, asset_type):
            return asset
    raise RuntimeError(f"Imported objects contain no {asset_type.__name__}: {paths}")


def import_texture(path: Path, destination: str, kind: str):
    texture = first_asset(import_task(path, destination, False), unreal.Texture2D)
    if kind == "normal":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif kind == "orm":
        texture.set_editor_property("srgb", False)
        texture.set_editor_property("compression_settings",unreal.TextureCompressionSettings.TC_MASKS)
    else:
        texture.set_editor_property("srgb",True)
    texture.set_editor_property("never_stream",False)
    texture.set_editor_property("max_texture_size",2048)
    texture.set_editor_property("mip_gen_settings",unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    return texture


def texture_sample(edit, material, texture, x, y, parameter_name):
    node = edit.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("texture", texture)
    node.set_editor_property("parameter_name", parameter_name)
    if parameter_name=="Normal": node.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif parameter_name=="ORM": node.set_editor_property("sampler_type",unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    return node


def create_material(asset_id: str, destination: str, textures: dict):
    if not textures:
        return None
    material_name = f"M_{clean_name(asset_id)}"
    material_path = f"{destination}/{material_name}"
    material = unreal.load_asset(material_path)
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            material_name, destination, unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        raise RuntimeError(f"Cannot create material: {material_path}")

    edit = unreal.MaterialEditingLibrary
    edit.delete_all_material_expressions(material)

    if textures.get("base_color"):
        node = texture_sample(edit, material, textures["base_color"], -600, -120, "BaseColor")
        edit.connect_material_property(node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    if textures.get("normal"):
        node = texture_sample(edit, material, textures["normal"], -600, 80, "Normal")
        edit.connect_material_property(node, "RGB", unreal.MaterialProperty.MP_NORMAL)
    if textures.get("orm"):
        node = texture_sample(edit, material, textures["orm"], -600, 300, "ORM")
        edit.connect_material_property(node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        edit.connect_material_property(node, "G", unreal.MaterialProperty.MP_ROUGHNESS)
        edit.connect_material_property(node, "B", unreal.MaterialProperty.MP_METALLIC)

    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError(f"Cannot save material: {material_path}")
    return material


def configure_lods(base_mesh, generated: Path, mesh_name: str, destination: str, lod_count: int, explicit_tangents=False):
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    integrated = [0]
    temp_root = f"/Game/_PipelineTemp/{clean_name(mesh_name)}"
    for index in range(1, lod_count):
        lod_file = generated / f"{mesh_name}_LOD{index}.fbx"
        source = first_asset(import_task(lod_file, temp_root, True,explicit_tangents=explicit_tangents), unreal.StaticMesh)
        # Match the render source's slot order using the same interfaces before
        # attaching a LOD. Reimport retains authored materials on the base while
        # temporary FBX meshes get defaults; UE would append duplicate slots.
        source_slots=list(source.get_editor_property("static_materials"))
        base_slots=list(base_mesh.get_editor_property("static_materials"))
        # Decimation can remove every polygon of a small handle/material. FBX
        # then omits that slot; match semantic names instead of array indices.
        def slot_name(slot):
            return str(slot.get_editor_property('imported_material_slot_name') or slot.get_editor_property('material_slot_name'))
        base_by_name={slot_name(slot):slot for slot in base_slots}
        for slot in source_slots:
            name=slot_name(slot)
            if name not in base_by_name: raise RuntimeError('LOD contains unknown material slot '+name)
            slot.set_editor_property('material_interface',base_by_name[name].get_editor_property('material_interface'))
        source.set_editor_property("static_materials",source_slots)
        result = subsystem.set_lod_from_static_mesh(base_mesh, index, source, 0, True)
        if result < 0:
            raise RuntimeError(f"Cannot attach LOD{index} from {lod_file}")
        integrated.append(index)
    if unreal.EditorAssetLibrary.does_directory_exist(temp_root):
        unreal.EditorAssetLibrary.delete_directory(temp_root)
    return integrated


def prepare(manifest_value=None) -> None:
    manifest_value = manifest_value or os.environ.get("WTG_ASSET_MANIFEST")
    if not manifest_value:
        raise RuntimeError("WTG_ASSET_MANIFEST is not set")
    manifest_path = project_path(manifest_value)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest['type']=='skeletal_mesh':
        import importlib.util
        spec=importlib.util.spec_from_file_location('wtg_skeletal_import',project_path(manifest['skeletal']['importer']))
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        return module.prepare(manifest)
    asset_id = manifest["id"]
    mesh_name = "SM_" + clean_name(asset_id)
    generated = ROOT / "Saved" / "Pipeline" / "generated" / asset_id
    blender_report = json.loads((generated / "blender-report.json").read_text(encoding="utf-8"))

    MARKER_ROOT.mkdir(parents=True, exist_ok=True)
    marker = MARKER_ROOT / f"{asset_id}.ok"
    marker.unlink(missing_ok=True)

    destination = manifest["unreal"]["destination"].rstrip("/")
    base_file = generated / f"{mesh_name}_LOD0.fbx"
    # The legacy FBX reimport path in UE 5.8 can ignore metre conversion even
    # with replace_existing_settings and stored convert_scene_unit enabled.
    # Build a fresh pipeline product in isolation, then replace the manifest's
    # stable package path. Never depend on old per-user FBX import state.
    stage=f"/Game/_PipelineTemp/Import_{clean_name(asset_id)}_{uuid.uuid4().hex}"
    explicit_tangents=blender_report.get('tangent_space')=='explicit'
    base_mesh = first_asset(import_task(base_file, stage, True, mesh_name,explicit_tangents), unreal.StaticMesh)

    expected_path = f"{destination}/{mesh_name}"
    current_path = base_mesh.get_path_name().split(".")[0]
    if current_path != expected_path:
        if unreal.EditorAssetLibrary.does_asset_exist(expected_path):
            unreal.EditorAssetLibrary.delete_asset(expected_path)
        if not unreal.EditorAssetLibrary.rename_asset(current_path, expected_path):
            raise RuntimeError(f"Cannot rename {current_path} to {expected_path}")
        base_mesh = unreal.load_asset(expected_path)
    if unreal.EditorAssetLibrary.does_directory_exist(stage):
        unreal.EditorAssetLibrary.delete_directory(stage)
    if base_mesh is None:
        raise RuntimeError(f"Cannot load imported base mesh {expected_path}")
    expected_slots=blender_report.get("material_slots")
    if expected_slots:
        slots=list(base_mesh.get_editor_property("static_materials"))
        if len(slots)<expected_slots: raise RuntimeError("Imported material slots missing")
        if len(slots)>expected_slots:
            base_mesh.set_editor_property("static_materials",slots[:expected_slots])

    mesh_subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mesh_subsystem.set_generate_lightmap_uv(base_mesh, True)
    nanite = mesh_subsystem.get_nanite_settings(base_mesh)
    nanite.set_editor_property("enabled", bool(manifest["unreal"]["nanite"]))
    mesh_subsystem.set_nanite_settings(base_mesh, nanite, True)

    # UE reports spheres/boxes/capsules separately from convex UCX hulls.
    def collision_shapes():
        return (mesh_subsystem.get_simple_collision_count(base_mesh)
                + mesh_subsystem.get_convex_collision_count(base_mesh))
    collision_count = collision_shapes()
    if manifest["collision"]["mode"] == "none" and collision_count:
        mesh_subsystem.remove_collisions(base_mesh)
        collision_count=collision_shapes()
    if manifest["collision"]["mode"] == "simple" and collision_count <= 0:
        hull_count = int(manifest["collision"].get("max_hulls", 1))
        if not mesh_subsystem.set_convex_decomposition_collisions(base_mesh, hull_count, 32, 100000):
            raise RuntimeError("Unreal failed to generate simple collision")
        collision_count = collision_shapes()
    if manifest["collision"]["mode"] == "simple" and collision_count <= 0:
        raise RuntimeError("Simple collision is required but none exists")
    if manifest["collision"]["mode"] == "authored" and collision_count!=blender_report.get("collision_shapes"):
        raise RuntimeError("Authored UCX collision shapes were not preserved")

    imported_textures = {}
    texture_destination = f"{destination}/Textures"
    for kind, value in manifest["textures"].items():
        if value:
            imported_textures[kind] = import_texture(project_path(value), texture_destination, kind)
    material = create_material(asset_id, destination, imported_textures)
    if material is not None:
        base_mesh.set_material(0, material)
    bindings={binding["slot"]:binding["asset"] for binding in manifest.get("materials",[])}
    slot_count=len(base_mesh.get_editor_property("static_materials"))
    if any(slot>=slot_count for slot in bindings): raise RuntimeError("Material binding slot does not exist")
    for slot,path in bindings.items():
        authored_material=unreal.load_asset(path)
        if not isinstance(authored_material,unreal.MaterialInterface): raise RuntimeError("Missing material "+path)
        base_mesh.set_material(slot,authored_material)
    if imported_textures and any(slot not in bindings for slot in range(1,slot_count)):
        raise RuntimeError("Multi-material mesh requires explicit bindings for every remaining slot")

    # Set real, distinct interfaces before UE's LOD copy. Its implementation
    # matches by interface identity; assigning later collapses all default-grid
    # sections onto slot zero even when source material names are different.
    integrated_lods=configure_lods(base_mesh,generated,mesh_name,destination,len(manifest['lod']['ratios']),explicit_tangents)
    actual_lod_count=mesh_subsystem.get_lod_count(base_mesh)
    expected_lod_count=len(manifest['lod']['ratios'])
    if actual_lod_count!=expected_lod_count:raise RuntimeError('Unexpected Unreal LOD count')

    sys.path.insert(0,str(ROOT/"Pipeline/unreal"))
    from city_quality import configure_mesh
    configure_mesh(base_mesh,manifest)
    if not unreal.EditorAssetLibrary.save_loaded_asset(base_mesh, False):
        raise RuntimeError(f"Cannot save {expected_path}")

    bounds=base_mesh.get_bounds()
    actual_height_cm=bounds.box_extent.z*2
    expected_bounds=blender_report.get("bounds_m")
    actual_bounds=[bounds.box_extent.x*2,bounds.box_extent.y*2,actual_height_cm]
    if expected_bounds and any(abs(actual-expected*100)>max(.5,expected) for actual,expected in zip(actual_bounds,expected_bounds)):
        raise RuntimeError(f"Imported bounds/scale mismatch: {actual_bounds} cm != {expected_bounds} m")
    expected_height_cm=manifest.get('dimensions',{}).get('height_m',0)*100
    if expected_height_cm and not expected_height_cm*.8<=actual_height_cm<=expected_height_cm*1.2:
        raise RuntimeError(f'Imported asset scale mismatch: height {actual_height_cm:.2f} cm, expected {expected_height_cm:.2f} cm')
    report = {
        "height_cm":actual_height_cm,
        "bounds_cm": actual_bounds,
        "material_slots": slot_count,
        "material_bindings": bindings,
        "asset_id": asset_id,
        "mesh": expected_path,
        "lods": integrated_lods,
        "expected_lods": expected_lod_count,
        "unreal_lod_count": actual_lod_count,
        "nanite": bool(mesh_subsystem.get_nanite_settings(base_mesh).get_editor_property("enabled")),
        "textures": {key: value.get_path_name() for key, value in imported_textures.items()},
        "material": material.get_path_name() if material else None,
        "collision": manifest["collision"]["mode"],
        "collision_count": collision_count,
        "blender_report": blender_report,
        "status": "PASS",
    }
    report_path = MARKER_ROOT / f"{asset_id}.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    marker.write_text("PASS\n", encoding="utf-8")
    unreal.log("WTG_ASSET_IMPORT_READY " + json.dumps(report))


def quit_after_editor_startup():
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    ticks=[0]
    started=time.monotonic()
    def settle(delta):
        ticks[0]+=1
        # NullRHI can tick 30 frames in a fraction of a second. Give editor
        # map/tool teardown a real settling interval before command-line exit.
        if ticks[0]>=30 and time.monotonic()-started>=3:
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(settle)


if __name__ == '__main__':
    try:
        prepare()
    except Exception:
        unreal.log_error(traceback.format_exc())
        raise
    finally:
        quit_after_editor_startup()
