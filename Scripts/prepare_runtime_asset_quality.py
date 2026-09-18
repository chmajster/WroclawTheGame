"""Normalize imported free models and emit a strict Unreal-side quality report.

Runs inside UnrealEditor-Cmd after Scripts/import_free_models.py.
It repairs generated LODs/collision/material gaps when Unreal supports doing so,
then fails the marker if the postconditions are not met.
"""
from __future__ import annotations

import json
import math
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
POLICY = ROOT / "Data/runtime_asset_qa.json"
IMPORT_MAP = ROOT / "Saved/FreeModelImportMap.json"
CHARACTER_CATALOG = ROOT / "Data/free_character_catalog.json"
OUT = ROOT / "Saved/RuntimeAssetQA/model_quality.json"
MARKER = ROOT / "Saved/RuntimeAssetQualityReady.ok"
DEST = "/Game/RuntimeAssetQA"


def vec3(value):
    return [float(value.x), float(value.y), float(value.z)]


def dimensions_from_bounds(bounds):
    extent = bounds.box_extent
    return [abs(float(extent.x)) * 2.0, abs(float(extent.y)) * 2.0, abs(float(extent.z)) * 2.0]


def finite_positive(values):
    return all(math.isfinite(v) and v > 0 for v in values)


def select_lod_rule(policy, vertices):
    selected = policy["static_mesh"]["lod_rules"][0]
    for rule in policy["static_mesh"]["lod_rules"]:
        if vertices >= int(rule["min_vertices"]):
            selected = rule
    return selected


def make_lod_options(rule):
    ratios = list(rule["ratios"])
    screens = list(rule["screen_sizes"])
    if len(ratios) != len(screens):
        raise RuntimeError("LOD policy ratios/screen_sizes length mismatch")

    # UE 5.8 exposes StaticMeshReductionOptions; keep the documented legacy
    # editor scripting structs as a compatibility fallback for minor API drift.
    options_type = getattr(unreal, "StaticMeshReductionOptions", None)
    settings_type = getattr(unreal, "StaticMeshReductionSettings", None)
    if options_type and settings_type:
        options = options_type()
        settings = []
        for ratio, screen in zip(ratios, screens):
            item = settings_type()
            item.set_editor_property("percent_triangles", float(ratio))
            item.set_editor_property("screen_size", float(screen))
            settings.append(item)
        options.set_editor_property("auto_compute_lod_screen_size", False)
        options.set_editor_property("reduction_settings", settings)
        return options, "subsystem"

    options_type = getattr(unreal, "EditorScriptingMeshReductionOptions", None)
    settings_type = getattr(unreal, "EditorScriptingMeshReductionSettings", None)
    if not options_type or not settings_type:
        raise RuntimeError("No supported Unreal static-mesh LOD scripting API is available")
    options = options_type()
    options.set_editor_property(
        "reduction_settings",
        [settings_type(float(screen), float(ratio)) for ratio, screen in zip(ratios, screens)],
    )
    options.set_editor_property("auto_compute_lod_screen_size", False)
    return options, "legacy"


def collision_box_enum():
    enum = getattr(unreal, "ScriptCollisionShapeType", None)
    if enum is None:
        enum = getattr(unreal, "ScriptingCollisionShapeType", None)
    if enum is None:
        raise RuntimeError("Unreal collision shape enum is unavailable")
    return enum.BOX


def make_fallback_material():
    path = DEST + "/M_RuntimeAssetFallback"
    material = unreal.load_asset(path)
    if material:
        return material
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset("M_RuntimeAssetFallback", DEST, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Cannot create runtime fallback material")
    edit = unreal.MaterialEditingLibrary
    color = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property("constant", unreal.LinearColor(0.18, 0.18, 0.18, 1.0))
    edit.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
    rough.set_editor_property("r", 0.55)
    edit.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, False):
        raise RuntimeError("Cannot save runtime fallback material")
    return material


def static_materials(mesh, subsystem):
    count = int(subsystem.get_number_materials(mesh))
    values = []
    for index in range(count):
        try:
            values.append(mesh.get_material(index))
        except Exception:
            values.append(None)
    return values


def repair_static_material(mesh, subsystem, fallback):
    materials = static_materials(mesh, subsystem)
    if not materials:
        mesh.set_material(0, fallback)
        return True
    changed = False
    for index, material in enumerate(materials):
        if material is None:
            mesh.set_material(index, fallback)
            changed = True
    return changed


