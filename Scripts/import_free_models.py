"""Import all audited CC0 model sources committed to the repository.

No network access is performed. Poly Haven, Kenney, OpenGameArt and project-owned
fallbacks are read from the two source catalogs and imported into /Game/FreeModels.
The resolved Unreal object paths are written to Saved/FreeModelImportMap.json.
"""
from pathlib import Path
import json
import re
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
POLY_CATALOG = ROOT / "Data/free_model_catalog.json"
EXTERNAL_CATALOG = ROOT / "Data/free_external_model_catalog.json"
MARKER = ROOT / "Saved/FreeModelsReady.ok"
IMPORT_MAP = ROOT / "Saved/FreeModelImportMap.json"


def clean(value):
    text = re.sub(r"[^A-Za-z0-9_]+", "_", str(value)).strip("_")
    return text or "Unknown"


def catalog_entries():
    entries = []
    for item in json.loads(POLY_CATALOG.read_text(encoding="utf-8")):
        entries.append({
            "id": item["slug"],
            "provider": "PolyHaven",
            "primary_path": item["primary_path"],
        })
    for item in json.loads(EXTERNAL_CATALOG.read_text(encoding="utf-8")):
        entries.append({
            "id": item["id"],
            "provider": item.get("provider", "External"),
            "primary_path": item["primary_path"],
        })
    ids = [x["id"] for x in entries]
    if len(ids) != len(set(ids)):
        raise RuntimeError("Duplicate free-model IDs across catalogs")
    return entries


def import_one(tools, item):
    source = ROOT / item["primary_path"]
    if not source.is_file():
        raise RuntimeError(f"Missing model source: {source}")
    destination = f"/Game/FreeModels/{clean(item['provider'])}/{clean(item['id'])}"
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tools.import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths") or [])
    if not imported:
        raise RuntimeError(f"Model import failed: {item['id']} ({source})")
    readable = []
    preferred = None
    for object_path in imported:
        asset = unreal.load_asset(object_path)
        if not asset:
            continue
        readable.append(object_path)
        if preferred is None and isinstance(asset, (unreal.StaticMesh, unreal.SkeletalMesh)):
            preferred = object_path
    if not readable:
        raise RuntimeError(f"Imported assets unreadable: {item['id']}")
    return {
        "provider": item["provider"],
        "source": item["primary_path"],
        "destination": destination,
        "primary_object": preferred or readable[0],
        "objects": readable,
    }


def run():
    MARKER.unlink(missing_ok=True)
    IMPORT_MAP.unlink(missing_ok=True)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    resolved = {}
    for item in catalog_entries():
        resolved[item["id"]] = import_one(tools, item)
    if not unreal.EditorAssetLibrary.save_directory("/Game/FreeModels", False, True):
        raise RuntimeError("Failed to save /Game/FreeModels")
    IMPORT_MAP.parent.mkdir(parents=True, exist_ok=True)
    IMPORT_MAP.write_text(
        json.dumps(resolved, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    MARKER.write_text(
        f"Imported {len(resolved)} audited CC0 model sources\n",
        encoding="utf-8",
    )
    unreal.log(f"WROCLAW_FREE_MODELS_READY {len(resolved)}")


try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
    IMPORT_MAP.unlink(missing_ok=True)
    raise
