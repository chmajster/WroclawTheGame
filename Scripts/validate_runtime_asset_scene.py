"""Validate model transforms and bindings on the generated campaign map inside Unreal."""
from __future__ import annotations

import json
import math
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
POLICY = ROOT / "Data/runtime_asset_qa.json"
CHAPTER = ROOT / "Data/chapter1.json"
BINDINGS = ROOT / "Data/model_bindings.json"
CHARACTERS = ROOT / "Data/free_character_catalog.json"
MODEL_REPORT = ROOT / "Saved/RuntimeAssetQA/model_quality.json"
RETARGET_REPORT = ROOT / "Saved/RuntimeAssetQA/retarget.json"
OUT = ROOT / "Saved/RuntimeAssetQA/scene.json"
MARKER = ROOT / "Saved/RuntimeAssetSceneReady.ok"
MAP = "/Game/Maps/Przebudzenie_Source"


def values3(vector):
    return [float(vector.x), float(vector.y), float(vector.z)]


def finite(values):
    return all(math.isfinite(value) for value in values)


def check_scale(label, vector, policy, errors, context):
    values = [abs(value) for value in values3(vector)]
    minimum = float(policy["transform"]["min_abs_scale"])
    maximum = float(policy["transform"]["max_abs_scale"])
    if not finite(values) or any(value < minimum or value > maximum for value in values):
        errors.append(f"{label}: invalid {context} scale {values}")
        return
    ratio = max(values) / max(min(values), minimum)
    if ratio > float(policy["transform"]["max_nonuniform_ratio"]):
        errors.append(f"{label}: excessive non-uniform {context} scale ratio {ratio:.3f}")


def check_rotation(label, rotation, policy, errors):
    values = [float(rotation.roll), float(rotation.pitch), float(rotation.yaw)]
    maximum = float(policy["transform"]["max_abs_rotation_degrees"])
    if not finite(values) or any(abs(value) > maximum for value in values):
        errors.append(f"{label}: invalid actor rotation {values}")


def component_mesh(component):
    if isinstance(component, unreal.StaticMeshComponent):
        return component.get_editor_property("static_mesh")
    if isinstance(component, unreal.SkeletalMeshComponent):
        return component.get_skeletal_mesh_asset()
    return None


def mesh_dimensions(mesh, scale):
    bounds = mesh.get_bounds()
    extent = bounds.box_extent
    raw = [abs(float(extent.x))*2, abs(float(extent.y))*2, abs(float(extent.z))*2]
    values = values3(scale)
    return [raw[index] * abs(values[index]) for index in range(3)]


def component_material_errors(label, component):
    missing = []
    for index in range(int(component.get_num_materials())):
        if component.get_material(index) is None:
            missing.append(index)
    return [f"{label}: missing runtime material slot {index}" for index in missing]


def actor_visuals(actor):
    result = []
    result.extend(actor.get_components_by_class(unreal.StaticMeshComponent))
    result.extend(actor.get_components_by_class(unreal.SkeletalMeshComponent))
    return [component for component in result if component_mesh(component) is not None]


def relevant_label(label):
    return label.startswith((
        "SliceBootstrap_action_",
        "SliceBootstrap_action_visual_",
        "SliceBootstrap_guard_",
        "SliceBootstrap_hide_",
        "SliceBootstrap_camera_",
        "SliceBootstrap_monitor_",
        "SliceBootstrap_npc_",
        "SliceBootstrap_environment_model_",
        "SliceBootstrap_environment_lamp_",
    ))


