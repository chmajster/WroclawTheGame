"""Validate model quality and transforms on the generated GIS city map."""
from __future__ import annotations

import json
import math
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
POLICY = ROOT / "Data/runtime_asset_qa.json"
BINDINGS = ROOT / "Data/model_bindings.json"
OUT = ROOT / "Saved/RuntimeAssetQA/geography_scene.json"
MARKER = ROOT / "Saved/RuntimeAssetGeographyReady.ok"
MAP = "/Game/Maps/Nadodrze_GIS"


def values3(vector):
    return [float(vector.x), float(vector.y), float(vector.z)]


def check_scale(label, vector, policy, errors):
    values = [abs(value) for value in values3(vector)]
    minimum = float(policy["transform"]["min_abs_scale"])
    maximum = float(policy["transform"]["max_abs_scale"])
    if not all(math.isfinite(value) for value in values) or any(
        value < minimum or value > maximum for value in values
    ):
        errors.append(f"{label}: invalid scale {values}")
        return
    if max(values) / max(min(values), minimum) > float(policy["transform"]["max_nonuniform_ratio"]):
        errors.append(f"{label}: excessive non-uniform scale {values}")


def actor_visuals(actor):
    result = []
    result.extend(actor.get_components_by_class(unreal.StaticMeshComponent))
    result.extend(actor.get_components_by_class(unreal.SkeletalMeshComponent))
    visuals = []
    for component in result:
        if isinstance(component, unreal.StaticMeshComponent):
            mesh = component.get_editor_property("static_mesh")
        else:
            mesh = component.get_skeletal_mesh_asset()
        if mesh is not None:
            visuals.append(component)
    return visuals


def check_mesh_component(label, component, errors):
    mesh = None
    if isinstance(component, unreal.StaticMeshComponent):
        mesh = component.get_editor_property("static_mesh")
    elif isinstance(component, unreal.SkeletalMeshComponent):
        mesh = component.get_skeletal_mesh_asset()
    if mesh is None:
        errors.append(f"{label}: missing mesh on {component.get_name()}")
        return None
    if int(component.get_num_materials()) <= 0:
        errors.append(f"{label}: mesh has no material slots")
    for index in range(int(component.get_num_materials())):
        if component.get_material(index) is None:
            errors.append(f"{label}: missing material {index}")
    return mesh


