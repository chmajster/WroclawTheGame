"""Native verification of production slots, scale, LODs, UV and UCX collision."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
marker=ROOT/'Saved/StartLocation/assets.ok';marker.unlink(missing_ok=True)
try:
    metadata=json.loads((ROOT/'SourceAssets/Models/Wroclaw/StartLocation/geometry.json').read_text(encoding='utf-8'))
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    reports=[]
    for record in metadata['assets']:
        path='/Game/Wroclaw/StartLocation/'+record['area']+'/SM_wtg_start_'+record['asset'].replace('-','_')
        mesh=unreal.load_asset(path)
        if not isinstance(mesh,unreal.StaticMesh):raise RuntimeError('Missing production mesh '+path)
        bounds=mesh.get_bounds();actual=[2*bounds.box_extent.x,2*bounds.box_extent.y,2*bounds.box_extent.z]
        if any(abs(a-b*100)>max(.5,b) for a,b in zip(actual,record['bounds_m'])):raise RuntimeError('Metric bounds mismatch '+path)
        slots=list(mesh.get_editor_property('static_materials'))
        names=[str(s.get_editor_property('imported_material_slot_name')) for s in slots]
        if names!=record['materials']:raise RuntimeError('Source/native material order mismatch '+str((path,names,record['materials'])))
        for slot,name in zip(slots,record['materials']):
            material=slot.get_editor_property('material_interface')
            if not material or material.get_name()!='MI_'+name:raise RuntimeError('Material binding mismatch '+path)
        lods=subsystem.get_lod_count(mesh)
        if lods!=3:raise RuntimeError('Missing LOD '+path)
        sections=[]
        for lod in range(lods):
            used=[subsystem.get_lod_material_slot(mesh,lod,i) for i in range(mesh.get_num_sections(lod))]
            if not used or any(i<0 or i>=len(slots) for i in used):raise RuntimeError('Invalid LOD section '+path)
            if len(slots)>1 and len(set(used))<2:raise RuntimeError('LOD materials collapsed onto one slot '+path)
            if subsystem.get_num_uv_channels(mesh,lod)<1:raise RuntimeError('LOD UV missing '+path)
            sections.append(used)
        hulls=subsystem.get_simple_collision_count(mesh)+subsystem.get_convex_collision_count(mesh)
        expected=record['collision_shapes'] or (1 if record['asset'] in ['curb','bike-rack','parked-car'] else 0)
        if hulls!=expected:raise RuntimeError('Collision hull count mismatch '+path)
        reports.append(dict(mesh=path,lods=lods,slots=names,lod_section_slots=sections,hulls=hulls,bounds_cm=actual))
    (marker.parent/'assets-report.json').write_text(json.dumps(dict(status='PASS',assets=reports),indent=2),encoding='utf-8')
    marker.write_text('PASS\n',encoding='utf-8');unreal.log('WTG_START_ASSETS_VERIFIED '+str(len(reports)))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