def skeletal_lod_count(mesh):
    method = getattr(mesh, "get_lod_num", None)
    if method:
        return int(method())
    return len(list(mesh.get_editor_property("lod_info")))


def skeletal_materials(mesh):
    items = list(mesh.get_editor_property("materials"))
    result = []
    for item in items:
        try:
            result.append(item.get_editor_property("material_interface"))
        except Exception:
            result.append(None)
    return result


def repair_skeletal_material(mesh, fallback):
    items = list(mesh.get_editor_property("materials"))
    if not items:
        material_type = getattr(unreal, "SkeletalMaterial", None)
        if material_type is None:
            return False
        item = material_type()
        item.set_editor_property("material_interface", fallback)
        mesh.set_editor_property("materials", [item])
        return True
    changed = False
    for item in items:
        if item.get_editor_property("material_interface") is None:
            item.set_editor_property("material_interface", fallback)
            changed = True
    if changed:
        mesh.set_editor_property("materials", items)
    return changed


def category_by_model_id():
    result = {}
    if CHARACTER_CATALOG.is_file():
        for item in json.loads(CHARACTER_CATALOG.read_text(encoding="utf-8")):
            result[item["id"]] = item.get("category")
    return result


def validate_dimensions(model_id, dims, minimum, maximum, errors):
    if not finite_positive(dims):
        errors.append(f"{model_id}: invalid bounds {dims}")
        return
    if min(dims) < minimum:
        errors.append(f"{model_id}: dimension below {minimum} cm: {dims}")
    if max(dims) > maximum:
        errors.append(f"{model_id}: dimension above {maximum} cm: {dims}")


def process_static(model_id, mesh, policy, fallback, static_subsystem):
    errors = []
    changed = []
    dims = dimensions_from_bounds(mesh.get_bounds())
    cfg = policy["static_mesh"]
    validate_dimensions(model_id, dims, float(cfg["min_dimension_cm"]), float(cfg["max_dimension_cm"]), errors)

    vertices = int(static_subsystem.get_number_verts(mesh, 0))
    if vertices <= 0:
        errors.append(f"{model_id}: LOD0 has no vertices")

    rule = select_lod_rule(policy, vertices)
    required_lods = int(rule["required_lods"])
    before_lods = int(static_subsystem.get_lod_count(mesh))
    if before_lods < required_lods:
        options, mode = make_lod_options(rule)
        if mode == "subsystem":
            result = int(static_subsystem.set_lods(mesh, options))
        else:
            result = int(unreal.EditorStaticMeshLibrary.set_lods(mesh, options))
        if result < required_lods:
            errors.append(f"{model_id}: failed to generate {required_lods} LODs, got {result}")
        else:
            changed.append(f"generated_lods:{result}")

    static_subsystem.set_generate_lightmap_uv(mesh, True)

    collision_before = int(static_subsystem.get_simple_collision_count(mesh))
    if bool(cfg["require_simple_collision"]) and collision_before <= 0:
        index = int(static_subsystem.add_simple_collisions(mesh, collision_box_enum()))
        if index < 0:
            errors.append(f"{model_id}: failed to add simple box collision")
        else:
            changed.append("generated_simple_collision")
    collision_after = int(static_subsystem.get_simple_collision_count(mesh))
    if bool(cfg["require_simple_collision"]) and collision_after <= 0:
        errors.append(f"{model_id}: no simple collision after repair")
    if collision_after > int(cfg["max_simple_collisions"]):
        errors.append(f"{model_id}: excessive simple collision count {collision_after}")

    material_repaired = repair_static_material(mesh, static_subsystem, fallback)
    if material_repaired:
        changed.append("repaired_material_slots")
    materials = static_materials(mesh, static_subsystem)
    if bool(cfg["require_material"]) and (not materials or any(value is None for value in materials)):
        errors.append(f"{model_id}: missing material slot")

    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
        errors.append(f"{model_id}: failed to save mesh")

    after_lods = int(static_subsystem.get_lod_count(mesh))
    return {
        "type": "StaticMesh",
        "path": mesh.get_path_name(),
        "dimensions_cm": dims,
        "vertices_lod0": vertices,
        "lods_before": before_lods,
        "lods_after": after_lods,
        "required_lods": required_lods,
        "collision_before": collision_before,
        "collision_after": collision_after,
        "materials": len(materials),
        "changes": changed,
        "errors": errors,
    }


