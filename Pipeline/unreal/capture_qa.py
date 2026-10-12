"""Capture deterministic editor screenshots from manifest-defined QA cameras."""
from __future__ import annotations

import json
import os
import re
import traceback
import time
import hashlib
import sys
from datetime import datetime, timezone
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup


def project_path(value: str) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (ROOT / path).resolve()


def prepare() -> None:
    # UE 5.8's command-line Python executor otherwise closes the editor as soon
    # as this startup function returns, before latent screenshot work ticks.
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
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
    report_path.unlink(missing_ok=True)

    if not camera_names:
        report = {"asset_id": asset_id, "status": "SKIPPED", "reason": "no QA cameras configured", "screenshots": []}
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        marker.write_text("SKIPPED\n", encoding="utf-8")
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()
        return

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(manifest["unreal"]["qa_map"]):
        raise RuntimeError(f"Cannot load QA map: {manifest['unreal']['qa_map']}")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # Editor worlds do not run BeginPlay. Apply the existing game's daylight
    # environment for architectural QA; do not save this preview state.
    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor,unreal.WTGDayNightEnvironment): actor.set_time_of_day(float(manifest['qa'].get('time_of_day_hours',12.)))
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
    safe_names = [re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip("._") or "camera" for name in camera_names]
    if len(set(safe_names)) != len(safe_names):
        raise RuntimeError("QA camera labels collide after filename sanitization")
    screenshot_files = [out / f"{name}.png" for name in safe_names]
    for filename in screenshot_files: filename.unlink(missing_ok=True)
    map_file=ROOT/"Content"/(manifest["unreal"]["qa_map"].removeprefix("/Game/")+".umap")
    native_files=[map_file]+sorted((ROOT/"Content/Wroclaw/TenementKit").rglob("*.uasset"))+sorted((ROOT/"Content/Wroclaw/StartLocation").rglob("*.uasset"))
    native_hashes={str(path.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(path.read_bytes()).hexdigest() for path in native_files if path.is_file()}

    @unreal.AutomationScheduler.add_latent_command
    def capture_all():
        try:
            unreal.AutomationLibrary.finish_loading_before_screenshot()
            for _ in range(30):
                yield
            for name, filename in zip(camera_names, screenshot_files):
                deadline=time.monotonic()+120
                task=unreal.AutomationLibrary.take_high_res_screenshot(width,height,str(filename),camera=cameras[name],comparison_notes=f"{asset_id}:{name}",delay=2.,force_game_view=True)
                if not task.is_valid_task(): raise RuntimeError("Invalid screenshot task: "+name)
                while not task.is_task_done():
                    if time.monotonic()>deadline: raise TimeoutError("Screenshot timed out: "+name)
                    yield
            if any(not path.is_file() or path.stat().st_size<100 for path in screenshot_files):
                raise RuntimeError("Fresh screenshot files missing")
            report={"asset_id":asset_id,"map":manifest["unreal"]["qa_map"],"status":"PASS",
                "visual_review":"NOT_REVIEWED","screenshots":[str(path.relative_to(ROOT)) for path in screenshot_files],
                "captured_utc":datetime.now(timezone.utc).isoformat(),"native_files_sha256":native_hashes,
                "sha256":{path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in screenshot_files}}
            report_path.write_text(json.dumps(report,indent=2),encoding="utf-8")
            marker.write_text("PASS\n",encoding="utf-8")
            unreal.log("WTG_SCREENSHOT_QA_READY "+json.dumps(report))
        except Exception:
            unreal.log_error(traceback.format_exc())
            report_path.write_text(json.dumps({"status":"FAILED","error":traceback.format_exc()},indent=2),encoding="utf-8")
        finally:
            quit_after_editor_startup()


try:
    prepare()
except Exception:
    unreal.log_error(traceback.format_exc())
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()
