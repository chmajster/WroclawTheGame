"""Capture deterministic editor screenshots from manifest-defined QA cameras."""
from __future__ import annotations

import json
import os
import traceback
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def prepare() -> None:
    manifest_value = os.environ.get("WTG_ASSET_MANIFEST")
    if not manifest_value:
        raise RuntimeError("WTG_ASSET_MANIFEST is not set")

    manifest = json.loads(project_path(manifest_value).read_text(encoding="utf-8"))
    asset_id = manifest["id"]
    camera_names = list(manifest["qa"]["cameras"])
    out = ROOT / "Saved" / "Pipeline" / "screenshots" / asset_id
    out.mkdir(parents=True, exist_ok=True)
    marker = out / "capture.ok"
    report_path = out / "capture.json"
    marker.unlink(missing_ok=True)

    if not camera_names:
        report = {"asset_id": asset_id, "status": "SKIPPED", "reason": "no QA cameras configured", "screenshots": []}
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        marker.write_text("SKIPPED\n", encoding="utf-8")
        unreal.SystemLibrary.quit_editor()
        return

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(manifest["unreal"]["qa_map"]):
        raise RuntimeError(f"Cannot load QA map: {manifest['unreal']['qa_map']}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cameras = {
        actor.get_actor_label(): actor
        for actor in actor_subsystem.get_all_level_actors()
        if isinstance(actor, unreal.CameraActor)
    }
    missing = [name for name in camera_names if name not in cameras]
    if missing:
        raise RuntimeError("Missing QA cameras: " + ", ".join(missing))

    width = int(manifest["qa"]["screenshot_width"])
    height = int(manifest["qa"]["screenshot_height"])
    screenshot_files = [out / f"{name}.png" for name in camera_names]

    @unreal.AutomationScheduler.add_latent_command
    def capture_all():
        unreal.AutomationLibrary.finish_loading_before_screenshot()
        for name, filename in zip(camera_names, screenshot_files):
            task = unreal.AutomationLibrary.take_high_res_screenshot(
                width,
                height,
                str(filename),
                camera=cameras[name],
                comparison_notes=f"{asset_id}:{name}",
                force_game_view=True,
            )
            if not task.is_valid_task():
                raise RuntimeError(f"Screenshot task is invalid for camera {name}")
            while not task.is_task_done():
                yield

    @unreal.AutomationScheduler.add_latent_command
    def finalize():
        missing_files = [str(path) for path in screenshot_files if not path.is_file()]
        if missing_files:
            raise RuntimeError("Screenshot files were not created: " + ", ".join(missing_files))
        report = {
            "asset_id": asset_id,
            "map": manifest["unreal"]["qa_map"],
            "status": "PASS",
            "screenshots": [str(path.relative_to(ROOT)) for path in screenshot_files],
        }
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        marker.write_text("PASS\n", encoding="utf-8")
        unreal.log("WTG_SCREENSHOT_QA_READY " + json.dumps(report))
        unreal.SystemLibrary.quit_editor()


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    unreal.SystemLibrary.quit_editor()
