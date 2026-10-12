"""Apply and verify saved native quality settings for the five production assets."""
import json
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/"Pipeline/unreal"))
from import_asset import quit_after_editor_startup, import_texture
from city_quality import configure_mesh, configure_instances

marker=ROOT/"Saved/CityDetails/quality.ok"; marker.parent.mkdir(parents=True,exist_ok=True); marker.unlink(missing_ok=True)
try:
    reports=[]
    editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for path in sorted((ROOT/"Pipeline/assets/tenements").glob("*.asset.json")):
        manifest=json.loads(path.read_text(encoding="utf-8"))
        mesh=unreal.load_asset(manifest["unreal"]["destination"]+"/SM_"+manifest["id"].replace("-","_"))
        if not isinstance(mesh,unreal.StaticMesh): raise RuntimeError("Missing mesh "+manifest["id"])
        for kind,source in manifest["textures"].items():
            if source: import_texture(ROOT/source,manifest["unreal"]["destination"]+"/Textures",kind)
        configure_mesh(mesh,manifest)
        n=editor.get_nanite_settings(mesh)
        reports.append({"id":manifest["id"],"nanite":n.enabled,"fallback_percent":n.fallback_percent_triangles,
            "lod_screen_sizes":list(editor.get_lod_screen_sizes(mesh)),"slots":len(mesh.static_materials)})
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level("/Game/Maps/Nadodrze_Playable"): raise RuntimeError("District missing")
    count=0
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if actor.get_actor_label().startswith("CityKit_"):
            for component in actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
                configure_instances(component); count+=1
    if count<=0: raise RuntimeError("No kit HISM components")
    if not levels.save_current_level(): raise RuntimeError("Map quality save failed")
    report={"status":"PASS","assets":reports,"hism_components":count,"world_partition":"NOT_CONVERTED","fps":"NOT_MEASURED"}
    (marker.parent/"quality.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    marker.write_text("PASS\n",encoding="utf-8"); unreal.log("WTG_CITY_QUALITY_PASS "+json.dumps(report))
except Exception:
    unreal.log_error(traceback.format_exc()); raise
finally: quit_after_editor_startup()
