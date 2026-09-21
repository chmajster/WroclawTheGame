#!/usr/bin/env python3
"""Fail if current gameplay content contains an unresolved model slot.

This audit is intentionally source/data based. Unreal visual QA is a separate acceptance
gate, but the repository must never regress to unbound gameplay objects or visible
Engine/BasicShapes cube proxies.
"""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

RUNTIME_VISUAL_SOURCES = (
    "Source/WroclawTheGame/Interaction/SliceProp.cpp",
    "Source/WroclawTheGame/World/CityActivity.cpp",
    "Source/WroclawTheGame/World/CityInteriorDoor.cpp",
    "Source/WroclawTheGame/World/WorldInteraction.cpp",
    "Source/WroclawTheGame/World/ResidentNPC.cpp",
    "Source/WroclawTheGame/World/CityPopulation.cpp",
    "Source/WroclawTheGame/AI/SliceEnemy.cpp",
    "Source/WroclawTheGame/Vehicles/DriveableVehicle.cpp",
    "Source/WroclawTheGame/Character/SliceCharacter.cpp",
    "Source/WroclawTheGame/World/RoadBlockSystem.cpp",
    "Source/WroclawTheGame/Interaction/NoiseThrowable.cpp",
)

REQUIRED_SYSTEMS = {
    "player_fallback",
    "player_fallback_female",
    "resident_npc",
    "enemy_guard",
    "ambient_pedestrian",
    "ambient_vehicle",
    "driveable_vehicle_body",
    "driveable_vehicle_wheel",
    "city_interior_door",
    "surveillance_camera",
    "cctv_monitor",
    "hide_container",
    "hide_park",
    "hide_shelf",
    "city_activity_marker",
    "street_lamp",
    "environment_sign_backing",
    "roadblock_barrier",
    "noise_throwable",
    "environment_sign_backing",
    "roadblock_barrier",
    "noise_throwable",
    "city_interior_sofa",
    "city_interior_table",
    "city_interior_chair",
    "city_interior_cabinet",
    "city_interior_shelf",
    "city_interior_lamp",
}


def read_json(path: str):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def fail(messages: list[str]) -> None:
    if messages:
        raise SystemExit("Model coverage audit failed:\n- " + "\n- ".join(messages))


