"""Check source-sector Nanite, bounds and thin-ground SDF policy in native UE."""
import json
import sys
import traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/'Pipeline/unreal'))
from import_asset import quit_after_editor_startup
out=ROOT/'Saved/StartLocation';marker=out/'rendering.ok';marker.unlink(missing_ok=True)
try:
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level('/Game/Maps/Nadodrze_Playable'):raise RuntimeError('Map missing')
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    records=[]
    for name in json.loads((ROOT/'Saved/PlayableDistrict/Meshes/meshes.json').read_text(encoding='utf-8')):
        source=json.loads((ROOT/'Saved/PlayableDistrict/Meshes'/name).read_text(encoding='utf-8'))
        if not source['triangles']:continue
        mesh=unreal.load_asset('/Game/Generated/GIS/PlayableDistrict/'+Path(name).stem)
        if not mesh:raise RuntimeError('Mesh missing '+name)
        nanite=editor.get_nanite_settings(mesh)
        enabled=bool(nanite.enabled)
        if enabled!=(source['kind']!='water'):raise RuntimeError('Nanite policy '+name)
        if enabled and (not nanite.explicit_tangents or abs(nanite.fallback_percent_triangles-1)>1e-6):raise RuntimeError('Fallback/tangents '+name)
        build=editor.get_lod_build_settings(mesh,0)
        df=float(build.distance_field_resolution_scale)
        if (df>0)!=(source['kind']=='building'):raise RuntimeError('SDF policy '+name)
        bounds=unreal.GeoMeshLibrary.get_authored_mesh_bounds(mesh)
        extent=bounds.max-bounds.min
        if max(extent.x,extent.y)>12800.1:raise RuntimeError('Oversized source sector '+name)
        if unreal.GeoMeshLibrary.get_authored_mesh_basis_stats(mesh).y or unreal.GeoMeshLibrary.get_authored_mesh_basis_stats(mesh).z:raise RuntimeError('Invalid basis '+name)
        records.append(dict(name=name,kind=source['kind'],nanite=enabled,distance_field_resolution_scale=df,triangles=len(source['triangles'])//3,width_cm=[extent.x,extent.y]))
    report=dict(status='PASS',records=records,sector_size_cm=12800,world_partition=False,
        notes=['Full collision fallback preserved. Opaque geometry uses Nanite; water retains its translucent raster path.',
               'Thin open ground skips mesh SDF generation; component raster/VSM shadows retained.',
               'Skeletal hero and masked foliage retain conventional LOD/culling; no WPO added.'])
    (out/'rendering-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    marker.write_text('PASS\n');unreal.log('WTG_START_RENDERING_PASS '+str(len(records)))
except Exception:unreal.log_error(traceback.format_exc());raise
finally:quit_after_editor_startup()
