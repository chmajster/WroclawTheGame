"""Build and capture a deterministic male/female character QA gallery.

This is the rendering gate for animation pose, IK/retarget result and visible clipping.
The script invalidates stale visual review when its input fingerprint changes.
"""
from __future__ import annotations

import hashlib
import json
import re
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
POLICY = ROOT / "Data/runtime_asset_qa.json"
MODEL_MAP = ROOT / "Saved/FreeModelImportMap.json"
RETARGET_MAP = ROOT / "Saved/RetargetedAnimationMap.json"
MODEL_REPORT = ROOT / "Saved/RuntimeAssetQA/model_quality.json"
RETARGET_REPORT = ROOT / "Saved/RuntimeAssetQA/retarget.json"
SCENE_REPORT = ROOT / "Saved/RuntimeAssetQA/scene.json"
OUT_DIR = ROOT / "Saved/RuntimeAssetQA/character_screenshots"
CAPTURE = OUT_DIR / "capture.json"
MARKER = ROOT / "Saved/RuntimeCharacterVisualsReady.ok"
VISUAL_REVIEW = ROOT / "Saved/RuntimeAssetQA/visual_review.json"
MAP_PATH = "/Game/RuntimeAssetQA/CharacterQA"


def safe(value):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", str(value)).strip("._") or "pose"


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def input_fingerprint():
    digest = hashlib.sha256()
    paths = [
        POLICY,
        MODEL_REPORT,
        RETARGET_REPORT,
        SCENE_REPORT,
        RETARGET_MAP,
        ROOT / "Scripts/capture_runtime_character_qa.py",
        ROOT / "Scripts/prepare_character_creator.py",
        ROOT / "Source/WroclawTheGame/Character/CharacterAppearanceComponent.cpp",
    ]
    for path in paths:
        if not path.is_file():
            raise RuntimeError(f"Character QA input missing: {path}")
        digest.update(str(path.relative_to(ROOT)).encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def load_mesh(imported, model_id, expected):
    record = imported.get(model_id)
    path = record.get("primary_object") if record else None
    asset = unreal.load_asset(path) if path else None
    if not isinstance(asset, expected):
        raise RuntimeError(f"{model_id} is not {expected.__name__}: {path}")
    return asset


def spawn_skeletal(actors, mesh, location, label):
    actor = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, location, unreal.Rotator(0, -90, 0))
    if not actor:
        raise RuntimeError(f"Cannot spawn {label}")
    actor.set_actor_label(label)
    component = actor.get_component_by_class(unreal.SkeletalMeshComponent)
    if not component:
        raise RuntimeError(f"SkeletalMeshComponent missing: {label}")
    component.set_skeletal_mesh_asset(mesh)
    component.set_collision_profile_name("NoCollision")
    component.set_update_animation_in_editor(True)
    return actor, component


def attach_follower(actors, mesh, body_component, location, label):
    actor, component = spawn_skeletal(actors, mesh, location, label)
    component.set_leader_pose_component(body_component, True, False)
    return actor, component


def fresh_capture(fingerprint, poses):
    if not CAPTURE.is_file():
        return False
    try:
        report = json.loads(CAPTURE.read_text(encoding="utf-8"))
    except Exception:
        return False
    if report.get("status") != "PASS" or report.get("input_fingerprint") != fingerprint:
        return False
    if sorted(report.get("poses", [])) != sorted(poses):
        return False
    for item in report.get("screenshots", []):
        path = ROOT / item["path"]
        if not path.is_file() or path.stat().st_size < 1024:
            return False
        if sha256_file(path) != item.get("sha256"):
            return False
    return True


