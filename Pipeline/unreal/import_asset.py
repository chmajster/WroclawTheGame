"""Import a Blender pipeline product into Unreal Engine and configure PBR/LOD/Nanite."""
from __future__ import annotations

import json
import os
import re
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
MARKER_ROOT = ROOT / "Saved" / "Pipeline" / "import"


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def clean_name(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]+", "_", value).strip("_")


def import_task(filename: Path, destination: str, static_mesh: bool = False):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    if static_mesh:
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        static_options = options.get_editor_property("static_mesh_import_data")
        static_options.set_editor_property("combine_meshes", True)
        static_options.set_editor_property("generate_lightmap_u_vs", True)
        static_options.set_editor_property("auto_generate_collision", False)
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
    unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    return texture


def texture_sample(edit, material, texture, x, y, parameter_name):
    node = edit.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("texture", texture)
    node.set_editor_property("parameter_name", parameter_name)
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


def configure_lods(base_mesh, generated: Path, mesh_name: str, destination: str, lod_count: int):
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    integrated = [0]
    temp_root = f"/Game/_PipelineTemp/{clean_name(mesh_name)}"
    for index in range(1, lod_count):
        lod_file = generated / f"{mesh_name}_LOD{index}.fbx"
        source = first_asset(import_task(lod_file, temp_root, True), unreal.StaticMesh)
        result = subsystem.set_lod_from_static_mesh(base_mesh, index, source, 0, True)
        if result < 0:
            raise RuntimeError(f"Cannot attach LOD{index} from {lod_file}")
        integrated.append(index)
    if unreal.EditorAssetLibrary.does_directory_exist(temp_root):
        unreal.EditorAssetLibrary.delete_directory(temp_root)
    return integrated


def prepare() -> None:
    manifest_value = os.environ.get("WTG_ASSET_MANIFEST")
    if not manifest_value:
        raise RuntimeError("WTG_ASSET_MANIFEST is not set")
    manifest_path = project_path(manifest_value)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    asset_id = manifest["id"]
    mesh_name = "SM_" + clean_name(asset_id)
    generated = ROOT / "Saved" / "Pipeline" / "generated" / asset_id
    blender_report = json.loads((generated / "blender-report.json").read_text(encoding="utf-8"))

    MARKER_ROOT.mkdir(parents=True, exist_ok=True)
    marker = MARKER_ROOT / f"{asset_id}.ok"
    marker.unlink(missing_ok=True)

    destination = manifest["unreal"]["destination"].rstrip("/")
    base_file = generated / f"{mesh_name}_LOD0.fbx"
    base_mesh = first_asset(import_task(base_file, destination, True), unreal.StaticMesh)

    expected_path = f"{destination}/{mesh_name}"
    current_path = base_mesh.get_path_name().split(".")[0]
    if current_path != expected_path:
        if unreal.EditorAssetLibrary.does_asset_exist(expected_path):
            unreal.EditorAssetLibrary.delete_asset(expected_path)
        if not unreal.EditorAssetLibrary.rename_asset(current_path, expected_path):
            raise RuntimeError(f"Cannot rename {current_path} to {expected_path}")
        base_mesh = unreal.load_asset(expected_path)
    if base_mesh is None:
        raise RuntimeError(f"Cannot load imported base mesh {expected_path}")

    integrated_lods = configure_lods(
        base_mesh,
        generated,
        mesh_name,
        destination,
        len(manifest["lod"]["ratios"]),
    )

    mesh_subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mesh_subsystem.set_generate_lightmap_uv(base_mesh, True)
    nanite = mesh_subsystem.get_nanite_settings(base_mesh)
    nanite.set_editor_property("enabled", bool(manifest["unreal"]["nanite"]))
    mesh_subsystem.set_nanite_settings(base_mesh, nanite, True)

    actual_lod_count = mesh_subsystem.get_lod_count(base_mesh)
    expected_lod_count = len(manifest["lod"]["ratios"])
    if actual_lod_count != expected_lod_count:
        raise RuntimeError(f"Unexpected Unreal LOD count: {actual_lod_count} != {expected_lod_count}")

    collision_count = mesh_subsystem.get_simple_collision_count(base_mesh)
    if manifest["collision"]["mode"] == "simple" and collision_count <= 0:
        hull_count = int(manifest["collision"].get("max_hulls", 1))
        if not mesh_subsystem.set_convex_decomposition_collisions(base_mesh, hull_count, 32, 100000):
            raise RuntimeError("Unreal failed to generate simple collision")
        collision_count = mesh_subsystem.get_simple_collision_count(base_mesh)
    if manifest["collision"]["mode"] == "simple" and collision_count <= 0:
        raise RuntimeError("Simple collision is required but none exists")

    imported_textures = {}
    texture_destination = f"{destination}/Textures"
    for kind, value in manifest["textures"].items():
        if value:
            imported_textures[kind] = import_texture(project_path(value), texture_destination, kind)
    material = create_material(asset_id, destination, imported_textures)
    if material is not None:
        base_mesh.set_material(0, material)

    if not unreal.EditorAssetLibrary.save_loaded_asset(base_mesh, False):
        raise RuntimeError(f"Cannot save {expected_path}")

    report = {
        "asset_id": asset_id,
        "mesh": expected_path,
        "lods": integrated_lods,
        "expected_lods": expected_lod_count,\n        "unreal_lod_count": actual_lod_count,
        "nanite": bool(manifest["unreal"]["nanite"]),
        "textures": {key: value.get_path_name() for key, value in imported_textures.items()},
        "material": material.get_path_name() if material else None,
        "collision": manifest["collision"]["mode"],\n        "collision_count": collision_count,
        "blender_report": blender_report,
        "status": "PASS",
    }
    report_path = MARKER_ROOT / f"{asset_id}.json"
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    marker.write_text("PASS\n", encoding="utf-8")
    unreal.log("WTG_ASSET_IMPORT_READY " + json.dumps(report))


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
finally:
    unreal.SystemLibrary.quit_editor()