def process_skeletal(model_id, mesh, category, policy, fallback, skeletal_subsystem):
    errors = []
    changed = []
    cfg = policy["skeletal_mesh"]
    dims = dimensions_from_bounds(mesh.get_bounds())

    max_dim = float(cfg["max_dimension_cm"])
    validate_dimensions(model_id, dims, 0.1, max_dim, errors)
    if category in set(cfg.get("height_limits_for_categories", [])):
        height = dims[2]
        if height < float(cfg["min_height_cm"]) or height > float(cfg["max_height_cm"]):
            errors.append(
                f"{model_id}: body height {height:.2f} cm outside "
                f"{cfg['min_height_cm']}..{cfg['max_height_cm']}"
            )

    skeleton = mesh.get_editor_property("skeleton")
    if bool(cfg["require_skeleton"]) and skeleton is None:
        errors.append(f"{model_id}: missing skeleton")

    before_lods = skeletal_lod_count(mesh)
    required_lods = int(cfg["required_lods"])
    if before_lods < required_lods:
        if not unreal.SkeletalMeshEditorSubsystem.regenerate_lod(mesh, required_lods, False, False):
            errors.append(f"{model_id}: failed to regenerate {required_lods} skeletal LODs")
        else:
            changed.append(f"generated_skeletal_lods:{required_lods}")
    after_lods = skeletal_lod_count(mesh)
    if after_lods < required_lods:
        errors.append(f"{model_id}: only {after_lods} skeletal LODs after repair")

    if repair_skeletal_material(mesh, fallback):
        changed.append("repaired_material_slots")
    materials = skeletal_materials(mesh)
    if bool(cfg["require_material"]) and (not materials or any(value is None for value in materials)):
        errors.append(f"{model_id}: missing skeletal material slot")

    physics_asset = mesh.get_editor_property("physics_asset")
    needs_physics = category in set(cfg.get("require_physics_asset_for_categories", []))
    if needs_physics and physics_asset is None:
        physics_asset = unreal.SkeletalMeshEditorSubsystem.create_physics_asset(mesh, True, 0)
        if physics_asset is None:
            errors.append(f"{model_id}: failed to create physics asset")
        else:
            changed.append("generated_physics_asset")
    if needs_physics and mesh.get_editor_property("physics_asset") is None:
        errors.append(f"{model_id}: body still has no physics asset")

    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
        errors.append(f"{model_id}: failed to save skeletal mesh")

    vertices = int(skeletal_subsystem.get_num_verts(mesh, 0))
    if vertices <= 0:
        errors.append(f"{model_id}: skeletal LOD0 has no vertices")

    return {
        "type": "SkeletalMesh",
        "category": category,
        "path": mesh.get_path_name(),
        "dimensions_cm": dims,
        "vertices_lod0": vertices,
        "lods_before": before_lods,
        "lods_after": after_lods,
        "required_lods": required_lods,
        "materials": len(materials),
        "physics_asset": mesh.get_editor_property("physics_asset").get_path_name()
        if mesh.get_editor_property("physics_asset")
        else None,
        "changes": changed,
        "errors": errors,
    }


def run():
    MARKER.unlink(missing_ok=True)
    OUT.unlink(missing_ok=True)
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    imported = json.loads(IMPORT_MAP.read_text(encoding="utf-8"))
    categories = category_by_model_id()

    fallback = make_fallback_material()
    static_subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    skeletal_subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)

    assets = {}
    errors = []
    for model_id, metadata in sorted(imported.items()):
        object_path = metadata.get("primary_object")
        asset = unreal.load_asset(object_path) if object_path else None
        if isinstance(asset, unreal.StaticMesh):
            result = process_static(model_id, asset, policy, fallback, static_subsystem)
        elif isinstance(asset, unreal.SkeletalMesh):
            result = process_skeletal(
                model_id, asset, categories.get(model_id), policy, fallback, skeletal_subsystem
            )
        else:
            result = {
                "type": type(asset).__name__ if asset else None,
                "path": object_path,
                "errors": [f"{model_id}: primary object is not a mesh"],
                "changes": [],
            }
        assets[model_id] = result
        errors.extend(result["errors"])

    report = {
        "status": "PASS" if not errors else "FAIL",
        "asset_count": len(assets),
        "errors": errors,
        "assets": assets,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    if errors:
        raise RuntimeError("Runtime model quality failed:\n" + "\n".join(errors))
    MARKER.write_text(f"PASS {len(assets)} meshes\n", encoding="utf-8")
    unreal.log(f"WTG_RUNTIME_ASSET_QUALITY_PASS {len(assets)}")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