def prepare():
    MARKER.unlink(missing_ok=True)
    policy = json.loads(POLICY.read_text(encoding="utf-8"))
    poses = list(policy["character"]["visual_qa_poses"])
    fingerprint = input_fingerprint()

    if fresh_capture(fingerprint, poses):
        MARKER.write_text("PASS cached character screenshots\n", encoding="utf-8")
        unreal.log("WTG_RUNTIME_CHARACTER_CAPTURE_PASS_CACHED")
        unreal.SystemLibrary.quit_editor()
        return

    VISUAL_REVIEW.unlink(missing_ok=True)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for png in OUT_DIR.glob("*.png"):
        png.unlink()

    imported = json.loads(MODEL_MAP.read_text(encoding="utf-8"))
    retargeted = json.loads(RETARGET_MAP.read_text(encoding="utf-8"))
    male = load_mesh(imported, "quaternius-ubc-superhero-male", unreal.SkeletalMesh)
    female = load_mesh(imported, "quaternius-ubc-superhero-female", unreal.SkeletalMesh)

    parts = {
        "male": [
            load_mesh(imported, "quaternius-ubc-hair-simple-parted", unreal.SkeletalMesh),
            load_mesh(imported, "quaternius-ubc-beard", unreal.SkeletalMesh),
            load_mesh(imported, "quaternius-ubc-eyebrows-regular", unreal.SkeletalMesh),
        ],
        "female": [
            load_mesh(imported, "quaternius-ubc-hair-buzzed-female", unreal.SkeletalMesh),
            load_mesh(imported, "quaternius-ubc-eyebrows-female", unreal.SkeletalMesh),
        ],
    }

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not levels.new_level("/Game/RuntimeAssetQA/CharacterQAScratch"):
            raise RuntimeError("Cannot switch to character QA scratch level")
        if not unreal.EditorAssetLibrary.delete_asset(MAP_PATH):
            raise RuntimeError("Cannot replace character QA map")
    if not levels.new_level(MAP_PATH):
        raise RuntimeError("Cannot create character QA map")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    camera_records = []
    y_step = 420.0
    for index, semantic in enumerate(poses):
        record = retargeted.get(semantic)
        if not record:
            raise RuntimeError(f"Retargeted QA semantic missing: {semantic}")
        y = index * y_step
        for variant, x, mesh in (("male", -80.0, male), ("female", 80.0, female)):
            anim_path = record.get("targets", {}).get(variant)
            animation = unreal.load_asset(anim_path) if anim_path else None
            if not isinstance(animation, unreal.AnimationAsset):
                raise RuntimeError(f"Retargeted animation missing: {semantic}/{variant}")
            location = unreal.Vector(x, y, 0)
            body_actor, body = spawn_skeletal(
                actors, mesh, location, f"RuntimeQA_{safe(semantic)}_{variant}_Body"
            )
            body.play_animation(animation, False)
            body.set_position(float(animation.get_play_length()) * 0.5, False)
            body.set_play_rate(0.0)
            for part_index, part_mesh in enumerate(parts[variant]):
                attach_follower(
                    actors,
                    part_mesh,
                    body,
                    location,
                    f"RuntimeQA_{safe(semantic)}_{variant}_Part{part_index}",
                )

        camera = actors.spawn_actor_from_class(
            unreal.CameraActor,
            unreal.Vector(520, y, 110),
            unreal.MathLibrary.find_look_at_rotation(unreal.Vector(520, y, 110), unreal.Vector(0, y, 95)),
        )
        if not camera:
            raise RuntimeError(f"Cannot create QA camera for {semantic}")
        camera_name = "RuntimeQA_" + safe(semantic)
        camera.set_actor_label(camera_name)
        camera_records.append((semantic, camera_name, OUT_DIR / f"{safe(semantic)}.png"))

    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, (len(poses)-1)*y_step/2, -2))
    plane = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")
    if floor and plane:
        floor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(plane)
        floor.set_actor_scale3d(unreal.Vector(12, max(12, len(poses)*4), 1))

    sun = actors.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-35, -30, 0)
    )
    if sun:
        sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property("intensity", 5.0)
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))

    if not levels.save_current_level():
        raise RuntimeError("Cannot save character QA level")

    width, height = [int(value) for value in policy["character"]["visual_qa_resolution"]]
    camera_by_name = {
        actor.get_actor_label(): actor
        for actor in actors.get_all_level_actors()
        if isinstance(actor, unreal.CameraActor)
    }

    @unreal.AutomationScheduler.add_latent_command
    def capture_all():
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        for semantic, camera_name, filename in camera_records:
            camera = camera_by_name.get(camera_name)
            if not camera:
                raise RuntimeError(f"QA camera disappeared: {camera_name}")
            task = unreal.AutomationLibrary.take_high_res_screenshot(
                width,
                height,
                str(filename),
                camera=camera,
                comparison_notes=f"runtime-character:{semantic}",
                force_game_view=True,
            )
            if not task.is_valid_task():
                raise RuntimeError(f"Invalid screenshot task: {semantic}")
            while not task.is_task_done():
                yield

    @unreal.AutomationScheduler.add_latent_command
    def finalize():
        screenshots = []
        for semantic, _, filename in camera_records:
            if not filename.is_file() or filename.stat().st_size < 1024:
                raise RuntimeError(f"Character QA screenshot missing/empty: {filename}")
            screenshots.append({
                "semantic": semantic,
                "path": str(filename.relative_to(ROOT)),
                "bytes": filename.stat().st_size,
                "sha256": sha256_file(filename),
            })
        report = {
            "status": "PASS",
            "input_fingerprint": fingerprint,
            "poses": poses,
            "variants": ["male", "female"],
            "resolution": [width, height],
            "screenshots": screenshots,
        }
        CAPTURE.write_text(json.dumps(report, indent=2, ensure_ascii=False)+"\n", encoding="utf-8")
        MARKER.write_text(f"PASS {len(screenshots)} character screenshots\n", encoding="utf-8")
        unreal.log("WTG_RUNTIME_CHARACTER_CAPTURE_PASS " + json.dumps(report))
        unreal.SystemLibrary.quit_editor()


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
    unreal.SystemLibrary.quit_editor()
