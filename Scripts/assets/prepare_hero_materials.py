"""Refresh only the existing civilian's PBR; no skeleton/catalog/animation changes."""
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve();sys.path.insert(0,str(ROOT/'Scripts/assets'));sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
from import_hero_models import materials
try:
    result=materials()
    if len(result)!=8:raise RuntimeError('Civilian material set incomplete')
    (ROOT/'Saved/StartLocation/hero-materials.ok').write_text('PASS\n')
    unreal.log('WTG_HERO_MATERIALS_PASS')
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
