"""Import audited CC0 animation libraries committed to SourceAssets.

No network access is performed here. The fetch step pins and verifies source bytes;
this script only imports them into /Game/FreeAnimations and records resolved UE paths.
"""
from pathlib import Path
import json
import re
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
CATALOGS = [
    ROOT / "Data/free_animation_catalog.json",
    ROOT / "Data/free_kaykit_animation_catalog.json",
]
MARKER = ROOT / "Saved/FreeAnimationsReady.ok"
IMPORT_MAP = ROOT / "Saved/FreeAnimationImportMap.json"


def clean(value):
    text = re.sub(r"[^A-Za-z0-9_]+", "_", str(value)).strip("_")
    return text or "Unknown"


def import_one(tools, item):
    source = ROOT / item["primary_path"]
    if not source.is_file():
        raise RuntimeError(f"Missing animation source: {source}")

    destination = f"/Game/FreeAnimations/{clean(item.get('provider', 'External'))}/{clean(item['id'])}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])

    imported = list(task.get_editor_property("imported_object_paths") or [])
    readable = []
    animations = []
    skeletons = []
    meshes = []
    for object_path in imported:
        asset = unreal.load_asset(object_path)
        if not asset:
            continue
        readable.append(object_path)
        if isinstance(asset, unreal.AnimationAsset):
            animations.append(object_path)
        elif isinstance(asset, unreal.Skeleton):
            skeletons.append(object_path)
        elif isinstance(asset, unreal.SkeletalMesh):
            meshes.append(object_path)

    if not readable:
        raise RuntimeError(f"Animation import produced no readable assets: {item['id']}")
    if not animations:
        raise RuntimeError(
            f"Animation import produced no AnimationAsset objects: {item['id']} "
            f"(source declares {item.get('clip_count', '?')} clips)"
        )

    return {
        "provider": item.get("provider", "External"),
        "source": item["primary_path"],
        "destination": destination,
        "root_motion_source": bool(item.get("root_motion", False)),
        "declared_clip_count": int(item.get("clip_count", 0)),
        "imported_animation_count": len(animations),
        "animations": animations,
        "skeletons": skeletons,
        "meshes": meshes,
        "objects": readable,
    }


def run():
    MARKER.unlink(missing_ok=True)
    IMPORT_MAP.unlink(missing_ok=True)
    entries = []
    for catalog in CATALOGS:
        if not catalog.is_file():
            raise RuntimeError(f"Missing animation catalog: {catalog}")
        batch = json.loads(catalog.read_text(encoding="utf-8"))
        if not batch:
            raise RuntimeError(f"Animation catalog is empty: {catalog}")
        entries.extend(batch)
    ids = [item["id"] for item in entries]
    if len(ids) != len(set(ids)):
        raise RuntimeError("Duplicate animation IDs")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    resolved = {}
    for item in entries:
        resolved[item["id"]] = import_one(tools, item)

    if not unreal.EditorAssetLibrary.save_directory("/Game/FreeAnimations", False, True):
        raise RuntimeError("Failed to save /Game/FreeAnimations")

    IMPORT_MAP.parent.mkdir(parents=True, exist_ok=True)
    IMPORT_MAP.write_text(
        json.dumps(resolved, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    total = sum(item["imported_animation_count"] for item in resolved.values())
    MARKER.write_text(
        f"Imported {len(resolved)} audited CC0 animation libraries ({total} AnimationAsset objects)\n",
        encoding="utf-8",
    )
    unreal.log(f"WROCLAW_FREE_ANIMATIONS_READY {len(resolved)} {total}")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
    IMPORT_MAP.unlink(missing_ok=True)
    raise