def run():
    MARKER.unlink(missing_ok=True)
    OUT.unlink(missing_ok=True)
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    chapter = json.loads(CHAPTER.read_text(encoding="utf-8"))
    bindings = json.loads(BINDINGS.read_text(encoding="utf-8"))
    character_ids = {
        item["id"] for item in json.loads(CHARACTERS.read_text(encoding="utf-8"))
        if item.get("category") == "body"
    }

    model_report = json.loads(MODEL_REPORT.read_text(encoding="utf-8"))
    retarget_report = json.loads(RETARGET_REPORT.read_text(encoding="utf-8"))
    errors = []
    if model_report.get("status") != "PASS":
        errors.append("runtime model-quality report is not PASS")
    if retarget_report.get("status") != "PASS":
        errors.append("animation retarget report is not PASS")

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level(MAP):
        raise RuntimeError(f"Cannot load QA map {MAP}")
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(actor_subsystem.get_all_level_actors())
    by_label = {actor.get_actor_label(): actor for actor in actors}

    checked = {}
    for label, actor in sorted(by_label.items()):
        if not relevant_label(label):
            continue
        check_scale(label, actor.get_actor_scale3d(), policy, errors, "actor")
        check_rotation(label, actor.get_actor_rotation(), policy, errors)
        visuals = actor_visuals(actor)
        checked[label] = []
        for component in visuals:
            mesh = component_mesh(component)
            relative_scale = component.get_editor_property("relative_scale3d")
            check_scale(label, relative_scale, policy, errors, "component")
            dims = mesh_dimensions(mesh, relative_scale)
            if not finite(dims) or min(dims) <= 0:
                errors.append(f"{label}: invalid visual dimensions {dims}")
            if max(dims) > float(policy["static_mesh"]["max_dimension_cm"]):
                errors.append(f"{label}: visual dimensions exceed scene policy {dims}")
            errors.extend(component_material_errors(label, component))
            checked[label].append({
                "component": component.get_name(),
                "mesh": mesh.get_path_name(),
                "dimensions_cm": dims,
                "materials": int(component.get_num_materials()),
            })

    physical = [
        action for action in chapter["actions"]
        if action["kind"] not in ("virtual", "zone")
        and action.get("position", [0, 0])[:2] != [0, 0]
    ]
    for action in physical:
        action_id = action["id"]
        actor_label = "SliceBootstrap_action_" + action_id
        visual_label = "SliceBootstrap_action_visual_" + action_id
        actor = by_label.get(actor_label)
        if actor is None:
            errors.append(f"{action_id}: generated action actor is missing")
            continue
        model_id = bindings["actions"].get(action_id)
        if not model_id:
            errors.append(f"{action_id}: model binding is missing")
            continue
        if model_id in character_ids:
            visual = by_label.get(visual_label)
            if visual is None or not actor_visuals(visual):
                errors.append(f"{action_id}: skeletal action visual is missing")
        elif not actor_visuals(actor):
            errors.append(f"{action_id}: static action visual is missing")

    system_expectations = {
        "guards": ("SliceBootstrap_guard_", len(json.loads((ROOT/"Data/openworld.json").read_text(encoding="utf-8"))["guards"])),
        "npcs": ("SliceBootstrap_npc_", len(json.loads((ROOT/"Data/openworld.json").read_text(encoding="utf-8"))["npc"])),
        "hides": ("SliceBootstrap_hide_", len(json.loads((ROOT/"Data/openworld.json").read_text(encoding="utf-8"))["hides"])),
        "monitors": ("SliceBootstrap_monitor_", len(json.loads((ROOT/"Data/openworld.json").read_text(encoding="utf-8"))["cameras"])),
    }
    counts = {}
    for name, (prefix, expected) in system_expectations.items():
        count = sum(1 for label in by_label if label.startswith(prefix))
        counts[name] = {"expected": expected, "actual": count}
        if count != expected:
            errors.append(f"{name}: generated actor count {count} != {expected}")

    retargeted = retarget_report.get("retargeted_bindings", {})
    required_retarget = [
        key for key, value in json.loads((ROOT/"Data/animation_bindings.json").read_text(encoding="utf-8"))["bindings"].items()
        if value.get("requires_retarget")
    ]
    for semantic in required_retarget:
        record = retargeted.get(semantic)
        if not record:
            errors.append(f"{semantic}: retarget QA mapping is missing")
            continue
        for variant in ("male", "female"):
            path = record.get("targets", {}).get(variant)
            asset = unreal.load_asset(path) if path else None
            if not isinstance(asset, unreal.AnimationAsset):
                errors.append(f"{semantic}/{variant}: retargeted AnimationAsset is missing")

    report = {
        "status": "PASS" if not errors else "FAIL",
        "map": MAP,
        "checked_actor_count": len(checked),
        "physical_action_count": len(physical),
        "system_counts": counts,
        "errors": errors,
        "actors": checked,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False)+"\n", encoding="utf-8")
    if errors:
        raise RuntimeError("Runtime asset scene QA failed:\n" + "\n".join(errors))
    MARKER.write_text(f"PASS {len(checked)} actors\n", encoding="utf-8")
    unreal.log(f"WTG_RUNTIME_ASSET_SCENE_PASS {len(checked)}")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
