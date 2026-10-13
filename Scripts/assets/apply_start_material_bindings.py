"""Update existing real meshes from manifest bindings without reimporting geometry."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
try:
    metadata=json.loads((ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json').read_text(encoding='utf-8'))
    changed=[]
    for record in metadata['assets']:
        manifest=json.loads((ROOT/'Pipeline/assets/start_location'/(record['asset']+'.asset.json')).read_text(encoding='utf-8'))
        mesh=unreal.load_asset(manifest['unreal']['destination']+'/SM_wtg_start_'+record['asset'].replace('-','_'))
        dirty=False
        for b in manifest['materials']:
            mat=unreal.load_asset(b['asset'])
            if not mat:raise RuntimeError('Missing '+b['asset'])
            if mesh.get_material(b['slot'])!=mat:mesh.set_material(b['slot'],mat);dirty=True
        if dirty:unreal.EditorAssetLibrary.save_loaded_asset(mesh,False);changed.append(record['asset'])
    marker=ROOT/'Saved/StartLocation/bindings.ok';marker.write_text('PASS\n');unreal.log('WTG_START_BINDINGS_PASS '+str(changed))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