def main() -> None:
    errors: list[str] = []
    chapter = read_json("Data/chapter1.json")
    bindings = read_json("Data/model_bindings.json")
    poly = read_json("Data/free_model_catalog.json")
    external = read_json("Data/free_external_model_catalog.json")
    characters = read_json("Data/free_character_catalog.json")
    opening = read_json("Data/opening_scene_models.json")
    authored_props = read_json("Data/prop_model_bindings.json")
    facade_bindings = read_json("Data/facade_asset_bindings.json")
    roof_bindings = read_json("Data/roof_asset_bindings.json")

    catalogs: dict[str, dict] = {}
    for collection, id_key in ((poly, "slug"), (external, "id"), (characters, "id")):
        for item in collection:
            model_id = item[id_key]
            if model_id in catalogs:
                errors.append(f"duplicate model id: {model_id}")
                continue
            catalogs[model_id] = item
            if item.get("license") != "CC0-1.0":
                errors.append(f"non-CC0 model binding source: {model_id}")
            primary = item.get("primary_path")
            if not primary or not (ROOT / primary).is_file():
                errors.append(f"missing primary source: {model_id} -> {primary}")

    item_ids = {item["id"] for item in chapter["items"]}
    if item_ids != set(bindings["items"]):
        errors.append(
            f"inventory bindings differ: missing={sorted(item_ids-set(bindings['items']))} "
            f"extra={sorted(set(bindings['items'])-item_ids)}"
        )

    physical_actions = {
        action["id"]
        for action in chapter["actions"]
        if action["kind"] not in ("virtual", "zone")
        and action.get("position", [0, 0])[:2] != [0, 0]
    }
    if physical_actions != set(bindings["actions"]):
        errors.append(
            f"physical action bindings differ: missing={sorted(physical_actions-set(bindings['actions']))} "
            f"extra={sorted(set(bindings['actions'])-physical_actions)}"
        )

    systems = set(bindings["systems"])
    if systems != REQUIRED_SYSTEMS:
        errors.append(
            f"system model bindings differ: missing={sorted(REQUIRED_SYSTEMS-systems)} "
            f"extra={sorted(systems-REQUIRED_SYSTEMS)}"
        )

    facade_refs = set()
    for values in facade_bindings["groups"].values():
        facade_refs.update(values)
    facade_refs.update(facade_bindings["procedural"].values())
    if any(str(value).startswith("procedural:") for value in facade_refs):
        errors.append("facade bindings still contain procedural proxy IDs")

    roof_refs = set(roof_bindings["shapes"].values()) | set(roof_bindings["details"].values())
    required_roof_shapes = {"flat", "gabled", "hipped", "pyramidal", "mansard", "dome", "onion"}
    if set(roof_bindings["shapes"]) != required_roof_shapes:
        errors.append(
            f"roof shape bindings differ: missing={sorted(required_roof_shapes-set(roof_bindings['shapes']))} "
            f"extra={sorted(set(roof_bindings['shapes'])-required_roof_shapes)}"
        )
    if set(roof_bindings["details"]) != {"chimney", "dormer"}:
        errors.append("roof detail bindings must contain chimney and dormer")

    referenced = (
        set(bindings["items"].values())
        | set(bindings["actions"].values())
        | set(bindings["systems"].values())
        | {item["model"] for item in opening}
        | {item["model"] for item in authored_props}
        | facade_refs
        | roof_refs
    )
    unknown = sorted(referenced - set(catalogs))
    if unknown:
        errors.append(f"unknown referenced model ids: {unknown}")

    bodies = {item["id"]: item for item in characters if item.get("category") == "body"}
    for required in ("quaternius-ubc-superhero-male", "quaternius-ubc-superhero-female"):
        if required not in bodies:
            errors.append(f"missing rigged character body: {required}")
    for category in ("hair", "beard", "eyebrows"):
        if not any(item.get("category") == category for item in characters):
            errors.append(f"missing character part category: {category}")

    if bindings["systems"]["player_fallback"] not in bodies:
        errors.append("male player fallback is not a rigged character body")
    if bindings["systems"]["player_fallback_female"] not in bodies:
        errors.append("female player fallback is not a rigged character body")
    for key in ("resident_npc", "enemy_guard", "ambient_pedestrian"):
        if bindings["systems"][key] not in bodies:
            errors.append(f"{key} is not bound to a rigged character body")

    for source in RUNTIME_VISUAL_SOURCES:
        text = (ROOT / source).read_text(encoding="utf-8")
        if "/Engine/BasicShapes/Cube" in text or "/Engine/BasicShapes/Sphere" in text:
            errors.append(f"visible engine primitive proxy remains in runtime source: {source}")

    prepare_content = (ROOT / "Scripts/prepare_content.py").read_text(encoding="utf-8")
    prepare_geography = (ROOT / "Scripts/prepare_geography.py").read_text(encoding="utf-8")
    if "model_bindings['actions']" not in prepare_content:
        errors.append("campaign generator does not consume complete action model bindings")
    if "model_bindings['actions']" not in prepare_geography:
        errors.append("GIS campaign migration does not consume complete action model bindings")
    if any(value == "clipboard" for value in bindings["actions"].values()):
        errors.append("physical action model bindings still use generic clipboard proxy")
    if "model_bindings['actions']" not in prepare_geography:
        errors.append("GIS campaign migration does not consume complete action model bindings")
    if any(value == "clipboard" for value in bindings["actions"].values()):
        errors.append("physical action model bindings still use generic clipboard proxy")
    for token in (
        "ambient_pedestrian",
        "ambient_vehicle",
        "driveable_vehicle_body",
        "driveable_vehicle_wheel",
        "city_interior_door",
        "city_activity_marker",
        "city_interior_sofa",
        "city_interior_table",
        "environment_sign_backing",
        "hide_park",
    ):
        if token not in prepare_geography:
            errors.append(f"GIS generator does not consume system model binding: {token}")

    if "WROCLAW_ROOF_INSTANCES" not in prepare_geography or "roof_details.json" not in prepare_geography:
        errors.append("GIS generator does not bake mesh-backed roof catalogue")
    if "add_opening_trim" not in prepare_geography:
        errors.append("GIS generator does not bake sill/lintel opening meshes")
    roadblock = (ROOT / "Source/WroclawTheGame/World/RoadBlockSystem.cpp").read_text(encoding="utf-8")
    throwable = (ROOT / "Source/WroclawTheGame/Interaction/NoiseThrowable.cpp").read_text(encoding="utf-8")
    if "ConstructorHelpers::FObjectFinder" in roadblock and "/Game/FreeModels" in roadblock:
        errors.append("roadblock loads generated FreeModels in constructor instead of BeginPlay")
    if "ConstructorHelpers::FObjectFinder" in throwable and "/Game/FreeModels" in throwable:
        errors.append("throwable loads generated FreeModels in constructor instead of BeginPlay")

    fail(errors)
    print(
        f"Model coverage OK: {len(catalogs)} catalogued models, "
        f"{len(item_ids)} inventory items, {len(physical_actions)} physical actions, "
        f"{len(REQUIRED_SYSTEMS)} runtime system slots."
    )


if __name__ == "__main__":
    main()
