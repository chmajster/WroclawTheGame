"""Import the audited Poly Haven FBX source bundles into Unreal Engine.

This script performs no network access. Source files and provenance are committed
under SourceAssets/Models/PolyHaven and Data/free_model_catalog.json.
"""
from pathlib import Path
import json
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
CATALOG = ROOT / "Data/free_model_catalog.json"
MARKER = ROOT / "Saved/FreeModelsReady.ok"

def run():
    MARKER.unlink(missing_ok=True)
    entries = json.loads(CATALOG.read_text(encoding="utf-8"))
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for item in entries:
        source = ROOT / item["primary_path"]
        if not source.is_file():
            raise RuntimeError(f"Missing model source: {source}")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property(
            "destination_path",
            f"/Game/FreeModels/PolyHaven/{item['slug']}",
        )
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tools.import_asset_tasks([task])
        imported = task.get_editor_property("imported_object_paths")
        if not imported:
            raise RuntimeError(f"FBX import failed: {item['slug']}")
        for object_path in imported:
            if not unreal.load_asset(object_path):
                raise RuntimeError(f"Imported asset unreadable: {object_path}")
    unreal.EditorAssetLibrary.save_directory("/Game/FreeModels", False, True)
    MARKER.parent.mkdir(parents=True, exist_ok=True)
    MARKER.write_text("Audited Poly Haven CC0 model sources imported\n", encoding="utf-8")
    unreal.log("WROCLAW_FREE_MODELS_READY")

try:
    run()
except Exception:
    unreal.log_error(traceback.format_exc())
    MARKER.unlink(missing_ok=True)
    raise
