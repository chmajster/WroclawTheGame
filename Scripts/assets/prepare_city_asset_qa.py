"""Build reproducible kit studio and GIS-derived street cameras for real UE captures."""
import json
import math
import sys
import traceback
from pathlib import Path
import unreal

ROOT=Path(unreal.Paths.project_dir()).resolve()
sys.path.insert(0,str(ROOT/"Pipeline/unreal"))
from import_asset import quit_after_editor_startup

marker=ROOT/"Saved/CityDetails/qa-map.ok"; marker.parent.mkdir(parents=True,exist_ok=True); marker.unlink(missing_ok=True)


def camera(actors,label,position,target,ortho=None):
    p=unreal.Vector(*position); delta=unreal.Vector(*target)-p
    actor=actors.spawn_actor_from_class(unreal.CameraActor,p,unreal.MathLibrary.find_look_at_rotation(p,unreal.Vector(*target)))
    actor.set_actor_label(label)
    component=actor.get_component_by_class(unreal.CameraComponent)
    component.set_editor_property("constrain_aspect_ratio",False)
    settings=component.get_editor_property("post_process_settings")
    settings.set_editor_property("override_auto_exposure_method",True)
    settings.set_editor_property("auto_exposure_method",unreal.AutoExposureMethod.AEM_HISTOGRAM)
    for field,value in [("auto_exposure_min_brightness",12.),("auto_exposure_max_brightness",12.)]:
        settings.set_editor_property("override_"+field,True); settings.set_editor_property(field,value)
    component.set_editor_property("post_process_settings",settings)
    component.set_editor_property("post_process_blend_weight",1.)
    if ortho:
        component.set_editor_property("projection_mode",unreal.CameraProjectionMode.ORTHOGRAPHIC)
        component.set_editor_property("ortho_width",ortho)
    else: component.set_editor_property("field_of_view",65.)
    return actor


try:
    levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    policy=json.loads((ROOT/"Data/city_detail_policy.json").read_text(encoding="utf-8"))
    qa_map="/Game/Maps/TenementKit_QA"
    if unreal.EditorAssetLibrary.does_asset_exist(qa_map): success=levels.load_level(qa_map)
    else: success=levels.new_level(qa_map)
    if not success: raise RuntimeError("Cannot create/load kit studio")
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label().startswith("AssetQA_") or actor.get_actor_label() in {"Kit_Front","Kit_Oblique"}: actors.destroy_actor(actor)
    positions={"railing":(0,0,60),"slab":(0,0,0),"entrance":(350,0,0),"door_leaf":(350,8,0),"cornice":(720,0,180)}
    for role,path in policy["meshes"].items():
        mesh=unreal.load_asset(path)
        if not isinstance(mesh,unreal.StaticMesh): raise RuntimeError("Missing "+path)
        actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*positions[role]))
        actor.set_actor_label("AssetQA_"+role)
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=180,roll=0),False)
        actor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(mesh)
    floor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(350,0,-5))
    floor.set_actor_label("AssetQA_Floor"); floor.set_actor_scale3d(unreal.Vector(14,10,.1))
    floor.get_component_by_class(unreal.StaticMeshComponent).set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
    environment=actors.spawn_actor_from_class(unreal.WTGDayNightEnvironment,unreal.Vector())
    environment.set_actor_label("AssetQA_Environment")
    environment.set_editor_property("advance_time",False); environment.set_time_of_day(12.)
    camera(actors,"Kit_Front",(350,1400,220),(350,0,190),1100)
    camera(actors,"Kit_Oblique",(1050,1000,530),(350,0,180),1200)
    if not levels.save_current_level(): raise RuntimeError("Studio save failed")
    if not levels.load_level(policy["map"]): raise RuntimeError("District missing")
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() in {"CityKit_Street","CityKit_Balcony"}: actors.destroy_actor(actor)
    plan=json.loads((ROOT/"Saved/CityDetails/placement.json").read_text(encoding="utf-8"))
    for role,label in [("entrance","CityKit_Street"),("railing","CityKit_Balcony")]:
        item=next(item for item in plan["items"] if item["role"]==role)
        p=item["position_cm"]; angle=math.radians(item["yaw_deg"]-90)
        distance=600 if role=="entrance" else 800
        target=[p[0],p[1],p[2]+150 if role=="entrance" else p[2]+40]
        position=[p[0]+math.cos(angle)*distance,p[1]+math.sin(angle)*distance,target[2]+30]
        camera(actors,label,position,target)
    if not levels.save_current_level(): raise RuntimeError("District camera save failed")
    marker.write_text("PASS\n",encoding="utf-8"); unreal.log("WTG_CITY_ASSET_QA_MAP_PASS")
except Exception:
    unreal.log_error(traceback.format_exc()); raise
finally: quit_after_editor_startup()