def run():
    MARKER.unlink(missing_ok=True)
    OUT.unlink(missing_ok=True)
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    bindings = json.loads(BINDINGS.read_text(encoding="utf-8"))["systems"]
    errors = []

    model_report = json.loads((ROOT/"Saved/RuntimeAssetQA/model_quality.json").read_text(encoding="utf-8"))
    retarget_report = json.loads((ROOT/"Saved/RuntimeAssetQA/retarget.json").read_text(encoding="utf-8"))
    if model_report.get("status") != "PASS":
        errors.append("model quality report is not PASS")
    if retarget_report.get("status") != "PASS":
        errors.append("retarget report is not PASS")

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level(MAP):
        raise RuntimeError(f"Cannot load {MAP}")
    actors = list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
    by_label = {actor.get_actor_label(): actor for actor in actors}

    doors = [actor for actor in actors if isinstance(actor, unreal.CityInteriorDoor)]
    activities = [actor for actor in actors if isinstance(actor, unreal.CityActivity)]
    populations = [actor for actor in actors if isinstance(actor, unreal.CityPopulation)]
    cars = [actor for actor in actors if isinstance(actor, unreal.DriveableVehicle)]
    interior_props = [
        actor for actor in actors
        if isinstance(actor, unreal.StaticMeshActor)
        and any(
            actor.get_actor_label().endswith("_" + suffix)
            for suffix in ("sofa", "table", "chair", "cabinet", "shelf", "lamp")
        )
    ]

    for actor in doors + activities + cars + interior_props:
        label = actor.get_actor_label()
        check_scale(label, actor.get_actor_scale3d(), policy, errors)
        static_components = actor.get_components_by_class(unreal.StaticMeshComponent)
        if not static_components:
            errors.append(f"{label}: no StaticMeshComponent")
        for component in static_components:
            check_mesh_component(label, component, errors)

    if not doors:
        errors.append("GIS map has no CityInteriorDoor actors")
    if not activities:
        errors.append("GIS map has no CityActivity actors")
    if len(populations) != 1:
        errors.append(f"GIS map expected one CityPopulation, got {len(populations)}")
    if not cars:
        errors.append("GIS map has no DriveableVehicle")
    if not interior_props:
        errors.append("GIS map has no furnished interior props")

    for population in populations:
        vehicle = population.get_editor_property("vehicle_mesh")
        pedestrian = population.get_editor_property("pedestrian_mesh")
        if not isinstance(vehicle, unreal.StaticMesh):
            errors.append("CityPopulation vehicle_mesh is missing")
        if not isinstance(pedestrian, unreal.SkeletalMesh):
            errors.append("CityPopulation pedestrian_mesh is missing")

    for car in cars:
        body = car.get_editor_property("body_visual")
        wheels = list(car.get_editor_property("wheel_visuals"))
        if not body or not isinstance(body.get_editor_property("static_mesh"), unreal.StaticMesh):
            errors.append("DriveableVehicle body visual mesh is missing")
        if len(wheels) != 4:
            errors.append(f"DriveableVehicle expected four wheel visuals, got {len(wheels)}")
        for index, wheel in enumerate(wheels):
            if not isinstance(wheel.get_editor_property("static_mesh"), unreal.StaticMesh):
                errors.append(f"DriveableVehicle wheel {index} mesh is missing")

    campaign_input = os.environ.get("WTG_CAMPAIGN_GIS_INPUT")
    campaign_counts = None
    if campaign_input:
        campaign_dir = Path(campaign_input)
        chapter = json.loads((campaign_dir/"chapter1.json").read_text(encoding="utf-8"))
        world = json.loads((campaign_dir/"openworld.json").read_text(encoding="utf-8"))
        environment = json.loads((campaign_dir/"environment.json").read_text(encoding="utf-8"))
        physical = [
            action for action in chapter["actions"]
            if action["kind"] not in ("virtual", "zone")
            and action.get("position", [0, 0])[:2] != [0, 0]
        ]

        missing_actions = []
        invisible_actions = []
        for action in physical:
            label = "CampaignGIS_action_" + action["id"]
            visual_label = "CampaignGIS_action_visual_" + action["id"]
            actor = by_label.get(label)
            visual = by_label.get(visual_label)
            if actor is None:
                missing_actions.append(action["id"])
                continue
            if not actor_visuals(actor) and (visual is None or not actor_visuals(visual)):
                invisible_actions.append(action["id"])
        if missing_actions:
            errors.append("Campaign GIS missing action actors: " + ", ".join(missing_actions))
        if invisible_actions:
            errors.append("Campaign GIS action actors without visual meshes: " + ", ".join(invisible_actions))

        expected_groups = {
            "guards": ("CampaignGIS_guard_", len(world["guards"])),
            "npcs": ("CampaignGIS_npc_", len(world["npc"])),
            "hides": ("CampaignGIS_hide_", len(world["hides"])),
            "cameras": ("CampaignGIS_camera_", len(world["cameras"])),
            "monitors": ("CampaignGIS_monitor_", len(world["cameras"])),
            "sign_backings": (
                "CampaignGIS_environment_signback_",
                sum(1 for item in environment if item["type"] == "sign"),
            ),
        }
        campaign_counts = {}
        for name, (prefix, expected) in expected_groups.items():
            group = [(label, actor) for label, actor in by_label.items() if label.startswith(prefix)]
            campaign_counts[name] = {"expected": expected, "actual": len(group)}
            if len(group) != expected:
                errors.append(f"Campaign GIS {name}: actor count {len(group)} != {expected}")
            for label, actor in group:
                if not actor_visuals(actor):
                    errors.append(f"{label}: migrated campaign actor has no visual mesh")

        expected_lamps = sum(
            1 for item in environment
            if item["type"] == "light"
            and item["id"] in {
                "environment_253","environment_255","environment_257",
                "environment_259","environment_261","environment_263","environment_265"
            }
        )
        actual_lamps = sum(1 for label in by_label if label.startswith("CampaignGIS_environment_lamp_"))
        campaign_counts["street_lamps"] = {"expected": expected_lamps, "actual": actual_lamps}
        if actual_lamps != expected_lamps:
            errors.append(f"Campaign GIS street_lamps: actor count {actual_lamps} != {expected_lamps}")

        for label, actor in by_label.items():
            if label.startswith("CampaignGIS_environment_lamp_") and not actor_visuals(actor):
                errors.append(f"{label}: migrated street lamp has no visual mesh")

    report = {
        "status": "PASS" if not errors else "FAIL",
        "map": MAP,
        "counts": {
            "doors": len(doors),
            "activities": len(activities),
            "populations": len(populations),
            "cars": len(cars),
            "interior_props": len(interior_props),
        },
        "campaign_counts": campaign_counts,
        "bindings": {
            key: bindings[key]
            for key in (
                "ambient_pedestrian",
                "ambient_vehicle",
                "driveable_vehicle_body",
                "driveable_vehicle_wheel",
                "city_interior_door",
                "city_activity_marker",
                "city_interior_sofa",
                "city_interior_table",
                "city_interior_chair",
                "city_interior_cabinet",
                "city_interior_shelf",
                "city_interior_lamp",
            )
        },
        "errors": errors,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(report, indent=2, ensure_ascii=False)+"\n", encoding="utf-8")
    if errors:
        raise RuntimeError("GIS runtime asset QA failed:\n" + "\n".join(errors))
    MARKER.write_text("PASS\n", encoding="utf-8")
    unreal.log("WTG_RUNTIME_ASSET_GIS_PASS")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
finally:
    unreal.SystemLibrary.quit_editor()
