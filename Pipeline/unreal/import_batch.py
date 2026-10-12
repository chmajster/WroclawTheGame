"""Import an explicit manifest list in one editor process; success marker is fresh."""
import json
import os
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/"Pipeline/unreal"))
from import_asset import prepare, quit_after_editor_startup

marker=ROOT/"Saved/Pipeline/import/batch.ok"
marker.parent.mkdir(parents=True,exist_ok=True)
marker.unlink(missing_ok=True)
try:
    paths=json.loads(os.environ["WTG_ASSET_MANIFESTS"])
    if not isinstance(paths,list) or not paths: raise ValueError("Nonempty explicit manifest list required")
    for path in paths: prepare(path)
    marker.write_text("PASS\n",encoding="utf-8")
    unreal.log("WTG_BATCH_IMPORT_PASS "+str(len(paths)))
except Exception:
    unreal.log_error(traceback.format_exc())
    raise
finally:
    # Let editor startup/tool contexts and asynchronous asset work settle before
    # closing. Immediate quit inside the startup Python call can destroy a
    # partially initialized ModeManagerInteractiveToolsContext in UE 5.8.
    quit_after_editor_startup()
